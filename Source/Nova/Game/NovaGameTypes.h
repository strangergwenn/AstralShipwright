// Nova project - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "Engine/DataAsset.h"
#include "NovaGameTypes.generated.h"

/*----------------------------------------------------
    General purpose types
----------------------------------------------------*/

/** Gameplay constants */
namespace ENovaConstants
{
constexpr int32 MaxContractsCount   = 5;
constexpr int32 MaxPlayerCount      = 3;
constexpr int32 MaxCompartmentCount = 10;
constexpr int32 MaxModuleCount      = 4;
constexpr int32 MaxEquipmentCount   = 4;

const FString DefaultLevel = TEXT("Space");
};    // namespace ENovaConstants

/** Serialization way */
enum class ENovaSerialize : uint8
{
	JsonToData,
	DataToJson
};

/*----------------------------------------------------
    Description types
----------------------------------------------------*/

/** Component description */
UCLASS(ClassGroup = (Nova))
class UNovaAssetDescription : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Procedurally generate a screenshot of this asset */
	UFUNCTION(Category = Nova, BlueprintCallable, CallInEditor)
	void UpdateAssetRender();

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

	// Generated texture file
	UPROPERTY()
	FSlateBrush AssetRender;
};
