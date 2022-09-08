// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuCareer.h"

#include "NovaMainMenu.h"

#include "Player/NovaPlayerController.h"

#include "Nova.h"

#include "Neutron/Player/NeutronPlayerController.h"

#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/UI/Widgets/NeutronModalPanel.h"

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
		
		// Info bar
		+ SScrollBox::Slot()
		.HAlign(HAlign_Center)
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(this, &SNovaMainMenuCareer::GetCareerInfo)
				.TextStyle(&Theme.InfoFont)
			]

			+ SHorizontalBox::Slot()
		]
		
		// Main unlock layout
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

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(Level > 0 ? GetPriceText(PC->GetComponentUnlockCost(Level)) : FText())
					.TextStyle(&Theme.InfoFont)
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
						.HAlign(HAlign_Right)
						[
							SNew(SImage)
							.Image(this, &SNovaMainMenuCareer::GetComponentIcon, Asset)
							.ColorAndOpacity(FLinearColor(1, 1, 1, 5)) // ensure final alpha will be >=1
						]
					]
				]
			];

			UnlockButtons.Add(UnlockButton);

			// clang-format on
		}
	}

	// Create ourselves a modal panel
	ModalPanel = Menu->CreateModalPanel();
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

void SNovaMainMenuCareer::UpdateGameObjects()
{
	PC = MenuManager.IsValid() ? MenuManager->GetPC<ANovaPlayerController>() : nullptr;
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

FText SNovaMainMenuCareer::GetCareerInfo() const
{
	return FText::FormatNamed(
		LOCTEXT("CareerInfo",
			"Spacecraft components can currently be unlocked up to level {level}. Unlock new components to increase the available level!"),
		TEXT("level"), FText::AsNumber(PC->GetComponentUnlockLevel()));
}

bool SNovaMainMenuCareer::IsComponentUnlockable(const UNovaTradableAssetDescription* Asset) const
{
	return PC->IsComponentUnlockable(Asset);
}

bool SNovaMainMenuCareer::IsComponentUnlocked(const UNovaTradableAssetDescription* Asset) const
{
	return PC->IsComponentUnlocked(Asset);
}

FText SNovaMainMenuCareer::GetComponentHelpText(const UNovaTradableAssetDescription* Asset) const
{
	const UNovaModuleDescription*    Module    = Cast<UNovaModuleDescription>(Asset);
	const UNovaEquipmentDescription* Equipment = Cast<UNovaEquipmentDescription>(Asset);
	FText                            Details;

	if (PC->IsComponentUnlockable(Asset, &Details))
	{
		if (Module)
		{
			Details =
				FText::FormatNamed(LOCTEXT("UnlockFormatModule", "Unlock this module. {details}"), TEXT("details"), Module->Description);
		}
		else if (Equipment)
		{
			Details = FText::FormatNamed(
				LOCTEXT("UnlockFormatEquipment", "Unlock this equipment. {details}"), TEXT("details"), Equipment->Description);
		}
		else
		{
			Details = FText::FormatNamed(
				LOCTEXT("UnlockFormatCompartment", "Unlock this compartment. {details}"), TEXT("details"), Asset->GetInlineDescription());
		}
	}

	return Details;
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
	FText UnlockDetails =
		FText::FormatNamed(LOCTEXT("UnlockComponentDetails", "Confirm the unlocking of {component} for {credits} credits"),
			TEXT("component"), Asset->Name, TEXT("credits"), GetPriceText(PC->GetComponentUnlockCost(Asset)));

	ModalPanel->Show(LOCTEXT("UnlockComponentTitle", "Unlock component"),
		UnlockDetails,    //
		FSimpleDelegate::CreateLambda(
			[=]()
			{
				PC->UnlockComponent(Asset);
			}));
}

#undef LOCTEXT_NAMESPACE
