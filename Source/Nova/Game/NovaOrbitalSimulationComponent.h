// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Nova/Game/NovaGameTypes.h"
#include "Nova/Spacecraft/NovaSpacecraft.h"

#include "NovaOrbitalSimulationComponent.generated.h"

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

	/** Compute a trajectory */
	TSharedPtr<FNovaTrajectory> ComputeTrajectory(const class UNovaArea* Source, const class UNovaArea* Destination);

	/** Get the trajectory map for all areas */
	TMap<const class UNovaArea*, FNovaTrajectory> GetAreaTrajectories() const
	{
		return AreaTrajectories;
	}

	/** Get the trajectory map for all spacecraft */
	TMap<TWeakPtr<struct FNovaSpacecraft>, FNovaTrajectory> GetSpacecraftTrajectories() const
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
	TArray<const class UNovaArea*>                          Areas;
	TMap<const class UNovaArea*, FNovaTrajectory>           AreaTrajectories;
	TMap<TWeakPtr<struct FNovaSpacecraft>, FNovaTrajectory> SpacecraftTrajectories;
};
