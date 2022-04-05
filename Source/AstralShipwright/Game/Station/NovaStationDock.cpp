// Astral Shipwright - Gwennaël Arbona

#include "NovaStationDock.h"

#include "Game/NovaArea.h"
#include "Game/NovaGameState.h"

#include "Nova.h"

#include "Components/DecalComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaStationDock::ANovaStationDock() : Super()
{}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void ANovaStationDock::BeginPlay()
{
	Super::BeginPlay();

	// Get current area
	ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));
	const UNovaArea* Area = GameState->GetCurrentArea();
	NCHECK(Area);

	// Process meshes
	TArray<UStaticMeshComponent*> MeshComponents;
	GetComponents(MeshComponents);
	for (UStaticMeshComponent* Component : MeshComponents)
	{
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0));
		if (DynamicMaterial == nullptr)
		{
			DynamicMaterial = Component->CreateAndSetMaterialInstanceDynamic(0);
			NCHECK(DynamicMaterial);
		}

		DynamicMaterial->SetVectorParameterValue("PaintColor", Area->PaintColor);
		DynamicMaterial->SetVectorParameterValue("LightColor", Area->LightColor);
		DynamicMaterial->SetScalarParameterValue("DirtyIntensity", Area->DirtyIntensity);
	}

	// Process decals
	TArray<UDecalComponent*> DecalComponents;
	GetComponents(DecalComponents);
	for (UDecalComponent* Component : DecalComponents)
	{
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0));
		if (DynamicMaterial == nullptr)
		{
			DynamicMaterial = Component->CreateDynamicMaterialInstance();
			NCHECK(DynamicMaterial);
		}

		DynamicMaterial->SetVectorParameterValue("PaintColor", Area->DecalColor);
	}

	// Process lights
	TArray<ULightComponent*> LightComponents;
	GetComponents(LightComponents);
	for (ULightComponent* Component : LightComponents)
	{
		FLinearColor TonedDownColor = Area->LightColor.Desaturate(0.9f);
		Component->SetLightColor(TonedDownColor);
	}
}
