// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuOperations.h"

#include "NovaMainMenu.h"

#include "Game/NovaGameState.h"
#include "Player/NovaPlayerController.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/System/NovaSpacecraftPropellantSystem.h"
#include "Spacecraft/System/NovaSpacecraftProcessingSystem.h"

#include "UI/Widgets/NovaTradingPanel.h"
#include "UI/Widgets/NovaModuleGroupsPanel.h"
#include "UI/Widgets/NovaTradableAssetItem.h"

#include "Nova.h"

#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronGameInstance.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/UI/Widgets/NeutronFadingWidget.h"
#include "Neutron/UI/Widgets/NeutronKeyLabel.h"
#include "Neutron/UI/Widgets/NeutronModalPanel.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuOperations"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuOperations::SNovaMainMenuOperations()
	: PC(nullptr)
	, GameState(nullptr)
	, Spacecraft(nullptr)
	, SpacecraftPawn(nullptr)
	, CurrentCompartmentIndex(INDEX_NONE)
	, CurrentModuleIndex(INDEX_NONE)
{}

void SNovaMainMenuOperations::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme& Theme     = FNeutronStyleSet::GetMainTheme();
	const int32              FullWidth = ENovaConstants::MaxCompartmentCount * FNeutronStyleSet::GetButtonSize("InventoryButtonSize").Width;
	MenuManager                        = InArgs._MenuManager;

	// Parent constructor
	SNeutronNavigationPanel::Construct(SNeutronNavigationPanel::FArguments().Menu(InArgs._Menu));

	// Local data
	TSharedPtr<SVerticalBox> CompartmentBox;

	// clang-format off
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
			SNew(SBox)
			.MinDesiredWidth(FullWidth)
			.HAlign(HAlign_Fill)
			[
				SNew(SVerticalBox)
				
				// Bulk trading title
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.HeadingFont)
					.Text(LOCTEXT("BatchTrade", "Batch trading"))
				]

				// Bulk trading box
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					// Batch buying
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNeutronNew(SNeutronButton)
						.Text(LOCTEXT("BulkBuy", "Bulk buy"))
						.HelpText(LOCTEXT("BulkBuyHelp", "Bulk buy resources across all compartments"))
						.Enabled(this, &SNovaMainMenuOperations::IsBulkTradeEnabled)
						.OnClicked(this, &SNovaMainMenuOperations::OnBatchBuy)
						.Size("HighButtonSize")
					]

					// Batch selling
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNeutronNew(SNeutronButton)
						.Text(LOCTEXT("BulkSell", "Bulk sell"))
						.HelpText(LOCTEXT("BulkSellHelp", "Bulk sell resources across all compartments"))
						.Enabled(this, &SNovaMainMenuOperations::IsBulkTradeEnabled)
						.OnClicked(this, &SNovaMainMenuOperations::OnBatchSell)
						.Size("HighButtonSize")
					]

					// Propellant data
					+ SHorizontalBox::Slot()
					[
						SNew(SNeutronButtonLayout)
						.Size("HighButtonSize")
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(Theme.VerticalContentPadding)
							[
								SNew(STextBlock)
								.TextStyle(&Theme.HeadingFont)
								.Text(LOCTEXT("Propellant", "Propellant"))
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(Theme.VerticalContentPadding)
							.HAlign(HAlign_Left)
							[
								SNew(SNeutronRichText)
								.TextStyle(&Theme.MainFont)
								.Text(FNeutronTextGetter::CreateSP(this, &SNovaMainMenuOperations::GetPropellantText))
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SProgressBar)
								.Style(&Theme.ProgressBarStyle)
								.Percent(this, &SNovaMainMenuOperations::GetPropellantRatio)
							]

							+ SVerticalBox::Slot()
						]
					]
			
					// Propellant button
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNeutronNew(SNeutronButton)
						.HelpText(LOCTEXT("TradePropellantHelp", "Trade propellant with this station"))
						.Action(FNeutronPlayerInput::MenuPrimary)
						.ActionFocusable(false)
						.Size("HighButtonSize")
						.Enabled_Lambda([=]()
						{
							return SpacecraftPawn && SpacecraftPawn->IsDocked() && !SpacecraftPawn->HasModifications();
						})
						.OnClicked(this, &SNovaMainMenuOperations::OnRefillPropellant)
						.Content()
						[
							SNew(SOverlay)
						
							+ SOverlay::Slot()
							[
								SNew(SScaleBox)
								.Stretch(EStretch::ScaleToFill)
								[
									SNew(SImage)
									.Image(&UNovaResource::GetPropellant()->AssetRender)
								]
							]

							+ SOverlay::Slot()
							.VAlign(VAlign_Top)
							[
								SNew(SBorder)
								.BorderImage(&Theme.MainMenuDarkBackground)
								.Padding(Theme.ContentPadding)
								[
									SNew(STextBlock)
									.TextStyle(&Theme.MainFont)
									.Text(LOCTEXT("TradePropellant", "Trade propellant"))
								]
							]
						]
					]
				]
			]
		]
		
		// Module groups & equipment
		+ SScrollBox::Slot()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(SBorder)
			.BorderImage(&Theme.MainMenuGenericBorder)
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredWidth(FullWidth / 2)
					[
						SNew(SVerticalBox)
		
						// Module groups title
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.VerticalContentPadding)
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
								.TextStyle(&Theme.HeadingFont)
								.Text(LOCTEXT("ModuleGroupsTitle", "Module groups"))
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SNeutronKeyLabel)
								.Action(FNeutronPlayerInput::MenuAltPrimary)
							]
						]

						// Module groups controls
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(ModuleGroupsBox, SVerticalBox)
						]
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredWidth(FullWidth / 2)
					[
						SNew(SVerticalBox)
		
						// Equipment title
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.VerticalContentPadding)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.HeadingFont)
							.Text(LOCTEXT("EquipmentTitle", "Equipment"))
						]

						// Equipment controls
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(EquipmentBox, SVerticalBox)
						]
					]
				]
			]
		]

		// Compartments title
		+ SScrollBox::Slot()
		.Padding(Theme.VerticalContentPadding)
		.HAlign(HAlign_Center)
		[
			SAssignNew(CompartmentBox, SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.TextStyle(&Theme.HeadingFont)
				.Text(LOCTEXT("CompartmentsTitle", "Compartments"))
			]
		]
	];

	// Module line generator
	auto BuildModuleLine = [&](int32 ModuleIndex)
	{
		TSharedPtr<SHorizontalBox> ModuleLineBox;
		CompartmentBox->AddSlot()
		[
			SAssignNew(ModuleLineBox, SHorizontalBox)
		];

		for (int32 CompartmentIndex = 0; CompartmentIndex < ENovaConstants::MaxCompartmentCount; CompartmentIndex++)
		{
			ModuleLineBox->AddSlot()
			.AutoWidth()
			[
				SNeutronNew(SNeutronButton)
				.Size("InventoryButtonSize")
				.HelpText(this, &SNovaMainMenuOperations::GetModuleHelpText, CompartmentIndex, ModuleIndex)
				.Enabled(this, &SNovaMainMenuOperations::IsModuleEnabled, CompartmentIndex, ModuleIndex)
				.OnClicked(this, &SNovaMainMenuOperations::OnInteractWithModule, CompartmentIndex, ModuleIndex)
				.Content()
				[
					SNew(SOverlay)

					// Resource picture
					+ SOverlay::Slot()
					[
						SNew(SScaleBox)
						.Stretch(EStretch::ScaleToFill)
						[
							SNew(SNeutronImage)
							.Image(FNeutronImageGetter::CreateSP(this, &SNovaMainMenuOperations::GetModuleImage, CompartmentIndex, ModuleIndex))
						]
					]

					// Compartment index
					+ SOverlay::Slot()
					[
						SNew(SVerticalBox)

						// Name & details
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(&Theme.MainMenuDarkBackground)
							.Padding(Theme.ContentPadding)
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SNeutronText)
									.Text(FNeutronTextGetter::CreateSP(this, &SNovaMainMenuOperations::GetModuleText, CompartmentIndex, ModuleIndex))
									.TextStyle(&Theme.MainFont)
									.AutoWrapText(true)
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SNeutronRichText)
									.Text(FNeutronTextGetter::CreateSP(this, &SNovaMainMenuOperations::GetModuleDetails, CompartmentIndex, ModuleIndex))
									.TextStyle(&Theme.MainFont)
									.AutoWrapText(true)
								]
							]
						]
					
						+ SVerticalBox::Slot()

						// Module group
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.ColorAndOpacity_Lambda([=]()
							{
								if (IsValidModule(CompartmentIndex, ModuleIndex))
								{
									const FNovaModuleGroup* ModuleGroup = SpacecraftPawn->FindModuleGroup(CompartmentIndex, ModuleIndex);
									if (ModuleGroup)
									{
										return ModuleGroup->Color;
									}
								}

								return FLinearColor::White;
							})
							.BorderImage(&Theme.MainMenuGenericBackground)
							.Padding(Theme.ContentPadding)
							.Visibility_Lambda([=]()
							{
								if (IsValidModule(CompartmentIndex, ModuleIndex))
								{
									return EVisibility::Visible;
								}

								return EVisibility::Collapsed;
							})
							[
								SNew(SNeutronRichText)
								.Text(FNeutronTextGetter::CreateLambda([=]() -> FText
								{
									if (IsValidModule(CompartmentIndex, ModuleIndex))
									{
										const FNovaModuleGroup* ModuleGroup = SpacecraftPawn->FindModuleGroup(CompartmentIndex, ModuleIndex);
										if (ModuleGroup)
										{
											return FText::FormatNamed(INVTEXT("<img src=\"{icon}\"/> {index}"),
												TEXT("icon"), FNovaSpacecraft::GetModuleGroupIcon(ModuleGroup->Type),
												TEXT("index"), FText::AsNumber(ModuleGroup->Index + 1));
										}
									}
							
									return FText();
								}))
								.TextStyle(&Theme.MainFont)
								.WrapTextAt(1.9f * FNeutronStyleSet::GetButtonSize("CompartmentButtonSize").Width)
								.AutoWrapText(false)
							]
						]
					]
				]
			];
		}
	};
	// clang-format on

	// Build the trading panels
	GenericModalPanel = Menu->CreateModalPanel();
	ModuleGroupsPanel = Menu->CreateModalPanel<SNovaModuleGroupsPanel>();
	TradingModalPanel = Menu->CreateModalPanel<SNovaTradingPanel>();

	// Build the resource list panel
	// clang-format off
	SAssignNew(ResourceListView, SNeutronListView<const UNovaResource*>)
	.Panel(GenericModalPanel.Get())
	.ItemsSource(&ResourceList)
	.OnGenerateItem(this, &SNovaMainMenuOperations::GenerateResourceItem)
	.OnGenerateTooltip(this, &SNovaMainMenuOperations::GenerateResourceTooltip)
	.OnSelectionDoubleClicked(this, &SNovaMainMenuOperations::OnBuyResource)
	.Horizontal(true)
	.ButtonSize("LargeListButtonSize");
	// clang-format on

	// Build the procedural cargo lines
	for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
	{
		BuildModuleLine(ModuleIndex);
	}

	AveragedPropellantRatio.SetPeriod(1.0f);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuOperations::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNeutronTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	if (Spacecraft && GameState)
	{
		AveragedPropellantRatio.Set(PropellantSystem->GetCurrentPropellantMass() / PropellantSystem->GetPropellantCapacity(), DeltaTime);
	}
}

void SNovaMainMenuOperations::Show()
{
	SNeutronTabPanel::Show();

	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	NCHECK(Spacecraft);
	ProcessingSystem->Load(*Spacecraft);

	// Add module groups
	for (int32 ProcessingGroupIndex = 0; ProcessingGroupIndex < ProcessingSystem->GetProcessingGroupCount(); ProcessingGroupIndex++)
	{
		const FNovaModuleGroup& Group = ProcessingSystem->GetModuleGroup(ProcessingGroupIndex);

		TSharedPtr<SHorizontalBox> StatusBox;
		TSharedPtr<SNeutronButton> EnableButton;

		// clang-format off
		ModuleGroupsBox->AddSlot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(SBorder)
			.BorderImage(&Theme.MainMenuGenericBackground)
			.Padding(0)
			[
				SNew(SHorizontalBox)
			
				// Text-based information
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SNeutronButtonLayout)
					.Size("OperationsButtonSize")
					[
						SNew(SHorizontalBox)

						// Group icon & index
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(Theme.ContentPadding)
						.VAlign(VAlign_Center)
						[
							SNew(SBorder)
							.Padding(0)
							.BorderImage(new FSlateNoResource)
							.ColorAndOpacity(Group.Color)
							[
								SNew(SRichTextBlock)
								.TextStyle(&Theme.HeadingFont)
								.Text(FText::FormatNamed(INVTEXT("<img src=\"{icon}\"/>\n{index}"),
																TEXT("icon"), FNovaSpacecraft::GetModuleGroupIcon(Group.Type),
																TEXT("index"), FText::AsNumber(Group.Index + 1)))
								.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
								+ SRichTextBlock::ImageDecorator()
							]
						]
		
						// Production status
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SAssignNew(StatusBox, SHorizontalBox)
						]
					
						// Resources breakdown
						+ SHorizontalBox::Slot()
						.Padding(Theme.ContentPadding)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.MainFont)
							.Text_Lambda([=]()
							{
								// Resource inputs
								FString InputList;
								for (const UNovaResource* Input : ProcessingSystem->GetInputResources(Group.Index))
								{
									InputList += InputList.Len() ? TEXT(", ") : FString();
									InputList += Input->Name.ToString();
								}

								// Resource outputs
								FString OutputList;
								for (const UNovaResource* Output : ProcessingSystem->GetOutputResources(Group.Index))
								{
									OutputList += OutputList.Len() ? TEXT(", ") : FString();
									OutputList += Output->Name.ToString();
								}

								// Final string
								FText InputLine = InputList.Len() ? FText::FormatNamed(LOCTEXT("ProcessingInput", "Input resources: {list}"),
									TEXT("list"), FText::FromString(InputList)) : FText();
								FText OutputLine = OutputList.Len() ? FText::FormatNamed(LOCTEXT("ProcessingOutput", "Output resources: {list}"),
									TEXT("list"), FText::FromString(OutputList)) : FText();
								return FText::FromString(InputLine.ToString() + "\n" + OutputLine.ToString());
							})
						]
					]
				]

				// Processing group activity toggle
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNeutronAssignNew(EnableButton, SNeutronButton)
					.Size("HalfButtonSize")
					.Toggle(true)
					.Text_Lambda([=]()
					{
						return ProcessingSystem->IsProcessingGroupActive(Group.Index) ? LOCTEXT("StopProcessing", "Stop") : LOCTEXT("StartProcessing", "Start");
					})
					.HelpText_Lambda([=]()
					{
						auto Status = ProcessingSystem->GetProcessingGroupStatus(Group.Index);
						if (Status.Num() == 0)
						{
							return LOCTEXT("ProcessingNoneHelp", "This module group doesn't have a valid configuration");
						}
						else if (Status.Contains(ENovaSpacecraftProcessingSystemStatus::Docked))
						{
							return LOCTEXT("ProcessingDockedHelp", "Modules cannot be activated while docked");
						}
						else
						{
							return LOCTEXT("ProcessingHelp", "Toggle activity for this module group");
						}
					})
					.Enabled_Lambda([=]()
					{
						auto Status = ProcessingSystem->GetProcessingGroupStatus(Group.Index);
						return Status.Num() && !Status.Contains(ENovaSpacecraftProcessingSystemStatus::Docked);
					})
				]
			
				// Processing group inspection
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNeutronNew(SNeutronButton)
					.Size("HalfButtonSize")
					.Text(LOCTEXT("ProcessingGroupDetails", "Details"))
					.HelpText(LOCTEXT("ProcessingGroupDetailsHelp", "Inspect resource processing for this module group"))
					.OnClicked(FSimpleDelegate::CreateLambda([=]()
					{
						ModuleGroupsPanel->OpenModuleGroup(ProcessingSystem, Group.Index);
					}))
				]
			]
		];
		// clang-format on

		if (ProcessingGroupIndex == 0)
		{
			DefaultModuleButton = EnableButton;
		}

		// Add status icons
		for (int32 Index = 0; Index < ProcessingSystem->GetProcessingGroupStatus(Group.Index).Num(); Index++)
		{
			// clang-format off
			StatusBox->AddSlot()
			.AutoWidth()
			[
				SNew(SNeutronImage)
				.Image(FNeutronImageGetter::CreateLambda([=]()
				{
					switch (ProcessingSystem->GetProcessingGroupStatus(Group.Index)[Index])
					{
						default:
						case ENovaSpacecraftProcessingSystemStatus::Stopped:
						case ENovaSpacecraftProcessingSystemStatus::Docked:
						case ENovaSpacecraftProcessingSystemStatus::Blocked:
							return FNeutronStyleSet::GetBrush("Icon/SB_Warning");
						case ENovaSpacecraftProcessingSystemStatus::Processing:
							return FNeutronStyleSet::GetBrush("Icon/SB_On");
					}
				}))
				.ColorAndOpacity_Lambda([=]()
				{
					switch (ProcessingSystem->GetProcessingGroupStatus(Group.Index)[Index])
					{
						default:
						case ENovaSpacecraftProcessingSystemStatus::Stopped:
						case ENovaSpacecraftProcessingSystemStatus::Docked:
							return FLinearColor::White;
						case ENovaSpacecraftProcessingSystemStatus::Processing:
							return Theme.PositiveColor;
						case ENovaSpacecraftProcessingSystemStatus::Blocked:
							return Theme.NegativeColor;
					}
				})
			];
			// clang-format on
		}
	}

	// No group found
	if (ProcessingSystem->GetProcessingGroupCount() == 0)
	{
		// clang-format off
		ModuleGroupsBox->AddSlot()
		.AutoHeight()
		.Padding(Theme.ContentPadding)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.InfoFont)
			.Text(LOCTEXT("NoModuleGroup", "This spacecraft doesn't have active module groups"))
		];
		// clang-format on
	}

	// TODO: equipment

	// No equipment found
	if (true)
	{
		// clang-format off
		EquipmentBox->AddSlot()
		.AutoHeight()
		.Padding(Theme.ContentPadding)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.InfoFont)
			.Text(LOCTEXT("NoEquipment", "This spacecraft doesn't have active equipment"))
		];
		// clang-format on
	}

	GetMenu()->RefreshNavigationPanel();
}

void SNovaMainMenuOperations::Hide()
{
	SNeutronTabPanel::Hide();

	ModuleGroupsBox->ClearChildren();
	EquipmentBox->ClearChildren();
	DefaultModuleButton.Reset();
}

void SNovaMainMenuOperations::UpdateGameObjects()
{
	PC             = MenuManager.IsValid() ? MenuManager->GetPC<ANovaPlayerController>() : nullptr;
	GameState      = IsValid(PC) ? MenuManager->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	Spacecraft     = IsValid(PC) ? PC->GetSpacecraft() : nullptr;
	SpacecraftPawn = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
	PropellantSystem =
		IsValid(GameState) && Spacecraft ? GameState->GetSpacecraftSystem<UNovaSpacecraftPropellantSystem>(Spacecraft) : nullptr;
	ProcessingSystem =
		IsValid(GameState) && Spacecraft ? GameState->GetSpacecraftSystem<UNovaSpacecraftProcessingSystem>(Spacecraft) : nullptr;
}

void SNovaMainMenuOperations::OnKeyPressed(const FKey& Key)
{
	if (MenuManager->GetMenu()->IsActionKey(FNeutronPlayerInput::MenuAltPrimary, Key))
	{
		if (DefaultModuleButton.IsValid())
		{
			GetMenu()->SetFocusedButton(DefaultModuleButton, true);
		}
	}
}

/*----------------------------------------------------
    Resource list
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuOperations::GenerateResourceItem(const UNovaResource* Resource)
{
	return SNew(SNovaTradableAssetItem).Asset(Resource).GameState(GameState).Dark(true);
}

FText SNovaMainMenuOperations::GenerateResourceTooltip(const UNovaResource* Resource)
{
	return FText::FormatNamed(LOCTEXT("ResourceHelp", "Buy {resource}"), TEXT("resource"), Resource->Name);
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

bool SNovaMainMenuOperations::IsBulkTradeEnabled() const
{
	return SpacecraftPawn->IsDocked() && !SpacecraftPawn->HasModifications();
}

TOptional<float> SNovaMainMenuOperations::GetPropellantRatio() const
{
	return AveragedPropellantRatio.Get();
}

FText SNovaMainMenuOperations::GetPropellantText() const
{
	if (SpacecraftPawn && Spacecraft)
	{
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits = 0;

		float RemainingDeltaV = Spacecraft->GetPropulsionMetrics().GetRemainingDeltaV(
			Spacecraft->GetCurrentCargoMass(), PropellantSystem->GetCurrentPropellantMass());

		return FText::FormatNamed(
			LOCTEXT("PropellantFormat",
				"<img src=\"/Text/Propellant\"/> {remaining} T out of {total} T <img src=\"/Text/Thrust\"/> {deltav} m/s delta-v"),
			TEXT("remaining"), FText::AsNumber(PropellantSystem->GetCurrentPropellantMass(), &Options), TEXT("total"),
			FText::AsNumber(PropellantSystem->GetPropellantCapacity(), &Options), TEXT("deltav"),
			FText::AsNumber(RemainingDeltaV, &Options));
	}

	return FText();
}

bool SNovaMainMenuOperations::IsValidModule(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (Spacecraft && IsValid(SpacecraftPawn) && CompartmentIndex >= 0 && CompartmentIndex < Spacecraft->Compartments.Num() &&
		IsValid(GetModule(CompartmentIndex, ModuleIndex).Description))
	{
		const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

		// Cargo
		if (Desc->IsA<UNovaCargoModuleDescription>())
		{
			return Spacecraft->Compartments[CompartmentIndex].GetCargoCapacity(ModuleIndex) > 0;
		}

		// Propellant
		else if (Desc->IsA<UNovaPropellantModuleDescription>())
		{
			return true;
		}

		// Processing
		else if (Desc->IsA<UNovaProcessingModuleDescription>())
		{
			return true;
		}
	}

	return false;
}

const FNovaCompartmentModule& SNovaMainMenuOperations::GetModule(int32 CompartmentIndex, int32 ModuleIndex) const
{
	return Spacecraft->Compartments[CompartmentIndex].Modules[ModuleIndex];
}

bool SNovaMainMenuOperations::IsModuleEnabled(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (IsValidModule(CompartmentIndex, ModuleIndex))
	{
		const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

		// Cargo
		if (Desc->IsA<UNovaCargoModuleDescription>())
		{
			const FNovaSpacecraftCargo& Cargo = ProcessingSystem->GetCargo(CompartmentIndex, ModuleIndex);
			return (SpacecraftPawn->IsDocked() || IsValid(Cargo.Resource)) && !SpacecraftPawn->HasModifications();
		}

		// Propellant
		else if (Desc->IsA<UNovaPropellantModuleDescription>())
		{
			return SpacecraftPawn->IsDocked() && !SpacecraftPawn->HasModifications();
		}

		// Processing
		else if (Desc->IsA<UNovaProcessingModuleDescription>())
		{
			return true;
		}
	}

	return false;
}

FText SNovaMainMenuOperations::GetModuleHelpText(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (IsValidModule(CompartmentIndex, ModuleIndex))
	{
		const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

		// Cargo
		if (Desc->IsA<UNovaCargoModuleDescription>())
		{
			if (SpacecraftPawn && SpacecraftPawn->IsDocked())
			{
				return LOCTEXT("TradeSlotHelp", "Trade with this cargo slot");
			}
			else
			{
				return LOCTEXT("InspectSlotHelp", "Inspect this cargo slot");
			}
		}

		// Propellant
		else if (Desc->IsA<UNovaPropellantModuleDescription>())
		{
			return LOCTEXT("PropellantHelp", "Trade propellant");
		}

		// Processing
		else if (Desc->IsA<UNovaProcessingModuleDescription>())
		{
			return LOCTEXT("ProcessingHelp", "Control resource processing");
		}
	}

	return FText();
}

const FSlateBrush* SNovaMainMenuOperations::GetModuleImage(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (IsValidModule(CompartmentIndex, ModuleIndex))
	{
		const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

		// Cargo
		if (Desc->IsA<UNovaCargoModuleDescription>())
		{
			const FNovaSpacecraftCargo& Cargo = ProcessingSystem->GetCargo(CompartmentIndex, ModuleIndex);
			if (IsValid(Cargo.Resource))
			{
				return &Cargo.Resource->AssetRender;
			}
		}

		// Propellant
		else if (Desc->IsA<UNovaPropellantModuleDescription>())
		{
			return &UNovaResource::GetPropellant()->AssetRender;
		}

		// Processing
		else if (Desc->IsA<UNovaProcessingModuleDescription>())
		{
			return &Desc->AssetRender;
		}
	}

	return &UNovaResource::GetEmpty()->AssetRender;
}

FText SNovaMainMenuOperations::GetModuleText(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (IsValidModule(CompartmentIndex, ModuleIndex))
	{
		const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

		// Cargo
		if (Desc->IsA<UNovaCargoModuleDescription>())
		{
			const FNovaSpacecraftCargo& Cargo = ProcessingSystem->GetCargo(CompartmentIndex, ModuleIndex);
			if (IsValid(Cargo.Resource))
			{
				return Cargo.Resource->Name;
			}
			else
			{
				return LOCTEXT("EmptyCargo", "Empty");
			}
		}

		// Propellant
		else if (Desc->IsA<UNovaPropellantModuleDescription>())
		{
			return LOCTEXT("PropellantModule", "Propellant");
		}

		// Processing
		else if (Desc->IsA<UNovaProcessingModuleDescription>())
		{
			return Desc->Name;
		}
	}

	return FText();
}

FText SNovaMainMenuOperations::GetModuleDetails(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (IsValidModule(CompartmentIndex, ModuleIndex))
	{
		const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

		// Cargo
		if (Desc->IsA<UNovaCargoModuleDescription>())
		{
			const FNovaCompartment&     Compartment = Spacecraft->Compartments[CompartmentIndex];
			const FNovaSpacecraftCargo& Cargo       = ProcessingSystem->GetCargo(CompartmentIndex, ModuleIndex);

			int32 Amount   = Cargo.Amount;
			int32 Capacity = Compartment.GetCargoCapacity(ModuleIndex);

			return FText::FormatNamed(LOCTEXT("CargoAmountFormat", "<img src=\"/Text/Cargo\"/> {amount} T / {capacity} T"), TEXT("amount"),
				FText::AsNumber(Amount), TEXT("capacity"), FText::AsNumber(Capacity));
		}

		// Propellant
		else if (Desc->IsA<UNovaPropellantModuleDescription>())
		{
			FNumberFormattingOptions Options;
			Options.MaximumFractionalDigits = 0;

			float RemainingDeltaV = Spacecraft->GetPropulsionMetrics().GetRemainingDeltaV(
				Spacecraft->GetCurrentCargoMass(), PropellantSystem->GetCurrentPropellantMass());

			return FText::FormatNamed(LOCTEXT("PropellantPercentFormat", "<img src=\"/Text/Propellant\"/> {percent}%"), TEXT("percent"),
				FText::AsNumber(100 * PropellantSystem->GetCurrentPropellantMass() / PropellantSystem->GetPropellantCapacity(), &Options));
		}

		// Processing
		else if (Desc->IsA<UNovaProcessingModuleDescription>())
		{
			switch (ProcessingSystem->GetModuleStatus(CompartmentIndex, ModuleIndex))
			{
				case ENovaSpacecraftProcessingSystemStatus::Stopped:
					return LOCTEXT("ProcessingStopped", "Stopped");
					break;
				case ENovaSpacecraftProcessingSystemStatus::Processing:
					return LOCTEXT("ProcessingProcessing", "Active");
					break;
				case ENovaSpacecraftProcessingSystemStatus::Blocked:
					return LOCTEXT("ProcessingBlocked", "Blocked");
					break;
				case ENovaSpacecraftProcessingSystemStatus::Docked:
					return LOCTEXT("ProcessingDocked", "Stopped");
					break;
			}
		}
	}

	return FText();
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuOperations::OnRefillPropellant()
{
	TradingModalPanel->Trade(PC, GameState->GetCurrentArea(), UNovaResource::GetPropellant(), INDEX_NONE, INDEX_NONE);
}

void SNovaMainMenuOperations::OnBatchBuy()
{
	CurrentCompartmentIndex = INDEX_NONE;
	CurrentModuleIndex      = INDEX_NONE;

	auto TradeResource = [this]()
	{
		TradingModalPanel->Trade(PC, GameState->GetCurrentArea(), ResourceListView->GetSelectedItem(), INDEX_NONE, INDEX_NONE);
	};

	// Fill the resource list
	ResourceList.Empty();
	for (const UNovaResource* Resource : GameState->GetResourcesSold())
	{
		ResourceList.Add(Resource);
	}
	ResourceListView->Refresh(0);

	// Proceed with the modal panel
	GenericModalPanel->Show(
		FText::FormatNamed(LOCTEXT("SelectBulkBuyFormat", "Bulk buy from {station}"), TEXT("station"), GameState->GetCurrentArea()->Name),
		ResourceList.Num() == 0 ? LOCTEXT("NoBulkBuyResource", "No resources are available for sale at this station") : FText(),
		FSimpleDelegate::CreateLambda(TradeResource), FSimpleDelegate(), FSimpleDelegate(), ResourceListView);
}

void SNovaMainMenuOperations::OnBatchSell()
{
	CurrentCompartmentIndex = INDEX_NONE;
	CurrentModuleIndex      = INDEX_NONE;

	auto TradeResource = [this]()
	{
		TradingModalPanel->Trade(PC, GameState->GetCurrentArea(), ResourceListView->GetSelectedItem(), INDEX_NONE, INDEX_NONE);
	};

	// Fill the resource list
	ResourceList.Empty();
	for (const UNovaResource* Resource : Spacecraft->GetOwnedResources())
	{
		if (!GameState->IsResourceSold(Resource))
		{
			ResourceList.Add(Resource);
		}
	}
	ResourceListView->Refresh(0);

	// Proceed with the modal panel
	GenericModalPanel->Show(
		FText::FormatNamed(LOCTEXT("SelectBulkSellFormat", "Bulk sell to {station}"), TEXT("station"), GameState->GetCurrentArea()->Name),
		ResourceList.Num() == 0 ? LOCTEXT("NoBulkSellResource", "No resources in cargo can be sold at this station") : FText(),
		FSimpleDelegate::CreateLambda(TradeResource), FSimpleDelegate(), FSimpleDelegate(), ResourceListView);
}

void SNovaMainMenuOperations::OnInteractWithModule(int32 CompartmentIndex, int32 ModuleIndex)
{
	NCHECK(GameState);
	NCHECK(GameState->GetCurrentArea());

	const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

	// Cargo
	if (Desc->IsA<UNovaCargoModuleDescription>())
	{
		const FNovaSpacecraftCargo& Cargo = ProcessingSystem->GetCargo(CompartmentIndex, ModuleIndex);

		// Docked mode : trade with resource
		if (SpacecraftPawn->IsDocked())
		{
			// Valid resource in hold - allow trading it directly
			if (IsValid(Cargo.Resource))
			{
				TradingModalPanel->Trade(PC, GameState->GetCurrentArea(), Cargo.Resource, CompartmentIndex, ModuleIndex);
			}

			// Cargo hold is empty, pick a resource to buy first
			else
			{
				CurrentCompartmentIndex = CompartmentIndex;
				CurrentModuleIndex      = ModuleIndex;

				auto BuyResource = [this, CompartmentIndex, ModuleIndex]()
				{
					TradingModalPanel->Trade(
						PC, GameState->GetCurrentArea(), ResourceListView->GetSelectedItem(), CompartmentIndex, ModuleIndex);
				};

				//	Fill the resource list
				ResourceList = GameState->GetResourcesSold();
				ResourceListView->Refresh(0);

				// Proceed with the modal panel
				GenericModalPanel->Show(FText::FormatNamed(LOCTEXT("SelectResourceFormat", "For sale at {station}"), TEXT("station"),
											GameState->GetCurrentArea()->Name),
					ResourceList.Num() == 0 ? LOCTEXT("NoResource", "No resources are available for sale at this station") : FText(),
					FSimpleDelegate::CreateLambda(BuyResource), FSimpleDelegate(), FSimpleDelegate(), ResourceListView);
			}
		}

		// Show the resource
		else
		{
			NCHECK(IsValid(Cargo.Resource));
			TradingModalPanel->Inspect(PC, GameState->GetCurrentArea(), Cargo.Resource, CompartmentIndex, ModuleIndex);
		}
	}

	// Propellant
	else if (Desc->IsA<UNovaPropellantModuleDescription>())
	{
		OnRefillPropellant();
	}

	// Processing
	else if (Desc->IsA<UNovaProcessingModuleDescription>())
	{
		NCHECK(Spacecraft);
		ModuleGroupsPanel->OpenModuleGroup(ProcessingSystem, CompartmentIndex, ModuleIndex);
	}
}

void SNovaMainMenuOperations::OnBuyResource()
{
	GenericModalPanel->Hide();

	TradingModalPanel->Trade(
		PC, GameState->GetCurrentArea(), ResourceListView->GetSelectedItem(), CurrentCompartmentIndex, CurrentModuleIndex);
}

#undef LOCTEXT_NAMESPACE
