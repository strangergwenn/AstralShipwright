// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/Widget/NovaModalPanel.h"

/** Trading panel */
class SNovaTradingPanel : public SNovaModalPanel
{
	/*----------------------------------------------------
	    Slate args
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaTradingPanel)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)

	SLATE_END_ARGS()

public:
	SNovaTradingPanel() : Spacecraft(nullptr), Resource(nullptr), CompartmentIndex(INDEX_NONE)
	{}

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

public:
	/** Start trading */
	void StartTrade(class ANovaPlayerController* TargetPC, const class UNovaResource* TargetResource, int32 TargetCompartmentIndex);

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	bool IsConfirmEnabled() const override;

	FText GetResourceDetails() const;

	TOptional<float> GetCargoProgress() const;

	FText GetCargoAmount() const;
	FText GetCargoCapacity() const;
	FText GetCargoDetails() const;

	FText            GetTransactionDetails() const;
	ENovaInfoBoxType GetTransactionType() const;
	double           GetTransactionValue() const;

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
	TSharedPtr<class SNovaSlider>       AmountSlider;
	TSharedPtr<class SNovaResourceItem> ResourceItem;

	// Data
	class ANovaPlayerController*  PC;
	const struct FNovaSpacecraft* Spacecraft;
	const UNovaResource*          Resource;
	float                         InitialAmount;
	float                         Capacity;
	int32                         CompartmentIndex;
};
