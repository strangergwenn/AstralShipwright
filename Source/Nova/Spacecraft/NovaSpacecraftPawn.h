// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Nova/Actor/NovaTurntablePawn.h"
#include "NovaSpacecraft.h"
#include "NovaSpacecraftPawn.generated.h"


/** Current assembly state */
enum class ENovaAssemblyState : uint8
{
	Idle,
	LoadingDematerializing,
	Moving,
	Building
};

/** Display modes for assemblies */
enum class ENovaAssemblyDisplayFilter : uint8
{
	ModulesOnly,
	ModulesStructure,
	ModulesStructureEquipments,
	ModulesStructureEquipmentsWiring,
	All
};


/** Main assembly actor that allows building boats */
UCLASS(Config = Game, ClassGroup = (Nova))
class ANovaSpacecraftPawn : public ANovaTurntablePawn
{
	friend class ANovaCaptureActor;

	GENERATED_BODY()

public:

	ANovaSpacecraftPawn();


	/*----------------------------------------------------
		Loading & saving
	----------------------------------------------------*/

	static void SerializeJson(TSharedPtr<FNovaSpacecraft>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);


	/*----------------------------------------------------
		Multiplayer
	----------------------------------------------------*/

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetSpacecraft(const struct FNovaSpacecraft& NewSpacecraft);

	UFUNCTION()
	void OnServerSpacecraftReplicated();


	/*----------------------------------------------------
		Gameplay
	----------------------------------------------------*/

	virtual void Tick(float DeltaTime) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual TPair<FVector, FVector> GetTurntableBounds() const override
	{
		return TPair<FVector, FVector>(CurrentOrigin, CurrentExtent);
	}

	/** Check if the assembly is idle */
	bool IsIdle() const
	{
		return AssemblyState == ENovaAssemblyState::Idle;
	}

	/** Get a list of compartment kits that can be added at a (new) index */
	TArray<const class UNovaCompartmentDescription*> GetCompatibleCompartments(int32 Index) const;
	
	/** get a list of compatible modules that can be added at a compartment index, and a module slot index */
	TArray<const class UNovaModuleDescription*> GetCompatibleModules(int32 Index, int32 SlotIndex) const;
	
	/** get a list of compatible equipments that can be added at a compartment index, and an equipment slot index */
	TArray<const class UNovaEquipmentDescription*> GetCompatibleEquipments(int32 Index, int32 SlotIndex) const;

	/** Save this assembly **/
	void SaveAssembly();

	/** Store a copy of this spacecraft and start editing it */
	void SetSpacecraft(const TSharedPtr<FNovaSpacecraft> NewSpacecraft);

	/** Get a shared pointer to the current spacecraft state */
	const TSharedPtr<FNovaSpacecraft> GetSpacecraft() const
	{
		return Spacecraft;
	}

	/** Insert a new compartment into the assembly */
	bool InsertCompartment(FNovaCompartment CompartmentRequest, int32 Index);

	/** Remove a compartment from the assembly */
	bool RemoveCompartment(int32 Index);

	/** Request updating of the assembly */
	void RequestAssemblyUpdate()
	{
		StartAssemblyUpdate();
	}


	/** Get a compartment */
	FNovaCompartment& GetCompartment(int32 Index);

	/** Get the current number of compartments */
	int32 GetCompartmentCount() const;

	/** Return which compartment index a primitive belongs to, or INDEX_NONE */
	int32 GetCompartmentIndexByPrimitive(const class UPrimitiveComponent* Component);


	/** Set this compartment to immediate mode (no animations or shader transitions) */
	void SetImmediateMode(bool Value)
	{
		ImmediateMode = Value;
	}

	/** Update the visual preferences with an element-type filter and a compartment-type filter */
	void SetDisplayFilter(ENovaAssemblyDisplayFilter Filter, int32 CompartmentIndex);

	/** Check the current filter state */
	ENovaAssemblyDisplayFilter GetDisplayFilter() const
	{
		return DisplayFilterType;
	}

	/** Check if we're currently focusing a single compartment */
	bool IsFilteringCompartment() const
	{
		return DisplayFilterIndex != INDEX_NONE;
	}

	/** Set the compartment to highlight, none if INDEX_NONE */
	void SetHighlightCompartment(int32 Index)
	{
		CurrentHighlightCompartment = Index;
	}


	/*----------------------------------------------------
		Compartment assembly internals
	----------------------------------------------------*/

protected:

	/** Update the assembly after a new compartment has been added or removed */
	void StartAssemblyUpdate();

	/** Update the assembly state */
	void UpdateAssembly();

	/** Update the display filter */
	void UpdateDisplayFilter();

	/** Create a new compartment component */
	class UNovaSpacecraftCompartmentComponent* CreateCompartment(
		const FNovaCompartment& Compartment);

	/** Run a difference process on a compartment assembly and call Callback on elements needing updating */
	void ProcessCompartmentIfDifferent(
		class UNovaSpacecraftCompartmentComponent* CompartmentComponent,
		const FNovaCompartment& Compartment,
		FNovaAssemblyCallback Callback);

	/** Run a list process on a compartment assembly and call Callback on all elements */
	void ProcessCompartment(
		class UNovaSpacecraftCompartmentComponent* CompartmentComponent,
		const FNovaCompartment& Compartment,
		FNovaAssemblyCallback Callback);

	/** Update the bounds */
	void UpdateBounds();
	

	/*----------------------------------------------------
		Components
	----------------------------------------------------*/

protected:

	// Camera pitch scene container
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaSpacecraftMovementComponent* MovementComponent;


protected:

	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

	// Assembly data
	TSharedPtr<FNovaSpacecraft>                   Spacecraft;
	ENovaAssemblyState                            AssemblyState;
	TArray<class UNovaSpacecraftCompartmentComponent*> CompartmentComponents;

	// Server-side spacecraft
	UPROPERTY(ReplicatedUsing = OnServerSpacecraftReplicated)
	FNovaSpacecraft                               ServerSpacecraft;

	// Asset loading
	bool                                          WaitingAssetLoading;
	TArray<FSoftObjectPath>                       CurrentAssets;
	TArray<FSoftObjectPath>                       RequestedAssets;

	// Display state
	int32                                         CurrentHighlightCompartment;
	ENovaAssemblyDisplayFilter                    DisplayFilterType;
	int32                                         DisplayFilterIndex;
	FVector                                       CurrentOrigin;
	FVector                                       CurrentExtent;
	bool                                          ImmediateMode;

};
