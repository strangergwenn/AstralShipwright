// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "Nova/Game/NovaGameTypes.h"

#include "NovaAssetManager.generated.h"

/** Catalog of dynamic assets to load in game */
UCLASS(ClassGroup = (Nova))
class UNovaAssetManager : public UObject
{
	GENERATED_BODY()

public:
	UNovaAssetManager();

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	/** Search all searched assets */
	void Initialize();

	/** Get a static instance of the catalog */
	static UNovaAssetManager* Get()
	{
		return Singleton;
	}

	/** Find the component with the GUID that matches Identifier */
	const class UNovaAssetDescription* GetAsset(FGuid Identifier) const
	{
		const UNovaAssetDescription* const* Entry = Catalog.Find(Identifier);

		return Entry ? *Entry : nullptr;
	}

	/** Find the component with the GUID that matches Identifier */
	template <typename T>
	const T* GetAsset(FGuid Identifier) const
	{
		return Cast<T>(GetAsset(Identifier));
	}

	/** Find all assets of a class */
	template <typename T>
	TArray<const T*> GetAssets() const
	{
		TArray<const T*> Result;

		for (const auto& Entry : Catalog)
		{
			if (Entry.Value->IsA<T>() && !Entry.Value->Hidden)
			{
				Result.Add(Cast<T>(Entry.Value));
			}
		}

		return Result;
	}

	/** Find the default asset of a class */
	template <typename T>
	const T* GetDefaultAsset() const
	{
		const auto Entry = DefaultAssets.Find(T::StaticClass());
		if (Entry)
		{
			return Cast<T>(*Entry);
		}
		else
		{
			return nullptr;
		}
	}

	/** Load an asset asynchronously */
	void LoadAsset(FSoftObjectPath Entry, FStreamableDelegate Callback);

	/** Load a collection of assets synchronously */
	void LoadAssets(TArray<FSoftObjectPath> Assets);

	/** Load a collection of assets asynchronously */
	void LoadAssets(TArray<FSoftObjectPath> Assets, FStreamableDelegate Callback);

	/** Unload an asset asynchronously */
	void UnloadAsset(FSoftObjectPath Asset);

	/*----------------------------------------------------
	    Public data
	----------------------------------------------------*/

	// Singleton pointer
	static UNovaAssetManager* Singleton;

	// All assets
	UPROPERTY()
	TMap<FGuid, const class UNovaAssetDescription*> Catalog;

	// Default assets
	UPROPERTY()
	TMap<TSubclassOf<UNovaAssetDescription>, const class UNovaAssetDescription*> DefaultAssets;

	// Asynchronous asset loader
	FStreamableManager StreamableManager;
};
