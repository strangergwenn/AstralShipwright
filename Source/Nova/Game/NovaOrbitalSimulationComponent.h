// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Nova/Spacecraft/NovaSpacecraft.h"

#include "NovaOrbitalSimulationComponent.generated.h"

/** Spacecraft trajectory data */
struct FNovaSpacecraftTrajectory
{
	FNovaSpacecraftTrajectory(float SA, float CP) : StartAltitude(SA), EndAltitude(SA), StartPhase(0), EndPhase(360), CurrentPhase(CP)
	{}

	FNovaSpacecraftTrajectory(float SA, float EA, float SP, float EP, float CP)
		: StartAltitude(SA), EndAltitude(EA), StartPhase(SP), EndPhase(EP), CurrentPhase(CP)
	{}

	float StartAltitude;
	float EndAltitude;
	float StartPhase;
	float EndPhase;
	float CurrentPhase;
};

/** Orbital simulation component that ticks orbiting spacecraft */
UCLASS(ClassGroup = (Nova))
class UNovaOrbitalSimulationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNovaOrbitalSimulationComponent();

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

public:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Get the trajectory map for all areas */
	TMap<const class UNovaArea*, FNovaSpacecraftTrajectory> GetAreaTrajectories() const
	{
		return AreaTrajectories;
	}

	/** Get the trajectory map for all spacecraft */
	TMap<TWeakPtr<struct FNovaSpacecraft>, FNovaSpacecraftTrajectory> GetSpacecraftTrajectories() const
	{
		return SpacecraftTrajectories;
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Update all trajectories based on the current time */
	void UpdateTrajectories();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Local state
	TArray<const class UNovaArea*>                                    Areas;
	TMap<const class UNovaArea*, FNovaSpacecraftTrajectory>           AreaTrajectories;
	TMap<TWeakPtr<struct FNovaSpacecraft>, FNovaSpacecraftTrajectory> SpacecraftTrajectories;
};
