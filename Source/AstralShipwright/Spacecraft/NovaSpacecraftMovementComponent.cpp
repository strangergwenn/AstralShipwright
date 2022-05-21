// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftMovementComponent.h"
#include "NovaSpacecraftPawn.h"

#include "Actor/NovaActorTools.h"
#include "Actor/NovaPlayerStart.h"

#include "Game/NovaAsteroid.h"
#include "Game/NovaGameMode.h"
#include "Game/NovaGameState.h"
#include "Game/NovaOrbitalSimulationComponent.h"

#include "Player/NovaPlayerController.h"
#include "Nova.h"

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

	, PreviousOrbitalLocation(FVector::ZeroVector)
	, CurrentOrbitalLocation(FVector::ZeroVector)

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
	// Angular defaults
	LinearDeadDistance          = 1;
	MaxLinearVelocity           = 50;
	MaxSlowLinearAcceleration   = 10;
	MaxDeltaVForThrusters       = 5;
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

	// Fetch data from the spacecraft
	const ANovaSpacecraftPawn* SpacecraftPawn = GetOwner<ANovaSpacecraftPawn>();
	NCHECK(SpacecraftPawn);
	if (SpacecraftPawn->IsSpacecraftValid())
	{
		LinearAcceleration  = SpacecraftPawn->GetPropulsionMetrics().ThrusterThrust / SpacecraftPawn->GetCurrentMass();
		AngularAcceleration = 4 * LinearAcceleration;
		NCHECK(FMath::IsFinite(LinearAcceleration));
	}

	// Initialize the movement component on server when it doesn't have a start actor
	if (GetLocalRole() == ROLE_Authority && !IsInitialized())
	{
		ANovaPlayerController* PC    = SpacecraftPawn->GetController<ANovaPlayerController>();
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
	ProcessOrbitalMovement(DeltaTime);
	ProcessLinearAttitude(DeltaTime);
	ProcessAngularAttitude(DeltaTime);
	ProcessMeasurementsAfterAttitude(DeltaTime);
	ProcessMovement(DeltaTime);
}

void UNovaSpacecraftMovementComponent::Initialize(const ANovaPlayerStart* Start)
{
	NCHECK(IsValid(Start));
	NCHECK(IsValid(UpdatedComponent));
	NCHECK(GetLocalRole() == ROLE_Authority);
	const ANovaSpacecraftPawn* SpacecraftPawn = GetOwner<ANovaSpacecraftPawn>();

	// Reset the state
	DockState.Actor = Start;
	if (SpacecraftPawn->GetPlayerState() == nullptr)
	{
		DockState.IsDocked = false;
	}
	ResetState();

	// Reset attitude
	FTransform InitialTransform = GetInitialTransform();
	UpdatedComponent->SetWorldTransform(InitialTransform);
	AttitudeCommand.Location  = InitialTransform.GetLocation();
	AttitudeCommand.Direction = InitialTransform.GetRotation().Vector();

	// Reset the attitude command
	if (DockState.IsDocked)
	{
		NLOG("UNovaSpacecraftMovementComponent::Initialize : docked at '%s'", Start ? *Start->GetName() : nullptr);

		MovementCommand = FNovaMovementCommand(ENovaMovementState::Docked);
	}
	else
	{
		const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
		NCHECK(GameState);
		const UNovaArea* CurrentArea = GameState->GetCurrentArea();
		NCHECK(CurrentArea);

		if (CurrentArea->IsInSpace)
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
		const UNovaArea* CurrentArea = GameState->GetCurrentArea();
		return IsInitialized() && GetState() == ENovaMovementState::Idle && CurrentArea && !CurrentArea->IsInSpace;
	}
	else
	{
		return false;
	}
}

bool UNovaSpacecraftMovementComponent::IsAlignedToManeuver() const
{
	if (GetNextManeuver())
	{
		const FVector ManeuverDirection = GetManeuverDirection();
		float CurrentAngle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(UpdatedComponent->GetForwardVector(), ManeuverDirection)));

		return CurrentAngle < AngularDeadDistance;
	}

	return false;
}

bool UNovaSpacecraftMovementComponent::CanManeuver() const
{
	// Fetch the trajectory state
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));
	const FNovaTrajectory* PlayerTrajectory = GameState->GetOrbitalSimulation()->GetPlayerTrajectory();

	return PlayerTrajectory && GetState() == ENovaMovementState::Idle && IsAlignedToManeuver();
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

void UNovaSpacecraftMovementComponent::StartOrbiting()
{
	NLOG("UNovaSpacecraftMovementComponent::StartOrbiting ('%s')", *GetRoleString(this));

	CompletionCallback = FSimpleDelegate();
	RequestMovement(FNovaMovementCommand(ENovaMovementState::Orbiting));
}

void UNovaSpacecraftMovementComponent::StopOrbiting(FSimpleDelegate Callback)
{
	NLOG("UNovaSpacecraftMovementComponent::StopOrbiting ('%s')", *GetRoleString(this));

	CompletionCallback = Callback;
	RequestMovement(FNovaMovementCommand(ENovaMovementState::ExitingOrbit));
}

void UNovaSpacecraftMovementComponent::Anchor(FSimpleDelegate Callback)
{
	NLOG("UNovaSpacecraftMovementComponent::Anchor ('%s')", *GetRoleString(this));

	CompletionCallback = Callback;
	RequestMovement(FNovaMovementCommand(ENovaMovementState::Anchoring));
}

void UNovaSpacecraftMovementComponent::ExitAnchor(FSimpleDelegate Callback)
{
	NLOG("UNovaSpacecraftMovementComponent::ExitAnchor ('%s')", *GetRoleString(this));

	CompletionCallback = Callback;
	RequestMovement(FNovaMovementCommand(ENovaMovementState::ExitingAnchor));
}

/*----------------------------------------------------
High level movement
----------------------------------------------------*/

void UNovaSpacecraftMovementComponent::ProcessState()
{
	// Get helpful data
	TArray<AActor*> Asteroids;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaAsteroid::StaticClass(), Asteroids);
	const FVector WaitingPointLocation     = IsValid(DockState.Actor) ? DockState.Actor->GetWaitingPointLocation() : FVector::ZeroVector;
	const FVector AsteroidLocation         = Asteroids.Num() ? Asteroids[0]->GetActorLocation() : FVector::ZeroVector;
	const FVector CurrentLocation          = UpdatedComponent->GetComponentLocation();
	const FVector AsteroidRelativeLocation = CurrentLocation - AsteroidLocation;
	const FVector AsteroidRelativeWaitingPointLocation = WaitingPointLocation - AsteroidLocation;

	switch (MovementCommand.State)
	{
		// Default state
		case ENovaMovementState::Idle:
			AttitudeCommand.Direction = GetManeuverDirection();
			break;

		// Idle states
		case ENovaMovementState::Docked:
		case ENovaMovementState::Anchored:
			break;

		// Docking procedure
		case ENovaMovementState::Docking:
			DockState.IsDocked       = true;
			AttitudeCommand.Velocity = FVector::ZeroVector;
			if (!IsValid(MovementCommand.Target))
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Docking : aborted");

				MovementCommand.State = ENovaMovementState::Idle;
			}
			else if (MovementCommand.Dirty)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Docking : starting");

				AttitudeCommand.Location  = MovementCommand.Target->GetActorLocation() - CurrentOrbitalLocation;
				AttitudeCommand.Direction = FVector(1, 0, 0);
				AttitudeCommand.Velocity  = FVector::ZeroVector;
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
			AttitudeCommand.Velocity = FVector::ZeroVector;
			DockState.IsDocked       = false;
			if (MovementCommand.Dirty)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Undocking : starting");

				AttitudeCommand.Location = WaitingPointLocation - CurrentOrbitalLocation;
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

		// Orbiting asteroid
		case ENovaMovementState::Orbiting:
			AttitudeCommand.Direction  = FVector::CrossProduct(AsteroidRelativeLocation.GetSafeNormal(), FVector(0, 0, 1));
			AttitudeCommand.Velocity   = AttitudeCommand.Direction * 20;
			AttitudeCommand.Location   = CurrentLocation + AttitudeCommand.Direction * 20;
			AttitudeCommand.Location.Z = WaitingPointLocation.Z;
			AttitudeCommand.Location -= CurrentOrbitalLocation;
			break;

		// Existing an asteroid orbit
		case ENovaMovementState::ExitingOrbit: {
			AttitudeCommand.Velocity    = FVector::ZeroVector;
			const FVector ExitDirection = AsteroidRelativeWaitingPointLocation.GetSafeNormal();
			const FVector ShipDirection = AsteroidRelativeLocation.GetSafeNormal();

			// Simple direct exit
			if (FVector::DotProduct(ExitDirection, ShipDirection) > 0)
			{
				if (MovementCommand.Dirty)
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : ExitingOrbit : direct exit");

					AttitudeCommand.Location = WaitingPointLocation;
				}
				else if (LinearAttitudeIdle && AngularAttitudeIdle)
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : ExitingOrbit : done");

					MovementCommand.State = ENovaMovementState::Idle;

					SignalCompletion();
				}
			}

			// Asteroid-avoiding exit
			else
			{
				// Compute avoidance point
				const TArray<FVector> AsteroidAvoidancePoints = {
					FQuat(FVector(0, 0, 1), PI / 2).RotateVector(AsteroidRelativeWaitingPointLocation),
					FQuat(FVector(0, 0, 1), -PI / 2).RotateVector(AsteroidRelativeWaitingPointLocation)};
				int32 AvoidanceIndex = (AsteroidAvoidancePoints[1] - AsteroidRelativeLocation).Size() >
				                               (AsteroidAvoidancePoints[0] - AsteroidRelativeLocation).Size()
				                         ? 0
				                         : 1;

				if (MovementCommand.Dirty)
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : ExitingOrbit : asteroid-avoiding exit");

					AttitudeCommand.Location = AsteroidAvoidancePoints[AvoidanceIndex];
				}
				else if (LinearAttitudeDistance < 50 && AttitudeCommand.Location != WaitingPointLocation)
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : ExitingOrbit : asteroid avoided");

					AttitudeCommand.Location = WaitingPointLocation;
				}
				else if (LinearAttitudeIdle && AngularAttitudeIdle)
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : ExitingOrbit : done");

					MovementCommand.State = ENovaMovementState::Idle;

					SignalCompletion();
				}
			}
		}
		break;

		// Anchoring to asteroid
		case ENovaMovementState::Anchoring:
			break;

		// Existing an asteroid anchor
		case ENovaMovementState::ExitingAnchor:
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
	NLOG("UNovaSpacecraftMovementComponent::OnDockStateReplicated : resetting to '%s' (%d), IsDocked %d",
		DockState.Actor ? *DockState.Actor->GetName() : TEXT("nullptr"), DockState.Actor != PreviousDockState.Actor, DockState.IsDocked);

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
	CurrentOrbitalLocation      = FVector::ZeroVector;
	PreviousOrbitalLocation     = FVector::ZeroVector;
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
	LinearAttitudeDistance = UNovaActorTools::SolveVelocity(CurrentLinearVelocity, AttitudeCommand.Velocity,
		(UpdatedComponent->GetComponentLocation() - CurrentOrbitalLocation) / 100.0, AttitudeCommand.Location / 100.0,
		GetMaximumAcceleration(), MaxLinearVelocity, LinearDeadDistance, DeltaTime);
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
		// if (FMath::IsNearlyZero(AttitudeCommand.Direction.Z))
		//{
		//	// Roll angle of the final resting rotation around the desired direction
		//	auto GetRollAngle = [&](const FQuat& Rotation)
		//	{
		//		FQuat       Swing, Twist;
		//		const FQuat ResultingOrientation = Rotation.GetNormalized() * UpdatedComponent->GetComponentQuat();
		//		ResultingOrientation.ToSwingTwist(AttitudeCommand.Direction, Swing, Twist);
		//		return Twist.GetAngle();
		//	};

		//	// Extract the roll angle, build a correction, test it and apply the one that works
		//	const float DesiredRoll         = FMath::DegreesToRadians(AttitudeCommand.Roll);
		//	const float ActorRollRadians    = GetRollAngle(TargetRotation);
		//	FQuat       FixedTargetRotation = FQuat(AttitudeCommand.Direction, DesiredRoll + ActorRollRadians) * TargetRotation;
		//	if (GetRollAngle(FixedTargetRotation) > ActorRollRadians)
		//	{
		//		FixedTargetRotation = FQuat(AttitudeCommand.Direction, DesiredRoll - ActorRollRadians) * TargetRotation;
		//	}
		//	TargetRotation = FixedTargetRotation;
		//}

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
	auto UpdateVelocity = [](double& Value, double Target, double MaxDelta)
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
		FVector    ActorTranslation = (CurrentOrbitalLocation - PreviousOrbitalLocation) + CurrentLinearVelocity * 100 * DeltaTime;
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

void UNovaSpacecraftMovementComponent::ProcessOrbitalMovement(float DeltaTime)
{
	// Get basic game state pointers
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	const UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);
	const UNovaArea* CurrentArea = GameState->GetCurrentArea();
	NCHECK(CurrentArea);
	const ANovaSpacecraftPawn* SpacecraftPawn = GetOwner<ANovaSpacecraftPawn>();
	NCHECK(SpacecraftPawn);

	// Derive movement from the location of all bodies
	if (DockState.Actor)
	{
		const FVector2D PlayerLocation     = OrbitalSimulation->GetPlayerCartesianLocation();
		const FVector2D SpacecraftLocation = OrbitalSimulation->GetSpacecraftCartesianLocation(SpacecraftPawn->GetSpacecraftIdentifier());

		// Get the relative orbital location
		FVector2D LocationInKilometers;
		if (!CurrentArea->IsInSpace)
		{
			const FVector2D AreaLocation = OrbitalSimulation->GetAreaLocation(CurrentArea).GetCartesianLocation<true>();

			LocationInKilometers = SpacecraftLocation - AreaLocation;
		}
		else
		{
			LocationInKilometers = SpacecraftLocation - PlayerLocation;
		}

		// Transform the location accounting for angle and scale
		const FVector2D PlayerDirection       = PlayerLocation.GetSafeNormal();
		double          PlayerAngle           = 180 + FMath::RadiansToDegrees(FMath::Atan2(PlayerDirection.X, PlayerDirection.Y));
		LocationInKilometers                  = LocationInKilometers.GetRotated(PlayerAngle);
		const FVector RelativeOrbitalLocation = FVector(0, -LocationInKilometers.X, LocationInKilometers.Y) * 1000 * 100;

		// Derive the required translation from the previous state
		PreviousOrbitalLocation = CurrentOrbitalLocation;
		CurrentOrbitalLocation  = RelativeOrbitalLocation;
	}
}

void UNovaSpacecraftMovementComponent::OnHit(const FHitResult& Hit, const FVector& HitVelocity)
{}

const FNovaManeuver* UNovaSpacecraftMovementComponent::GetNextManeuver() const
{
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));
	const ANovaSpacecraftPawn* SpacecraftPawn = GetOwner<ANovaSpacecraftPawn>();
	NCHECK(IsValid(SpacecraftPawn));

	const FNovaTrajectory* Trajectory =
		GameState->GetOrbitalSimulation()->GetSpacecraftTrajectory(SpacecraftPawn->GetSpacecraftIdentifier());
	return Trajectory ? Trajectory->GetNextManeuver(GameState->GetCurrentTime()) : nullptr;
}

FVector UNovaSpacecraftMovementComponent::GetManeuverDirection() const
{
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));
	const ANovaSpacecraftPawn* SpacecraftPawn = GetOwner<ANovaSpacecraftPawn>();
	NCHECK(IsValid(SpacecraftPawn));

	const FNovaTrajectory* Trajectory =
		GameState->GetOrbitalSimulation()->GetSpacecraftTrajectory(SpacecraftPawn->GetSpacecraftIdentifier());

	const FNovaManeuver* Maneuver = Trajectory ? Trajectory->GetManeuver(GameState->GetCurrentTime()) : nullptr;
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
	FTransform InitialTransform = UpdatedComponent->GetComponentTransform();

	const ANovaSpacecraftPawn* SpacecraftPawn = GetOwner<ANovaSpacecraftPawn>();

	// AI (start at the dock interface)
	if (IsValid(SpacecraftPawn) && SpacecraftPawn->GetPlayerState() == nullptr)
	{
		NLOG("UNovaSpacecraftMovementComponent::GetInitialTransform : AI");
		InitialTransform.SetLocation(DockState.Actor->GetWaitingPointLocation());
	}

	// Start docked
	else if (DockState.IsDocked)
	{
		NLOG("UNovaSpacecraftMovementComponent::GetInitialTransform : docked");
		InitialTransform = DockState.Actor->GetActorTransform();
	}

	// Start at the dock interface
	else
	{
		NLOG("UNovaSpacecraftMovementComponent::GetInitialTransform : at waiting point");
		InitialTransform.SetLocation(DockState.Actor->GetWaitingPointLocation());
	}

	return InitialTransform;
}

void UNovaSpacecraftMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaSpacecraftMovementComponent, MovementCommand);
	DOREPLIFETIME(UNovaSpacecraftMovementComponent, AttitudeCommand);
	DOREPLIFETIME(UNovaSpacecraftMovementComponent, DockState);
}

/*----------------------------------------------------
    Getters
----------------------------------------------------*/

const FVector UNovaSpacecraftMovementComponent::GetThrusterAcceleration() const
{
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));

	FVector Acceleration = MeasuredAcceleration;

	const FNovaTrajectory* Trajectory = GameState->GetOrbitalSimulation()->GetPlayerTrajectory();
	if (Trajectory)
	{
		const FNovaManeuver* CurrentManeuver = Trajectory->GetManeuver(GameState->GetCurrentTime());
		if (CurrentManeuver && CurrentManeuver->DeltaV < MaxDeltaVForThrusters)
		{
			Acceleration += GetManeuverDirection() * LinearAcceleration;
		}
	}

	return Acceleration;
}

#undef LOCTEXT_NAMESPACE
