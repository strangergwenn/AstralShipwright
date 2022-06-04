// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "NovaGameTypes.h"
#include "NovaAISpacecraft.h"
#include "NovaOrbitalSimulationTypes.h"
#include "NovaAISimulationComponent.generated.h"

/** AI states */
UENUM()
enum class ENovaAISpacecraftState : uint8
{
	Idle,
	Trajectory,
	Station,
	Undocking
};

/** AI spacecraft save */
USTRUCT()
struct FNovaAISpacecraftStateSave
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid SpacecraftIdentifier;

	UPROPERTY()
	const class UNovaAISpacecraftDescription* SpacecraftClass;

	UPROPERTY()
	FString SpacecraftName;

	UPROPERTY()
	const class UNovaArea* TargetArea;

	UPROPERTY()
	ENovaAISpacecraftState CurrentState;

	UPROPERTY()
	FNovaTime CurrentStateStartTime;

	UPROPERTY()
	FNovaOrbit Orbit;

	UPROPERTY()
	FNovaTrajectory Trajectory;
};

/** AI save */
USTRUCT()
struct FNovaAIStateSave
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FNovaAISpacecraftStateSave> SpacecraftStates;

	UPROPERTY()
	int32 PatrolRandomStreamSeed;
};

/** AI spacecraft data */
USTRUCT()
struct FNovaAISpacecraftState
{
	FNovaAISpacecraftState()
		: PhysicalSpacecraft(nullptr), TargetArea(nullptr), CurrentState(ENovaAISpacecraftState::Idle), CurrentStateStartTime(0)
	{}

	GENERATED_BODY()

	UPROPERTY()
	class ANovaSpacecraftPawn* PhysicalSpacecraft;

	UPROPERTY()
	const class UNovaArea* TargetArea;

	ENovaAISpacecraftState CurrentState;

	FNovaTime CurrentStateStartTime;
};

/** AI spacecraft control component */
UCLASS(ClassGroup = (Nova))
class UNovaAISimulationComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UNovaAISimulationComponent();

	/*----------------------------------------------------
	    Loading & saving
	----------------------------------------------------*/

	FNovaAIStateSave Save() const;

	void Load(const FNovaAIStateSave& SaveData);

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Get a physical spacecraft */
	const class ANovaSpacecraftPawn* GetPhysicalSpacecraft(FGuid Identifier) const
	{
		const FNovaAISpacecraftState* SpacecraftStatePtr = SpacecraftDatabase.Find(Identifier);
		return SpacecraftStatePtr ? SpacecraftStatePtr->PhysicalSpacecraft : nullptr;
	}

	/** Load the physical spacecraft for given identifier */
	void SetRequestedPhysicalSpacecraft(FGuid Identifier)
	{
		AlwaysLoadedSpacecraft = Identifier;
	}

	/*----------------------------------------------------
	    Internals high-level
	----------------------------------------------------*/

protected:

	/** Update the quotas map */
	void ProcessQuotas();

	/** Handle the spawning and de-spawning of physical spacecraft */
	void ProcessSpawning();

	/** Handle travel and movement decisions for spacecraft */
	void ProcessNavigation();

	/*----------------------------------------------------
	    Helpers
	----------------------------------------------------*/

protected:

	/** Create new game data */
	void CreateGame();

	/** Get a ship name for a tug or mining ship */
	FString GetTechnicalShipName(FRandomStream& RandomStream, int32 Index) const;

	/** Change the spacecraft state */
	void SetSpacecraftState(FNovaAISpacecraftState& State, ENovaAISpacecraftState NewState);

	/** Compute a trajectory between two orbits */
	void StartTrajectory(const struct FNovaOrbit& SourceOrbit, const struct FNovaOrbit& DestinationOrbit, FNovaTime DeltaTime,
		const TArray<FGuid>& Spacecraft);

	/** Find an area to travel to */
	const class UNovaArea* FindArea(const struct FNovaOrbitalLocation* SourceLocation) const;

	/** Find a patrol orbit to travel to */
	void FindPatrolOrbit(struct FNovaOrbit& DestinationOrbit) const;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:

	// Database
	UPROPERTY()
	TMap<FGuid, FNovaAISpacecraftState> SpacecraftDatabase;

	// General state
	TArray<FString>                     TechnicalNamePrefixes;
	TArray<FString>                     TechnicalNameSuffixes;
	FGuid                               AlwaysLoadedSpacecraft;
	TMap<const class UNovaArea*, int32> AreasQuotas;
	FRandomStream                       PatrolRandomStream;
};
