// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"

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
	    Callbacks
	----------------------------------------------------*/

	void OnTradeWithSlot(int32 Index, ENovaResourceType Type);

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Game objects
	TWeakObjectPtr<UNovaMenuManager> MenuManager;
	class ANovaPlayerController*     PC;
	class ANovaSpacecraftPawn*       SpacecraftPawn;

	// Slate widgets
	TSharedPtr<class SNovaTradingPanel> TradingModalPanel;
};
