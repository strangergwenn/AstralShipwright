// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaListView.h"

#include "Online.h"

/** High-level orbit characteristics */
struct FNovaTrajectoryCharacteristics
{
	float  IntermediateAltitude;
	double Duration;
	double DeltaV;
};

/** Flight menu */
class SNovaMainMenuFlight : public SNovaTabPanel
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
	SNovaMainMenuFlight() : CurrentTrajectoryDisplayTime(0), NeedTrajectoryDisplayUpdate(false)
	{}

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void Show() override;

	virtual void Hide() override;

	virtual void HorizontalAnalogInput(float Value) override;

	virtual void VerticalAnalogInput(float Value) override;

	virtual TSharedPtr<SNovaButton> GetDefaultFocusButton() const override;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Get the spacecraft pawn */
	class ANovaSpacecraftPawn* GetSpacecraftPawn() const;

	/** Get the spacecraft movement component */
	class UNovaSpacecraftMovementComponent* GetSpacecraftMovement() const;

	/** Simulate trajectories to go between areas */
	void SimulateTrajectories(const class UNovaArea* Source, const class UNovaArea* Destination);

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	bool IsUndockEnabled() const;
	bool IsDockEnabled() const;

	TArray<FLinearColor> GetDeltaVGradient() const
	{
		return TrajectoryDeltaVGradientData;
	}

	TArray<FLinearColor> GetDurationGradient() const
	{
		return TrajectoryDurationGradientData;
	}

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

	void OnUndock();
	void OnDock();

	void OnAltitudeSliderChanged(float Altitude);

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	TWeakObjectPtr<UNovaMenuManager> MenuManager;
	float                            TrajectoryDisplayFadeTime;

	// Trajectory data
	TArray<FNovaTrajectoryCharacteristics> SimulatedTrajectories;
	TArray<FLinearColor>                   TrajectoryDeltaVGradientData;
	TArray<FLinearColor>                   TrajectoryDurationGradientData;
	float                                  CurrentTrajectoryDisplayTime;
	bool                                   NeedTrajectoryDisplayUpdate;

	// Slate widgets
	TSharedPtr<class SRetainerWidget> MapRetainer;
	TSharedPtr<class SNovaOrbitalMap> OrbitalMap;
	TSharedPtr<class SNovaButton>     UndockButton;
	TSharedPtr<class SNovaButton>     DockButton;
};
