// Astral Shipwright - Gwennaël Arbona

#include "NovaGameTypes.h"
#include "Nova.h"

#include "Neutron/Actor/NeutronCaptureActor.h"

#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"

/*----------------------------------------------------
    Asset description
----------------------------------------------------*/

FNeutronAssetPreviewSettings UNovaPreviableTradableAssetDescription::GetPreviewSettings() const
{
	FNeutronAssetPreviewSettings Settings;

#if WITH_EDITORONLY_DATA

	Settings.Offset   = PreviewOffset;
	Settings.Rotation = PreviewRotation;
	Settings.Scale    = PreviewScale;

#endif    // WITH_EDITORONLY_DATA

	return Settings;
}

void UNovaPreviableTradableAssetDescription::ConfigurePreviewActor(class AActor* Actor) const
{
#if WITH_EDITORONLY_DATA

	NCHECK(Actor->GetClass() == AStaticMeshActor::StaticClass());
	AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);

	if (!PreviewMesh.IsNull())
	{
		PreviewMesh.LoadSynchronous();
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(PreviewMesh.Get());
	}

	if (!PreviewMaterial.IsNull())
	{
		PreviewMaterial.LoadSynchronous();
		MeshActor->GetStaticMeshComponent()->SetMaterial(0, PreviewMaterial.Get());
	}

#endif    // WITH_EDITORONLY_DATA
}

/*----------------------------------------------------
    Resources
----------------------------------------------------*/

const UNovaResource* UNovaResource::GetEmpty()
{
	return UNeutronAssetManager::Get()->GetDefaultAsset<UNovaResource>();
}

const UNovaResource* UNovaResource::GetPropellant()
{
	return UNeutronAssetManager::Get()->GetAsset<UNovaResource>(FGuid("{78816A80-4E59-9D15-DC6D-DFB769D0B188}"));
}
