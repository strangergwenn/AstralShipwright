// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/Actor/NeutronActorTools.h"
#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronTabView.h"
#include "Neutron/UI/Widgets/NeutronListView.h"

#include "Online.h"

enum class ENovaResourceType : uint8;

/** Inventory menu */
class SNovaMainMenuOperations
	: public SNeutronTabPanel
	, public INeutronGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuOperations)
	{}

	SLATE_ARGUMENT(class SNeutronMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNeutronMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	SNovaMainMenuOperations();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void Show() override;

	virtual void Hide() override;

	virtual void UpdateGameObjects() override;

	/*----------------------------------------------------
	    Resource list
	----------------------------------------------------*/

	TSharedRef<SWidget> GenerateResourceItem(const class UNovaResource* Resource);

	FText GenerateResourceTooltip(const class UNovaResource* Resource);

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

	FText GetSlotHelpText() const;

	TOptional<float> GetPropellantRatio() const;

	FText GetPropellantText() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

	void OnRefillPropellant();

	void OnTradeWithSlot(int32 CompartmentIndex, int32 ModuleIndex);

	void OnBuyResource();

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Game objects
	TWeakObjectPtr<UNeutronMenuManager> MenuManager;
	class ANovaPlayerController*        PC;
	class ANovaGameState*               GameState;
	const struct FNovaSpacecraft*       Spacecraft;
	const class ANovaSpacecraftPawn*    SpacecraftPawn;
	int32                               CurrentCompartmentIndex;
	int32                               CurrentModuleIndexx;
	TNeutronTimedAverage<float>         AveragedPropellantRatio;

	// Resource list
	TArray<const class UNovaResource*>                       ResourceList;
	TSharedPtr<SNeutronListView<const class UNovaResource*>> ResourceListView;

	// Slate widgets
	TSharedPtr<class SNeutronModalPanel> GenericModalPanel;
	TSharedPtr<class SNovaTradingPanel>  TradingModalPanel;
};
