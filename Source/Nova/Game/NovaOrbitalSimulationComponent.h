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

	/** Get the trajectory data */
	TMap<const class UNovaArea*, FNovaTrajectory> GetTrajectories() const
	{
		return Trajectories;
	}

	/** Get the position data */
	TMap<const class UNovaArea*, float> GetPositions() const
	{
		return Positions;
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Update all trajectories */
	void UpdateTrajectories();

	/** Update all positions based on the current time */
	void UpdatePositions();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Local state
	TArray<const class UNovaArea*>                Areas;
	TMap<const class UNovaArea*, FNovaTrajectory> Trajectories;
	TMap<const class UNovaArea*, float>           Positions;
};
