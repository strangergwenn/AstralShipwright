// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"

#include "Game/NovaOrbitalSimulationTypes.h"

// Callback type
DECLARE_DELEGATE_TwoParams(FOnTrajectoryChanged, const FNovaTrajectory&, bool);

/** Orbital trajectory trade-off calculator */
class SNovaTrajectoryCalculator : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate args
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaTrajectoryCalculator)
		: _MenuManager(nullptr)
		, _Panel(nullptr)
		, _FadeTime(ENeutronUIConstants::FadeDurationShort)
		, _MinAltitude(200)
		, _MaxAltitude(2000)
		, _DeltaVActionName(NAME_None)
		, _DurationActionName(NAME_None)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNeutronMenuManager>, MenuManager)
	SLATE_ARGUMENT(class SNeutronNavigationPanel*, Panel)
	SLATE_ARGUMENT(float, FadeTime)
	SLATE_ATTRIBUTE(float, CurrentAlpha)

	SLATE_ARGUMENT(float, MinAltitude)
	SLATE_ARGUMENT(float, MaxAltitude)
	SLATE_ARGUMENT(FName, DeltaVActionName)
	SLATE_ARGUMENT(FName, DurationActionName)

	SLATE_EVENT(FOnTrajectoryChanged, OnTrajectoryChanged)

	SLATE_END_ARGS()

public:

	SNovaTrajectoryCalculator();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	/** Reset the widget */
	void Reset();

	/** Simulate trajectories to go between orbits */
	void SimulateTrajectories(
		const struct FNovaOrbit& Source, const struct FNovaOrbit& Destination, const TArray<FGuid>& SpacecraftIdentifiers);

	/** Optimize for Delta-V */
	void OptimizeForDeltaV();

	/** Optimize for travel time */
	void OptimizeForDuration();

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:

	TArray<FLinearColor> GetDeltaVGradient() const
	{
		return TrajectoryDeltaVGradientData;
	}

	TArray<FLinearColor> GetDurationGradient() const
	{
		return TrajectoryDurationGradientData;
	}

	FSlateColor GetBorderColor() const;

	bool  CanEditTrajectory() const;
	FText GetDeltaVText() const;
	FText GetDurationText() const;

	void OnAltitudeSliderChanged(float Altitude);

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Settings
	TWeakObjectPtr<UNeutronMenuManager> MenuManager;
	float                               TrajectoryFadeTime;
	TAttribute<float>                   CurrentAlpha;
	FOnTrajectoryChanged                OnTrajectoryChanged;
	int32                               AltitudeStep;

	// Trajectory data
	TArray<FGuid>                PlayerIdentifiers;
	TMap<float, FNovaTrajectory> SimulatedTrajectories;
	float                        MinDeltaV;
	float                        MinDeltaVWithTolerance;
	float                        MaxDeltaV;
	float                        MinDuration;
	float                        MaxDuration;
	float                        MinDeltaVAltitude;
	float                        MinDurationAltitude;

	// Display data
	TArray<FLinearColor> TrajectoryDeltaVGradientData;
	TArray<FLinearColor> TrajectoryDurationGradientData;
	float                CurrentTrajectoryDisplayTime;
	bool                 NeedTrajectoryDisplayUpdate;
	float                CurrentAltitude;

	// Slate widgets
	TSharedPtr<class SNeutronSlider> Slider;
	TSharedPtr<class STextBlock>     PropellantText;
};
