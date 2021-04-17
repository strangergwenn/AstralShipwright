// Nova project - Gwennaël Arbona

#include "NovaSpacecraftDriveComponent.h"
#include "NovaSpacecraftMovementComponent.h"
#include "Nova/Actor/NovaMeshInterface.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Nova.h"

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
	ExhaustPower.SetPeriod(0.15f);

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
		const UNovaOrbitalSimulationComponent* Simulation     = UNovaOrbitalSimulationComponent::Get(this);
		const ANovaSpacecraftPawn*             SpacecraftPawn = Cast<ANovaSpacecraftPawn>(GetOwner());
		INovaMeshInterface*                    ParentMesh     = Cast<INovaMeshInterface>(GetAttachParent());
		NCHECK(ParentMesh);

		// Get the intensity
		float EngineIntensity = 0.0f;
		if (IsValid(Simulation) && IsValid(SpacecraftPawn) && !ParentMesh->IsDematerializing())
		{
			EngineIntensity = Simulation->GetCurrentSpacecraftThrustFactor(SpacecraftPawn->GetSpacecraftIdentifier());
		}

		// Apply power
		ExhaustPower.Set(EngineIntensity, DeltaTime);
		if (IsValid(ExhaustMaterial))
		{
			ExhaustMaterial->SetScalarParameterValue("EngineIntensity", ExhaustPower.Get());
		}
	}
}
