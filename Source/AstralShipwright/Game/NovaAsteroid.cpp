// Astral Shipwright - Gwennaël Arbona

#include "NovaAsteroid.h"
#include "NovaAsteroidSimulationComponent.h"

#include "Components/StaticMeshComponent.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaAsteroid::ANovaAsteroid() : Super()
{
	// Create the main mesh
	AsteroidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Asteroid"));
	SetRootComponent(AsteroidMesh);

	// Defaults
	SetActorTickEnabled(true);
	SetReplicates(false);
	bAlwaysRelevant = true;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaAsteroid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// float CollisionSize = AsteroidMesh->GetCollisionShape().GetExtent().Size();

	//// Get player ship
	// AFlareGame* Game = Cast<AFlareGame>(GetWorld()->GetAuthGameMode());
	// FCHECK(Game);
	// AFlarePlayerController* PC = Game->GetPC();
	// FCHECK(PC);
	// AFlareSpacecraft* ShipPawn = PC->GetShipPawn();

	//// Update if close to player and visible
	// if (GetWorld()->TimeSeconds - GetLastRenderTime() < 0.5)
	//{
	//	// World data
	//	FVector AsteroidLocation = GetActorLocation();
	//	FVector SunDirection     = Game->GetPlanetarium()->GetSunDirection();
	//	SunDirection.Normalize();

	//	// Compute new FX locations
	//	for (int32 Index = 0; Index < Effects.Num(); Index++)
	//	{
	//		FVector RandomDirection = FVector::CrossProduct(SunDirection, EffectsKernels[Index]);
	//		RandomDirection.Normalize();
	//		FVector StartPoint = AsteroidLocation + RandomDirection * CollisionSize;

	//		// Trace params
	//		FHitResult            HitResult(ForceInit);
	//		FCollisionQueryParams TraceParams(FName(TEXT("Asteroid Trace")), false, NULL);
	//		TraceParams.bTraceComplex           = true;
	//		TraceParams.bReturnPhysicalMaterial = false;
	//		ECollisionChannel CollisionChannel  = ECollisionChannel::ECC_WorldDynamic;

	//		// Trace
	//		bool FoundHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartPoint, AsteroidLocation, CollisionChannel, TraceParams);
	//		if (FoundHit && HitResult.Component == this && Effects[Index])
	//		{
	//			FVector EffectLocation = HitResult.Location;

	//			if (!Effects[Index]->IsActive())
	//			{
	//				Effects[Index]->Activate();
	//			}
	//			Effects[Index]->SetWorldLocation(EffectLocation);
	//			Effects[Index]->SetWorldRotation(SunDirection.Rotation());
	//		}
	//		else
	//		{
	//			Effects[Index]->Deactivate();
	//		}
	//	}
	//}

	//// Disable all
	// else
	//{
	//	for (int32 Index = 0; Index < Effects.Num(); Index++)
	//	{
	//		Effects[Index]->Deactivate();
	//	}
	//}
}

void ANovaAsteroid::Initialize(const FNovaAsteroid& Asteroid)
{
	// Effects.Empty();
	// EffectsKernels.Empty();

	// int32 Multiplier = Asteroid ? Asteroid->EffectsMultiplier : 1;

	//// Create random effects
	// for (int32 Index = 0; Index < Multiplier * EffectsCount; Index++)
	//{
	//	EffectsKernels.Add(FMath::VRand());

	//	UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAttached(
	//		DustEffect, this, NAME_None, GetActorLocation(), FRotator::ZeroRotator, EAttachLocation::KeepWorldPosition, false);

	//	PSC->SetWorldScale3D(EffectsScale * FVector(1, 1, 1));
	//	Effects.Add(PSC);
	//}
}
