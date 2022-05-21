// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "UI/NovaUI.h"
#include "UI/Widget/NovaTabView.h"

#include "Online.h"

// Definitions
constexpr int32 HUDCount        = 5;
constexpr int32 DefaultHUDIndex = 2;

/** HUD data structure */
struct FNovaHUDData
{
	FText                         HelpText;
	TSharedPtr<SWidget>           OverviewWidget;
	TSharedPtr<SWidget>           DetailedWidget;
	TSharedPtr<class SNovaButton> DefaultFocus;
};

/** Flight menu */
class SNovaMainMenuFlight
	: public SNovaTabPanel
	, public INovaGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuFlight)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	SNovaMainMenuFlight();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void Show() override;

	virtual void Hide() override;

	virtual void UpdateGameObjects() override;

	virtual void Next() override;

	virtual void Previous() override;

	virtual void ZoomIn() override;

	virtual void ZoomOut() override;

	virtual void HorizontalAnalogInput(float Value) override;

	virtual void VerticalAnalogInput(float Value) override;

	virtual void OnClicked(const FVector2D& Position) override;

	virtual TSharedPtr<SNovaButton> GetDefaultFocusButton() const override;

	virtual bool IsClickInsideMenuAllowed() const override
	{
		return false;
	}

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:

	FText GetDockUndockText() const;
	FText GetDockUndockHelp() const;
	bool  CanDockUndock(FText* Help = nullptr) const;

	FText GetFastFowardHelp() const;
	bool  CanFastForward() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:

	void SetHUDIndex(int32 Index);
	void SetHUDIndexCallback(int32 Index);

	void OnFastForward();

	void OnDockUndock();

	/*----------------------------------------------------
	    Helpers
	----------------------------------------------------*/

protected:

	FKey GetPreviousItemKey() const;
	FKey GetNextItemKey() const;
	bool IsInSpace() const;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Game objects
	TWeakObjectPtr<UNovaMenuManager>        MenuManager;
	class ANovaPlayerController*            PC;
	class ANovaSpacecraftPawn*              SpacecraftPawn;
	class UNovaSpacecraftMovementComponent* SpacecraftMovement;
	class ANovaGameState*                   GameState;
	class UNovaOrbitalSimulationComponent*  OrbitalSimulation;

	// Slate widgets
	TSharedPtr<class SHorizontalBox> HUDSwitcher;
	TSharedPtr<class SNovaHUDPanel>  HUDPanel;
	TSharedPtr<class SBox>           HUDBox;
	TSharedPtr<class SNovaButton>    DockButton;
	TSharedPtr<class SNovaButton>    TerminateButton;
	TSharedPtr<class SNovaButton>    AlignManeuverButton;
	TSharedPtr<class SNovaButton>    FastForwardButton;
	TSharedPtr<class SBox>           InvisibleWidget;

	// HUD state
	FNovaCarouselAnimation<HUDCount> HUDAnimation;
	FNovaHUDData                     HUDData[HUDCount];
	int32                            CurrentHUDIndex;
};
