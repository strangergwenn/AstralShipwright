// Nova project - Gwennaël Arbona

#include "NovaAssetManager.h"
#include "Nova/Nova.h"

#include "AssetRegistryModule.h"

// Statics
UNovaAssetManager* UNovaAssetManager::Singleton = nullptr;

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
