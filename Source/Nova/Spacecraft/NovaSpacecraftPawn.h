// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Nova/Actor/NovaTurntablePawn.h"
#include "Nova/UI/NovaUITypes.h"
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
		return FMath::InterpEaseInOut(0.0f, 1.0f, CurrentAlpha, ENovaUIConstants::EaseStandard);
	}

protected:
	int32 DesiredIndex;
	int32 CurrentIndex;
	float CurrentAlpha;
	float FadeDuration;
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
	    General gameplay
	----------------------------------------------------*/

	virtual void Tick(float DeltaTime) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual TPair<FVector, FVector> GetTurntableBounds() const override
	{
		return TPair<FVector, FVector>(CurrentOrigin, CurrentExtent);
	}

	/** Return the spacecraft identifier */
	UFUNCTION(Category = Nova, BlueprintCallable)
	FGuid GetSpacecraftIdentifier() const
	{
		return Spacecraft.IsValid() ? Spacecraft->Identifier : FGuid();
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

	/** Dock at a particular location */
	void Dock(FSimpleDelegate Callback = FSimpleDelegate());

	/** Undock from the current dock */
	void Undock(FSimpleDelegate Callback = FSimpleDelegate());

	/** Check the dock state */
	bool IsDocked() const;

	/** Load the persistent state of systems from the spacecraft */
	void LoadSystems();

	UFUNCTION(Reliable, Server)
	void ServerLoadSystems();

	/** Save the persistent state of systems into the spacecraft */
	void SaveSystems();

	UFUNCTION(Reliable, Server)
	void ServerSaveSystems();

	/*----------------------------------------------------
	    Assembly interface
	----------------------------------------------------*/

	/** Check for modifications against the player ship */
	bool HasModifications() const;

	/** Revert the pawn to the game state version */
	void ResetSpacecraft()
	{
		Spacecraft.Reset();
	}

	/** Revert the pawn to the game state version */
	bool IsSpacecraftValid(FText* Details = nullptr)
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

	/** Get a list of compartment kits that can be added at a (new) index */
	TArray<const class UNovaCompartmentDescription*> GetCompatibleCompartments(int32 CompartmentIndex) const
	{
		NCHECK(Spacecraft.IsValid());
		return Spacecraft->GetCompatibleCompartments(CompartmentIndex);
	}

	/** Get a list of compatible modules that can be added at a compartment index, and a module slot index */
	TArray<const class UNovaModuleDescription*> GetCompatibleModules(int32 CompartmentIndex, int32 SlotIndex) const
	{
		NCHECK(Spacecraft.IsValid());
		return Spacecraft->GetCompatibleModules(CompartmentIndex, SlotIndex);
	}

	/** Get a list of compatible equipments that can be added at a compartment index, and an equipment slot index */
	TArray<const class UNovaEquipmentDescription*> GetCompatibleEquipments(int32 CompartmentIndex, int32 SlotIndex) const
	{
		NCHECK(Spacecraft.IsValid());
		return Spacecraft->GetCompatibleEquipments(CompartmentIndex, SlotIndex);
	}

	/** Save this assembly **/
	void ApplyAssembly();

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

	/** Get a compartment */
	const FNovaCompartment& GetCompartment(int32 Index) const;

	/** Get a compartment description */
	FNovaSpacecraftCompartmentMetrics GetCompartmentMetrics(int32 Index) const;

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
	void SetHighlightedCompartment(int32 Index)
	{
		HighlightedCompartment.SetDesired(Index);
	}

	/** Set the compartment to outline, none if INDEX_NONE */
	void SetOutlinedCompartment(int32 Index)
	{
		OutlinedCompartment.SetDesired(Index);
	}

	/** Get the compartment highlight alpha */
	float GetHighlightAlpha() const
	{
		return HighlightedCompartment.GetAlpha();
	}

	/** Get the compartment outline alpha */
	float GetOutlineAlpha() const
	{
		return OutlinedCompartment.GetAlpha();
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

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:
	// Camera pitch scene container
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaSpacecraftMovementComponent* MovementComponent;

	// Propellant system
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaSpacecraftPropellantSystem* PropellantSystem;

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Assembly data
	TSharedPtr<FNovaSpacecraft>                        Spacecraft;
	ENovaAssemblyState                                 AssemblyState;
	TArray<class UNovaSpacecraftCompartmentComponent*> CompartmentComponents;

	// Asset loading
	bool                    WaitingAssetLoading;
	TArray<FSoftObjectPath> CurrentAssets;
	TArray<FSoftObjectPath> RequestedAssets;

	// Outlining
	FNovaSpacecraftPawnCompartmentIndex HighlightedCompartment;
	FNovaSpacecraftPawnCompartmentIndex OutlinedCompartment;

	// Display state
	ENovaAssemblyDisplayFilter DisplayFilterType;
	int32                      DisplayFilterIndex;
	FVector                    CurrentOrigin;
	FVector                    CurrentExtent;
	bool                       ImmediateMode;
};
