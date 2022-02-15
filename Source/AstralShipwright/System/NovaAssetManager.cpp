// Astral Shipwright - Gwennaël Arbona

#include "NovaAssetManager.h"
#include "Actor/NovaCaptureActor.h"
#include "Nova.h"

#include "AssetRegistryModule.h"
#include "Dom/JsonObject.h"

// Statics
UNovaAssetManager* UNovaAssetManager::Singleton = nullptr;

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

	// Capture
	if (CaptureActors.Num() > 0)
	{
		ANovaCaptureActor* CaptureActor = Cast<ANovaCaptureActor>(CaptureActors[0]);
		NCHECK(CaptureActor);

		CaptureActor->RenderAsset(this, AssetRender);
	}

#endif    // WITH_EDITOR
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

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaAssetManager::UNovaAssetManager() : Super()
{}

/*----------------------------------------------------
    Public methods
----------------------------------------------------*/

void UNovaAssetManager::Initialize()
{
	Singleton = this;
	Catalog.Empty();

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

#if WITH_EDITOR
	Registry.SearchAllAssets(true);
#endif

	// Get assets
	TArray<FAssetData> AssetList;
	Registry.GetAssetsByClass(UNovaAssetDescription::StaticClass()->GetFName(), AssetList, true);
	for (FAssetData Asset : AssetList)
	{
		const UNovaAssetDescription* Entry = Cast<UNovaAssetDescription>(Asset.GetAsset());
		NCHECK(Entry);

		Catalog.Add(TPair<FGuid, const class UNovaAssetDescription*>(Entry->Identifier, Entry));

		if (Entry->Default)
		{
			DefaultAssets.Add(TPair<TSubclassOf<UNovaAssetDescription>, const class UNovaAssetDescription*>(Entry->GetClass(), Entry));
		}
	}
}

void UNovaAssetManager::LoadAsset(FSoftObjectPath Asset, FStreamableDelegate Callback)
{
	TArray<FSoftObjectPath> Assets;
	Assets.Add(Asset);

	StreamableManager.RequestAsyncLoad(Assets, Callback);
}

void UNovaAssetManager::LoadAssets(TArray<FSoftObjectPath> Assets)
{
	for (FSoftObjectPath Asset : Assets)
	{
		StreamableManager.LoadSynchronous(Asset);
	}
}

void UNovaAssetManager::LoadAssets(TArray<FSoftObjectPath> Assets, FStreamableDelegate Callback)
{
	StreamableManager.RequestAsyncLoad(Assets, Callback);
}

void UNovaAssetManager::UnloadAsset(FSoftObjectPath Asset)
{
	StreamableManager.Unload(Asset);
}
