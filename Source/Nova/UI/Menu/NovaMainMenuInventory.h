// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaListView.h"

#include "Nova/Actor/NovaActorTools.h"

#include "Online.h"

enum class ENovaResourceType : uint8;

/** Inventory menu */
class SNovaMainMenuInventory
	: public SNovaTabPanel
	, public INovaGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuInventory)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	SNovaMainMenuInventory();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void Show() override;

	virtual void Hide() override;

	virtual void UpdateGameObjects() override;

	virtual TSharedPtr<SNovaButton> GetDefaultFocusButton() const override;

	/*----------------------------------------------------
	    Resource list
	----------------------------------------------------*/

	TSharedRef<SWidget> GenerateResourceItem(const class UNovaResource* Resource);

	const FSlateBrush* GetResourceIcon(const class UNovaResource* Resource) const;

	FText GenerateResourceTooltip(const class UNovaResource* Resource);

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

	TOptional<float> GetPropellantRatio() const;

	FText GetPropellantText() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

	void OnRefuelPropellant();

	void OnTradeWithSlot(int32 Index, ENovaResourceType Type);

	void OnBuyResource();

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Game objects
	TWeakObjectPtr<UNovaMenuManager> MenuManager;
	class ANovaPlayerController*     PC;
	class ANovaGameState*            GameState;
	const struct FNovaSpacecraft*    Spacecraft;
	const class ANovaSpacecraftPawn* SpacecraftPawn;
	int32                            CurrentCompartmentIndex;
	TNovaTimedAverage<float>         AveragedPropellantRatio;

	// Resource list
	TArray<const class UNovaResource*>                    ResourceList;
	TSharedPtr<SNovaListView<const class UNovaResource*>> ResourceListView;

	// Slate widgets
	TSharedPtr<class SNovaModalPanel>   GenericModalPanel;
	TSharedPtr<class SNovaTradingPanel> TradingModalPanel;
	TSharedPtr<class SNovaButton>       RefuelButton;
};
