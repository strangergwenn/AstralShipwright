// Astral Shipwright - Gwennaël Arbona

#include "NovaOverlay.h"

#include "UI/Widgets/NovaEventDisplay.h"
#include "UI/Widgets/NovaNotification.h"

#include "Nova.h"

#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/UI/Widgets/NeutronFadingWidget.h"

#define LOCTEXT_NAMESPACE "SNovaOverlay"

/*----------------------------------------------------
    Overlay implementation
----------------------------------------------------*/

void SNovaOverlay::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	MenuManager                    = InArgs._MenuManager;

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
			.BorderImage(&Theme.Black)
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

void SNovaOverlay::Notify(const FText& Text, const FText& Subtext, ENeutronNotificationType Type)
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
