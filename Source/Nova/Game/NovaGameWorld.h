// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"

#include "NovaGameTypes.h"
#include "Nova/Spacecraft/NovaSpacecraft.h"

#include "NovaGameWorld.generated.h"

/** Spacecraft database with fast array replication and fast lookup */
USTRUCT()
struct FNovaSpacecraftDatabase : public FFastArraySerializer
{
	GENERATED_BODY()

public:
	/** Add a new spacecraft to the database or update an existing entry with the same identifier */
	void AddOrUpdate(const FNovaSpacecraft& Spacecraft);

	/** Remove a spacecraft from the database */
	void Remove(const FNovaSpacecraft& Spacecraft);

	/** Get a spacecraft */
	FNovaSpacecraft* Get(const FGuid& Identifier)
	{
		TPair<int32, FNovaSpacecraft*>* Entry = Map.Find(Identifier);
		return Entry ? Entry->Value : nullptr;
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNovaSpacecraft, FNovaSpacecraftDatabase>(Array, DeltaParms, *this);
	}

	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

protected:
	// Replicated state
	UPROPERTY()
	TArray<FNovaSpacecraft> Array;

	// Local state
	TMap<FGuid, TPair<int32, FNovaSpacecraft*>> Map;
};

/** Enable fast replication */
template <>
struct TStructOpsTypeTraits<FNovaSpacecraftDatabase> : public TStructOpsTypeTraitsBase2<FNovaSpacecraftDatabase>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/** World manager class */
UCLASS(ClassGroup = (Nova))
class ANovaGameWorld : public AActor
{
	GENERATED_BODY()

public:
	ANovaGameWorld();

	/*----------------------------------------------------
	    Loading & saving
	----------------------------------------------------*/

	TSharedPtr<struct FNovaWorldSave> Save() const;

	void Load(TSharedPtr<struct FNovaWorldSave> SaveData);

	static void SerializeJson(
		TSharedPtr<struct FNovaWorldSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

public:
	/** Register a new player spacecraft */
	void AddOrUpdateSpacecraft(const FNovaSpacecraft Spacecraft);

	/** Return a pointer for a spacecraft by identifier */
	FNovaSpacecraft* GetSpacecraft(FGuid Identifier);

	/** Return the orbital simulation class */
	class UNovaOrbitalSimulationComponent* GetOrbitalSimulation() const
	{
		return OrbitalSimulationComponent;
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Update the local database */
	void UpdateDatabase();

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:
	// Global orbital simulation manager
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaOrbitalSimulationComponent* OrbitalSimulationComponent;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Replicated spacecraft database
	UPROPERTY(Replicated)
	FNovaSpacecraftDatabase SpacecraftDatabase;

	// Local state
	TArray<const class UNovaArea*> Areas;
};
