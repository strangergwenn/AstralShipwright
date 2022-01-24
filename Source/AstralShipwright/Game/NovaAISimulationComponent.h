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
	Docked
};

/** AI spacecraft data */
USTRUCT()
struct FNovaAISpacecraftState
{
	FNovaAISpacecraftState() : PhysicalSpacecraft(nullptr), State(ENovaAISpacecraftState::Idle)
	{}

	GENERATED_BODY()

	UPROPERTY()
	class ANovaSpacecraftPawn* PhysicalSpacecraft;

	ENovaAISpacecraftState State;
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

	/** Handle travel decisions for spacecraft */
	void ProcessNavigation();

	/** Handle the movement of physical spacecraft */
	void ProcessPhysicalMovement();

	/*----------------------------------------------------
	    Helpers
	----------------------------------------------------*/

protected:
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
