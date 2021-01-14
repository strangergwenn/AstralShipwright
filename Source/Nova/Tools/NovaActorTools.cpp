// Nova project - Gwennaël Arbona

#include "NovaActorTools.h"

#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/Nova.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"
#include "Components/SceneComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "AssetRegistryModule.h"
#include "Camera/CameraShake.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


/*----------------------------------------------------
	Helper class
----------------------------------------------------*/

void UNovaActorTools::SortComponentsByClosestDistance(TArray<USceneComponent*>& Components, const FVector& BaseLocation)
{
	Components.Sort([BaseLocation](USceneComponent& A, USceneComponent& B)
	{
		FVector LocationA = FVector(A.GetComponentLocation() - BaseLocation);
		FVector LocationB = FVector(B.GetComponentLocation() - BaseLocation);
		return (LocationA.Size() < LocationB.Size());
	});
}

float UNovaActorTools::GetPlayerLatency(class APlayerController* PC)
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

float UNovaActorTools::GetPlayerLatency(class APlayerState* PlayerState)
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

FVector UNovaActorTools::GetVelocityCollisionResponse(const FVector& Velocity, const FHitResult& Hit, float BaseRestitution, const FVector& WorldUp)
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

void UNovaActorTools::PlayCameraShake(TSubclassOf<UCameraShakeBase> Shake, AActor* Owner, float Scale,
	float AttenuationStartDistance, float AttenuationEndDistance)
{
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(Owner->GetGameInstance<UNovaGameInstance>()->GetFirstLocalPlayerController());

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	float SourceDistance = (CameraLocation - Owner->GetActorLocation()).Size();
	float Alpha = 1 - FMath::Clamp((SourceDistance - AttenuationStartDistance) / (AttenuationEndDistance - AttenuationStartDistance), 0.0f, 1.0f);

	PC->ClientStartCameraShake(Shake, Alpha * Scale);
}


/*----------------------------------------------------
	Location interpolator
----------------------------------------------------*/

FNovaMovementInterpolator::FNovaMovementInterpolator(const FTransform& StartTransform, const FVector& StartVelocity, const FVector& StartAngularVelocity,
	const FTransform& EndTransform, const FVector& EndVelocity, const FVector& EndAngularVelocity, float Duration)
{
	InterpolationDuration = Duration;
	float VelocityToDerivative = InterpolationDuration * 100;
	float AngularVelocityToDerivative = InterpolationDuration;

	StartLocation = StartTransform.GetLocation();
	StartRotation = StartTransform.GetRotation();
	StartAngularDerivative = StartAngularVelocity * AngularVelocityToDerivative;
	StartDerivative = StartVelocity * VelocityToDerivative;

	TargetLocation = EndTransform.GetLocation();
	TargetRotation = EndTransform.GetRotation();
	TargetDerivative = EndVelocity * VelocityToDerivative;
	TargetAngularDerivative = EndAngularVelocity * AngularVelocityToDerivative;
}

void FNovaMovementInterpolator::Get(FVector& OutLocation, FQuat& OutRotation, FVector& OutVelocity, FVector& OutAngularVelocity, float Time) const
{
	float LerpRatio = Time / InterpolationDuration;
	float VelocityToDerivative = InterpolationDuration * 100;
	float AngularVelocityToDerivative = InterpolationDuration;

	OutLocation = FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	OutRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);
	OutVelocity = FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio) / VelocityToDerivative;
	OutAngularVelocity = FMath::CubicInterpDerivative(StartRotation.Vector(), StartAngularDerivative, TargetRotation.Vector(), TargetAngularDerivative, LerpRatio) / AngularVelocityToDerivative;
}

void FNovaMovementInterpolator::Get(FVector& OutLocation, FVector& OutVelocity, float Time) const
{
	float LerpRatio = Time / InterpolationDuration;
	float VelocityToDerivative = InterpolationDuration * 100;

	OutLocation = FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	OutVelocity = FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio) / VelocityToDerivative;
}
