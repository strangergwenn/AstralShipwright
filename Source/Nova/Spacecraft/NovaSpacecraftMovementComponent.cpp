// Nova project - Gwennaël Arbona

#include "NovaSpacecraftMovementComponent.h"

#include "Nova/Actor/NovaPlayerStart.h"

#include "Nova/Game/NovaGameMode.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Tools/NovaActorTools.h"
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
	LinearAcceleration     = 8;
	LinearMainAcceleration = 50;
	AngularAcceleration    = 30;
	VectoringAngle         = 7.5f;

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
		ANovaPlayerController* PC        = GetOwner<APawn>()->GetController<ANovaPlayerController>();
		ANovaPlayerStart*      Start     = Cast<ANovaPlayerStart>(GetWorld()->GetAuthGameMode<ANovaGameMode>()->ChoosePlayerStart(PC));
		ANovaGameState*        GameState = GetWorld()->GetGameState<ANovaGameState>();

		if (IsValid(Start) && IsValid(GameState))
		{
			Initialize(Start, GameState->ShouldStartDocked());
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

	// Run high-level processing
	if (GetLocalRole() == ROLE_Authority)
	{
		ProcessState();
	}

	// Run movement implementation
	float DilatedDeltaTime = DeltaTime * GetWorld()->GetGameState<ANovaGameState>()->GetCurrentTimeDilationValue();
	ProcessMeasurementsBeforeAttitude(DilatedDeltaTime);
	ProcessLinearAttitude(DilatedDeltaTime);
	ProcessAngularAttitude(DilatedDeltaTime);
	ProcessMeasurementsAfterAttitude(DilatedDeltaTime);
	ProcessMovement(DilatedDeltaTime);
}

void UNovaSpacecraftMovementComponent::Initialize(const ANovaPlayerStart* Start, bool StartDocked)
{
	NLOG("UNovaSpacecraftMovementComponent::Initialize : '%s', StartDocked %d", Start ? *Start->GetName() : nullptr, StartDocked);

	NCHECK(IsValid(Start));
	NCHECK(IsValid(UpdatedComponent));
	NCHECK(GetLocalRole() == ROLE_Authority);

	// Reset attitude and transform
	if (IsValid(Start))
	{
		AttitudeCommand           = FNovaAttitudeCommand();
		AttitudeCommand.Location  = Start->GetActorLocation();
		AttitudeCommand.Direction = Start->GetActorForwardVector();
		UpdatedComponent->SetWorldTransform(Start->GetActorTransform());

		// Reset flight command
		InitParameters = FNovaMovementStartParameters(Start, StartDocked);
		if (StartDocked)
		{
			MovementCommand = FNovaMovementCommand(ENovaMovementState::Docked);
		}
		else if (Start->IsInSpace)
		{
			MovementCommand = FNovaMovementCommand(ENovaMovementState::Idle);
		}
		else
		{
			// TODO : enter maneuver needs to be handled here
			AttitudeCommand.Location = Start->GetEnterPointLocation(true);
			MovementCommand          = FNovaMovementCommand(ENovaMovementState::Stopping);
		}

		ResetState();
	}
}

bool UNovaSpacecraftMovementComponent::IsInitialized() const
{
	return IsValid(InitParameters.DockActor);
}

bool UNovaSpacecraftMovementComponent::CanDock() const
{
	return IsInitialized() && GetState() == ENovaMovementState::Idle && !InitParameters.DockActor->IsInSpace;
}

bool UNovaSpacecraftMovementComponent::CanUndock() const
{
	return IsInitialized() && GetState() == ENovaMovementState::Docked;
}

void UNovaSpacecraftMovementComponent::Dock(FSimpleDelegate Callback)
{
	NLOG("UNovaSpacecraftMovementComponent::Dock ('%s')", *GetRoleString(this));

	CompletionCallback = Callback;
	RequestMovement(FNovaMovementCommand(ENovaMovementState::Docking, InitParameters.DockActor));
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

/*----------------------------------------------------
    High level movement
----------------------------------------------------*/

void UNovaSpacecraftMovementComponent::ProcessState()
{
	// Fetch state data
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));
	const FNovaTrajectory* Trajectory      = GameState->GetOrbitalSimulation()->GetPlayerTrajectory();
	const FNovaManeuver*   CurrentManeuver = Trajectory ? Trajectory->GetCurrentManeuver(GameState->GetCurrentTime()) : nullptr;

	// Handle maneuvering in trajectories
	if (Trajectory && IsValid(InitParameters.DockActor))
	{
		// Get the next maneuver to drive direction if the upcoming maneuver hasn't started yet
		const FNovaManeuver* CurrentOrNextManeuver = CurrentManeuver;
		if (CurrentOrNextManeuver == nullptr)
		{
			CurrentOrNextManeuver = Trajectory->GetNextManeuver(GameState->GetCurrentTime());
		}

		// Handle direction
		if (CurrentOrNextManeuver)
		{
			const FVector ManeuverTargetLocation = InitParameters.DockActor->GetExitPointLocation(CurrentOrNextManeuver->DeltaV > 0);
			AttitudeCommand.Direction            = (ManeuverTargetLocation - UpdatedComponent->GetComponentLocation()).GetSafeNormal();
		}

		// Handle location
		if (CurrentManeuver)
		{
			AttitudeCommand.Location = InitParameters.DockActor->GetExitPointLocation(CurrentManeuver->DeltaV > 0);
		}

		// Toggle the main drive
		AttitudeCommand.MainDriveEnabled = CurrentManeuver != nullptr;
	}

	// Handle regular in-world maneuvering
	else
	{
		switch (MovementCommand.State)
		{
			// Idle states
			case ENovaMovementState::Idle:
			case ENovaMovementState::Docked:
				AttitudeCommand.MainDriveEnabled = false;
				break;

			// Docking procedure
			case ENovaMovementState::Docking:
				if (!IsValid(MovementCommand.Target))
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : Docking : aborted");

					MovementCommand.State            = ENovaMovementState::Idle;
					AttitudeCommand.MainDriveEnabled = false;
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
				AttitudeCommand.MainDriveEnabled = false;
				if (MovementCommand.Dirty)
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : Undocking : starting");

					AttitudeCommand.Location = InitParameters.DockActor->GetWaitingPointLocation();
				}
				else if (LinearAttitudeIdle && AngularAttitudeIdle)
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : Undocking : done");

					MovementCommand.State = ENovaMovementState::Idle;

					SignalCompletion();
				}
				else if (LinearAttitudeDistance < 50)
				{
					AttitudeCommand.Direction = FVector(0, 1, 0);
				}
				break;

			// Braking to zero
			case ENovaMovementState::Stopping:
				AttitudeCommand.MainDriveEnabled = false;
				AttitudeCommand.Velocity         = FVector::ZeroVector;
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

void UNovaSpacecraftMovementComponent::OnStartParametersReplicated()
{
	NLOG("UNovaSpacecraftMovementComponent::OnStartParametersReplicated : resetting to '%s', Docked %d, IsInSpace %d",
		InitParameters.DockActor ? *InitParameters.DockActor->GetName() : TEXT("nullptr"), InitParameters.StartDocked,
		InitParameters.DockActor ? InitParameters.DockActor->IsInSpace : false);

	if (InitParameters.DockActor)
	{
		NCHECK(IsValid(UpdatedComponent));
		UpdatedComponent->SetWorldTransform(InitParameters.DockActor->GetActorTransform());
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
	const UNovaArea* CurrentArea = GetWorld()->GetGameState<ANovaGameState>()->GetCurrentArea();

	if (CurrentArea && !CurrentArea->DisableSpacecraftMovement)
	{
		// Get the position data
		const FVector DeltaPosition          = (AttitudeCommand.Location - UpdatedComponent->GetComponentLocation()) / 100.0f;
		const FVector DeltaPositionDirection = DeltaPosition.GetSafeNormal();
		LinearAttitudeDistance               = FMath::Max(0.0f, DeltaPosition.Size() - LinearDeadDistance);

		// Get the velocity data
		FVector DeltaVelocity     = AttitudeCommand.Velocity - CurrentLinearVelocity;
		FVector DeltaVelocityAxis = DeltaVelocity;
		DeltaVelocityAxis.Normalize();

		const float CurrentAcceleration = IsMainDriveRunning() ? LinearMainAcceleration : LinearAcceleration;

		// Determine the time left to reach the final desired velocity
		float TimeToFinalVelocity = 0;
		if (!FMath::IsNearlyZero(DeltaVelocity.SizeSquared()))
		{
			TimeToFinalVelocity = DeltaVelocity.Size() / CurrentAcceleration;
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
		UpdateVelocity(CurrentLinearVelocity.X, RelativeResultSpeed.X, CurrentAcceleration * DeltaTime);
		UpdateVelocity(CurrentLinearVelocity.Y, RelativeResultSpeed.Y, CurrentAcceleration * DeltaTime);
		UpdateVelocity(CurrentLinearVelocity.Z, RelativeResultSpeed.Z, CurrentAcceleration * DeltaTime);
	}
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

		/*NLOG("ActAx = [%.2f %.2f %.2f] -> TrgAx = [%.2f %.2f %.2f] = RotDir = [%.2f %.2f %.2f] / Angle = %.1f, Dot %.4f / NewVel = [%.2f
		   %.2f %.2f]", ActorAxis.X, ActorAxis.Y, ActorAxis.Z, CurrentDesiredAttitude.Direction.X, CurrentDesiredAttitude.Direction.Y,
		   CurrentDesiredAttitude.Direction.Z, RotationDirection.X, RotationDirection.Y, RotationDirection.Z, AngularAttitudeDistance,
		   DotProduct, NewAngularVelocity.X, NewAngularVelocity.Y, NewAngularVelocity.Z);*/
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

void UNovaSpacecraftMovementComponent::OnHit(const FHitResult& Hit, const FVector& HitVelocity)
{}

void UNovaSpacecraftMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaSpacecraftMovementComponent, MovementCommand);
	DOREPLIFETIME(UNovaSpacecraftMovementComponent, AttitudeCommand);
	DOREPLIFETIME(UNovaSpacecraftMovementComponent, InitParameters);
}

#undef LOCTEXT_NAMESPACE
