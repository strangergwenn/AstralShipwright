// Astral Shipwright - Gwennaël Arbona

#include "NovaStationDock.h"

#include "Nova.h"

#include "Components/DecalComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaStationDock::ANovaStationDock() : Super()
{
	// Defaults
	PaintColor     = FLinearColor::White;
	LightColor     = FLinearColor::White;
	DecalColor     = FLinearColor::Black;
	DirtyIntensity = 0.5f;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void ANovaStationDock::BeginPlay()
{
	Super::BeginPlay();

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

		DynamicMaterial->SetVectorParameterValue("PaintColor", PaintColor);
		DynamicMaterial->SetVectorParameterValue("LightColor", LightColor);
		DynamicMaterial->SetScalarParameterValue("DirtyIntensity", DirtyIntensity);
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

		DynamicMaterial->SetVectorParameterValue("PaintColor", DecalColor);
	}

	// Process lights
	TArray<ULightComponent*> LightComponents;
	GetComponents(LightComponents);
	for (ULightComponent* Component : LightComponents)
	{
		FLinearColor TonedDownColor = LightColor.Desaturate(0.9f);
		Component->SetLightColor(TonedDownColor);
	}
}
