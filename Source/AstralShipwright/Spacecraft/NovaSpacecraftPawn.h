// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Neutron/Actor/NeutronTurntablePawn.h"
#include "Neutron/UI/NeutronUI.h"
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
	StructuresOnly,
	StructureModules,
	StructureModulesEquipment,
	StructureModulesEquipmentWiring,
	All
};

/** Index system with fading */
struct FNovaSpacecraftPawnCompartmentIndex
{
	FNovaSpacecraftPawnCompartmentIndex() : DesiredIndex(INDEX_NONE), CurrentIndex(INDEX_NONE), CurrentAlpha(0), FadeDuration(0)
	{}

	FNovaSpacecraftPawnCompartmentIndex(float FD) : DesiredIndex(INDEX_NONE), CurrentIndex(INDEX_NONE), CurrentAlpha(0), FadeDuration(FD)
	{}

	void Update(float DeltaTime)
	{
		if (CurrentIndex != DesiredIndex)
		{
			CurrentAlpha -= DeltaTime / FadeDuration;
		}
		else
		{
			CurrentAlpha += DeltaTime / FadeDuration;
		}
		CurrentAlpha = FMath::Clamp(CurrentAlpha, 0.0f, 1.0f);

		if (CurrentAlpha == 0)
		{
			CurrentIndex = DesiredIndex;
		}
	}

	void SetDesired(int32 Index)
	{
		DesiredIndex = Index;
	}

	int32 GetCurrent() const
	{
		return CurrentIndex;
	}

	float GetAlpha() const
	{
		return FMath::InterpEaseInOut(0.0f, 1.0f, CurrentAlpha, ENeutronUIConstants::EaseStandard);
	}

protected:

	int32 DesiredIndex;
	int32 CurrentIndex;
	float CurrentAlpha;
	float FadeDuration;
};

/** Main assembly actor that allows building boats */
UCLASS(ClassGroup = (Nova))
class ANovaSpacecraftPawn : public ANeutronTurntablePawn
{
	friend class UNovaCompartmentDescription;
	friend class UNovaModuleDescription;
	friend class UNovaEquipmentDescription;
	friend class UNovaAISpacecraftDescription;

	GENERATED_BODY()

public:

	ANovaSpacecraftPawn();

	/*----------------------------------------------------
	    General gameplay
	----------------------------------------------------*/

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual TPair<FVector, FVector> GetTurntableBounds() const override
	{
		return TPair<FVector, FVector>(CurrentOrigin, CurrentExtent);
	}

	/** Start editing the spacecraft */
	void SetEditing(bool Editing)
	{
		if (Editing != EditingSpacecraft)
		{
			NLOG("ANovaSpacecraftPawn::SetEditing %d", Editing);
		}
		EditingSpacecraft = Editing;
	}

	/** Check if we're editing */
	bool IsEditing() const
	{
		return EditingSpacecraft;
	}

	/** Share the identifier for the player spacecraft */
	void SetSpacecraftIdentifier(FGuid Identifier)
	{
		RequestedSpacecraftIdentifier = Identifier;
	}

	/** Return the spacecraft identifier */
	UFUNCTION(Category = Nova, BlueprintCallable)
	FGuid GetSpacecraftIdentifier() const
	{
		return RequestedSpacecraftIdentifier;
	}

	/** Get the spacecraft movement component */
	UFUNCTION(Category = Nova, BlueprintCallable)
	class UNovaSpacecraftMovementComponent* GetSpacecraftMovement() const
	{
		return MovementComponent;
	}

	/** Return a copy of the local spacecraft */
	FNovaSpacecraft GetSpacecraftCopy() const
	{
		return Spacecraft.IsValid() ? Spacecraft->GetSafeCopy() : FNovaSpacecraft();
	}

	/** Return the propulsion metrics */
	FNovaSpacecraftPropulsionMetrics GetPropulsionMetrics() const
	{
		return Spacecraft.IsValid() ? Spacecraft->GetPropulsionMetrics() : FNovaSpacecraftPropulsionMetrics();
	}

	/** Get the current spacecraft mass */
	float GetCurrentMass() const;

	/** Dock at the predefined location */
	void Dock(FSimpleDelegate Callback = FSimpleDelegate());

	/** Undock from the current dock */
	void Undock(FSimpleDelegate Callback = FSimpleDelegate());

	/** Check if we are docked */
	bool IsDocked() const;

	/** Check if we are docking*/
	bool IsDocking() const;

	/** Get a physical mesh used for anchoring */
	const class UPrimitiveComponent* GetAnchorComponent() const;

	/** Load the persistent state of systems from the spacecraft */
	void LoadSystems();

	UFUNCTION(Reliable, Server)
	void ServerLoadSystems();

	/** Save the persistent state of systems into the spacecraft */
	void SaveSystems();

	UFUNCTION(Reliable, Server)
	void ServerSaveSystems();

	/*----------------------------------------------------
	    Spacecraft pass-through for construction purposes only
	----------------------------------------------------*/

	/** Get a list of compartment kits that can be added at a (new) index */
	TArray<const class UNovaCompartmentDescription*> GetCompatibleCompartments(
		const class ANovaPlayerController* PC, int32 CompartmentIndex) const;

	/** Get a list of compatible modules that can be added at a compartment index, and a module slot index */
	TArray<const class UNovaModuleDescription*> GetCompatibleModules(
		const class ANovaPlayerController* PC, int32 CompartmentIndex, int32 SlotIndex) const;

	/** Get a list of compatible equipments that can be added at a compartment index, and an equipment slot index */
	TArray<const class UNovaEquipmentDescription*> GetCompatibleEquipment(
		const class ANovaPlayerController* PC, int32 CompartmentIndex, int32 SlotIndex) const;

	/** Find a specific module group for this spacecraft */
	const FNovaModuleGroup* FindModuleGroup(int32 CompartmentIndex, int32 ModuleIndex) const
	{
		NCHECK(Spacecraft.IsValid());
		return Spacecraft->FindModuleGroup(CompartmentIndex, ModuleIndex);
	}

	/** Get module groups for this spacecraft */
	const TArray<FNovaModuleGroup>& GetModuleGroups() const
	{
		NCHECK(Spacecraft.IsValid());
		return Spacecraft->GetModuleGroups();
	}

	/*----------------------------------------------------
	    Assembly interface
	----------------------------------------------------*/

	/** Check for modifications against the player ship */
	bool HasModifications() const;

	/** Revert the pawn to the game state version */
	void RevertModifications();

	/** Revert the pawn to the game state version */
	bool IsSpacecraftValid(FText* Details = nullptr) const
	{
		return Spacecraft.IsValid() && Spacecraft->IsValid(Details);
	}

	/** Rename the spacecraft */
	void RenameSpacecraft(FString Name)
	{
		NCHECK(Spacecraft.IsValid());
		Spacecraft->Name = Name;
	}

	/** Check if the assembly is idle */
	bool IsIdle() const
	{
		return AssemblyState == ENovaAssemblyState::Idle;
	}

	/** Save this assembly **/
	void ApplyAssembly();

	/** Insert a new compartment into the assembly */
	bool InsertCompartment(FNovaCompartment CompartmentRequest, int32 Index);

	/** Remove a compartment from the assembly */
	bool RemoveCompartment(int32 Index);

	/** Swap compartments */
	bool SwapCompartments(int32 IndexA, int32 IndexB);

	/** Swap modules */
	bool SwapModules(int32 CompartmentIndex, int32 IndexA, int32 IndexB);

	/** Swap equipment */
	bool SwapEquipment(int32 CompartmentIndex, int32 IndexA, int32 IndexB);

	/** Request updating of the assembly */
	void RequestAssemblyUpdate()
	{
		StartAssemblyUpdate();
	}

	/** Get a compartment */
	FNovaCompartment& GetCompartment(int32 Index);

	/** Get a compartment */
	const FNovaCompartment& GetCompartment(int32 Index) const;

	/** Get a compartment component */
	const class UNovaSpacecraftCompartmentComponent* GetCompartmentComponent(int32 Index) const;

	/** Get a compartment description */
	FNovaSpacecraftCompartmentMetrics GetCompartmentMetrics(int32 Index) const;

	/** Get the current number of compartments */
	int32 GetCompartmentCount() const;

	/** Get the current number of compartment components */
	int32 GetCompartmentComponentCount() const;

	/** Return which compartment index a primitive belongs to, or INDEX_NONE */
	int32 GetCompartmentIndexByPrimitive(const class UPrimitiveComponent* Component);

	/** Get the customization data */
	const struct FNovaSpacecraftCustomization& GetCustomization() const
	{
		NCHECK(Spacecraft.IsValid());
		return Spacecraft->GetCustomization();
	}

	/** Update the customization data */
	void UpdateCustomization(const struct FNovaSpacecraftCustomization& Customization);

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

	/** Check the current filter index */
	int32 GetCompartmentFilter() const
	{
		return DisplayFilterIndex;
	}

	/** Check if we're currently focusing a single compartment */
	bool IsFilteringCompartment() const
	{
		return DisplayFilterIndex != INDEX_NONE;
	}

	/** Set the compartment to highlight, none if INDEX_NONE */
	void SetHoveredCompartmentIndex(int32 Index)
	{
		HoveredCompartment.SetDesired(Index);
	}

	/** Set the compartment to outline, none if INDEX_NONE */
	void SetSelectedCompartmentIndex(int32 Index)
	{
		SelectedCompartment.SetDesired(Index);
	}

	/*----------------------------------------------------
	    Compartment assembly internals
	----------------------------------------------------*/

protected:

	/** Store a copy of a spacecraft and start editing it */
	void SetSpacecraft(const FNovaSpacecraft* NewSpacecraft);

	/** Update the assembly after a new compartment has been added or removed */
	void StartAssemblyUpdate();

	/** Update the assembly state */
	void UpdateAssembly();

	/** Update the display filter */
	void UpdateDisplayFilter();

	/** Create a new compartment component */
	class UNovaSpacecraftCompartmentComponent* CreateCompartment(const FNovaCompartment& Compartment);

	/** Run a difference process on a compartment assembly and call Callback on elements needing updating */
	void ProcessCompartmentIfDifferent(class UNovaSpacecraftCompartmentComponent* CompartmentComponent, const FNovaCompartment& Compartment,
		FNovaAssemblyCallback Callback);

	/** Run a list process on a compartment assembly and call Callback on all elements */
	void ProcessCompartment(class UNovaSpacecraftCompartmentComponent* CompartmentComponent, const FNovaCompartment& Compartment,
		FNovaAssemblyCallback Callback);

	/** Update the bounds */
	void UpdateBounds();

	/** Start moving compartments */
	void MoveCompartments();

	/** Build compartments */
	void BuildCompartments();

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:

	// Camera pitch scene container
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaSpacecraftMovementComponent* MovementComponent;

	// Crew system
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaSpacecraftCrewSystem* CrewSystem;

	// Power system
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaSpacecraftPowerSystem* PowerSystem;

	// Processing system
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaSpacecraftProcessingSystem* ProcessingSystem;

	// Propellant system
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaSpacecraftPropellantSystem* PropellantSystem;

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Identifier
	UPROPERTY(Replicated)
	FGuid RequestedSpacecraftIdentifier;

	// Assembly data
	TSharedPtr<FNovaSpacecraft>                        Spacecraft;
	ENovaAssemblyState                                 AssemblyState;
	TArray<class UNovaSpacecraftCompartmentComponent*> CompartmentComponents;
	bool                                               SelfDestruct;
	bool                                               EditingSpacecraft;

	// Asset loading
	bool                    WaitingAssetLoading;
	TArray<FSoftObjectPath> CurrentAssets;
	TArray<FSoftObjectPath> RequestedAssets;

	// Outlining
	FNovaSpacecraftPawnCompartmentIndex HoveredCompartment;
	FNovaSpacecraftPawnCompartmentIndex SelectedCompartment;

	// Display state
	ENovaAssemblyDisplayFilter DisplayFilterType;
	int32                      DisplayFilterIndex;
	FVector                    CurrentOrigin;
	FVector                    CurrentExtent;
	bool                       ImmediateMode;
};
