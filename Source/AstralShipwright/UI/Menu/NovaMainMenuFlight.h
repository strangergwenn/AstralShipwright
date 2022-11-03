// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronTabView.h"

#include "Online.h"

// Definitions
constexpr int32 HUDCount        = 3;
constexpr int32 DefaultHUDIndex = 1;

/** HUD data structure */
struct FNovaHUDData
{
	FText                            HelpText;
	TSharedPtr<SWidget>              OverviewWidget;
	TSharedPtr<SWidget>              DetailedWidget;
	TSharedPtr<class SNeutronButton> DefaultFocus;
};

/** Flight menu */
class SNovaMainMenuFlight
	: public SNeutronTabPanel
	, public INeutronGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuFlight)
	{}

	SLATE_ARGUMENT(class SNeutronMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNeutronMenuManager>, MenuManager)

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

	virtual TSharedPtr<SNeutronButton> GetDefaultFocusButton() const override;

	virtual bool IsClickInsideMenuAllowed() const override
	{
		return false;
	}

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:

	FText GetCrewText() const;
	FText GetPowerText() const;

	FText GetStatusText() const;
	FText GetStatusValue() const;

	FText GetDockUndockText() const;
	FText GetDockUndockHelp() const;
	bool  CanDockUndock(FText* Help = nullptr) const;

	bool CanOrbit() const;

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

	void OnRequestSalvage();

	/*----------------------------------------------------
	    Helpers
	----------------------------------------------------*/

protected:

	bool IsInSpace() const;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Game objects
	TWeakObjectPtr<UNeutronMenuManager>     MenuManager;
	class ANovaPlayerController*            PC;
	const struct FNovaSpacecraft*           Spacecraft;
	class ANovaSpacecraftPawn*              SpacecraftPawn;
	class UNovaSpacecraftMovementComponent* SpacecraftMovement;
	class ANovaGameState*                   GameState;
	class UNovaOrbitalSimulationComponent*  OrbitalSimulation;
	class UNovaSpacecraftCrewSystem*        CrewSystem;
	class UNovaSpacecraftProcessingSystem*  ProcessingSystem;
	class UNovaSpacecraftPowerSystem*       PowerSystem;

	// Slate widgets
	TSharedPtr<class SHorizontalBox> HUDSwitcher;
	TSharedPtr<class SNovaHUDPanel>  HUDPanel;
	TSharedPtr<class SBox>           HUDBox;

	// Modal panels
	TSharedPtr<class SNeutronModalPanel> GenericModalPanel;

	// Content widgets
	TSharedPtr<class SVerticalBox>   ModuleGroupsBox;
	TSharedPtr<class SNeutronButton> DockButton;
	TSharedPtr<class SNeutronButton> TerminateButton;
	TSharedPtr<class SNeutronButton> MiningRigButton;
	TSharedPtr<class SNeutronButton> AlignManeuverButton;
	TSharedPtr<class SNeutronButton> FastForwardButton;
	TSharedPtr<class SBox>           InvisibleWidget;

	// HUD state
	FNeutronCarouselAnimation<HUDCount> HUDAnimation;
	FNovaHUDData                        HUDData[HUDCount];
	int32                               CurrentHUDIndex;
};
