// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaModalListView.h"
#include "Nova/UI/Component/NovaOrbitalMap.h"

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

	virtual void HorizontalAnalogInput(float Value) override;

	virtual void VerticalAnalogInput(float Value) override;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Re-create the UI in the side panel */
	void UpdateSidePanel();

	/** Set the current destination */
	bool SelectDestination(const class UNovaArea* Destination);

	/** Remove the current destination */
	void ResetDestination();

	/** Check for destination validity */
	bool CanSelectDestination(const UNovaArea* Destination) const;

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
	// Trajectories
	void OnTrajectoryChanged(TSharedPtr<struct FNovaTrajectory> Trajectory);
	void OnCommitTrajectory();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Game objects
	TWeakObjectPtr<UNovaMenuManager>       MenuManager;
	class ANovaPlayerController*           PC;
	const struct FNovaSpacecraft*          Spacecraft;
	class ANovaGameState*                  GameState;
	class UNovaOrbitalSimulationComponent* OrbitalSimulation;

	// Slate widgets
	TSharedPtr<SNovaOrbitalMap>               OrbitalMap;
	TSharedPtr<class SNovaSidePanel>          SidePanel;
	TSharedPtr<class SNovaSidePanelContainer> SidePanelContainer;
	TSharedPtr<class SNovaHoverStack>         HoverText;

	// Side panel widgets
	TSharedPtr<class STextBlock>                AreaTitle;
	TSharedPtr<class SNovaTrajectoryCalculator> TrajectoryCalculator;
	TSharedPtr<class STextBlock>                FuelText;
	TSharedPtr<class SNovaButton>               CommitButton;
	TSharedPtr<class SVerticalBox>              StationTrades;

	// Local state
	bool                        HasHoveredObjects;
	const class UNovaArea*      SelectedDestination;
	TArray<FNovaOrbitalObject>  SelectedObjectList;
	TSharedPtr<FNovaTrajectory> CurrentSimulatedTrajectory;
};
