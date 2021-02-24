// Nova project - Gwennaël Arbona

#include "NovaAssetCatalog.h"
#include "Nova/Nova.h"

#include "AssetRegistryModule.h"

// Statics
UNovaAssetCatalog* UNovaAssetCatalog::Singleton = nullptr;

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaAssetCatalog::UNovaAssetCatalog() : Super()
{}

/*----------------------------------------------------
    Public methods
----------------------------------------------------*/

void UNovaAssetCatalog::Initialize()
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
		if (!Entry->Hidden)
		{
			Catalog.Add(TPair<FGuid, const class UNovaAssetDescription*>(Entry->Identifier, Entry));
		}
	}
}

void UNovaAssetCatalog::LoadAsset(FSoftObjectPath Asset, FStreamableDelegate Callback)
{
	TArray<FSoftObjectPath> Assets;
	Assets.Add(Asset);

	StreamableManager.RequestAsyncLoad(Assets, Callback);
}

void UNovaAssetCatalog::LoadAssets(TArray<FSoftObjectPath> Assets)
{
	for (FSoftObjectPath Asset : Assets)
	{
		StreamableManager.LoadSynchronous(Asset);
	}
}

void UNovaAssetCatalog::LoadAssets(TArray<FSoftObjectPath> Assets, FStreamableDelegate Callback)
{
	StreamableManager.RequestAsyncLoad(Assets, Callback);
}

void UNovaAssetCatalog::UnloadAsset(FSoftObjectPath Asset)
{
	StreamableManager.Unload(Asset);
}
