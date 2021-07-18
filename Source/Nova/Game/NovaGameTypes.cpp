// Nova project - Gwennaël Arbona

#include "NovaGameTypes.h"
#include "Nova/Actor/NovaCaptureActor.h"
#include "Nova/System/NovaAssetManager.h"
#include "Nova/Nova.h"

#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Dom/JsonObject.h"

/*----------------------------------------------------
    General purpose types
----------------------------------------------------*/

FText INovaDescriptibleInterface::GetFormattedDescription(FString Delimiter) const
{
	FString Result;

	for (const FText& Text : GetDescription())
	{
		if (Result.Len() > 0)
		{
			Result += Delimiter;
		}

		Result += Text.ToString();
	}

	return FText::FromString(Result);
}

/*----------------------------------------------------
    Asset description
----------------------------------------------------*/

void UNovaAssetDescription::UpdateAssetRender()
{
#if WITH_EDITOR

	// Find a capture actor
	TArray<AActor*> CaptureActors;
	for (const FWorldContext& World : GEngine->GetWorldContexts())
	{
		UGameplayStatics::GetAllActorsOfClass(World.World(), ANovaCaptureActor::StaticClass(), CaptureActors);
		if (CaptureActors.Num())
		{
			break;
		}
	}
	NCHECK(CaptureActors.Num());
	ANovaCaptureActor* CaptureActor = Cast<ANovaCaptureActor>(CaptureActors[0]);
	NCHECK(CaptureActor);

	// Capture it
	CaptureActor->RenderAsset(this, AssetRender);

#endif
}

void UNovaAssetDescription::SaveAsset(TSharedPtr<FJsonObject> Save, FString AssetName, const UNovaAssetDescription* Asset)
{
	if (Asset)
	{
		Save->SetStringField(AssetName, Asset->Identifier.ToString(EGuidFormats::Short));
	}
};

const UNovaAssetDescription* UNovaAssetDescription::LoadAsset(TSharedPtr<FJsonObject> Save, FString AssetName)
{
	const UNovaAssetDescription* Asset = nullptr;

	FString IdentifierString;
	if (Save->TryGetStringField(AssetName, IdentifierString))
	{
		FGuid AssetIdentifier;
		if (FGuid::Parse(IdentifierString, AssetIdentifier))
		{
			Asset = UNovaAssetManager::Get()->GetAsset(AssetIdentifier);
		}
	}

	return Asset;
};

struct FNovaAssetPreviewSettings UNovaAssetDescription::GetPreviewSettings() const
{
	return FNovaAssetPreviewSettings();
}

FNovaAssetPreviewSettings UNovaPreviableTradableAssetDescription::GetPreviewSettings() const
{
	FNovaAssetPreviewSettings Settings;

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

	if (PreviewMesh.IsValid())
	{
		PreviewMesh.LoadSynchronous();
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(PreviewMesh.Get());
	}

	if (PreviewMaterial.IsValid())
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
	return UNovaAssetManager::Get()->GetDefaultAsset<UNovaResource>();
}

const UNovaResource* UNovaResource::GetPropellant()
{
	return UNovaAssetManager::Get()->GetAsset<UNovaResource>(FGuid("{78816A80-4E59-9D15-DC6D-DFB769D0B188}"));
}
