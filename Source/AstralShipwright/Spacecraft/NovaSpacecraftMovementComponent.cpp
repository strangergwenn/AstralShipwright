// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftMovementComponent.h"
#include "NovaSpacecraftHatchComponent.h"
#include "NovaSpacecraftPawn.h"

#include "Game/NovaAsteroid.h"
#include "Game/NovaGameMode.h"
#include "Game/NovaGameState.h"
#include "Game/NovaPlayerStart.h"
#include "Game/NovaOrbitalSimulationComponent.h"

#include "Player/NovaPlayerController.h"
#include "Nova.h"

#include "Neutron/Actor/NeutronActorTools.h"

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
	LinearDeadDistance    = 0.5;
	MaxLinearVelocity     = 50;
	MinLinearAcceleration = 10;
	MaxLinearAcceleration = 50;
	MaxDeltaVForThrusters = 5;
	AngularDeadDistance   = 0.5f;
	MaxAngularVelocity    = 60;
	AngularOvershootRatio = 1.1f;

	// Physics defaults
	RestitutionCoefficient = 0.5f;

	// High level defaults
	OrbitingAccelerationTime   = 2.0;
	OrbitingMaxAngularVelocity = 3;
	OrbitingDistance           = 20000;

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
		LinearAcceleration = SpacecraftPawn->GetPropulsionMetrics().ThrusterThrust / SpacecraftPawn->GetCurrentMass();
		NCHECK(LinearAcceleration > 0 && FMath::IsFinite(LinearAcceleration));
		LinearAcceleration = FMath::Clamp(LinearAcceleration, MinLinearAcceleration, MaxLinearAcceleration);

		AngularAcceleration = 4 * LinearAcceleration;
	}

	// Initialize the movement component on server when it doesn't have a start actor
	if (GetLocalRole() == ROLE_Authority && !IsInitialized())
	{
		ANovaPlayerController*  PC    = SpacecraftPawn->GetController<ANovaPlayerController>();
		const ANovaPlayerStart* Start = Cast<ANovaPlayerStart>(GetWorld()->GetAuthGameMode<ANovaGameMode>()->ChoosePlayerStart(PC));

		if (IsValid(Start))
		{
			Initialize(Start);
		}
	}

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
	NCHECK(SpacecraftPawn);

	// Reset the state
	DockState.Actor = Start;
	if (SpacecraftPawn->GetPlayerState() == nullptr)
	{
		DockState.IsDocked = false;
	}
	ResetState();

	// Reset attitude
	const FTransform InitialTransform = GetInitialTransform();
	UpdatedComponent->SetWorldTransform(InitialTransform);
	AttitudeCommand.Location    = InitialTransform.GetLocation();
	AttitudeCommand.Orientation = InitialTransform.GetRotation();

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
		const double  CurrentAngle =
			FMath::RadiansToDegrees(acosf(FVector::DotProduct(UpdatedComponent->GetForwardVector(), ManeuverDirection)));

		return CurrentAngle < AngularDeadDistance && CurrentAngularVelocity.Size() < KINDA_SMALL_NUMBER;
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
	RequestMovement(FNovaMovementCommand(ENovaMovementState::DockingPhase1, DockState.Actor));
}

void UNovaSpacecraftMovementComponent::Undock(FSimpleDelegate Callback)
{
	NLOG("UNovaSpacecraftMovementComponent::Undock ('%s')", *GetRoleString(this));

	CompletionCallback = Callback;
	RequestMovement(FNovaMovementCommand(ENovaMovementState::UndockingPhase1));
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
	RequestMovement(FNovaMovementCommand(ENovaMovementState::AnchoringEntry));
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
	const FVector WaitingPointLocation = IsValid(DockState.Actor) ? DockState.Actor->GetWaitingPointLocation() : FVector::ZeroVector;
	const FVector StartPoint           = IsValid(DockState.Actor) ? DockState.Actor->GetActorLocation() : FVector::ZeroVector;
	const FVector DockingMidpoint      = FVector((WaitingPointLocation - StartPoint).X / 2.0, StartPoint.Y, StartPoint.Z);
	const FVector CurrentLocation      = UpdatedComponent->GetComponentLocation();

	// Get asteroid data
	const ANovaAsteroid* Asteroid                             = UNeutronActorTools::GetClosestActor<ANovaAsteroid>(this, CurrentLocation);
	const FVector        AsteroidLocation                     = IsValid(Asteroid) ? Asteroid->GetActorLocation() : FVector::ZeroVector;
	const FVector        AsteroidRelativeLocation             = CurrentLocation - AsteroidLocation;
	const FVector        AsteroidRelativeWaitingPointLocation = WaitingPointLocation - AsteroidLocation;

	switch (MovementCommand.State)
	{
		// Default state
		case ENovaMovementState::Idle:
			AttitudeCommand.Orientation = GetManeuverDirection().ToOrientationQuat();
			break;

		// Idle states
		case ENovaMovementState::Docked:
		case ENovaMovementState::Anchored:
			break;

		// Docking procedure 1
		case ENovaMovementState::DockingPhase1:
			DockState.IsDocked = true;
			if (!IsValid(MovementCommand.Target))
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : DockingPhase1 : aborted");

				MovementCommand.State    = ENovaMovementState::Idle;
				AttitudeCommand.Velocity = FVector::ZeroVector;
			}
			else if (MovementCommand.Dirty)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : DockingPhase1 : starting");

				AttitudeCommand.Location    = DockingMidpoint - CurrentOrbitalLocation;
				AttitudeCommand.Orientation = FVector(1, 0, 0).ToOrientationQuat();
				AttitudeCommand.Velocity    = FVector(-0.1 * MaxLinearVelocity, 0, 0);
			}
			else if (LinearAttitudeDistance < 10)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : DockingPhase1 : done");

				AttitudeCommand.Location = StartPoint - CurrentOrbitalLocation;
				AttitudeCommand.Velocity = FVector::ZeroVector;

				MovementCommand.State = ENovaMovementState::DockingPhase2;
			}
			break;

		// Docking procedure 2
		case ENovaMovementState::DockingPhase2:
			DockState.IsDocked       = true;
			AttitudeCommand.Velocity = FVector::ZeroVector;
			if (LinearAttitudeIdle && AngularAttitudeIdle)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : DockingPhase2 : done");

				MovementCommand.State = ENovaMovementState::Docked;
				SignalCompletion();
			}
			break;

		// Undocking procedure 1
		case ENovaMovementState::UndockingPhase1:
			AttitudeCommand.Velocity = FVector::ZeroVector;
			DockState.IsDocked       = false;
			if (MovementCommand.Dirty)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : UndockingPhase1 : starting");

				AttitudeCommand.Location = DockingMidpoint - CurrentOrbitalLocation;
				AttitudeCommand.Velocity = FVector(0.1 * MaxLinearVelocity, 0, 0);
			}
			else if (LinearAttitudeDistance < 10)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : UndockingPhase1 : done");

				AttitudeCommand.Location    = WaitingPointLocation - CurrentOrbitalLocation;
				AttitudeCommand.Orientation = FVector(1, 0, 0).ToOrientationQuat();
				AttitudeCommand.Velocity    = FVector::ZeroVector;

				MovementCommand.State = ENovaMovementState::UndockingPhase2;
			}
			break;

		// Undocking procedure 2
		case ENovaMovementState::UndockingPhase2:
			AttitudeCommand.Velocity = FVector::ZeroVector;
			DockState.IsDocked       = false;
			if (LinearAttitudeIdle && AngularAttitudeIdle)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : UndockingPhase2 : done");

				MovementCommand.State = ENovaMovementState::Idle;
				SignalCompletion();
			}
			break;

		// Orbiting asteroid
		case ENovaMovementState::Orbiting: {

			if (MovementCommand.Dirty)
			{
				InitialOrbitingTime    = GetWorld()->GetTimeSeconds();
				InitialOrbitingHeading = AsteroidRelativeLocation.HeadingAngle();
			}

			// Define the maximum allowed angular velocity to ensure reasonable acceleration
			const double MaxReasonableVelocityDelta = LinearAcceleration * OrbitingAccelerationTime;
			const double DesiredAngularVelocity =
				FMath::Min(MaxReasonableVelocityDelta / (OrbitingDistance / 100), FMath::DegreesToRadians(OrbitingMaxAngularVelocity));

			// Define the current heading target
			const double CurrentOrbitingHeading =
				InitialOrbitingHeading + DesiredAngularVelocity * (GetWorld()->GetTimeSeconds() - InitialOrbitingTime);

			// Proceed with simple orbiting math
			AttitudeCommand.Orientation = FQuat(FVector::UpVector, CurrentOrbitingHeading + DOUBLE_PI / 2).GetNormalized();
			AttitudeCommand.Location =
				AsteroidLocation + FQuat(FVector::UpVector, CurrentOrbitingHeading).RotateVector(FVector(OrbitingDistance, 0, 0));
			AttitudeCommand.Location.Z = WaitingPointLocation.Z;
			AttitudeCommand.Location -= CurrentOrbitalLocation;
			AttitudeCommand.Velocity = FVector::ZeroVector;
		}
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

					AttitudeCommand.Location = WaitingPointLocation - CurrentOrbitalLocation;
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
					FQuat(FVector::UpVector, DOUBLE_PI / 2).RotateVector(AsteroidRelativeWaitingPointLocation),
					FQuat(FVector::UpVector, -DOUBLE_PI / 2).RotateVector(AsteroidRelativeWaitingPointLocation)};
				const int32 AvoidanceIndex = (AsteroidAvoidancePoints[1] - AsteroidRelativeLocation).Size() >
				                                     (AsteroidAvoidancePoints[0] - AsteroidRelativeLocation).Size()
				                               ? 0
				                               : 1;

				if (MovementCommand.Dirty)
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : ExitingOrbit : asteroid-avoiding exit");

					AttitudeCommand.Location = AsteroidAvoidancePoints[AvoidanceIndex] - CurrentOrbitalLocation;
				}
				else if (LinearAttitudeDistance < 50 && AttitudeCommand.Location != WaitingPointLocation)
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : ExitingOrbit : asteroid avoided");

					AttitudeCommand.Location = WaitingPointLocation - CurrentOrbitalLocation;
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

		// Preparing to anchor to asteroid
		case ENovaMovementState::AnchoringEntry:
			AttitudeCommand.Velocity = FVector::ZeroVector;
			if (LinearAttitudeIdle && AngularAttitudeIdle)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : AnchoringEntry : done");

				// Trace params
				FHitResult            HitResult(ForceInit);
				FCollisionQueryParams TraceParams(FName(TEXT("Anchor Trace")), false, NULL);
				TraceParams.bTraceComplex           = true;
				TraceParams.bReturnPhysicalMaterial = false;
				TraceParams.AddIgnoredActor(GetOwner());
				ECollisionChannel CollisionChannel = ECollisionChannel::ECC_WorldDynamic;

				// Trace
				bool FoundHit =
					GetWorld()->LineTraceSingleByChannel(HitResult, CurrentLocation, AsteroidLocation, CollisionChannel, TraceParams);
				if (FoundHit && HitResult.GetActor() == Asteroid)
				{
					// Get spacecraft pointers
					const ANovaSpacecraftPawn* SpacecraftPawn = GetOwner<ANovaSpacecraftPawn>();
					NCHECK(SpacecraftPawn);
					const UPrimitiveComponent* AnchorComponent = SpacecraftPawn->GetAnchorComponent();
					NCHECK(IsValid(AnchorComponent));

					// Compute orientation
					const FQuat   HitPointOrientation = (HitResult.Location - AsteroidLocation).GetSafeNormal().ToOrientationQuat();
					const FVector RelativeDockDirection =
						GetOwner()->GetTransform().InverseTransformVector(-AnchorComponent->GetForwardVector());
					const FQuat RelativeAnchorOrientation = RelativeDockDirection.ToOrientationQuat();
					AttitudeCommand.Orientation           = HitPointOrientation * RelativeAnchorOrientation;

					// Compute location
					const FVector RelativeDockLocation =
						SpacecraftPawn->GetTransform().InverseTransformPosition(AnchorComponent->GetSocketLocation("Dock"));
					AttitudeCommand.Location =
						HitResult.Location - AttitudeCommand.Orientation.RotateVector(RelativeDockLocation) - CurrentOrbitalLocation;

					MovementCommand.State = ENovaMovementState::Anchoring;
				}
				else
				{
					NLOG("UNovaSpacecraftMovementComponent::ProcessState : Anchoring : failed");

					MovementCommand.State = ENovaMovementState::Orbiting;
				}

				SignalCompletion();
			}
			break;

		// Anchoring to asteroid
		case ENovaMovementState::Anchoring:
			AttitudeCommand.Velocity = FVector::ZeroVector;
			if (LinearAttitudeIdle && AngularAttitudeIdle)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Anchoring : done");

				MovementCommand.State = ENovaMovementState::Anchored;
				SignalCompletion();
			}

			break;

		// Existing an asteroid anchor
		case ENovaMovementState::ExitingAnchor:

			if (MovementCommand.Dirty)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : ExitingAnchor : starting");

				InitialOrbitingHeading = AsteroidRelativeLocation.HeadingAngle();
			}
			else if (LinearAttitudeIdle && AngularAttitudeIdle)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : ExitingAnchor : done");

				MovementCommand.State = ENovaMovementState::Idle;
				SignalCompletion();
			}
			else if (LinearAttitudeDistance < 20)
			{
				AttitudeCommand.Orientation = FQuat(FVector::UpVector, InitialOrbitingHeading + DOUBLE_PI / 2).GetNormalized();
			}

			// Define target
			AttitudeCommand.Location   = AsteroidLocation + OrbitingDistance * AsteroidRelativeLocation.GetSafeNormal();
			AttitudeCommand.Location.Z = WaitingPointLocation.Z;
			AttitudeCommand.Location -= CurrentOrbitalLocation;
			AttitudeCommand.Velocity = FVector::ZeroVector;

			break;

		// Braking to zero
		case ENovaMovementState::Stopping:
			AttitudeCommand.Velocity = FVector::ZeroVector;
			if (LinearAttitudeIdle && AngularAttitudeIdle)
			{
				NLOG("UNovaSpacecraftMovementComponent::ProcessState : Stopping : done");

				MovementCommand.State = ENovaMovementState::Idle;
				SignalCompletion();
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

		UpdatedComponent->SetWorldTransform(GetInitialTransform());

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
	LinearAttitudeDistance = UNeutronActorTools::SolveVelocity(CurrentLinearVelocity, AttitudeCommand.Velocity,
		(UpdatedComponent->GetComponentLocation() - CurrentOrbitalLocation) / 100.0, AttitudeCommand.Location / 100.0, LinearAcceleration,
		GetMaximumVelocity(), LinearDeadDistance, DeltaTime);
}

void UNovaSpacecraftMovementComponent::ProcessAngularAttitude(float DeltaTime)
{
	FVector      NewAngularVelocity = FVector::ZeroVector;
	const double AngularDistance    = UpdatedComponent->GetComponentQuat().AngularDistance(AttitudeCommand.Orientation);

	if (AngularDistance > FMath::DegreesToRadians(AngularDeadDistance))
	{
		// Determine a quaternion that represents the desired difference in orientation
		FQuat TargetRotation = AngularDistance < FMath::DegreesToRadians(180 - AngularDeadDistance)
		                         ? AttitudeCommand.Orientation * UpdatedComponent->GetComponentQuat().Inverse()
		                         : FRotator(0, 180, 0).Quaternion();

		// Extract the rotation axis and angle
		FVector RotationDirection;
		double  RemainingAngleRadians;
		TargetRotation.ToAxisAndAngle(RotationDirection, RemainingAngleRadians);
		AngularAttitudeDistance = FMath::RadiansToDegrees(RemainingAngleRadians);

		// Prevent against direction inversions
		if (AngularAttitudeDistance > 180 && AngularAttitudeDistance > AngularDistance)
		{
			AngularAttitudeDistance -= 360;
		}

		// This cannot be explained, but likely fixes inconsistent output by FindBetweenNormals
		RotationDirection *= FVector(-1, -1, 1);

		// Determine the time left to reach the final (zero) velocity
		double        TimeToFinalVelocity  = 0;
		const FVector AngularVelocityDelta = -CurrentAngularVelocity;
		if (!FMath::IsNearlyZero(AngularVelocityDelta.SizeSquared()))
		{
			const FVector Acceleration            = AngularVelocityDelta.GetSafeNormal() * AngularAcceleration;
			const double  AccelerationInAngleAxis = FMath::Abs(FVector::DotProduct(Acceleration, RotationDirection));
			TimeToFinalVelocity                   = (AngularVelocityDelta.Size() / AccelerationInAngleAxis);
		}

		// Determine the new angular velocity based on the remaining angle and velocity
		const double AngularStoppingDistance =
			(AngularVelocityDelta.Size() / 2) * (TimeToFinalVelocity + DeltaTime) / AngularOvershootRatio;
		if (!FMath::IsNearlyZero(AngularAttitudeDistance) && FMath::Abs(AngularAttitudeDistance) > AngularStoppingDistance)
		{
			const double OvershotAngularAttitudeDistance = AngularOvershootRatio * (AngularAttitudeDistance - AngularStoppingDistance);
			const double MaxUsefulAngularVelocity        = FMath::Min(OvershotAngularAttitudeDistance / DeltaTime, MaxAngularVelocity);
			NewAngularVelocity                           = RotationDirection * MaxUsefulAngularVelocity;
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

	if (Maneuver)
	{
		return ANovaPlayerStart::GetInterfacePointDirection(Maneuver->DeltaV);
	}

	return AttitudeCommand.Orientation.Vector();
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
