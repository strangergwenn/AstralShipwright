// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftDriveComponent.h"
#include "NovaSpacecraftMovementComponent.h"

#include "Game/NovaOrbitalSimulationComponent.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Nova.h"

#include "Neutron/Actor/NeutronMeshInterface.h"

#include "Materials/MaterialInstanceDynamic.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftDriveComponent::UNovaSpacecraftDriveComponent() : Super(), CurrentTemperature(0.0f), EngineIntensity(0.0f)
{
	// Settings
	SetAbsolute(false, false, true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrimaryComponentTick.bCanEverTick = true;

	// Defaults
	MaxTemperature      = 1400;
	TemperatureRampUp   = 1000;
	TemperatureRampDown = 50;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaSpacecraftDriveComponent::SetAdditionalAsset(TSoftObjectPtr<UObject> AdditionalAsset)
{
	ExhaustMesh = Cast<UStaticMesh>(AdditionalAsset.Get());
}

void UNovaSpacecraftDriveComponent::BeginPlay()
{
	Super::BeginPlay();

	// Assign mesh
	SetStaticMesh(ExhaustMesh);

	// Create exhaust metadata
	ExhaustPower.SetPeriod(0.25f);

	// Create exhaust material
	ExhaustMaterial = UMaterialInstanceDynamic::Create(GetMaterial(0), GetWorld());
	NCHECK(ExhaustMaterial);
	SetMaterial(0.0f, ExhaustMaterial);
}

void UNovaSpacecraftDriveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Get data
	UNovaSpacecraftMovementComponent* MovementComponent = GetOwner()->FindComponentByClass<UNovaSpacecraftMovementComponent>();
	NCHECK(MovementComponent);

	if (MovementComponent && IsValid(GetAttachParent()))
	{
		const UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(this);
		const ANovaSpacecraftPawn*             SpacecraftPawn    = GetOwner<ANovaSpacecraftPawn>();
		INeutronMeshInterface*                 ParentMesh        = Cast<INeutronMeshInterface>(GetAttachParent());
		NCHECK(ParentMesh);

		// Update the state when the intensity is 0 or 1
		if (ExhaustPower.Get() <= KINDA_SMALL_NUMBER || ExhaustPower.Get() >= 1 - KINDA_SMALL_NUMBER)
		{
			if (IsValid(OrbitalSimulation) && IsValid(SpacecraftPawn) && !ParentMesh->IsDematerializing())
			{
				EngineIntensity =
					OrbitalSimulation->GetCurrentSpacecraftThrustFactor(SpacecraftPawn->GetSpacecraftIdentifier(), FNovaTime());
			}
			else
			{
				EngineIntensity = 0.0f;
			}
		}

		// Apply power
		ExhaustPower.Set(EngineIntensity, DeltaTime);
		if (IsValid(ExhaustMaterial))
		{
			ExhaustMaterial->SetScalarParameterValue("EngineIntensity", ExhaustPower.Get());
		}

		// Process temperature
		CurrentTemperature += ((EngineIntensity == 0) ? -TemperatureRampDown : EngineIntensity * TemperatureRampUp) * DeltaTime;
		CurrentTemperature = FMath::Clamp(CurrentTemperature, 0, MaxTemperature);
		if (IsValid(ExhaustMaterial))
		{
			ParentMesh->RequestParameter("Temperature", CurrentTemperature);
		}
	}
}
