// Nova project - Gwennaël Arbona

#include "NovaSpacecraftMovementComponent.h"

#include "Nova/Nova.h"

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
	LinearAcceleration = 10;
	LinearDeadDistance = 1;

	// Angular defaults
	AngularAcceleration = 30;
	AngularControlIntensity = 2.0f;
	MaxLinearVelocity = 8;
	MaxAngularVelocity = 60;
	AngularOvershootRatio = 1.1f;
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

void UNovaSpacecraftMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	MovementCommand.State = ENovaMovementState::Docked;
}

void UNovaSpacecraftMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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
	int32 TestDuration = 4 * TestStepDuration;
	if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < TestStepDuration)
	{
		DesiredAttitude.Direction = FVector(0, 0, 1).GetSafeNormal();
	}
	else if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < 2 * TestStepDuration)
	{
		DesiredAttitude.Direction = FVector(0, 1, 0).GetSafeNormal();
	}
	else if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < 3 * TestStepDuration)
	{
		DesiredAttitude.Direction = FVector(0, -1, 0).GetSafeNormal();
	}
	else
	{
		DesiredAttitude.Direction = FVector(1, 1, 1).GetSafeNormal();
	}
#endif

	// Run high-level processing
	if (GetLocalRole() == ROLE_Authority)
	{
		ProcessState();
	}

	// Run movement implementation
	ProcessMeasurements(DeltaTime);
	ProcessLinearAttitude(DeltaTime);
	ProcessAngularAttitude(DeltaTime);
	ProcessMovement(DeltaTime);

	// Check the callback
	if (MovementCommand.State == ENovaMovementState::Docked || MovementCommand.State == ENovaMovementState::Idle)
	{
		IdleCallback.ExecuteIfBound();
		IdleCallback.Unbind();
	}
}

void UNovaSpacecraftMovementComponent::Dock(const class AActor* Target, FSimpleDelegate Callback)
{
	NLOG("UNovaSpacecraftMovementComponent::Dock");

	if (MovementCommand.State == ENovaMovementState::Idle)
	{
		IdleCallback = Callback;
		RequestMovement(FNovaMovementCommand(ENovaMovementState::Docking, Target));
	}
}

void UNovaSpacecraftMovementComponent::Undock(FSimpleDelegate Callback)
{
	NLOG("UNovaSpacecraftMovementComponent::Undock");

	if (MovementCommand.State == ENovaMovementState::Docked)
	{
		IdleCallback = Callback;
		RequestMovement(FNovaMovementCommand(ENovaMovementState::Undocking));
	}
}


/*----------------------------------------------------
	High level movement
----------------------------------------------------*/

void UNovaSpacecraftMovementComponent::ProcessState()
{
	switch (MovementCommand.State)
	{

	// Docking procedure
	case ENovaMovementState::Docking:
		if (!IsValid(MovementCommand.Target))
		{
			MovementCommand.State = ENovaMovementState::Idle;
		}
		else if (MovementCommand.Dirty)
		{
			DesiredAttitude.Location = MovementCommand.Target->GetActorLocation();
			DesiredAttitude.Direction = FVector(1, 0, 0);
		}
		else if (LinearAttitudeIdle && AngularAttitudeIdle)
		{
			MovementCommand.State = ENovaMovementState::Docked;
		}
		break;

	// Undocking procedure
	case ENovaMovementState::Undocking:
		if (MovementCommand.Dirty)
		{
			DesiredAttitude.Location = GetLocation(FVector(100, 0, 0));
		}
		else if (LinearAttitudeIdle && AngularAttitudeIdle)
		{
			MovementCommand.State = ENovaMovementState::Idle;
		}
		else if (LinearAttitudeDistance < 50)
		{
			DesiredAttitude.Direction = FVector(0, 1, 0);
		}
		break;

	default:
		break;
	}

	MovementCommand.Dirty = false;
}


/*----------------------------------------------------
	Networking
----------------------------------------------------*/

void UNovaSpacecraftMovementComponent::RequestMovement(const FNovaMovementCommand& Command)
{
	NLOG("UNovaSpacecraftMovementComponent::RequestMovement ('%s')", *GetRoleString(this));

	MovementCommand = Command;

	if (GetLocalRole() == ROLE_AutonomousProxy)
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

void UNovaSpacecraftMovementComponent::ProcessMeasurements(float DeltaTime)
{
	MeasuredAcceleration = ((CurrentLinearVelocity - PreviousVelocity) / DeltaTime);
	MeasuredAngularAcceleration = (CurrentAngularVelocity - PreviousAngularVelocity) * DeltaTime;
	PreviousVelocity = CurrentLinearVelocity;
	PreviousAngularVelocity = CurrentAngularVelocity;
}

void UNovaSpacecraftMovementComponent::ProcessLinearAttitude(float DeltaTime)
{
	// Get the position data
	const FVector DeltaPosition = (DesiredAttitude.Location - UpdatedComponent->GetComponentLocation()) / 100.0f;
	const FVector DeltaPositionDirection = DeltaPosition.GetSafeNormal();
	LinearAttitudeDistance = FMath::Max(0.0f, DeltaPosition.Size() - LinearDeadDistance);
	LinearAttitudeIdle = LinearAttitudeDistance == 0;

	// Get the velocity data
	FVector DeltaVelocity = DesiredAttitude.Velocity - CurrentLinearVelocity;
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
	float DistanceToStop = (DeltaVelocity.Size() / 2) * (TimeToFinalVelocity + DeltaTime);
	if (DistanceToStop > LinearAttitudeDistance)
	{
		RelativeResultSpeed = DesiredAttitude.Velocity;
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
		RelativeResultSpeed += DesiredAttitude.Velocity;
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
	FVector NewAngularVelocity = FVector::ZeroVector;
	const FVector ActorAxis = GetOwner()->GetActorForwardVector();
	const float DotProduct = FVector::DotProduct(ActorAxis, DesiredAttitude.Direction);

	if (DotProduct < AngularColinearityThreshold)
	{
		AngularAttitudeIdle = false;

		// Determine a quaternion that represents the desired difference in orientation
		FQuat TargetRotation = DotProduct > -AngularColinearityThreshold ?
			FQuat::FindBetweenNormals(ActorAxis, DesiredAttitude.Direction) :
			FRotator(0, 180, 0).Quaternion();

		// While on the horizontal plane, follow desired roll too
		if (FMath::IsNearlyZero(DesiredAttitude.Direction.Z))
		{
			// Roll angle of the final resting rotation around the desired direction
			auto GetRollAngle = [=](const FQuat& Rotation)
			{
				FQuat Swing, Twist;
				const FQuat ResultingOrientation = Rotation.GetNormalized() * UpdatedComponent->GetComponentQuat();
				ResultingOrientation.ToSwingTwist(DesiredAttitude.Direction, Swing, Twist);
				return Twist.GetAngle();
			};

			// Extract the roll angle, build a correction, test it and apply the one that works
			const float DesiredRoll = FMath::DegreesToRadians(DesiredAttitude.Roll);
			const float ActorRollRadians = GetRollAngle(TargetRotation);
			FQuat FixedTargetRotation = FQuat(DesiredAttitude.Direction, DesiredRoll + ActorRollRadians) * TargetRotation;
			if (GetRollAngle(FixedTargetRotation) > ActorRollRadians)
			{
				FixedTargetRotation = FQuat(DesiredAttitude.Direction, DesiredRoll - ActorRollRadians) * TargetRotation;
			}
			TargetRotation = FixedTargetRotation;
		}

		// Extract the rotation axis and angle
		FVector RotationDirection;
		float RemainingAngleRadians;
		TargetRotation.ToAxisAndAngle(RotationDirection, RemainingAngleRadians);
		float RemainingAngle = FMath::RadiansToDegrees(RemainingAngleRadians);

		// This cannot be explained, but likely fixes inconsistent output by FindBetweenNormals
		RotationDirection *= FVector(-1, -1, 1);

		// Determine the time left to reach the final (zero) velocity
		float TimeToFinalVelocity = 0;
		const FVector AngularVelocityDelta = -CurrentAngularVelocity;
		if (!FMath::IsNearlyZero(AngularVelocityDelta.SizeSquared()))
		{
			FVector Acceleration = AngularVelocityDelta.GetSafeNormal() * AngularAcceleration;
			float AccelerationInAngleAxis = FMath::Abs(FVector::DotProduct(Acceleration, RotationDirection));
			TimeToFinalVelocity = (AngularVelocityDelta.Size() / AccelerationInAngleAxis);
		}

		// Determine the new angular velocity based on the remaining angle and velocity
		float AngularStoppingDistance = (AngularVelocityDelta.Size() / 2) * (TimeToFinalVelocity + DeltaTime) / AngularOvershootRatio;
		if (!FMath::IsNearlyZero(RemainingAngle) && !(FMath::Abs(RemainingAngle) < AngularStoppingDistance))
		{
			float OvershotRemainingAngle = AngularOvershootRatio * (RemainingAngle - AngularStoppingDistance);
			float MaxUsefulAngularVelocity = FMath::Min(OvershotRemainingAngle / DeltaTime, MaxAngularVelocity);
			NewAngularVelocity = RotationDirection * MaxUsefulAngularVelocity;
		}

		/*NLOG("ActAx = [%.2f %.2f %.2f] -> TrgAx = [%.2f %.2f %.2f] = RotDir = [%.2f %.2f %.2f] / Angle = %.1f, Dot %.4f / NewVel = [%.2f %.2f %.2f]",
			ActorAxis.X, ActorAxis.Y, ActorAxis.Z,
			CurrentDesiredAttitude.Direction.X, CurrentDesiredAttitude.Direction.Y, CurrentDesiredAttitude.Direction.Z,
			RotationDirection.X, RotationDirection.Y, RotationDirection.Z,
			RemainingAngle, DotProduct,
			NewAngularVelocity.X, NewAngularVelocity.Y, NewAngularVelocity.Z);*/
	}
	else
	{
		AngularAttitudeIdle = true;
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
		CurrentLinearVelocity = CurrentLinearVelocity.GetClampedToMaxSize(MaxLinearVelocity);
		CurrentAngularVelocity = CurrentAngularVelocity.GetClampedToMaxSize(MaxAngularVelocity);

		// Update rotation
		FQuat ActorRotation = UpdatedComponent->GetComponentQuat();
		FQuat EffectiveRotationDelta = (FRotator(CurrentAngularVelocity.Y, CurrentAngularVelocity.Z, CurrentAngularVelocity.X) * DeltaTime).Quaternion();
		ActorRotation = (EffectiveRotationDelta * ActorRotation).GetNormalized();

		// Move safely and ensure de-penetration if required
		FHitResult Hit;
		FVector ActorTranslation = CurrentLinearVelocity * 100 * DeltaTime;
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
			ActorTranslation = CurrentLinearVelocity * 100 * DeltaTime;
			SlideAlongSurface(ActorTranslation, 1 - Hit.Time, Hit.Normal, Hit);
		}
	}
}

void UNovaSpacecraftMovementComponent::OnHit(const FHitResult& Hit, const FVector& HitVelocity)
{
}

void UNovaSpacecraftMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaSpacecraftMovementComponent, MovementCommand);
	DOREPLIFETIME(UNovaSpacecraftMovementComponent, DesiredAttitude);
}


#undef LOCTEXT_NAMESPACE
