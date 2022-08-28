// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuCareer.h"

#include "NovaMainMenu.h"

#include "Player/NovaPlayerController.h"

#include "Nova.h"

#include "Neutron/Player/NeutronPlayerController.h"

#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronMenuManager.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuHome"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

template <typename AssetType, typename BaseType>
void AppendAssetsSortedByLevel(const UNeutronAssetManager* AssetManager, TArray<const BaseType*>& CurrentList, uint8& MaxUnlockLevel)
{
	TArray<const AssetType*> AssetList = AssetManager->GetAssets<AssetType>();
	AssetList.Sort(
		[](const AssetType& A, const AssetType& B)
		{
			return A.UnlockLevel < B.UnlockLevel;
		});
	MaxUnlockLevel = FMath::Max(MaxUnlockLevel, AssetList.Last()->UnlockLevel);
	CurrentList.Append(AssetList);
}

void SNovaMainMenuCareer::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	MenuManager                    = InArgs._MenuManager;

	// Parent constructor
	SNeutronNavigationPanel::Construct(SNeutronNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	TSharedPtr<SHorizontalBox> MainUnlockBox;
	ChildSlot
	[
		SNew(SScrollBox)
		.Style(&Theme.ScrollBoxStyle)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.AnimateWheelScrolling(true)
		
		// Bulk trading
		+ SScrollBox::Slot()
		.HAlign(HAlign_Center)
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(MainUnlockBox, SHorizontalBox)
			]

			+ SHorizontalBox::Slot()
		]
	];
	// clang-format on

	// Get all assets sorted by type and ascending unlock levels
	uint8                                        MaxUnlockLevel = 0;
	TArray<const UNovaTradableAssetDescription*> FullAssetList;
	const UNeutronAssetManager*                  AssetManager = UNeutronAssetManager::Get();
	NCHECK(AssetManager);
	AppendAssetsSortedByLevel<UNovaCompartmentDescription>(AssetManager, FullAssetList, MaxUnlockLevel);
	AppendAssetsSortedByLevel<UNovaModuleDescription>(AssetManager, FullAssetList, MaxUnlockLevel);
	AppendAssetsSortedByLevel<UNovaEquipmentDescription>(AssetManager, FullAssetList, MaxUnlockLevel);

	// Add columns filled with assets
	for (int32 Level = 0; Level <= MaxUnlockLevel; Level++)
	{
		// clang-format off
		
		// Add the main layout
		TSharedPtr<SVerticalBox> UnlockBox;
		MainUnlockBox->AddSlot()
		.AutoWidth()
		[
			SNew(SBorder)
			.BorderImage(Level % 2 == 0 ? &Theme.MainMenuGenericBackground : new FSlateNoResource)
			.Padding(Theme.ContentPadding)
			[
				SAssignNew(UnlockBox, SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(Level > 0 ? FText::AsNumber(Level) : FText())
					.TextStyle(&Theme.NotificationFont)
				]
			]
		];

		// Add all assets matching that level
		for (const UNovaTradableAssetDescription* Asset : FullAssetList)
		{
			if (Asset->UnlockLevel != Level)
			{
				continue;
			}

			TSharedPtr<SNeutronButton> UnlockButton;
			UnlockBox->AddSlot()
			.AutoHeight()
			[
				SNeutronAssignNew(UnlockButton, SNeutronButton)
				.Size("InventoryButtonSize")
				.HelpText(this, &SNovaMainMenuCareer::GetComponentHelpText, Asset)
				.Enabled(this, &SNovaMainMenuCareer::IsComponentUnlockAllowed, Asset)
				.OnClicked(this, &SNovaMainMenuCareer::OnComponentUnlocked, Asset)
				.Content()
				[
					SNew(SOverlay)
					
					+ SOverlay::Slot()
					[
						SNew(SScaleBox)
						.Stretch(EStretch::ScaleToFill)
						[
							SNew(SImage)
							.Image(&Asset->AssetRender)
						]
					]

					+ SOverlay::Slot()
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(&Theme.MainMenuDarkBackground)
							.Padding(Theme.ContentPadding)
							[
								SNew(STextBlock)
								.TextStyle(&Theme.MainFont)
								.Text(Asset->Name)
							]
						]

						+ SVerticalBox::Slot()

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Left)
						[
							SNew(SImage)
							.Image(this, &SNovaMainMenuCareer::GetComponentIcon, Asset)
						]
					]
				]
			];

			UnlockButtons.Add(UnlockButton);

			// clang-format on
		}
	}
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuCareer::Show()
{
	SNeutronTabPanel::Show();
}

void SNovaMainMenuCareer::Hide()
{
	SNeutronTabPanel::Hide();
}

TSharedPtr<SNeutronButton> SNovaMainMenuCareer::GetDefaultFocusButton() const
{
	for (const TSharedPtr<SNeutronButton>& Button : UnlockButtons)
	{
		if (Button->IsButtonEnabled())
		{
			return Button;
		}
	}

	return SNeutronTabPanel::GetDefaultFocusButton();
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

bool SNovaMainMenuCareer::IsComponentUnlockable(const UNovaTradableAssetDescription* Asset) const
{
	return Cast<ANovaPlayerController>(MenuManager->GetPC())->IsComponentUnlockable(Asset);
}

bool SNovaMainMenuCareer::IsComponentUnlocked(const UNovaTradableAssetDescription* Asset) const
{
	return Cast<ANovaPlayerController>(MenuManager->GetPC())->IsComponentUnlocked(Asset);
}

FText SNovaMainMenuCareer::GetComponentHelpText(const UNovaTradableAssetDescription* Asset) const
{
	const UNovaModuleDescription*    Module    = Cast<UNovaModuleDescription>(Asset);
	const UNovaEquipmentDescription* Equipment = Cast<UNovaEquipmentDescription>(Asset);
	FText                            Details;

	if (Module)
	{
		Details = Module->Description;
	}
	else if (Equipment)
	{
		Details = Equipment->Description;
	}
	else
	{
		Details = Asset->GetInlineDescription();
	}

	return FText::FormatNamed(LOCTEXT("UnlockFormat", "Unlock this part. {details}"), TEXT("details"), Details);
}

const FSlateBrush* SNovaMainMenuCareer::GetComponentIcon(const UNovaTradableAssetDescription* Asset) const
{
	return IsComponentUnlocked(Asset) ? FNeutronStyleSet::GetBrush("Icon/SB_On") : new FSlateNoResource;
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuCareer::OnComponentUnlocked(const UNovaTradableAssetDescription* Asset)
{
	Cast<ANovaPlayerController>(MenuManager->GetPC())->UnlockComponent(Asset);
}

#undef LOCTEXT_NAMESPACE
