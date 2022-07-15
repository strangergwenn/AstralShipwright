// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftSystemInterface.h"
#include "Game/NovaOrbitalSimulationComponent.h"
#include "Components/SceneComponent.h"

#include "NovaSpacecraftProcessingSystem.generated.h"

/** Module state */
enum class ENovaSpacecraftProcessingSystemStatus : uint8
{
	Stopped,
	Processing,
	Blocked,
	Error
};

/** Full module state */
struct FNovaSpacecraftProcessingSystemModuleState
{
	FNovaSpacecraftProcessingSystemModuleState(const UNovaProcessingModuleDescription* M)
		: Status(ENovaSpacecraftProcessingSystemStatus::Stopped), Module(M)
	{}

	ENovaSpacecraftProcessingSystemStatus   Status;
	const UNovaProcessingModuleDescription* Module;
};

/** Resource processing system that transforms resources */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftProcessingSystem
	: public USceneComponent
	, public INovaSpacecraftSystemInterface
{
	GENERATED_BODY()

public:

	UNovaSpacecraftProcessingSystem();

	/*----------------------------------------------------
	    System implementation
	----------------------------------------------------*/

	virtual void Load(const FNovaSpacecraft& Spacecraft) override;

	virtual void Save(FNovaSpacecraft& Spacecraft) override;

	virtual void Update(FNovaTime InitialTime, FNovaTime FinalTime) override;

	/** Get cargo for a specific module */
	const FNovaSpacecraftCargo& GetCargo(int32 CompartmentIndex, int32 ModuleIndex) const
	{
		if (IsSpacecraftDocked())
		{
			const FNovaCompartment& Compartment = GetSpacecraft()->Compartments[CompartmentIndex];
			return Compartment.GetCargo(ModuleIndex);
		}
		else
		{
			return RealtimeCargo[CompartmentIndex][ModuleIndex];
		}
	}

	/** Get the current processing status for a module */
	ENovaSpacecraftProcessingSystemStatus GetModuleStatus(int32 CompartmentIndex, int32 ModuleIndex) const;

	/** Set the active state for a module */
	void SetModuleActive(int32 CompartmentIndex, int32 ModuleIndex, bool Active);

	/** Get the cargo type for a module */
	ENovaResourceType GetCargoType(int32 CompartmentIndex, int32 ModuleIndex) const
	{
		const FNovaCompartment& Compartment = GetSpacecraft()->Compartments[CompartmentIndex];
		return Compartment.GetCargoType(ModuleIndex);
	}

	/*----------------------------------------------------
	    Internal
	----------------------------------------------------*/
protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	TArray<TStaticArray<FNovaSpacecraftCargo, ENovaConstants::MaxModuleCount>> RealtimeCargo;
	TMap<TPair<int32, int32>, FNovaSpacecraftProcessingSystemModuleState>      ModuleStates;
};
