// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuCareer.h"

#include "NovaMainMenu.h"

#include "Game/NovaGameState.h"
#include "Player/NovaPlayerController.h"
#include "Spacecraft/System/NovaSpacecraftCrewSystem.h"

#include "Nova.h"

#include "Neutron/Player/NeutronPlayerController.h"

#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/UI/Widgets/NeutronModalPanel.h"
#include "Neutron/UI/Widgets/NeutronSlider.h"

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

SNovaMainMenuCareer::SNovaMainMenuCareer() : PC(nullptr), CrewSystem(nullptr), CurrentCrewValue(0)
{}

void SNovaMainMenuCareer::Construct(const FArguments& InArgs)
{
	// Get all assets sorted by type and ascending unlock levels
	uint8                                        MaxUnlockLevel = 0;
	TArray<const UNovaTradableAssetDescription*> FullAssetList;
	const UNeutronAssetManager*                  AssetManager = UNeutronAssetManager::Get();
	NCHECK(AssetManager);
	AppendAssetsSortedByLevel<UNovaCompartmentDescription>(AssetManager, FullAssetList, MaxUnlockLevel);
	AppendAssetsSortedByLevel<UNovaModuleDescription>(AssetManager, FullAssetList, MaxUnlockLevel);
	AppendAssetsSortedByLevel<UNovaEquipmentDescription>(AssetManager, FullAssetList, MaxUnlockLevel);

	// Data
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	MenuManager                    = InArgs._MenuManager;
	const int32 FullWidth          = (MaxUnlockLevel + 1) * (FNeutronStyleSet::GetButtonSize("InventoryButtonSize").Width +
                                                       Theme.ContentPadding.Left + Theme.ContentPadding.Right);

	// Parent constructor
	SNeutronNavigationPanel::Construct(SNeutronNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	TSharedPtr<SHorizontalBox> MainUnlockBox;
	ChildSlot
	[
		SAssignNew(MainContainer, SScrollBox)
		.Style(&Theme.ScrollBoxStyle)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.AnimateWheelScrolling(true)
		
		// Crew title
		+ SScrollBox::Slot()
		.HAlign(HAlign_Center)
		.Padding(Theme.VerticalContentPadding + FMargin(0, 20, 0, 0))
		[
			SNew(SBox)
			.WidthOverride(FullWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CrewTitle", "Crew"))
				.TextStyle(&Theme.HeadingFont)
			]
		]
		
		// Crew content
		+ SScrollBox::Slot()
		.HAlign(HAlign_Center)
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(SBox)
			.WidthOverride(FullWidth)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(Theme.ContentPadding)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CrewDetailsHint", "Crew can work across the entire spacecraft, and require a daily fee to continue working. Failing to pay any crew member will have them resign."))
					.TextStyle(&Theme.InfoFont)
					.AutoWrapText(true)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Bottom)
				[
					SNew(SHorizontalBox)
					
					+ SHorizontalBox::Slot()

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("AdjustCrew", "Adjust crew size"))
							.TextStyle(&Theme.MainFont)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNeutronAssignNew(CrewSlider, SNeutronSlider)
							.Size("DoubleButtonSize")
							.OnValueChanged(this, &SNovaMainMenuCareer::OnCrewChanged)
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Bottom)
					[
						SNeutronNew(SNeutronButton)
						.Icon(FNeutronStyleSet::GetBrush("Icon/SB_On"))
						.Action(FNeutronPlayerInput::MenuPrimary)
						.Text(LOCTEXT("Confirm", "Confirm"))
						.Enabled(this, &SNovaMainMenuCareer::CanConfirmCrew)
						.OnClicked(this, &SNovaMainMenuCareer::OnCrewConfirmed)
						.ActionFocusable(false)
					]

					+ SHorizontalBox::Slot()
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(Theme.ContentPadding)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(this, &SNovaMainMenuCareer::GetCrewChanges)
						.TextStyle(&Theme.MainFont)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(this, &SNovaMainMenuCareer::GetCrewDetails)
						.TextStyle(&Theme.MainFont)
						.AutoWrapText(true)
					]
				]
			]
		]
		
		// Unlocks title
		+ SScrollBox::Slot()
		.HAlign(HAlign_Center)
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(SBox)
			.WidthOverride(FullWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ComponentsTitle", "Spacecraft components"))
				.TextStyle(&Theme.HeadingFont)
			]
		]
		
		// Unlocks info bar
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
				.Text(this, &SNovaMainMenuCareer::GetUnlockInfo)
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
								.AutoWrapText(true)
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

	if (CrewSystem)
	{
		CrewSlider->SetMaxValue(CrewSystem->GetCrewCapacity());
		CrewSlider->SetCurrentValue(CrewSystem->GetCurrentCrew());
	}
}

void SNovaMainMenuCareer::Hide()
{
	SNeutronTabPanel::Hide();
}

void SNovaMainMenuCareer::UpdateGameObjects()
{
	PC                                = MenuManager.IsValid() ? MenuManager->GetPC<ANovaPlayerController>() : nullptr;
	const FNovaSpacecraft* Spacecraft = IsValid(PC) ? PC->GetSpacecraft() : nullptr;
	const ANovaGameState*  GameState  = IsValid(PC) ? MenuManager->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	CrewSystem = IsValid(GameState) && Spacecraft ? GameState->GetSpacecraftSystem<UNovaSpacecraftCrewSystem>(Spacecraft) : nullptr;
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

FText SNovaMainMenuCareer::GetCrewChanges() const
{
	return CrewSystem
	         ? FText::FormatNamed(LOCTEXT("NewCrewDetailsFormat", "Adjust crew size to {crew} for {credits} per day"), TEXT("crew"),
				   FText::AsNumber(CurrentCrewValue), TEXT("credits"), GetPriceText(CurrentCrewValue * CrewSystem->GetDailyCostPerCrew()))
	         : FText();
}

FText SNovaMainMenuCareer::GetCrewDetails() const
{
	return CrewSystem ? FText::FormatNamed(
							LOCTEXT("CrewDetailsFormat",
								"Currently employing {crew} crew ({required} are useful, capacity of {capacity}) for {credits} per day"),
							TEXT("crew"), FText::AsNumber(CrewSystem->GetCurrentCrew()), TEXT("required"),
							FText::AsNumber(CrewSystem->GetTotalRequiredCrew()), TEXT("capacity"),
							FText::AsNumber(CrewSystem->GetCrewCapacity()), TEXT("credits"), GetPriceText(CrewSystem->GetDailyCost()))
	                  : FText();
}

bool SNovaMainMenuCareer::CanConfirmCrew() const
{
	return CrewSystem && CurrentCrewValue != CrewSystem->GetCurrentCrew();
}

FText SNovaMainMenuCareer::GetUnlockInfo() const
{
	return PC ? FText::FormatNamed(LOCTEXT("CareerInfo",
									   "Spacecraft components can currently be unlocked up to level {level}. Unlock new components to "
	                                   "increase the available level!"),
					TEXT("level"), FText::AsNumber(PC->GetComponentUnlockLevel()))
	          : FText();
}

bool SNovaMainMenuCareer::IsComponentUnlockable(const UNovaTradableAssetDescription* Asset) const
{
	return PC && PC->IsComponentUnlockable(Asset);
}

bool SNovaMainMenuCareer::IsComponentUnlocked(const UNovaTradableAssetDescription* Asset) const
{
	return PC && PC->IsComponentUnlocked(Asset);
}

FText SNovaMainMenuCareer::GetComponentHelpText(const UNovaTradableAssetDescription* Asset) const
{
	const UNovaModuleDescription*    Module    = Cast<UNovaModuleDescription>(Asset);
	const UNovaEquipmentDescription* Equipment = Cast<UNovaEquipmentDescription>(Asset);
	FText                            Details;

	if (PC && PC->IsComponentUnlockable(Asset, &Details))
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

void SNovaMainMenuCareer::OnCrewChanged(float Value)
{
	CurrentCrewValue = FMath::RoundToInt(Value);
}

void SNovaMainMenuCareer::OnCrewConfirmed()
{
	if (PC)
	{
		int32 PreviousCrewCount = CrewSystem->GetCurrentCrew();
		PC->SetCurrentCrew(CurrentCrewValue);

		FText DetailsText =
			CurrentCrewValue > PreviousCrewCount
				? FText::FormatNamed(LOCTEXT("CrewIncreased", "Crew increased from {previous} to {current}, employees hired"),
					  TEXT("previous"), PreviousCrewCount, TEXT("current"), FText::AsNumber(CurrentCrewValue))
				: FText::FormatNamed(LOCTEXT("CrewDownsized", "Crew downsized from {previous} to {current}, employees dismissed"),
					  TEXT("previous"), PreviousCrewCount, TEXT("current"), FText::AsNumber(CurrentCrewValue));

		PC->Notify(LOCTEXT("CrewAdjusted", "Crew adjusted"), DetailsText);
	}
}

void SNovaMainMenuCareer::OnComponentUnlocked(const UNovaTradableAssetDescription* Asset)
{
	FText UnlockDetails = FText::FormatNamed(LOCTEXT("UnlockComponentDetails", "Confirm the unlocking of {component} for {credits}"),
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
