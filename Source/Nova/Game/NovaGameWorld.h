// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"

#include "NovaGameTypes.h"
#include "Nova/Spacecraft/NovaSpacecraft.h"

#include "NovaGameWorld.generated.h"

/** Time dilation settings */
UENUM()
enum class ENovaTimeDilation : uint8
{
	Normal,
	Level1,
	Level2,
	Level3,
	Level4
};

/** Spacecraft database with fast array replication and fast lookup */
USTRUCT()
struct FNovaSpacecraftDatabase : public FFastArraySerializer
{
	GENERATED_BODY()

	bool Add(const FNovaSpacecraft& Spacecraft)
	{
		return Cache.Add(*this, Array, Spacecraft);
	}

	void Remove(const FGuid& Identifier)
	{
		Cache.Remove(*this, Array, Identifier);
	}

	const FNovaSpacecraft* Get(const FGuid& Identifier) const
	{
		return Cache.Get(Identifier, Array);
	}

	TArray<FNovaSpacecraft>& Get()
	{
		return Array;
	}

	void UpdateCache()
	{
		Cache.Update(Array);
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNovaSpacecraft, FNovaSpacecraftDatabase>(Array, DeltaParms, *this);
	}

protected:
	UPROPERTY()
	TArray<FNovaSpacecraft> Array;

	TGuidCacheMap<FNovaSpacecraft> Cache;
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
	virtual void Tick(float DeltaTime) override;

	/** Get this actor */
	static ANovaGameWorld* Get(const UObject* Outer);

	/** Register or update a spacecraft */
	void UpdateSpacecraft(const FNovaSpacecraft& Spacecraft, bool IsPlayerSpacecraft);

	/** Return a pointer for a spacecraft by identifier */
	const FNovaSpacecraft* GetSpacecraft(FGuid Identifier)
	{
		return SpacecraftDatabase.Get(Identifier);
	}

	/** Return the orbital simulation class */
	class UNovaOrbitalSimulationComponent* GetOrbitalSimulation() const
	{
		return OrbitalSimulationComponent;
	}

	/** Get the current time dilation factor */
	void SetTimeDilation(ENovaTimeDilation Dilation);

	/** Increase time dilation by one level */
	void IncreaseTimeDilation()
	{
		if (ServerTimeDilation < ENovaTimeDilation::Level4)
		{
			SetTimeDilation(static_cast<ENovaTimeDilation>(static_cast<int32>(ServerTimeDilation) + 1));
		}
	}

	/** Decrease time dilation by one level */
	void DecreaseTimeDilation()
	{
		if (ServerTimeDilation > ENovaTimeDilation::Normal)
		{
			SetTimeDilation(static_cast<ENovaTimeDilation>(static_cast<int32>(ServerTimeDilation) - 1));
		}
	}

	/** Stop the time dilation */
	void StopTimeDilation()
	{
		SetTimeDilation(ENovaTimeDilation::Normal);
	}

	/** Get time dilation values */
	static float GetTimeDilationValue(ENovaTimeDilation Dilation)
	{
		constexpr float TimeDilationValues[] = {
			1,        // Normal
			60,       // Level1
			1200,     // Level2
			14400,    // Level3
			86400     // Level4
		};

		return TimeDilationValues[static_cast<int32>(Dilation)];
	}

	/** Get the current game time in minutes */
	double GetCurrentTime() const;

	/** Get the time in the last simulated frame in minutes */
	double GetPreviousTime() const;

	/** Get the current time dilation */
	ENovaTimeDilation GetCurrentTimeDilation() const
	{
		return ServerTimeDilation;
	}

	/** Get the current time dilation */
	double GetCurrentTimeDilationValue() const
	{
		return GetTimeDilationValue(ServerTimeDilation);
	}

	/** Check if we can dilate time */
	bool CanDilateTime(ENovaTimeDilation Dilation) const;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Update the spacecraft database */
	void ProcessSpacecraftDatabase();

	/** Process dilation wake events */
	void ProcessWakeEvents();

	/** Process time */
	void ProcessTime(float DeltaTime);

	/** Server replication event for time reconciliation */
	UFUNCTION()
	void OnServerTimeReplicated();

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Threshold in seconds above which the client time starts compensating
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MinimumTimeCorrectionThreshold;

	// Threshold in seconds above which the client time is at maximum compensation
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaximumTimeCorrectionThreshold;

	// Maximum time dilation applied to compensate time
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float TimeCorrectionFactor;

	// Time in minutes to leave players before a maneuver
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float TimeDilationManeuverDelay;

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

	// Replicated world time value
	UPROPERTY(ReplicatedUsing = OnServerTimeReplicated)
	double ServerTime;

	// Replicated world time dilation
	UPROPERTY(Replicated)
	ENovaTimeDilation ServerTimeDilation;

	// Local state
	double                         ClientTime;
	double                         ClientAdditionalTimeDilation;
	double                         PreviousTime;
	TArray<const class UNovaArea*> Areas;
};
