// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "UI/Widgets/NovaOrbitalMap.h"

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronTabView.h"
#include "Neutron/UI/Widgets/NeutronModalListView.h"

#include "Online.h"

/** Navigation menu */
class SNovaMainMenuNavigation
	: public SNeutronTabPanel
	, public INeutronGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuNavigation)
	{}

	SLATE_ARGUMENT(class SNeutronMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNeutronMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	SNovaMainMenuNavigation();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void Show() override;

	virtual void Hide() override;

	virtual void UpdateGameObjects() override;

	virtual void OnClicked(const FVector2D& Position) override;

	virtual void OnKeyPressed(const FKey& Key) override;

	virtual void HorizontalAnalogInput(float Value) override;

	virtual void VerticalAnalogInput(float Value) override;

	virtual void Previous() override;

	virtual void Next() override;

	virtual void ZoomIn() override;

	virtual void ZoomOut() override;

	virtual TSharedPtr<SNeutronButton> GetDefaultFocusButton() const override;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:

	/** Re-create the UI in the side panel */
	void UpdateSidePanel();

	/** Prepare a trajectory toward a new orbit */
	bool ComputeTrajectoryTo(const struct FNovaOrbit& Orbit);

	/** Remove the current destination */
	void ResetTrajectory();

	/** Check for valid spacecraft */
	bool HasValidSpacecraft() const;

	/** Check for trajectory validity */
	bool CanCommitTrajectoryInternal(FText* Details = nullptr) const;

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:

	bool  CanCommitTrajectory() const;
	FText GetCommitTrajectoryHelpText() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/
protected:

	// Side panel
	void OnShowSidePanel(const FNovaOrbitalObject& HoveredObjects);
	void OnHideSidePanel();

	// Trajectories
	void OnTrajectoryChanged(const struct FNovaTrajectory& Trajectory, bool HasEnoughPropellant);
	void OnCommitTrajectory();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Game objects
	TWeakObjectPtr<UNeutronMenuManager>     MenuManager;
	class ANovaPlayerController*            PC;
	const struct FNovaSpacecraft*           Spacecraft;
	class ANovaGameState*                   GameState;
	class UNovaOrbitalSimulationComponent*  OrbitalSimulation;
	class UNovaAsteroidSimulationComponent* AsteroidSimulation;

	// Slate widgets
	TSharedPtr<SNovaOrbitalMap>               OrbitalMap;
	TSharedPtr<class SNovaSidePanel>          SidePanel;
	TSharedPtr<class SNovaSidePanelContainer> SidePanelContainer;
	TSharedPtr<class SNovaHoverStack>         HoverStack;

	// Side panel widgets
	TSharedPtr<class STextBlock>                DestinationTitle;
	TSharedPtr<class SRichTextBlock>            DestinationDescription;
	TSharedPtr<class SNovaTrajectoryCalculator> TrajectoryCalculator;
	TSharedPtr<class SNeutronButton>            CommitButton;
	TSharedPtr<class SVerticalBox>              StationTrades;

	// Local state
	bool                       HasHoveredObjects;
	TArray<FNovaOrbitalObject> CurrentHoveredObjects;
	int32                      CurrentHoveredObjectIndex;
	FNovaOrbit                 DestinationOrbit;
	FNovaOrbitalObject         SelectedObject;
	bool                       CurrentTrajectoryHasEnoughPropellant;
};
