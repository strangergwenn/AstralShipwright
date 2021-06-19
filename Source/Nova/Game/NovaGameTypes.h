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
constexpr int32 MaxModuleCount      = 5;
constexpr int32 MaxEquipmentCount   = 8;

const FString DefaultLevel = TEXT("Space");
};    // namespace ENovaConstants

/** Serialization way */
enum class ENovaSerialize : uint8
{
	JsonToData,
	DataToJson
};

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

/*----------------------------------------------------
    Time type
----------------------------------------------------*/

/** Time type */
USTRUCT()
struct FNovaTime
{
	GENERATED_BODY();

	FNovaTime() : Minutes(0)
	{}

	FNovaTime(double M) : Minutes(M)
	{}

	bool IsValid() const
	{
		return Minutes >= 0;
	}

	bool operator==(const FNovaTime Other) const
	{
		return Minutes == Other.Minutes;
	}

	bool operator!=(const FNovaTime Other) const
	{
		return !operator==(Other);
	}

	bool operator<(const FNovaTime Other) const
	{
		return Minutes < Other.Minutes;
	}

	bool operator<=(const FNovaTime Other) const
	{
		return Minutes <= Other.Minutes;
	}

	bool operator>(const FNovaTime Other) const
	{
		return Minutes > Other.Minutes;
	}

	bool operator>=(const FNovaTime Other) const
	{
		return Minutes >= Other.Minutes;
	}

	FNovaTime operator+(const FNovaTime Other) const
	{
		return FNovaTime::FromMinutes(Minutes + Other.Minutes);
	}

	FNovaTime operator+=(const FNovaTime Other)
	{
		Minutes += Other.Minutes;
		return *this;
	}

	FNovaTime operator-() const
	{
		return FNovaTime::FromMinutes(-Minutes);
	}

	FNovaTime operator-(const FNovaTime Other) const
	{
		return FNovaTime::FromMinutes(Minutes - Other.Minutes);
	}

	FNovaTime operator-=(const FNovaTime Other)
	{
		Minutes -= Other.Minutes;
		return *this;
	}

	double operator/(const FNovaTime Other) const
	{
		return Minutes / Other.Minutes;
	}

	double AsMinutes() const
	{
		return Minutes;
	}

	double AsSeconds() const
	{
		return Minutes * 60.0;
	}

	static FNovaTime FromMinutes(double Value)
	{
		FNovaTime T;
		T.Minutes = Value;
		return T;
	}

	static FNovaTime FromSeconds(double Value)
	{
		FNovaTime T;
		T.Minutes = (Value / 60.0);
		return T;
	}

	UPROPERTY()
	double Minutes;
};

static FNovaTime operator*(const FNovaTime Time, const double Value)
{
	return FNovaTime::FromMinutes(Time.Minutes * Value);
}

static FNovaTime operator*(const double Value, const FNovaTime Time)
{
	return FNovaTime::FromMinutes(Time.Minutes * Value);
}

static FNovaTime operator/(const FNovaTime Time, const double Value)
{
	return FNovaTime::FromMinutes(Time.Minutes / Value);
}

static FNovaTime operator/(const double Value, const FNovaTime Time)
{
	return FNovaTime::FromMinutes(Value / Time.Minutes);
}

/*----------------------------------------------------
    Description types
----------------------------------------------------*/

/** Component description */
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

	// Generated texture file
	UPROPERTY()
	FSlateBrush AssetRender;
};

/** Description of a tradable asset */
UCLASS(ClassGroup = (Nova))
class UNovaTradableAssetDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	// Base price modulated by the economy
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float BasePrice = 10;
};

/*----------------------------------------------------
    Resources
----------------------------------------------------*/

/** Possible cargo types */
UENUM()
enum class ENovaResourceType : uint8
{
	General,
	Bulk,
	Liquid
};

/** Description of a resource */
UCLASS(ClassGroup = (Nova))
class UNovaResource : public UNovaTradableAssetDescription
{
	GENERATED_BODY()

	virtual struct FNovaAssetPreviewSettings GetPreviewSettings() const override;

	virtual void ConfigurePreviewActor(class AActor* Actor) const override;

public:
	/** Get a special empty resource */
	static const UNovaResource* GetEmpty();

	/** Get a special propellant resource */
	static const UNovaResource* GetPropellant();

public:
	// Type of cargo hold that will be required for this resource
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaResourceType Type = ENovaResourceType::Bulk;

	// Resource description
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FText Description;

#if WITH_EDITORONLY_DATA

	// Mesh to use to render the resource
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	class UStaticMesh* ResourceMesh;

	// Material to use to render the resource
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	class UMaterialInterface* ResourceMaterial;

	// Preview offset for the resource render, in units
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FVector PreviewOffset = FVector(25.0f, -25.0f, 0.0f);

	// Preview rotation for the resource render
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FRotator PreviewRotation = FRotator::ZeroRotator;

	// Preview scale for the resource render
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float PreviewScale = 4.25f;

#endif    // WITH_EDITORONLY_DATA
};

/*----------------------------------------------------
    Cache maps for fast lookup net serialized arrays
----------------------------------------------------*/

/** Map of FGuid -> T* that mirrors a TArray<T>, with FGuid == T.Identifier */
template <typename T>
struct TGuidCacheMap
{
	/** Add or update ArrayItem to the array Array held by structure Serializer */
	bool Add(FFastArraySerializer& Serializer, TArray<T>& Array, const T& ArrayItem)
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
	const T* Get(const FGuid& Identifier, const TArray<T>& Array) const
	{
		const TPair<int32, const T*>* Entry = Map.Find(Identifier);

		if (Entry)
		{
			NCHECK(Entry->Key >= 0 && Entry->Key < Array.Num());
			NCHECK(Entry->Value == &Array[Entry->Key]);
		}

		return Entry ? Entry->Value : nullptr;
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

		for (auto& IdentifierAndEntry : Map)
		{
			NCHECK(IdentifierAndEntry.Value.Key >= 0 && IdentifierAndEntry.Value.Key < Array.Num());
			NCHECK(IdentifierAndEntry.Value.Value == &Array[IdentifierAndEntry.Value.Key]);
		}
	}

	TMap<FGuid, TPair<int32, const T*>> Map;
};

/** Map of multiple FGuid -> T* that mirrors a TArray<T>, with TArray<FGuid> == T.Identifiers */
template <typename T>
struct TMultiGuidCacheMap
{
	/** Add or update ArrayItem to the array Array held by structure Serializer */
	bool Add(FFastArraySerializer& Serializer, TArray<T>& Array, const T& ArrayItem)
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
	const T* Get(const FGuid& Identifier, const TArray<T>& Array) const
	{
		const TPair<int32, const T*>* Entry = Map.Find(Identifier);

		if (Entry)
		{
			NCHECK(Entry->Key >= 0 && Entry->Key < Array.Num());
			NCHECK(Entry->Value == &Array[Entry->Key]);
		}

		return Entry ? Entry->Value : nullptr;
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

		for (auto& IdentifierAndEntry : Map)
		{
			NCHECK(IdentifierAndEntry.Value.Key >= 0 && IdentifierAndEntry.Value.Key < Array.Num());
			NCHECK(IdentifierAndEntry.Value.Value == &Array[IdentifierAndEntry.Value.Key]);
		}
	}

	TMap<FGuid, TPair<int32, const T*>> Map;
};
