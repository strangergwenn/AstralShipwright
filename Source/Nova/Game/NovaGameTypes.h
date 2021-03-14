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

/*----------------------------------------------------
    Cache maps for fast lookup net serialized arrays
----------------------------------------------------*/

/** Map of FGuid -> T* that mirrors a TArray<T>, with FGuid == T.Identifier */
template <typename T>
struct TGuidCacheMap
{
	/** Add or update ArrayItem to the array Array held by structure Serializer */
	bool AddOrUpdate(FFastArraySerializer& Serializer, TArray<T>& Array, const T& ArrayItem)
	{
		bool NewEntry = false;

		TPair<int32, const T*>* Entry = Map.Find(ArrayItem.Identifier);

		// Simple update
		if (Entry)
		{
			Array[Entry->Key] = ArrayItem;
			Serializer.MarkItemDirty(Array[Entry->Key]);
		}

		// Full addition
		else
		{
			const int32 NewIndex = Array.Add(ArrayItem);
			Serializer.MarkItemDirty(Array[NewIndex]);

			NewEntry = true;
		}

		Update(Array);

		return NewEntry;
	}

	/** Remove the item associated with Identifier from the array Array held by structure Serializer */
	void Remove(FFastArraySerializer& Serializer, TArray<T>& Array, const FGuid& Identifier)
	{
		TPair<int32, const T*>* Entry = Map.Find(Identifier);

		if (Entry)
		{
			Array.RemoveAt(Entry->Key);
			Serializer.MarkArrayDirty();
		}

		Update(Array);
	}

	/** Get an item by identifier */
	const T* Get(const FGuid& Identifier) const
	{
		const TPair<int32, const T*>* Entry = Map.Find(Identifier);
		return Entry ? Entry->Value : nullptr;
	}

	/** Handle removal of entries */
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 NewSize)
	{
		for (auto Iterator = Map.CreateIterator(); Iterator; ++Iterator)
		{
			if (RemovedIndices.Contains(Iterator.Value().Key))
			{
				Iterator.RemoveCurrent();
			}
		}
	}

	/** Update the map from the array Array */
	void Update(const TArray<T>& Array)
	{
		TArray<FGuid> KnownIdentifiers;

		// Database insertion and update
		int32 Index = 0;
		for (const T& ArrayItem : Array)
		{
			TPair<int32, const T*>* Entry = Map.Find(ArrayItem.Identifier);

			// Entry was found, update if necessary
			if (Entry)
			{
				if (*Entry->Value != ArrayItem)
				{
					Entry->Value = &ArrayItem;
				}
			}

			// Entry was not found, add it
			else
			{
				Map.Add(ArrayItem.Identifier, TPair<int32, const T*>(Index, &ArrayItem));
			}

			KnownIdentifiers.Add(ArrayItem.Identifier);
			Index++;
		}

		// Prune unused entries
		for (auto Iterator = Map.CreateIterator(); Iterator; ++Iterator)
		{
			if (!KnownIdentifiers.Contains(Iterator.Key()))
			{
				Iterator.RemoveCurrent();
			}
		}
	}

	TMap<FGuid, TPair<int32, const T*>> Map;
};

/** Map of multiple FGuid -> T* that mirrors a TArray<T>, with TArray<FGuid> == T.Identifiers */
template <typename T>
struct TMultiGuidCacheMap
{
	/** Add or update ArrayItem to the array Array held by structure Serializer */
	bool AddOrUpdate(FFastArraySerializer& Serializer, TArray<T>& Array, const T& ArrayItem)
	{
		bool NewEntry = false;

		// Find the existing entry
		int32 ExistingItemIndex = INDEX_NONE;
		for (const FGuid& Identifier : ArrayItem.Identifiers)
		{
			TPair<int32, const T*>* Entry = Map.Find(Identifier);
			if (Entry)
			{
				NCHECK(ExistingItemIndex == INDEX_NONE || Entry->Key == ExistingItemIndex);
				ExistingItemIndex = Entry->Key;
			}
		}

		// Simple update
		if (ExistingItemIndex != INDEX_NONE)
		{
			Array[ExistingItemIndex] = ArrayItem;
			Serializer.MarkItemDirty(Array[ExistingItemIndex]);
		}

		// Full addition
		else
		{
			const int32 NewIndex = Array.Add(ArrayItem);
			Serializer.MarkItemDirty(Array[NewIndex]);

			NewEntry = true;
		}

		Update(Array);

		return NewEntry;
	}

	/** Remove the item associated with Identifiers from the array Array held by structure Serializer
	 * All Identifiers are assumed to point to the same ItemArray
	 */
	void Remove(FFastArraySerializer& Serializer, TArray<T>& Array, const TArray<FGuid>& Identifiers)
	{
		bool IsDirty = false;

		// Find the existing entry
		int32 ExistingItemIndex = INDEX_NONE;
		for (const FGuid& Identifier : Identifiers)
		{
			TPair<int32, const T*>* Entry = Map.Find(Identifier);
			if (Entry)
			{
				NCHECK(ExistingItemIndex == INDEX_NONE || Entry->Key == ExistingItemIndex);
				ExistingItemIndex = Entry->Key;
			}
		}

		// Delete the entry
		if (ExistingItemIndex != INDEX_NONE)
		{
			Array.RemoveAt(ExistingItemIndex);
			Serializer.MarkArrayDirty();
		}

		Update(Array);
	}

	/** Get an item by identifier */
	const T* Get(const FGuid& Identifier) const
	{
		const TPair<int32, const T*>* Entry = Map.Find(Identifier);
		return Entry ? Entry->Value : nullptr;
	}

	/** Handle removal of entries */
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 NewSize)
	{
		for (auto Iterator = Map.CreateIterator(); Iterator; ++Iterator)
		{
			if (RemovedIndices.Contains(Iterator.Value().Key))
			{
				Iterator.RemoveCurrent();
			}
		}
	}

	/** Update the map from the array Array */
	void Update(const TArray<T>& Array)
	{
		TArray<FGuid> KnownIdentifiers;

		// Database insertion and update
		int32 Index = 0;
		for (const T& ArrayItem : Array)
		{
			for (const FGuid& Identifier : ArrayItem.Identifiers)
			{
				TPair<int32, const T*>* Entry = Map.Find(Identifier);

				// Entry was found, update if necessary
				if (Entry)
				{
					Entry->Value = &ArrayItem;
				}

				// Entry was not found, add it
				else
				{
					Map.Add(Identifier, TPair<int32, const T*>(Index, &ArrayItem));
				}

				KnownIdentifiers.Add(Identifier);
			}

			Index++;
		}

		// Prune unused entries
		for (auto Iterator = Map.CreateIterator(); Iterator; ++Iterator)
		{
			if (!KnownIdentifiers.Contains(Iterator.Key()))
			{
				Iterator.RemoveCurrent();
			}
		}
	}

	TMap<FGuid, TPair<int32, const T*>> Map;
};
