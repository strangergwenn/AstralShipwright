// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "NovaGameTypes.h"
#include "NovaAISpacecraft.h"
#include "NovaAISimulationComponent.generated.h"

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

	/** Get a specific spacecraft */
	const FNovaAISpacecraft* GetSpacecraft(FGuid Identifier) const
	{
		return SpacecraftDatabase.Find(Identifier);
	}

	/** Get all spacecraft */
	const TMap<FGuid, struct FNovaAISpacecraft>& GetSpacecrafts() const
	{
		return SpacecraftDatabase;
	}

	/** Get a physical spacecraft */
	const class ANovaSpacecraftPawn* GetPhysicalSpacecraft(FGuid Identifier) const
	{
		ANovaSpacecraftPawn* const* SpacecraftPawnPtr = PhysicalSpacecraftDatabase.Find(Identifier);
		return SpacecraftPawnPtr ? *SpacecraftPawnPtr : nullptr;
	}

	/** Load the physical spacecraft for given identifier */
	void SetRequestedPhysicalSpacecraft(FGuid Identifier)
	{
		AlwaysLoadedSpacecraft = Identifier;
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Database
	FGuid                                   AlwaysLoadedSpacecraft;
	TMap<FGuid, FNovaAISpacecraft>          SpacecraftDatabase;
	TMap<FGuid, class ANovaSpacecraftPawn*> PhysicalSpacecraftDatabase;
};
