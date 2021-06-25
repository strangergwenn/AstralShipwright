// Nova project - Gwennaël Arbona

#include "NovaOverlay.h"

#include "Nova/UI/Component/NovaEventDisplay.h"
#include "Nova/UI/Component/NovaNotification.h"
#include "Nova/UI/Widget/NovaFadingWidget.h"

#include "Nova/System/NovaMenuManager.h"
#include "Nova/Nova.h"

#define LOCTEXT_NAMESPACE "SNovaOverlay"

/*----------------------------------------------------
    Overlay implementation
----------------------------------------------------*/

void SNovaOverlay::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// clang-format off
	ChildSlot
	.VAlign(VAlign_Fill)
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SNew(SNovaEventDisplay)
			.MenuManager(InArgs._MenuManager)
		]

		+ SOverlay::Slot()
		[
			SAssignNew(FastForward, SBorder)
			.BorderImage(FNovaStyleSet::GetBrush("Common/SB_Black"))
		]

		+ SOverlay::Slot()
		[
			SAssignNew(Notification, SNovaNotification)
			.MenuManager(InArgs._MenuManager)
		]
	];
	// clang-format on

	// Initialization
	SetVisibility(EVisibility::HitTestInvisible);
	FastForward->SetVisibility(EVisibility::Collapsed);
}

void SNovaOverlay::Notify(const FText& Text, const FText& Subtext, ENovaNotificationType Type)
{
	Notification->Notify(Text, Subtext, Type);
}

void SNovaOverlay::StartFastForward()
{
	FastForward->SetVisibility(EVisibility::Visible);
}

void SNovaOverlay::StopFastForward()
{
	FastForward->SetVisibility(EVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE
