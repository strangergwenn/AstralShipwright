// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"

/** High-level orbit characteristics */
struct FNovaTrajectoryCharacteristics
{
	float  IntermediateAltitude;
	double Duration;
	double DeltaV;
};

/** Orbital trajectory trade-off calculator */
class SNovaTrajectoryCalculator : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate args
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaTrajectoryCalculator)
		: _MenuManager(nullptr)
		, _Panel(nullptr)
		, _MinAltitude(200)
		, _MaxAltitude(1000)
		, _DeltaVActionName(NAME_None)
		, _DurationActionName(NAME_None)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)
	SLATE_ARGUMENT(class SNovaNavigationPanel*, Panel)
	SLATE_ATTRIBUTE(float, CurrentAlpha)

	SLATE_ARGUMENT(float, MinAltitude)
	SLATE_ARGUMENT(float, MaxAltitude)
	SLATE_ARGUMENT(FName, DeltaVActionName)
	SLATE_ARGUMENT(FName, DurationActionName)

	SLATE_EVENT(FOnFloatValueChanged, OnAltitudeChanged)

	SLATE_END_ARGS()

public:
	SNovaTrajectoryCalculator() : CurrentTrajectoryDisplayTime(0), NeedTrajectoryDisplayUpdate(false)
	{}

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

	/** Simulate trajectories to go between areas */
	void SimulateTrajectories(const class UNovaArea* Source, const class UNovaArea* Destination);

	/** Optimize for Delta-V */
	void OptimizeForDeltaV();

	/** Optimize for travel time */
	void OptimizeForDuration();

	/*----------------------------------------------------
	    Content callbacks
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

	FText GetDeltaVText() const;
	FText GetDurationText() const;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	TWeakObjectPtr<UNovaMenuManager> MenuManager;
	TAttribute<float>                CurrentAlpha;
	FOnFloatValueChanged             OnAltitudeChanged;

	// Trajectory data
	TArray<FNovaTrajectoryCharacteristics> SimulatedTrajectoriesCharacteristics;
	float                                  MinDeltaV;
	float                                  MaxDeltaV;
	float                                  MinDuration;
	float                                  MaxDuration;
	FNovaTrajectoryCharacteristics         MinDeltaVTrajectoryCharacteristics;
	FNovaTrajectoryCharacteristics         MinDurationTrajectoryCharacteristics;

	// Display data
	TArray<FLinearColor> TrajectoryDeltaVGradientData;
	TArray<FLinearColor> TrajectoryDurationGradientData;
	float                CurrentTrajectoryDisplayTime;
	bool                 NeedTrajectoryDisplayUpdate;

	// Slate widgets
	TSharedPtr<class SNovaSlider> Slider;
};
