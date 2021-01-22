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

	, CurrentDesiredAcceleration(FVector::ZeroVector)
	, CurrentDesiredRotation(FRotator::ZeroRotator)
	, CurrentVelocity(FVector::ZeroVector)
	, CurrentAngularVelocity(FVector::ZeroVector)

	, PreviousVelocity(FVector::ZeroVector)
	, PreviousAngularVelocity(FVector::ZeroVector)
	, MeasuredAcceleration(FVector::ZeroVector)
	, MeasuredAngularAcceleration(FVector::ZeroVector)
{
	// Physics settings
	LinearAcceleration = 10;
	AngularAcceleration = 50;
	AngularControlIntensity = 2.0f;
	MaxLinearVelocity = 8;
	MaxAngularVelocity = 60;
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

	// Update history
	MeasuredAcceleration = ((CurrentVelocity - PreviousVelocity) / DeltaTime);
	MeasuredAngularAcceleration = (CurrentAngularVelocity - PreviousAngularVelocity) * DeltaTime;
	PreviousVelocity = CurrentVelocity;
	PreviousAngularVelocity = CurrentAngularVelocity;

	// DEBUG
	if (FMath::RoundToInt(GetWorld()->GetTimeSeconds()) % 10 > 5)
	{
		CurrentDesiredRotation = FRotator(90, 0, 0).Quaternion();
	}
	else
	{
		CurrentDesiredRotation = FRotator(0, 0, 0).Quaternion();
	}

	// Apply the physics movement
	ApplyMovement(DeltaTime);
}


/*----------------------------------------------------
	Internal movement implementation
----------------------------------------------------*/

void UNovaSpacecraftMovementComponent::ApplyMovement(float DeltaTime)
{
	if (GetOwner()->GetAttachParentActor() == nullptr)
	{
		auto UpdateAngularVelocity = [](float Current, float Target, float MaxDelta, float Max, float Intensity = 5)
		{
			float New = FMath::Clamp(Intensity * Target, Current - MaxDelta, Current + MaxDelta);
			New = FMath::Clamp(New, -Max, Max);
			return New;
		};

		// Get the current state
		FVector ActorLocation = UpdatedComponent->GetComponentLocation();
		FQuat ActorRotation = UpdatedComponent->GetComponentQuat();
		FQuat ActorRotationYaw = FRotator(0, ActorRotation.Rotator().Yaw, 0).Quaternion();

		// Update linear speed
		CurrentVelocity += CurrentDesiredAcceleration.GetClampedToMaxSize(LinearAcceleration) * DeltaTime;
		CurrentVelocity = CurrentVelocity.GetClampedToMaxSize(MaxLinearVelocity);

		// Update angular velocity
		FQuat DesiredRotation = CurrentDesiredRotation.GetNormalized();
		FQuat TargetRotationDelta = (DesiredRotation * ActorRotation.Inverse()).GetNormalized();
		CurrentAngularVelocity.Y = UpdateAngularVelocity(CurrentAngularVelocity.Y, TargetRotationDelta.Rotator().Pitch,
			AngularAcceleration * DeltaTime, MaxAngularVelocity, AngularControlIntensity);
		CurrentAngularVelocity.Z = UpdateAngularVelocity(CurrentAngularVelocity.Z, TargetRotationDelta.Rotator().Yaw,
			AngularAcceleration * DeltaTime, MaxAngularVelocity, AngularControlIntensity);
		CurrentAngularVelocity.X = UpdateAngularVelocity(CurrentAngularVelocity.X, TargetRotationDelta.Rotator().Roll,
			AngularAcceleration * DeltaTime, MaxAngularVelocity, AngularControlIntensity);

		// Update rotation
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
