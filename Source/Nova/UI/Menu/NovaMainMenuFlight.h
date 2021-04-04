// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaModalListView.h"

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
	SNovaMainMenuFlight() : SelectedDestination(nullptr)
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

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	TSharedRef<SWidget> GenerateDestinationItem(const class UNovaArea* Destination);
	FText               GetDestinationName(const class UNovaArea* Destination) const;
	const FSlateBrush*  GetDestinationIcon(const class UNovaArea* Destination) const;
	FText               GenerateDestinationTooltip(const class UNovaArea* Destination);
	void                OnSelectedDestinationChanged(const class UNovaArea* Destination, int32 Index);

	bool IsUndockEnabled() const;
	bool IsDockEnabled() const;

	void OnUndock();
	void OnDock();

	void OnTrajectoryChanged(TSharedPtr<struct FNovaTrajectory> Trajectory);
	void OnCommitTrajectory();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	TWeakObjectPtr<UNovaMenuManager> MenuManager;

	// Destination list
	const class UNovaArea*                           SelectedDestination;
	TArray<const class UNovaArea*>                   DestinationList;
	TSharedPtr<SNovaModalListView<const UNovaArea*>> DestinationListView;

	// Slate widgets
	TSharedPtr<class SNovaOrbitalMap>           OrbitalMap;
	TSharedPtr<class SNovaButton>               UndockButton;
	TSharedPtr<class SNovaButton>               DockButton;
	TSharedPtr<class SNovaTrajectoryCalculator> TrajectoryCalculator;
};
