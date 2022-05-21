// Astral Shipwright - Gwennaël Arbona

#include "NovaActorTools.h"

#include "Player/NovaPlayerController.h"
#include "System/NovaGameInstance.h"
#include "Nova.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"
#include "Components/SceneComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "AssetRegistryModule.h"
#include "Camera/CameraShakeBase.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

/*----------------------------------------------------
    Helper class
----------------------------------------------------*/

void UNovaActorTools::SortActorsByClosestDistance(TArray<AActor*>& Actors, const FVector& BaseLocation)
{
	Actors.Sort(
		[BaseLocation](AActor& A, AActor& B)
		{
			FVector LocationA = FVector(A.GetActorLocation() - BaseLocation);
			FVector LocationB = FVector(B.GetActorLocation() - BaseLocation);
			return (LocationA.Size() < LocationB.Size());
		});
}

void UNovaActorTools::SortComponentsByClosestDistance(TArray<USceneComponent*>& Components, const FVector& BaseLocation)
{
	Components.Sort(
		[BaseLocation](USceneComponent& A, USceneComponent& B)
		{
			FVector LocationA = FVector(A.GetComponentLocation() - BaseLocation);
			FVector LocationB = FVector(B.GetComponentLocation() - BaseLocation);
			return (LocationA.Size() < LocationB.Size());
		});
}

float UNovaActorTools::GetPlayerLatency(const class APlayerController* PC)
{
	if (PC)
	{
		return GetPlayerLatency(PC->PlayerState);
	}
	else
	{
		return 0;
	}
}

float UNovaActorTools::GetPlayerLatency(const class APlayerState* PlayerState)
{
	if (PlayerState)
	{
		return 0.5f * (PlayerState->ExactPing / 1000.0f);
	}
	else
	{
		return 0;
	}
}

double UNovaActorTools::SolveVelocity(double& CurrentVelocity, double TargetVelocity, double CurrentLocation, double TargetLocation,
	double MaximumAcceleration, double MaximumVelocity, double DeadDistance, float DeltaTime)
{
	// Get the input data
	double DeltaPosition  = TargetLocation - CurrentLocation;
	double TargetDistance = FMath::Max(0.0f, FMath::Abs(DeltaPosition) - DeadDistance);
	double DeltaVelocity  = TargetVelocity - CurrentVelocity;

	// Determine the time left to reach the final desired velocity
	double TimeToFinalVelocity = 0.0;
	if (!FMath::IsNearlyZero(DeltaVelocity))
	{
		TimeToFinalVelocity = FMath::Abs(DeltaVelocity) / MaximumAcceleration;
	}

	// Update desired velocity to match location & velocity inputs best
	double VelocityChange;
	double DistanceToStop = (FMath::Abs(DeltaVelocity) / 2.0) * (TimeToFinalVelocity + DeltaTime);
	if (DistanceToStop > TargetDistance)
	{
		VelocityChange = TargetVelocity;
	}
	else
	{
		double MaxAccurateVelocity = FMath::Min((TargetDistance - DistanceToStop) / DeltaTime, MaximumVelocity);
		VelocityChange             = FMath::Sign(DeltaPosition) * MaxAccurateVelocity + TargetVelocity;
	}

	// Update the velocity with limited acceleration
	auto UpdateVelocity = [](double& Value, double Target, double MaxDelta)
	{
		Value = FMath::Clamp(Target, Value - MaxDelta, Value + MaxDelta);
	};
	UpdateVelocity(CurrentVelocity, VelocityChange, MaximumAcceleration * DeltaTime);

	return TargetDistance;
}

double UNovaActorTools::SolveVelocity(FVector& CurrentVelocity, const FVector& TargetVelocity, const FVector& CurrentLocation,
	const FVector& TargetLocation, double MaximumAcceleration, double MaximumVelocity, double DeadDistance, float DeltaTime)
{
	// Get the position data
	const FVector DeltaPosition  = TargetLocation - CurrentLocation;
	double        TargetDistance = FMath::Max(0.0f, DeltaPosition.Size() - DeadDistance);

	// Get the velocity data
	FVector DeltaVelocity     = TargetVelocity - CurrentVelocity;
	FVector DeltaVelocityAxis = DeltaVelocity;
	DeltaVelocityAxis.Normalize();

	// Determine the time left to reach the final desired velocity
	double TimeToFinalVelocity = 0.0;
	if (!FMath::IsNearlyZero(DeltaVelocity.SizeSquared()))
	{
		TimeToFinalVelocity = DeltaVelocity.Size() / MaximumAcceleration;
	}

	// Update desired velocity to match location & velocity inputs best
	FVector VelocityChange;
	float   DistanceToStop = (DeltaVelocity.Size() / 2.0) * (TimeToFinalVelocity + DeltaTime);
	if (DistanceToStop > TargetDistance)
	{
		VelocityChange = TargetVelocity;
	}
	else
	{
		double MaxAccurateVelocity = FMath::Min((TargetDistance - DistanceToStop) / DeltaTime, MaximumVelocity);
		VelocityChange             = DeltaPosition.GetSafeNormal() * MaxAccurateVelocity + TargetVelocity;
	}

	// Update the velocity with limited acceleration
	auto UpdateVelocity = [](double& Value, double Target, double MaxDelta)
	{
		Value = FMath::Clamp(Target, Value - MaxDelta, Value + MaxDelta);
	};
	UpdateVelocity(CurrentVelocity.X, VelocityChange.X, MaximumAcceleration * DeltaTime);
	UpdateVelocity(CurrentVelocity.Y, VelocityChange.Y, MaximumAcceleration * DeltaTime);
	UpdateVelocity(CurrentVelocity.Z, VelocityChange.Z, MaximumAcceleration * DeltaTime);

	return TargetDistance;
}

FVector UNovaActorTools::GetVelocityCollisionResponse(
	const FVector& Velocity, const FHitResult& Hit, float BaseRestitution, const FVector& WorldUp)
{
	FVector Result = Velocity.MirrorByVector(Hit.Normal);

	// Only apply friction when the result velocity is going up
	if (FVector::DotProduct(Result, WorldUp) > 0)
	{
		Result *= BaseRestitution;
		if (Hit.PhysMaterial.IsValid())
		{
			Result *= Hit.PhysMaterial->Restitution;
		}
	}

	return Result;
}

void UNovaActorTools::PlayCameraShake(
	TSubclassOf<UCameraShakeBase> Shake, AActor* Owner, float Scale, float AttenuationStartDistance, float AttenuationEndDistance)
{
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(Owner->GetGameInstance<UNovaGameInstance>()->GetFirstLocalPlayerController());

	FVector  CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	float SourceDistance = (CameraLocation - Owner->GetActorLocation()).Size();
	float Alpha =
		1 - FMath::Clamp((SourceDistance - AttenuationStartDistance) / (AttenuationEndDistance - AttenuationStartDistance), 0.0f, 1.0f);

	PC->ClientStartCameraShake(Shake, Alpha * Scale);
}

/*----------------------------------------------------
    Location interpolator
----------------------------------------------------*/

FNovaMovementInterpolator::FNovaMovementInterpolator(const FTransform& StartTransform, const FVector& StartVelocity,
	const FVector& StartAngularVelocity, const FTransform& EndTransform, const FVector& EndVelocity, const FVector& EndAngularVelocity,
	float Duration)
{
	InterpolationDuration             = Duration;
	float VelocityToDerivative        = InterpolationDuration * 100;
	float AngularVelocityToDerivative = InterpolationDuration;

	StartLocation          = StartTransform.GetLocation();
	StartRotation          = StartTransform.GetRotation();
	StartAngularDerivative = StartAngularVelocity * AngularVelocityToDerivative;
	StartDerivative        = StartVelocity * VelocityToDerivative;

	TargetLocation          = EndTransform.GetLocation();
	TargetRotation          = EndTransform.GetRotation();
	TargetDerivative        = EndVelocity * VelocityToDerivative;
	TargetAngularDerivative = EndAngularVelocity * AngularVelocityToDerivative;
}

void FNovaMovementInterpolator::Get(
	FVector& OutLocation, FQuat& OutRotation, FVector& OutVelocity, FVector& OutAngularVelocity, float Time) const
{
	float LerpRatio                   = Time / InterpolationDuration;
	float VelocityToDerivative        = InterpolationDuration * 100;
	float AngularVelocityToDerivative = InterpolationDuration;

	OutLocation = FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	OutRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);
	OutVelocity =
		FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio) / VelocityToDerivative;
	OutAngularVelocity = FMath::CubicInterpDerivative(
							 StartRotation.Vector(), StartAngularDerivative, TargetRotation.Vector(), TargetAngularDerivative, LerpRatio) /
	                     AngularVelocityToDerivative;
}

void FNovaMovementInterpolator::Get(FVector& OutLocation, FVector& OutVelocity, float Time) const
{
	float LerpRatio            = Time / InterpolationDuration;
	float VelocityToDerivative = InterpolationDuration * 100;

	OutLocation = FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	OutVelocity =
		FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio) / VelocityToDerivative;
}
