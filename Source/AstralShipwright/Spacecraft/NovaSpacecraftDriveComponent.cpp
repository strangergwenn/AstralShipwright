// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftDriveComponent.h"
#include "NovaSpacecraftMovementComponent.h"

#include "Actor/NovaMeshInterface.h"
#include "Game/NovaOrbitalSimulationComponent.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Nova.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftDriveComponent::UNovaSpacecraftDriveComponent() : Super()
{
	// Settings
	SetAbsolute(false, false, true);
	PrimaryComponentTick.bCanEverTick = true;
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
	ExhaustPower.SetPeriod(0.4f);

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
		INovaMeshInterface*                    ParentMesh        = Cast<INovaMeshInterface>(GetAttachParent());
		NCHECK(ParentMesh);

		// Get the intensity
		float EngineIntensity = 0.0f;
		if (IsValid(OrbitalSimulation) && IsValid(SpacecraftPawn) && !ParentMesh->IsDematerializing())
		{
			EngineIntensity = OrbitalSimulation->GetCurrentSpacecraftThrustFactor(SpacecraftPawn->GetSpacecraftIdentifier());
		}

		// Apply power
		ExhaustPower.Set(EngineIntensity, DeltaTime);
		if (IsValid(ExhaustMaterial))
		{
			ExhaustMaterial->SetScalarParameterValue("EngineIntensity", ExhaustPower.Get());
		}
	}
}
