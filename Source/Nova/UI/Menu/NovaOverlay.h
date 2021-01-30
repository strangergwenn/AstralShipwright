// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/NovaUITypes.h"
#include "Nova/Game/NovaGameTypes.h"
#include "Widgets/SCompoundWidget.h"


class SNovaOverlay : public SCompoundWidget, public FGCObject
{
	/*----------------------------------------------------
		Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaOverlay)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
		Interaction
	----------------------------------------------------*/

	/** Show a text notification on the screen */
	void Notify(FText Text, ENovaNotificationType Type);

	virtual FString GetReferencerName() const override
	{
		return FString("SNovaOverlay");
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(NotificationIconMaterial);
	}
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;


	/*----------------------------------------------------
		Content callbacks
	----------------------------------------------------*/

protected:

	FText GetNotifyText() const;

	FLinearColor GetColor() const;

	FSlateColor GetBackgroundColor() const;

	FSlateColor GetTextColor() const;


	/*----------------------------------------------------
		Callbacks
	----------------------------------------------------*/

protected:


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

protected:
	
	// Menu reference
	TWeakObjectPtr<class UNovaMenuManager>        MenuManager;

	// Notification icon
	TSharedPtr<FSlateBrush>                       NotificationIconBrush;
	class UMaterialInstanceDynamic*               NotificationIconMaterial;

	// Settings
	float                                         NotifyFadeDuration;
	float                                         NotifyDisplayDuration;

	// Notification state
	FText                                         DesiredNotifyText;
	ENovaNotificationType                         DesiredNotifyType;
	FText                                         CurrentNotifyText;
	float                                         CurrentNotifyFadeTime;
	float                                         CurrentNotifyDisplayTime;
	float                                         CurrentNotifyAlpha;

};
