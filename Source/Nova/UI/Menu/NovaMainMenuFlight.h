// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaListView.h"

#include "Online.h"

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
	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void Show() override;

	virtual void Hide() override;

	virtual void AbilityPrimary() override;

	virtual void AbilitySecondary() override;

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

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	bool IsUndockEnabled() const;
	bool IsDockEnabled() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

	void OnUndock();
	void OnDock();

	void OnTrajectoryChanged(TSharedPtr<struct FNovaTrajectory> Trajectory);

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	TWeakObjectPtr<UNovaMenuManager> MenuManager;

	// Slate widgets
	TSharedPtr<class SRetainerWidget>           MapRetainer;
	TSharedPtr<class SNovaOrbitalMap>           OrbitalMap;
	TSharedPtr<class SNovaButton>               UndockButton;
	TSharedPtr<class SNovaButton>               DockButton;
	TSharedPtr<class SNovaTrajectoryCalculator> TrajectoryCalculator;
};
