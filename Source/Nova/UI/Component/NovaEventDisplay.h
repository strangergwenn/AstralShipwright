// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaFadingWidget.h"

/** Event display type */
enum class ENovaEventDisplayType : uint8
{
	StaticText,
	DynamicText,
	StaticTextWithDetails
};

/** Event data */
struct FNovaEventDisplayData
{
	FNovaEventDisplayData() : Type(ENovaEventDisplayType::StaticText)
	{}

	bool operator==(const FNovaEventDisplayData& Other) const
	{
		return Text.EqualTo(Other.Text) && Type == Other.Type;
	}

	bool operator!=(const FNovaEventDisplayData& Other) const
	{
		return !operator==(Other);
	}

	FText                 Text;
	ENovaEventDisplayType Type;
};

/** Event notification widget */
class SNovaEventDisplay : public SNovaFadingWidget<false>
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaEventDisplay)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	SNovaEventDisplay();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual bool IsDirty() const override
	{
		return DesiredState != CurrentState;
	}

	virtual void OnUpdate() override
	{
		CurrentState = DesiredState;
	}

protected:
	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	EVisibility GetMainVisibility() const;
	EVisibility GetDetailsVisibility() const;

	FLinearColor GetDisplayColor() const;

	FText GetMainText() const;
	FText GetTimeText() const;
	FText GetDetailsText() const;

	const FSlateBrush* GetIcon() const;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	TWeakObjectPtr<UNovaMenuManager> MenuManager;

	// Current state
	FNovaEventDisplayData DesiredState;
	FNovaEventDisplayData CurrentState;
	FText                 TimeText;
	FText                 DetailsText;
	bool                  IsValidDetails;
};
