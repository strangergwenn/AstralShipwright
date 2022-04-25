// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StreamableManager.h"
#include "NovaAssetManager.generated.h"

/*----------------------------------------------------
    Description base classes
----------------------------------------------------*/

/** Interface for objects that will provide a description interface */
class INovaDescriptibleInterface
{
public:
	/** Return the full description of this asset as inline text */
	FText GetInlineDescription() const
	{
		return GetFormattedDescription("  ");
	}

	/** Return the full description of this asset as paragraph text */
	FText GetParagraphDescription() const
	{
		return GetFormattedDescription("\n");
	}

	/** Return the formatted description of this asset */
	FText GetFormattedDescription(FString Delimiter) const;

	/** Return details on this asset */
	virtual TArray<FText> GetDescription() const
	{
		return TArray<FText>();
	}
};

/** Asset description */
UCLASS(ClassGroup = (Nova))
class UNovaAssetDescription
	: public UDataAsset
	, public INovaDescriptibleInterface
{
	GENERATED_BODY()

public:
	/** Procedurally generate a screenshot of this asset */
	UFUNCTION(Category = Nova, BlueprintCallable, CallInEditor)
	void UpdateAssetRender();

	// Write an asset description to JSON
	static void SaveAsset(TSharedPtr<class FJsonObject> Save, FString AssetName, const UNovaAssetDescription* Asset);

	// Get an asset description from JSON
	static const UNovaAssetDescription* LoadAsset(TSharedPtr<class FJsonObject> Save, FString AssetName);

	template <typename T>
	static const T* LoadAsset(TSharedPtr<class FJsonObject> Save, FString AssetName)
	{
		return Cast<T>(LoadAsset(Save, AssetName));
	}

	/** Get a list of assets to load before use*/
	virtual TArray<FSoftObjectPath> GetAsyncAssets() const
	{
		TArray<FSoftObjectPath> Result;

		for (TFieldIterator<FSoftObjectProperty> PropIt(GetClass()); PropIt; ++PropIt)
		{
			FSoftObjectProperty* Property = *PropIt;
			FSoftObjectPtr       Ptr      = Property->GetPropertyValue(Property->ContainerPtrToValuePtr<int32>(this));
			if (!Ptr.IsNull())
			{
				Result.AddUnique(Ptr.ToSoftObjectPath());
			}
		}

		return Result;
	}

	/** Get the desired display settings when taking shots of this asset */
	virtual struct FNovaAssetPreviewSettings GetPreviewSettings() const;

	/** Configure the target actor for taking shots of this asset */
	virtual void ConfigurePreviewActor(class AActor* Actor) const
	{}

public:
	// Identifier
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	FGuid Identifier;

	// Display name
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	FText Name;

	// Whether this asset is a special hidden one
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	bool Hidden;

	// Whether this asset is a special default asset
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	bool Default;

	// Generated texture file
	UPROPERTY()
	FSlateBrush AssetRender;
};

/*----------------------------------------------------
    Asset manager
----------------------------------------------------*/

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
	const UNovaAssetDescription* GetAsset(FGuid Identifier) const
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

	/** Find all assets of a class and sort */
	template <typename T>
	TArray<const T*> GetSortedAssets() const
	{
		TArray<const T*> Result = GetAssets<T>();

		Result.Sort();

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
	TMap<FGuid, const UNovaAssetDescription*> Catalog;

	// Default assets
	UPROPERTY()
	TMap<TSubclassOf<UNovaAssetDescription>, const UNovaAssetDescription*> DefaultAssets;

	// Asynchronous asset loader
	FStreamableManager StreamableManager;
};
