// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftSystemInterface.h"
#include "Components/SceneComponent.h"

#include "NovaSpacecraftCrewSystem.generated.h"

/** Crew system */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftCrewSystem
	: public USceneComponent
	, public INovaSpacecraftSystemInterface
{
	GENERATED_BODY()

public:

	UNovaSpacecraftCrewSystem();

	/*----------------------------------------------------
	    System implementation
	----------------------------------------------------*/

	virtual void Load(const FNovaSpacecraft& Spacecraft) override;

	virtual void Save(FNovaSpacecraft& Spacecraft) override;

	virtual void Update(FNovaTime InitialTime, FNovaTime FinalTime) override;

	/*----------------------------------------------------
	    Crew
	----------------------------------------------------*/

	/** Get the current active crew count */
	int32 GetCurrentCrew() const;

	/** Get the required crew count for a group to be active */
	int32 GetRequiredCrew(int32 ProcessingGroupIndex) const;

	/** Get the total required crew count */
	int32 GetTotalRequiredCrew() const;

	/** Get the current busy crew count for a group */
	int32 GetBusyCrew(int32 ProcessingGroupIndex) const;

	/** Get the total current busy crew count */
	int32 GetTotalBusyCrew() const;

	/** Get the total current available crew count */
	int32 GetTotalAvailableCrew() const
	{
		return FMath::Max(GetCurrentCrew() - GetTotalBusyCrew(), 0);
	}

	/** Get the current active crew */
	int32 GetCrewCapacity() const;

	/** Return the daily crew cost */
	FNovaCredits GetDailyCost() const
	{
		return GetCurrentCrew() * GetDailyCostPerCrew();
	}

	/** Return the daily crew cost per head */
	FNovaCredits GetDailyCostPerCrew() const;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:

	/** Get the processing system */
	const class UNovaSpacecraftProcessingSystem* GetProcessingSystem() const;

	/** Get the player controller */
	class ANovaPlayerController* GetPC() const;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Local state
	FNovaTime CurrentTimeSincePayday;
};
