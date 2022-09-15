// Astral Shipwright - Gwennaël Arbona

#include "NovaStationDock.h"

#include "Game/NovaArea.h"
#include "Game/NovaGameState.h"

#include "Nova.h"

#include "Components/DecalComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaStationDock::ANovaStationDock() : Super()
{
	// Defaults
	SystemDistance = 5000;
}

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
		// Create or get a material instance
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0));
		if (DynamicMaterial == nullptr)
		{
			DynamicMaterial = Component->CreateAndSetMaterialInstanceDynamic(0);
			NCHECK(DynamicMaterial);
		}

		// Set material parameters
		DynamicMaterial->SetVectorParameterValue("PaintColor", Area->PaintColor);
		DynamicMaterial->SetVectorParameterValue("DecalColor", Area->DecalColor);
		DynamicMaterial->SetTextureParameterValue("PaintColorDecal", Area->DecalTexture);
		DynamicMaterial->SetVectorParameterValue("LightColor", Area->LightColor);
		DynamicMaterial->SetScalarParameterValue("DirtyIntensity", Area->DirtyIntensity);
		DynamicMaterial->SetScalarParameterValue("Temperature", Area->Temperature);

		// Spawn effects
		FVector          ParticleSystemLocation = Component->GetComponentLocation() + SystemDistance * Component->GetUpVector();
		UNiagaraSystem** ParticleSystemEntry    = MeshToSystem.Find(Component->GetStaticMesh());
		if (ParticleSystemEntry)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(*ParticleSystemEntry, Component, NAME_None, ParticleSystemLocation,
				Component->GetRightVector().Rotation(), EAttachLocation::KeepWorldPosition, false);
		}
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
