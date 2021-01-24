// Nova project - Gwennaël Arbona

#include "NovaSpacecraftMovementComponent.h"

#include "Nova/Nova.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"


#define LOCTEXT_NAMESPACE "UNovaSpacecraftMovementComponent"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

UNovaSpacecraftMovementComponent::UNovaSpacecraftMovementComponent()
	: Super()

	, CurrentDesiredLocation(FVector::ZeroVector)
	, CurrentDesiredVelocity(FVector::ZeroVector)
	, CurrentDesiredDirection(FVector::ZeroVector)
	, CurrentDesiredRoll(0)

	, CurrentVelocity(FVector::ZeroVector)
	, CurrentAngularVelocity(FVector::ZeroVector)

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

void UNovaSpacecraftMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if 0
	int32 TestStepDuration = 5;
	int32 TestDuration = 4 * TestStepDuration;
	CurrentDesiredVelocity = FVector::ZeroVector;
	if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < TestStepDuration)
	{
		CurrentDesiredLocation = FVector(0, 0, 1000);
	}
	else if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < 2 * TestStepDuration)
	{
		CurrentDesiredLocation = FVector(0, 1000, 0);
		CurrentDesiredVelocity = FVector(0, 1, 0);
	}
	else if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < 3 * TestStepDuration)
	{
		CurrentDesiredLocation = FVector(0, -1000, 0);
	}
	else
	{
		CurrentDesiredLocation = FVector(1000, 1000, 0);
	}
#elif 0
	int32 TestStepDuration = 10;
	int32 TestDuration = 4 * TestStepDuration;
	if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < TestStepDuration)
	{
		CurrentDesiredDirection = FVector(0, 0, 1).GetSafeNormal();
	}
	else if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < 2 * TestStepDuration)
	{
		CurrentDesiredDirection = FVector(0, 1, 0).GetSafeNormal();
	}
	else if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % TestDuration < 3 * TestStepDuration)
	{
		CurrentDesiredDirection = FVector(0, -1, 0).GetSafeNormal();
	}
	else
	{
		CurrentDesiredDirection = FVector(1, 1, 1).GetSafeNormal();
	}
#endif

	// Run all processes
	ProcessMeasurements(DeltaTime);
	ProcessLinearAttitude(DeltaTime);
	ProcessAngularAttitude(DeltaTime);
	ProcessMovement(DeltaTime);
}


/*----------------------------------------------------
	Internal movement implementation
----------------------------------------------------*/

void UNovaSpacecraftMovementComponent::ProcessMeasurements(float DeltaTime)
{
	MeasuredAcceleration = ((CurrentVelocity - PreviousVelocity) / DeltaTime);
	MeasuredAngularAcceleration = (CurrentAngularVelocity - PreviousAngularVelocity) * DeltaTime;
	PreviousVelocity = CurrentVelocity;
	PreviousAngularVelocity = CurrentAngularVelocity;
}

void UNovaSpacecraftMovementComponent::ProcessLinearAttitude(float DeltaTime)
{
	// Get the position data
	const FVector DeltaPosition = (CurrentDesiredLocation - UpdatedComponent->GetComponentLocation()) / 100.0f;
	const FVector DeltaPositionDirection = DeltaPosition.GetSafeNormal();
	const float Distance = FMath::Max(0.0f, DeltaPosition.Size() - LinearDeadDistance);

	// Get the velocity data
	FVector DeltaVelocity = CurrentDesiredVelocity - CurrentVelocity;
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
	if (DistanceToStop > Distance)
	{
		RelativeResultSpeed = CurrentDesiredVelocity;
	}
	else
	{
		float MaxPreciseSpeed = FMath::Min((Distance - DistanceToStop) / DeltaTime, MaxLinearVelocity);
		if (DistanceToStop > Distance)
		{
			MaxPreciseSpeed = FMath::Min(MaxPreciseSpeed, CurrentVelocity.Size());
		}

		RelativeResultSpeed = DeltaPositionDirection;
		RelativeResultSpeed *= MaxPreciseSpeed;
		RelativeResultSpeed += CurrentDesiredVelocity;
	}

	// Update the linear velocity based on acceleration
	auto UpdateVelocity = [](float& Value, float Target, float MaxDelta)
	{
		Value = FMath::Clamp(Target, Value - MaxDelta, Value + MaxDelta);
	};
	UpdateVelocity(CurrentVelocity.X, RelativeResultSpeed.X, LinearAcceleration * DeltaTime);
	UpdateVelocity(CurrentVelocity.Y, RelativeResultSpeed.Y, LinearAcceleration * DeltaTime);
	UpdateVelocity(CurrentVelocity.Z, RelativeResultSpeed.Z, LinearAcceleration * DeltaTime);
}

void UNovaSpacecraftMovementComponent::ProcessAngularAttitude(float DeltaTime)
{
	FVector NewAngularVelocity = FVector::ZeroVector;
	const FVector ActorAxis = GetOwner()->GetActorForwardVector();
	const float DotProduct = FVector::DotProduct(ActorAxis, CurrentDesiredDirection);

	if (DotProduct < AngularColinearityThreshold)
	{
		// Determine a quaternion that represents the desired difference in orientation
		FQuat TargetRotation = DotProduct > -AngularColinearityThreshold ?
			FQuat::FindBetweenNormals(ActorAxis, CurrentDesiredDirection) :
			FRotator(0, 180, 0).Quaternion();

		// While on the horizontal plane, follow desired roll too
		if (FMath::IsNearlyZero(CurrentDesiredDirection.Z))
		{
			// Roll angle of the final resting rotation around the desired direction
			auto GetRollAngle = [=](const FQuat& Rotation)
			{
				FQuat Swing, Twist;
				const FQuat ResultingOrientation = Rotation.GetNormalized() * UpdatedComponent->GetComponentQuat();
				ResultingOrientation.ToSwingTwist(CurrentDesiredDirection, Swing, Twist);
				return Twist.GetAngle();
			};

			// Extract the roll angle, build a correction, test it and apply the one that works
			const float DesiredRoll = FMath::DegreesToRadians(CurrentDesiredRoll);
			const float ActorRollRadians = GetRollAngle(TargetRotation);
			FQuat FixedTargetRotation = FQuat(CurrentDesiredDirection, DesiredRoll + ActorRollRadians) * TargetRotation;
			if (GetRollAngle(FixedTargetRotation) > ActorRollRadians)
			{
				FixedTargetRotation = FQuat(CurrentDesiredDirection, DesiredRoll - ActorRollRadians) * TargetRotation;
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
			CurrentDesiredDirection.X, CurrentDesiredDirection.Y, CurrentDesiredDirection.Z,
			RotationDirection.X, RotationDirection.Y, RotationDirection.Z,
			RemainingAngle, DotProduct,
			NewAngularVelocity.X, NewAngularVelocity.Y, NewAngularVelocity.Z);*/
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
		CurrentVelocity = CurrentVelocity.GetClampedToMaxSize(MaxLinearVelocity);
		CurrentAngularVelocity = CurrentAngularVelocity.GetClampedToMaxSize(MaxAngularVelocity);

		// Update rotation
		FQuat ActorRotation = UpdatedComponent->GetComponentQuat();
		FQuat EffectiveRotationDelta = (FRotator(CurrentAngularVelocity.Y, CurrentAngularVelocity.Z, CurrentAngularVelocity.X) * DeltaTime).Quaternion();
		ActorRotation = (EffectiveRotationDelta * ActorRotation).GetNormalized();

		// Move safely and ensure de-penetration if required
		FHitResult Hit;
		FVector ActorTranslation = CurrentVelocity * 100 * DeltaTime;
		SafeMoveUpdatedComponent(ActorTranslation, ActorRotation, true, Hit);

		// Process invalid location
		if (Hit.bStartPenetrating)
		{
			CurrentVelocity = -CurrentVelocity;
		}

		// Process impacts
		if (Hit.IsValidBlockingHit())
		{
			OnHit(Hit, CurrentVelocity);

			CurrentVelocity = -RestitutionCoefficient * CurrentVelocity;
			ActorTranslation = CurrentVelocity * 100 * DeltaTime;
			SlideAlongSurface(ActorTranslation, 1 - Hit.Time, Hit.Normal, Hit);
		}
	}
}

void UNovaSpacecraftMovementComponent::OnHit(const FHitResult& Hit, const FVector& HitVelocity)
{
}


#undef LOCTEXT_NAMESPACE
