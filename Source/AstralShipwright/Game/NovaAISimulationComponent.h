// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "NovaGameTypes.h"
#include "NovaAISpacecraft.h"
#include "NovaAISimulationComponent.generated.h"

/** AI states */
enum class ENovaAISpacecraftState : uint8
{
	Idle,
	Trajectory,
	Station,
	Undocking
};

/** AI spacecraft data */
USTRUCT()
struct FNovaAISpacecraftState
{
	FNovaAISpacecraftState() : PhysicalSpacecraft(nullptr), CurrentState(ENovaAISpacecraftState::Idle), CurrentStateStartTime(0)
	{}

	GENERATED_BODY()

	UPROPERTY()
	class ANovaSpacecraftPawn* PhysicalSpacecraft;

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
	    Interface
	----------------------------------------------------*/

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Reset the component */
	void Initialize();

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
	/** Handle the spawning and de-spawning of physical spacecraft */
	void ProcessSpawning();

	/** Handle travel and movement decisions for spacecraft */
	void ProcessNavigation();

	/*----------------------------------------------------
	    Helpers
	----------------------------------------------------*/

protected:
	/** Change the spacecraft state */
	void SetSpacecraftState(FNovaAISpacecraftState& State, ENovaAISpacecraftState NewState);

	/** Compute a trajectory between two orbits */
	void StartTrajectory(const struct FNovaOrbit& SourceOrbit, const struct FNovaOrbit& DestinationOrbit, FNovaTime DeltaTime,
		const TArray<FGuid>& Spacecraft);

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Database
	UPROPERTY()
	TMap<FGuid, FNovaAISpacecraftState> SpacecraftDatabase;

	// General state
	FGuid AlwaysLoadedSpacecraft;
};
