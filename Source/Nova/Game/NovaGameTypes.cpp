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

FNovaAssetPreviewSettings::FNovaAssetPreviewSettings()
	: Class(AStaticMeshActor::StaticClass()), RequireCustomPrimitives(false), UsePowerfulLight(false), Scale(1.0f)
{}

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

/*----------------------------------------------------
    Resources
----------------------------------------------------*/

FNovaAssetPreviewSettings UNovaResource::GetPreviewSettings() const
{
	FNovaAssetPreviewSettings Settings;

	Settings.Scale = PreviewScale;

	return Settings;
}

void UNovaResource::ConfigurePreviewActor(class AActor* Actor) const
{
	NCHECK(Actor->GetClass() == AStaticMeshActor::StaticClass());

	AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
	MeshActor->GetStaticMeshComponent()->SetStaticMesh(ResourceMesh);
	MeshActor->GetStaticMeshComponent()->SetMaterial(0, ResourceMaterial);
}
