// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/UI/Widgets/NeutronInfoText.h"
#include "Neutron/UI/Widgets/NeutronModalPanel.h"

/** Trading panel */
class SNovaTradingPanel : public SNeutronModalPanel
{
	/*----------------------------------------------------
	    Slate args
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaTradingPanel)
	{}

	SLATE_ARGUMENT(class SNeutronMenu*, Menu)
	SLATE_ARGUMENT(FNeutronAsyncCondition, IsConfirmEnabled)

	SLATE_END_ARGS()

public:

	SNovaTradingPanel() : Spacecraft(nullptr), Area(nullptr), Resource(nullptr), CompartmentIndex(INDEX_NONE)
	{}

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

public:

	/** Show contents without trading */
	void Inspect(class ANovaPlayerController* TargetPC, const class UNovaArea* TargetArea, const class UNovaResource* TargetResource,
		int32 TargetCompartmentIndex)
	{
		ShowPanelInternal(TargetPC, TargetArea, TargetResource, TargetCompartmentIndex, false);
	}

	/** Start trading */
	void Trade(class ANovaPlayerController* TargetPC, const class UNovaArea* TargetArea, const class UNovaResource* TargetResource,
		int32 TargetCompartmentIndex)
	{
		ShowPanelInternal(TargetPC, TargetArea, TargetResource, TargetCompartmentIndex, true);
	}

protected:

	/** Implementation of the modal panel */
	void ShowPanelInternal(class ANovaPlayerController* TargetPC, const class UNovaArea* TargetArea,
		const class UNovaResource* TargetResource, int32 TargetCompartmentIndex, bool AllowTrade);

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:

	bool IsConfirmEnabled() const override;

	FText GetResourceDetails() const;

	TOptional<float> GetCargoProgress() const;

	FText GetCargoAmount() const;
	FText GetCargoMinValue() const;
	FText GetCargoMaxValue() const;
	FText GetCargoDetails() const;

	FText               GetTransactionDetails() const;
	ENeutronInfoBoxType GetTransactionType() const;
	FNovaCredits        GetTransactionValue() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:

	/** Confirm the trade and proceed */
	void OnConfirmTrade();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Slate widgets
	TSharedPtr<class SNeutronSlider>         AmountSlider;
	TSharedPtr<class SNovaTradableAssetItem> ResourceItem;
	TSharedPtr<class SHorizontalBox>         CaptionBox;
	TSharedPtr<class SNeutronInfoText>       InfoText;

	// Parameters
	TWeakObjectPtr<ANovaPlayerController> PC;
	const struct FNovaSpacecraft*         Spacecraft;
	const class UNovaArea*                Area;
	const class UNovaResource*            Resource;

	// Data
	float InitialAmount;
	float MinAmount;
	float MaxAmount;
	float Capacity;
	int32 CompartmentIndex;
	bool  IsTradeAllowed;
};
