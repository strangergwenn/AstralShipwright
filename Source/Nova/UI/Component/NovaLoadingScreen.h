// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Nova/UI/NovaUI.h"

#include "Widgets/SCompoundWidget.h"
#include "Slate/DeferredCleanupSlateBrush.h"

class SNovaLoadingScreen : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaLoadingScreen) : _Settings(nullptr), _Material(nullptr)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)
	SLATE_ARGUMENT(const class UNovaLoadingScreenSetup*, Settings)
	SLATE_ARGUMENT(class UMaterialInstanceDynamic*, Material)

	SLATE_END_ARGS()

public:
	/*----------------------------------------------------
	    Constructor
	----------------------------------------------------*/

	SNovaLoadingScreen() : SCompoundWidget(), AnimatedMaterialInstance(nullptr), LoadingScreenTime(0), CurrentAlpha(0)
	{}

	virtual void Construct(const FArguments& InArgs);

public:
	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	/** Set the current rendering alpha */
	void SetFadeAlpha(float Alpha);

	/** Get the current rendering alpha */
	float GetFadeAlpha() const
	{
		return CurrentAlpha;
	}

	/** Get the current fade alpha */
	FSlateColor GetColor() const;

	/** Get the time elapsed on the loading screen */
	float GetCurrentTime() const;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

protected:
	/*----------------------------------------------------
	    Private data
	----------------------------------------------------*/

	// Data
	TSharedPtr<FDeferredCleanupSlateBrush> LoadingScreenAnimatedBrush;
	class UMaterialInstanceDynamic*        AnimatedMaterialInstance;
	float                                  LoadingScreenTime;
	float                                  CurrentAlpha;
};
