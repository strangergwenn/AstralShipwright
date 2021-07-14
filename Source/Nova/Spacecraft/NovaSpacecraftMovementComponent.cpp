// Nova project - Gwennaël Arbona

#include "NovaSpacecraftMovementComponent.h"

#include "Nova/Actor/NovaActorTools.h"
#include "Nova/Actor/NovaPlayerStart.h"

#include "Nova/Game/NovaGameMode.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Nova.h"

#include "GameFramework/PlayerStart.h"
#include "Engine/PlayerStartPIE.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "UNovaSpacecraftMovementComponent"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftMovementComponent::UNovaSpacecraftMovementComponent()
	: Super()

	, MainDriveEnabled(false)

	, CurrentLinearVelocity(FVector::ZeroVector)
	, CurrentAngularVelocity(FVector::ZeroVector)

	, LinearAttitudeIdle(false)
	, AngularAttitudeIdle(false)
	, LinearAttitudeDistance(0)
	, PreviousVelocity(FVector::ZeroVector)
	, PreviousAngularVelocity(FVector::ZeroVector)
	, MeasuredAcceleration(FVector::ZeroVector)
	, MeasuredAngularAcceleration(FVector::ZeroVector)
{
	// Linear defaults
	LinearAcceleration  = 8;
	AngularAcceleration = 30;

	// Angular defaults
	LinearDeadDistance          = 1;
	MaxLinearVelocity           = 50;
	AngularDeadDistance         = 0.5f;
	MaxAngularVelocity          = 60;
	AngularOvershootRatio       = 1.1f;
	AngularColinearityThreshold = 0.999999f;

	// Physics defaults
	RestitutionCoefficient = 0.5f;

	// Settings
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

/*----------------------------------------------------
    Movement API
----------------------------------------------------*/

void UNovaSpacecraftMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Initialize the movement component on server when it doesn't have a start actor
	if (GetLocalRole() == ROLE_Authority && !IsInitialized())
	{
		ANovaPlayerController* PC    = GetOwner<APawn>()->GetController<ANovaPlayerController>();
		ANovaPlayerStart*      Start = Cast<ANovaPlayerStart>(GetWorld()->GetAuthGameMode<ANovaGameMode>()->ChoosePlayerStart(PC));

		if (IsValid(Start))
		{
			Initialize(Start);
		}
	}

#if 0
	int32 TestStepDuration = 5;
	int32 TestDuration = 4 * TestStepDuration;
	CurrentDesiredAttitude.Velocity = FVector::ZeroVector;
	if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < TestStepDuration)
	{
		CurrentDesiredAttitude.Location = FVector(0, 0, 1000);
	}
	else if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < 2 * TestStepDuration)
	{
		CurrentDesiredAttitude.Location = FVector(0, 1000, 0);
		CurrentDesiredAttitude.Velocity = FVector(0, 1, 0);
	}
	else if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < 3 * TestStepDuration)
	{
		CurrentDesiredAttitude.Location = FVector(0, -1000, 0);
	}
	else
	{
		CurrentDesiredAttitude.Location = FVector(1000, 1000, 0);
	}
#elif 0
	int32 TestStepDuration = 10;
	int32 TestDuration     = 4 * TestStepDuration;
	if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < TestStepDuration)
	{
		AttitudeCommand.Direction = FVector(0, 0, 1).GetSafeNormal();
	}
	else if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < 2 * TestStepDuration)
	{
		AttitudeCommand.Direction = FVector(0, 1, 0).GetSafeNormal();
	}
	else if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < 3 * TestStepDuration)
	{
		AttitudeCommand.Direction = FVector(0, -1, 0).GetSafeNormal();
	}
	else
	{
		AttitudeCommand.Direction = FVector(1, 1, 1).GetSafeNormal();
	}
#endif

	// Initialize state
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();

	// Run high-level processing
	if (GetLocalRole() == ROLE_Authority)
	{
		ProcessState();
	}

	// Run movement implementation
	ProcessMeasurementsBeforeAttitude(DeltaTime);
	if (IsValid(GameState) && GameState->IsUsingTrajectoryMovement())
	{
		ProcessTrajectoryMovement(DeltaTime);
	}
	else
	{
		ProcessLinearAttitude(DeltaTime);
		ProcessAngularAttitude(DeltaTime);
		ProcessMeasurementsAfterAttitude(DeltaTime);
		ProcessMovement(DeltaTime);
	}
}

void UNovaSpacecraftMovementComponent::Initialize(const ANovaPlayerStart* Start)
{
	NCHECK(IsValid(Start));
	NCHECK(IsValid(UpdatedComponent));
	NCHECK(GetLocalRole() == ROLE_Authority);

	// Reset the state
	DockState.Actor = Start;
	ResetState();

	// Reset attitude
	FTransform DesiredTransform = GetInitialTransform();
	UpdatedComponent->SetWorldTransform(DesiredTransform);
	AttitudeCommand.Location  = DesiredTransform.GetLocation();
	AttitudeCommand.Direction = DesiredTransform.GetRotation().Vector();

	// Reset the attitude command
	if (DockState.IsDocked)
	{
		NLOG("UNovaSpacecraftMovementComponent::Initialize : docked at '%s'", Start ? *Start->GetName() : nullptr);

		MovementCommand = FNovaMovementCommand(ENovaMovementState::Docked);
	}
	else
	{
		if (Start->IsInSpace)
		{
			NLOG("UNovaSpacecraftMovementComponent::Initialize : idle in space at '%s'", Start ? *Start->GetName() : nullptr);
		}
		else
		{
			NLOG("UNovaSpacecraftMovementComponent::Initialize : maneuvering at '%s'", Start ? *Start->GetName() : nullptr);
		}

		MovementCommand = FNovaMovementCommand(ENovaMovementState::Idle);
	}
}

bool UNovaSpacecraftMovementComponent::IsInitialized() const
{
	return IsValid(DockState.Actor) && IsValid(DockState.Actor->GetLevel());
}

void UNovaSpacecraftMovementComponent::Reset()
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("UNovaSpacecraftMovementComponent::Reset");

	DockState.Actor = nullptr;
}

bool UNovaSpacecraftMovementComponent::CanDock() const
{
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	if (GameState)
	{
		const FNovaTrajectory* Trajectory = GameState->GetOrbitalSimulation()->GetPlayerTrajectory();

		return IsInitialized() && GetState() == ENovaMovementState::Idle && !DockState.Actor->IsInSpace && Trajectory == nullptr;
	}
	else
	{
		return false;
	}
}

bool UNovaSpacecraftMovementComponent::CanUndock() const
{
	return IsInitialized() && GetState() == ENovaMovementState::Docked;
}

void UNovaSpacecraftMovementComponent::Dock(FSimpleDelegate Callback)
{
	NLOG("UNovaSpacecraftMovementComponent::Dock ('%s')", *GetRoleString(this));

	CompletionCallback = Callback;
	RequestMovement(FNovaMovementCommand(ENovaMovementState::Docking, DockState.Actor));
}

void UNovaSpacecraftMovementComponent::Undock(FSimpleDelegate Callback)
{
	NLOG("UNovaSpacecraftMovementComponent::Undock ('%s')", *GetRoleString(this));

	CompletionCallback = Callback;
	RequestMovement(FNovaMovementCommand(ENovaMovementState::Undocking));
}

void UNovaSpacecraftMovementComponent::Stop(FSimpleDelegate Callback)
{
	NLOG("UNovaSpacecraftMovementComponent::Stop ('%s')", *GetRoleString(this));

	CompletionCallback = Callback;
	RequestMovement(FNovaMovementCommand(ENovaMovementState::Stopping));
}

void UNovaSpacecraftMovementComponent::AlignToNextManeuver()
{
	// Fetch the trajectory state
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));
	const FNovaTrajectory* Trajectory   = GameState->GetOrbitalSimulation()->GetPlayerTrajectory();
	const FNovaManeuver*   NextManeuver = Trajectory ? Trajectory->GetNextManeuver(GameState->GetCurrentTime()) : nullptr;

	// Move to the orientation state
	if (NextManeuver)
	{
		FVector ManeuverDirection = GetManeuverDirection();
		if (AttitudeCommand.Direction != ManeuverDirection)
		{
			NLOG("UNovaSpacecraftMovementComponent::AlignToNextManeuver : orientating");
			AttitudeCommand.Direction = ManeuverDirection;
			MovementCommand.State     = ENovaMovementState::Orientating;
		}
	}
}

bool UNovaSpacecraftMovementComponent::IsAlignedToNextManeuver() const
{
	// Fetch the trajectory state
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));
	const FNovaTrajectory* Trajectory   = GameState->GetOrbitalSimulation()->GetPlayerTrajectory();
	const FNovaManeuver*   NextManeuver = Trajectory ? Trajectory->GetNextManeuver(GameState->GetCurrentTime()) : nullptr;

	// Check the orientation state
	if (NextManeuver)
	{
		const FVector ManeuverDirection = GetManeuverDirection();
		float CurrentAngle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(UpdatedComponent->GetForwardVector(), ManeuverDirection)));

		return CurrentAngle < AngularDeadDistance;
	}

	return false;
}

void UNovaSpacecraftMovementComponent::EnableMainDrive()
{
	NLOG("UNovaSpacecraftMovementComponent::EnableMainDrive ('%s')", *GetRoleString(this));

	// Server
	if (GetLocalRole() == ROLE_Authority)
	{
		MainDriveEnabled = true;
	}

	// Authoritative client
	else if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerEnableMainDrive();
	}
}

void UNovaSpacecraftMovementComponent::DisableMainDrive()
{
	NLOG("UNovaSpacecraftMovementComponent::DisableMainDrive");

	NCHECK(GetLocalRole() == ROLE_Authority);

	MainDriveEnabled = false;
}

void UNovaSpacecraftMovementComponent::ServerEnableMainDrive_Implementation()
{
	EnableMainDrive();
}

/*----------------------------------------------------
    High level movement
----------------------------------------------------*/

void UNovaSpacecraftMovementComponent::ProcessState()
{
	// Fetch the trajectory state
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));
	const FNovaTrajectory* Trajectory   = GameState->GetOrbitalSimulation()->GetPlayerTrajectory();
	const FNovaManeuver*   NextManeuver = Trajectory ? Trajectory->GetNextManeuver(GameState->GetCurrentTime()) : nullptr;

	switch (MovementCommand.State)
	{
		// Idle states
		case ENovaMovementState::Docked:
		case ENovaMovementState::Idle:
			break;

		// Orientating state that serves as a sub-state for Idle without identifying as such
		case ENovaMovementState::Orientating:
			if (AngularAttitudeIdle)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Orientating : done");
				MovementCommand.State = ENovaMovementState::Idle;
			}
			break;

		// Docking procedure
		case ENovaMovementState::Docking:
			DockState.IsDocked = true;
			if (!IsValid(MovementCommand.Target))
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Docking : aborted");

				MovementCommand.State = ENovaMovementState::Idle;
			}
			else if (MovementCommand.Dirty)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Docking : starting");

				AttitudeCommand.Location  = MovementCommand.Target->GetActorLocation();
				AttitudeCommand.Direction = FVector(1, 0, 0);
			}
			else if (LinearAttitudeIdle && AngularAttitudeIdle)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Docking : done");

				MovementCommand.State = ENovaMovementState::Docked;

				SignalCompletion();
			}
			break;

		// Undocking procedure
		case ENovaMovementState::Undocking:
			DockState.IsDocked = false;
			if (MovementCommand.Dirty)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Undocking : starting");

				AttitudeCommand.Location = DockState.Actor->GetWaitingPointLocation();
			}
			else if (LinearAttitudeIdle && AngularAttitudeIdle)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Undocking : done");

				MovementCommand.State = ENovaMovementState::Idle;

				SignalCompletion();
			}
			else if (LinearAttitudeDistance < 50)
			{
				AttitudeCommand.Direction = FVector(1, 0, 0);
			}
			break;

		// Braking to zero
		case ENovaMovementState::Stopping:
			AttitudeCommand.Velocity = FVector::ZeroVector;
			if (LinearAttitudeIdle && AngularAttitudeIdle)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Stopping : done");

				MovementCommand.State = ENovaMovementState::Idle;
			}
			break;

		default:
			break;
	}

	MovementCommand.Dirty = false;
}

void UNovaSpacecraftMovementComponent::SignalCompletion()
{
	CompletionCallback.ExecuteIfBound();
	CompletionCallback.Unbind();

	if (GetLocalRole() == ROLE_Authority)
	{
		ClientSignalCompletion();
	}
}

void UNovaSpacecraftMovementComponent::ClientSignalCompletion_Implementation()
{
	if (GetLocalRole() != ROLE_Authority)
	{
		SignalCompletion();
	}
}

/*----------------------------------------------------
    Networking
----------------------------------------------------*/

void UNovaSpacecraftMovementComponent::RequestMovement(const FNovaMovementCommand& Command)
{
	NLOG("UNovaSpacecraftMovementComponent::RequestMovement ('%s')", *GetRoleString(this));

	// Server
	if (GetLocalRole() == ROLE_Authority)
	{
		MovementCommand       = Command;
		MovementCommand.Dirty = true;
	}

	// Authoritative client
	else if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerRequestMovement(Command);
	}
}

void UNovaSpacecraftMovementComponent::ServerRequestMovement_Implementation(const FNovaMovementCommand& Command)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	RequestMovement(Command);
}

/*----------------------------------------------------
    Internal movement implementation
----------------------------------------------------*/

void UNovaSpacecraftMovementComponent::OnDockStateReplicated(const FNovaMovementDockState& PreviousDockState)
{
	NLOG("UNovaSpacecraftMovementComponent::OnDockStateReplicated : resetting to '%s' (%d), IsDocked %d, IsInSpace %d",
		DockState.Actor ? *DockState.Actor->GetName() : TEXT("nullptr"), DockState.Actor != PreviousDockState.Actor, DockState.IsDocked,
		DockState.Actor ? DockState.Actor->IsInSpace : false);

	if (IsValid(DockState.Actor) && DockState.Actor != PreviousDockState.Actor)
	{
		NCHECK(IsValid(UpdatedComponent));

		FTransform DesiredTransform = GetInitialTransform();
		UpdatedComponent->SetWorldTransform(DesiredTransform);

		ResetState();
	}
}

void UNovaSpacecraftMovementComponent::ResetState()
{
	CurrentLinearVelocity       = FVector::ZeroVector;
	CurrentAngularVelocity      = FVector::ZeroVector;
	PreviousVelocity            = FVector::ZeroVector;
	PreviousAngularVelocity     = FVector::ZeroVector;
	MeasuredAcceleration        = FVector::ZeroVector;
	MeasuredAngularAcceleration = FVector::ZeroVector;
}

void UNovaSpacecraftMovementComponent::ProcessMeasurementsBeforeAttitude(float DeltaTime)
{
	LinearAttitudeIdle  = LinearAttitudeDistance < LinearDeadDistance;
	AngularAttitudeIdle = AngularAttitudeDistance < AngularDeadDistance;

	MeasuredAcceleration        = ((CurrentLinearVelocity - PreviousVelocity) / DeltaTime);
	MeasuredAngularAcceleration = (CurrentAngularVelocity - PreviousAngularVelocity) * DeltaTime;
	PreviousVelocity            = CurrentLinearVelocity;
	PreviousAngularVelocity     = CurrentAngularVelocity;
}

void UNovaSpacecraftMovementComponent::ProcessMeasurementsAfterAttitude(float DeltaTime)
{
	LinearAttitudeIdle  = LinearAttitudeDistance < LinearDeadDistance;
	AngularAttitudeIdle = AngularAttitudeDistance < AngularDeadDistance;
}

void UNovaSpacecraftMovementComponent::ProcessLinearAttitude(float DeltaTime)
{
	// Get the position data
	const FVector DeltaPosition          = (AttitudeCommand.Location - UpdatedComponent->GetComponentLocation()) / 100.0f;
	const FVector DeltaPositionDirection = DeltaPosition.GetSafeNormal();
	LinearAttitudeDistance               = FMath::Max(0.0f, DeltaPosition.Size() - LinearDeadDistance);

	// Get the velocity data
	FVector DeltaVelocity     = AttitudeCommand.Velocity - CurrentLinearVelocity;
	FVector DeltaVelocityAxis = DeltaVelocity;
	DeltaVelocityAxis.Normalize();

	// Determine the time left to reach the final desired velocity
	float TimeToFinalVelocity = 0;
	if (!FMath::IsNearlyZero(DeltaVelocity.SizeSquared()))
	{
		TimeToFinalVelocity = DeltaVelocity.Size() / LinearAcceleration;
	}

	// Update desired velocity to match location & velocity inputs best
	FVector RelativeResultSpeed;
	float   DistanceToStop = (DeltaVelocity.Size() / 2) * (TimeToFinalVelocity + DeltaTime);
	if (DistanceToStop > LinearAttitudeDistance)
	{
		RelativeResultSpeed = AttitudeCommand.Velocity;
	}
	else
	{
		float MaxPreciseSpeed = FMath::Min((LinearAttitudeDistance - DistanceToStop) / DeltaTime, MaxLinearVelocity);
		if (DistanceToStop > LinearAttitudeDistance)
		{
			MaxPreciseSpeed = FMath::Min(MaxPreciseSpeed, CurrentLinearVelocity.Size());
		}

		RelativeResultSpeed = DeltaPositionDirection;
		RelativeResultSpeed *= MaxPreciseSpeed;
		RelativeResultSpeed += AttitudeCommand.Velocity;
	}

	// Update the linear velocity based on acceleration
	auto UpdateVelocity = [](float& Value, float Target, float MaxDelta)
	{
		Value = FMath::Clamp(Target, Value - MaxDelta, Value + MaxDelta);
	};
	UpdateVelocity(CurrentLinearVelocity.X, RelativeResultSpeed.X, LinearAcceleration * DeltaTime);
	UpdateVelocity(CurrentLinearVelocity.Y, RelativeResultSpeed.Y, LinearAcceleration * DeltaTime);
	UpdateVelocity(CurrentLinearVelocity.Z, RelativeResultSpeed.Z, LinearAcceleration * DeltaTime);
}

void UNovaSpacecraftMovementComponent::ProcessAngularAttitude(float DeltaTime)
{
	FVector       NewAngularVelocity = FVector::ZeroVector;
	const FVector ActorAxis          = GetOwner()->GetActorForwardVector();
	const float   DotProduct         = FVector::DotProduct(ActorAxis, AttitudeCommand.Direction);

	if (DotProduct < AngularColinearityThreshold)
	{
		// Determine a quaternion that represents the desired difference in orientation
		FQuat TargetRotation = DotProduct > -AngularColinearityThreshold ? FQuat::FindBetweenNormals(ActorAxis, AttitudeCommand.Direction)
																		 : FRotator(0, 180, 0).Quaternion();

		// While on the horizontal plane, follow desired roll too
		if (FMath::IsNearlyZero(AttitudeCommand.Direction.Z))
		{
			// Roll angle of the final resting rotation around the desired direction
			auto GetRollAngle = [&](const FQuat& Rotation)
			{
				FQuat       Swing, Twist;
				const FQuat ResultingOrientation = Rotation.GetNormalized() * UpdatedComponent->GetComponentQuat();
				ResultingOrientation.ToSwingTwist(AttitudeCommand.Direction, Swing, Twist);
				return Twist.GetAngle();
			};

			// Extract the roll angle, build a correction, test it and apply the one that works
			const float DesiredRoll         = FMath::DegreesToRadians(AttitudeCommand.Roll);
			const float ActorRollRadians    = GetRollAngle(TargetRotation);
			FQuat       FixedTargetRotation = FQuat(AttitudeCommand.Direction, DesiredRoll + ActorRollRadians) * TargetRotation;
			if (GetRollAngle(FixedTargetRotation) > ActorRollRadians)
			{
				FixedTargetRotation = FQuat(AttitudeCommand.Direction, DesiredRoll - ActorRollRadians) * TargetRotation;
			}
			TargetRotation = FixedTargetRotation;
		}

		// Extract the rotation axis and angle
		FVector RotationDirection;
		float   RemainingAngleRadians;
		TargetRotation.ToAxisAndAngle(RotationDirection, RemainingAngleRadians);
		AngularAttitudeDistance = FMath::RadiansToDegrees(RemainingAngleRadians);

		// This cannot be explained, but likely fixes inconsistent output by FindBetweenNormals
		RotationDirection *= FVector(-1, -1, 1);

		// Determine the time left to reach the final (zero) velocity
		float         TimeToFinalVelocity  = 0;
		const FVector AngularVelocityDelta = -CurrentAngularVelocity;
		if (!FMath::IsNearlyZero(AngularVelocityDelta.SizeSquared()))
		{
			FVector Acceleration            = AngularVelocityDelta.GetSafeNormal() * AngularAcceleration;
			float   AccelerationInAngleAxis = FMath::Abs(FVector::DotProduct(Acceleration, RotationDirection));
			TimeToFinalVelocity             = (AngularVelocityDelta.Size() / AccelerationInAngleAxis);
		}

		// Determine the new angular velocity based on the remaining angle and velocity
		float AngularStoppingDistance = (AngularVelocityDelta.Size() / 2) * (TimeToFinalVelocity + DeltaTime) / AngularOvershootRatio;
		if (!FMath::IsNearlyZero(AngularAttitudeDistance) && FMath::Abs(AngularAttitudeDistance) > AngularStoppingDistance)
		{
			float OvershotAngularAttitudeDistance = AngularOvershootRatio * (AngularAttitudeDistance - AngularStoppingDistance);
			float MaxUsefulAngularVelocity        = FMath::Min(OvershotAngularAttitudeDistance / DeltaTime, MaxAngularVelocity);
			NewAngularVelocity                    = RotationDirection * MaxUsefulAngularVelocity;
		}

#if 0
		NLOG(
			"ActAx = [%.2f %.2f %.2f] -> TrgAx = [%.2f %.2f %.2f] = RotDir = [%.2f %.2f %.2f] / Angle = %.1f, Dot %.4f / NewVel = [%.2f "
			"%.2f %.2f]",
			ActorAxis.X, ActorAxis.Y, ActorAxis.Z, AttitudeCommand.Direction.X, AttitudeCommand.Direction.Y, AttitudeCommand.Direction.Z,
			RotationDirection.X, RotationDirection.Y, RotationDirection.Z, AngularAttitudeDistance, DotProduct, NewAngularVelocity.X,
			NewAngularVelocity.Y, NewAngularVelocity.Z);
#endif
	}
	else
	{
		AngularAttitudeDistance = 0;
	}

	// Update the angular velocity based on acceleration
	auto UpdateVelocity = [](float& Value, float Target, float MaxDelta)
	{
		Value = FMath::Clamp(Target, Value - MaxDelta, Value + MaxDelta);
	};
	UpdateVelocity(CurrentAngularVelocity.X, NewAngularVelocity.X, AngularAcceleration * DeltaTime);
	UpdateVelocity(CurrentAngularVelocity.Y, NewAngularVelocity.Y, AngularAcceleration * DeltaTime);
	UpdateVelocity(CurrentAngularVelocity.Z, NewAngularVelocity.Z, AngularAcceleration * DeltaTime);
}

void UNovaSpacecraftMovementComponent::ProcessMovement(float DeltaTime)
{
	if (GetOwner()->GetAttachParentActor() == nullptr)
	{
		// Apply limits
		CurrentLinearVelocity  = CurrentLinearVelocity.GetClampedToMaxSize(MaxLinearVelocity);
		CurrentAngularVelocity = CurrentAngularVelocity.GetClampedToMaxSize(MaxAngularVelocity);

		// Update rotation
		FQuat ActorRotation = UpdatedComponent->GetComponentQuat();
		FQuat EffectiveRotationDelta =
			(FRotator(CurrentAngularVelocity.Y, CurrentAngularVelocity.Z, CurrentAngularVelocity.X) * DeltaTime).Quaternion();
		ActorRotation = (EffectiveRotationDelta * ActorRotation).GetNormalized();

		// Move safely and ensure de-penetration if required
		FHitResult Hit;
		FVector    ActorTranslation = CurrentLinearVelocity * 100 * DeltaTime;
		SafeMoveUpdatedComponent(ActorTranslation, ActorRotation, true, Hit);

		// Process invalid location
		if (Hit.bStartPenetrating)
		{
			CurrentLinearVelocity = -CurrentLinearVelocity;
		}

		// Process impacts
		if (Hit.IsValidBlockingHit())
		{
			OnHit(Hit, CurrentLinearVelocity);

			CurrentLinearVelocity = -RestitutionCoefficient * CurrentLinearVelocity;
			ActorTranslation      = CurrentLinearVelocity * 100 * DeltaTime;
			SlideAlongSurface(ActorTranslation, 1 - Hit.Time, Hit.Normal, Hit);
		}
	}
}

void UNovaSpacecraftMovementComponent::ProcessTrajectoryMovement(float DeltaTime)
{
	const ANovaGameState*  GameState  = GetWorld()->GetGameState<ANovaGameState>();
	const FNovaTrajectory* Trajectory = GameState->GetOrbitalSimulation()->GetPlayerTrajectory();

	if (DockState.Actor && Trajectory && Trajectory->IsValid())
	{
		const FNovaTime StartTime         = Trajectory->GetStartTime();
		const FNovaTime ManeuverStartTime = Trajectory->GetFirstManeuverStartTime();
		const FNovaTime ArrivalTime       = Trajectory->GetArrivalTime();
		const FNovaTime CurrentTime       = GameState->GetCurrentTime();
		const bool      IsEnteringArea    = FMath::Abs(CurrentTime - ArrivalTime) < FMath::Abs(CurrentTime - ManeuverStartTime);

		double CurrentPosition = 0;
		double CurrentVelocity = 0;
		double RemainingTime   = IsEnteringArea ? (ArrivalTime - CurrentTime).AsSeconds() : (CurrentTime - StartTime).AsSeconds();
		RemainingTime          = FMath::Max(RemainingTime, 0.0);

		// Decompose the trajectory into segments of acceleration x duration
		FNovaTime                     CurrentManeuverTime = Trajectory->GetStartTime();
		TArray<TPair<double, double>> AccelerationsAndDurations;
		for (const FNovaManeuver& Maneuver : Trajectory->Maneuvers)
		{
			if (Maneuver.Time > CurrentManeuverTime)
			{
				AccelerationsAndDurations.Add(TPair<double, double>(0, (Maneuver.Time - CurrentManeuverTime).AsSeconds()));
			}
			AccelerationsAndDurations.Add(
				TPair<double, double>(Maneuver.DeltaV / Maneuver.Duration.AsSeconds(), Maneuver.Duration.AsSeconds()));
			CurrentManeuverTime = Maneuver.Time + Maneuver.Duration;
		}
		NCHECK(CurrentManeuverTime == ArrivalTime);

		// Segment processing function
		auto ProcessSegment = [&](int32 SegmentIndex)
		{
			const double& Acceleration  = AccelerationsAndDurations[SegmentIndex].Key;
			const double& Duration      = AccelerationsAndDurations[SegmentIndex].Value;
			const double  StartVelocity = CurrentVelocity;
			const double  EffectiveTime = FMath::Min(RemainingTime, Duration);

			CurrentVelocity += Acceleration * EffectiveTime;
			CurrentPosition += EffectiveTime * (CurrentVelocity + StartVelocity) / 2.0;

			RemainingTime -= EffectiveTime;
			return RemainingTime > 0;
		};

		// When entering, start from the desired end goal and go back in time
		if (IsEnteringArea)
		{
			for (int32 SegmentIndex = AccelerationsAndDurations.Num() - 1; SegmentIndex >= 0; SegmentIndex--)
			{
				if (!ProcessSegment(SegmentIndex))
				{
					break;
				}
			}
		}

		// When exiting, process them linearly
		else
		{
			for (int32 SegmentIndex = 0; SegmentIndex < AccelerationsAndDurations.Num(); SegmentIndex++)
			{
				if (!ProcessSegment(SegmentIndex))
				{
					break;
				}
			}
		}

		// Get the current direction to apply
		int32         CurrentManeuverIndex  = IsEnteringArea ? Trajectory->Maneuvers.Num() - 1 : 0;
		float         ClosestManeuverDeltaV = Trajectory->Maneuvers[CurrentManeuverIndex].DeltaV;
		const FVector Direction             = DockState.Actor->GetInterfacePointDirection(ClosestManeuverDeltaV);
		UpdatedComponent->SetWorldRotation(Direction.Rotation());

		// Get the current location to apply
		FVector Location = DockState.Actor->GetWaitingPointLocation() + 100 * CurrentPosition * DockState.Actor->GetOrbitalAxis();
		UpdatedComponent->SetWorldLocation(Location);

		// Signal the end location we want for when the non-trajectory state machine kicks back in
		if (IsEnteringArea && GetLocalRole() == ROLE_Authority)
		{
			AttitudeCommand.Location = DockState.Actor->GetWaitingPointLocation();
		}
	}
}

void UNovaSpacecraftMovementComponent::OnHit(const FHitResult& Hit, const FVector& HitVelocity)
{}

FVector UNovaSpacecraftMovementComponent::GetManeuverDirection() const
{
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));
	const FNovaTrajectory* Trajectory = GameState->GetOrbitalSimulation()->GetPlayerTrajectory();

	const FNovaManeuver* Maneuver = Trajectory ? Trajectory->GetCurrentManeuver(GameState->GetCurrentTime()) : nullptr;
	if (Maneuver == nullptr)
	{
		Maneuver = Trajectory ? Trajectory->GetNextManeuver(GameState->GetCurrentTime()) : nullptr;
	}

	if (DockState.Actor && Maneuver)
	{
		return DockState.Actor->GetInterfacePointDirection(Maneuver->DeltaV);
	}

	return AttitudeCommand.Direction;
}

FTransform UNovaSpacecraftMovementComponent::GetInitialTransform() const
{
	// Start docked
	if (DockState.IsDocked)
	{
		NLOG("UNovaSpacecraftMovementComponent::GetInitialTransform : docked");
		return DockState.Actor->GetActorTransform();
	}

	// Probably trajectory based
	else
	{
		// Fetch the trajectory state
		const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
		NCHECK(IsValid(GameState));
		FNovaTime              CurrentTime  = GameState->GetCurrentTime();
		const FNovaTrajectory* Trajectory   = GameState->GetOrbitalSimulation()->GetPlayerTrajectory();
		const FNovaManeuver*   NextManeuver = Trajectory ? Trajectory->GetNextManeuver(CurrentTime) : nullptr;

		// Maneuver override
		if (NextManeuver)
		{
			FRotator DesiredRotation = DockState.Actor->GetInterfacePointDirection(NextManeuver->DeltaV).Rotation();

			// Exit maneuver
			if (Trajectory->GetFirstManeuverStartTime() > CurrentTime)
			{
				NLOG("UNovaSpacecraftMovementComponent::GetInitialTransform : exit maneuver");
				return FTransform(DesiredRotation, DockState.Actor->GetWaitingPointLocation());
			}

			// Orbiting between areas
			else if (Trajectory && Trajectory->GetRemainingManeuverCount(CurrentTime) > 1)
			{
				NLOG("UNovaSpacecraftMovementComponent::GetInitialTransform : orbiting");
				return FTransform(DesiredRotation, FVector::ZeroVector);
			}

			// Enter maneuver
			else
			{
				NLOG("UNovaSpacecraftMovementComponent::GetInitialTransform : enter maneuver");
				return FTransform(DesiredRotation, DockState.Actor->GetInterfacePointLocation(NextManeuver->DeltaV));
			}
		}
	}

	return DockState.Actor->GetActorTransform();
}

void UNovaSpacecraftMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaSpacecraftMovementComponent, MovementCommand);
	DOREPLIFETIME(UNovaSpacecraftMovementComponent, AttitudeCommand);
	DOREPLIFETIME(UNovaSpacecraftMovementComponent, DockState);
	DOREPLIFETIME(UNovaSpacecraftMovementComponent, MainDriveEnabled);
}

#undef LOCTEXT_NAMESPACE
