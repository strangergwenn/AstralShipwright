// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "UI/NovaUI.h"
#include "UI/Widget/NovaTabView.h"
#include "UI/Widget/NovaModalListView.h"
#include "UI/Component/NovaOrbitalMap.h"

#include "Online.h"

/** Navigation menu */
class SNovaMainMenuNavigation
	: public SNovaTabPanel
	, public INovaGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuNavigation)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

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

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	virtual void HorizontalAnalogInput(float Value) override;

	virtual void VerticalAnalogInput(float Value) override;

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
	void OnShowSidePanel(const TArray<FNovaOrbitalObject>& HoveredObjects);
	void OnHideSidePanel();

	// Trajectories
	void OnTrajectoryChanged(const struct FNovaTrajectory& Trajectory, bool HasEnoughPropellant);
	void OnCommitTrajectory();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Game objects
	TWeakObjectPtr<UNovaMenuManager>        MenuManager;
	class ANovaPlayerController*            PC;
	const struct FNovaSpacecraft*           Spacecraft;
	class ANovaGameState*                   GameState;
	class UNovaOrbitalSimulationComponent*  OrbitalSimulation;
	class UNovaAsteroidSimulationComponent* AsteroidSimulation;

	// Slate widgets
	TSharedPtr<SNovaOrbitalMap>               OrbitalMap;
	TSharedPtr<class SNovaSidePanel>          SidePanel;
	TSharedPtr<class SNovaSidePanelContainer> SidePanelContainer;
	TSharedPtr<class SNovaHoverStack>         HoverText;

	// Side panel widgets
	TSharedPtr<class STextBlock>                DestinationTitle;
	TSharedPtr<class STextBlock>                DestinationDescription;
	TSharedPtr<class SNovaTrajectoryCalculator> TrajectoryCalculator;
	TSharedPtr<class SNovaButton>               CommitButton;
	TSharedPtr<class SVerticalBox>              StationTrades;

	// Local state
	bool                       HasHoveredObjects;
	FNovaOrbit                 DestinationOrbit;
	TArray<FNovaOrbitalObject> SelectedObjectList;
	bool                       CurrentTrajectoryHasEnoughPropellant;
};
