// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuOperations.h"

#include "NovaMainMenu.h"

#include "Game/NovaGameState.h"
#include "Player/NovaPlayerController.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/System/NovaSpacecraftCrewSystem.h"
#include "Spacecraft/System/NovaSpacecraftPropellantSystem.h"
#include "Spacecraft/System/NovaSpacecraftProcessingSystem.h"
#include "Spacecraft/System/NovaSpacecraftPowerSystem.h"

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
		SAssignNew(MainContainer, SScrollBox)
		.Style(&Theme.ScrollBoxStyle)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.AnimateWheelScrolling(true)
		
		// Operations header
		+ SScrollBox::Slot()
		.HAlign(HAlign_Center)
		.Padding(Theme.VerticalContentPadding + FMargin(0, 20, 0, 0))
		[
			SNew(SBox)
			.MinDesiredWidth(FullWidth)
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)

				// Crew
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SNeutronButtonLayout)
					.Size("HighButtonSizeAlt")
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.VerticalContentPadding)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.HeadingFont)
							.Text(LOCTEXT("CrewTitle", "Crew"))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.VerticalContentPadding)
						[
							SNew(SNeutronRichText)
							.TextStyle(&Theme.MainFont)
							.Text(FNeutronTextGetter::CreateRaw(this, &SNovaMainMenuOperations::GetCrewText))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SProgressBar)
							.Style(&Theme.ProgressBarStyle)
							.Percent(this, &SNovaMainMenuOperations::GetCrewRatio)
						]

						+ SVerticalBox::Slot()
					]
				]

				// Power
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SNeutronButtonLayout)
					.Size("HighButtonSizeAlt")
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.VerticalContentPadding)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.HeadingFont)
							.Text(LOCTEXT("PowerTitle", "Power"))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.VerticalContentPadding)
						[
							SNew(SRichTextBlock)
							.TextStyle(&Theme.MainFont)
							.Text(this, &SNovaMainMenuOperations::GetPowerText)
							.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
							+ SRichTextBlock::ImageDecorator()
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SProgressBar)
							.Style(&Theme.ProgressBarStyle)
							.Percent(this, &SNovaMainMenuOperations::GetPowerRatio)
						]

						+ SVerticalBox::Slot()
					]
				]

				// Energy
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SNeutronButtonLayout)
					.Size("HighButtonSizeAlt")
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.VerticalContentPadding)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.HeadingFont)
							.Text(LOCTEXT("EnergyTitle", "Batteries"))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.VerticalContentPadding)
						[
							SNew(SRichTextBlock)
							.TextStyle(&Theme.MainFont)
							.Text(this, &SNovaMainMenuOperations::GetEnergyText)
							.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
							+ SRichTextBlock::ImageDecorator()
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SProgressBar)
							.Style(&Theme.ProgressBarStyle)
							.Percent(this, &SNovaMainMenuOperations::GetEnergyRatio)
						]

						+ SVerticalBox::Slot()
					]
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
					SNeutronDefaultNew(SNeutronButton)
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
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SNeutronKeyLabel)
									.Action(FNeutronPlayerInput::MenuPrimary)
								]
						
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.TextStyle(&Theme.MainFont)
									.Text(LOCTEXT("TradePropellant", "Trade propellant"))
								]
							]
						]
					]
				]
	
				// Batch buying
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNeutronNew(SNeutronButton)
					.HelpText(LOCTEXT("BulkBuyHelp", "Bulk buy resources across all compartments"))
					.Action(FNeutronPlayerInput::MenuSecondary)
					.ActionFocusable(false)
					.Enabled(this, &SNovaMainMenuOperations::IsBulkTradeEnabled)
					.OnClicked(this, &SNovaMainMenuOperations::OnBatchBuy)
					.Size("HighButtonSize")
					.Content()
					[
						SNew(SOverlay)
						
						+ SOverlay::Slot()
						[
							SNew(SScaleBox)
							.Stretch(EStretch::ScaleToFill)
							[
								SNew(SImage)
								.Image(&UNovaResource::GetGeneric()->AssetRender)
							]
						]

						+ SOverlay::Slot()
						.VAlign(VAlign_Top)
						[
							SNew(SBorder)
							.BorderImage(&Theme.MainMenuDarkBackground)
							.Padding(Theme.ContentPadding)
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SNeutronKeyLabel)
									.Action(FNeutronPlayerInput::MenuSecondary)
								]
						
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.TextStyle(&Theme.MainFont)
									.Text(LOCTEXT("BulkBuy", "Bulk buy"))
								]
							]
						]
					]
				]

				// Batch selling
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNeutronNew(SNeutronButton)
					.HelpText(LOCTEXT("BulkSellHelp", "Bulk sell resources across all compartments"))
					.Action(FNeutronPlayerInput::MenuAltSecondary)
					.ActionFocusable(false)
					.Enabled(this, &SNovaMainMenuOperations::IsBulkTradeEnabled)
					.OnClicked(this, &SNovaMainMenuOperations::OnBatchSell)
					.Size("HighButtonSize")
					.Content()
					[
						SNew(SOverlay)
						
						+ SOverlay::Slot()
						[
							SNew(SScaleBox)
							.Stretch(EStretch::ScaleToFill)
							[
								SNew(SImage)
								.Image(&UNovaResource::GetGeneric()->AssetRender)
							]
						]

						+ SOverlay::Slot()
						.VAlign(VAlign_Top)
						[
							SNew(SBorder)
							.BorderImage(&Theme.MainMenuDarkBackground)
							.Padding(Theme.ContentPadding)
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SNeutronKeyLabel)
									.Action(FNeutronPlayerInput::MenuAltSecondary)
								]
						
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.TextStyle(&Theme.MainFont)
									.Text(LOCTEXT("BulkSell", "Bulk sell"))
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
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
								.TextStyle(&Theme.HeadingFont)
							.Text(LOCTEXT("EquipmentTitle", "Equipment"))
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SNeutronKeyLabel)
								.Action(FNeutronPlayerInput::MenuAltPrimary)
							]
						]

						// Equipment controls
						+ SVerticalBox::Slot()
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
	.ButtonSize("LargeListButtonSize");
	// clang-format on

	// Build the procedural cargo lines
	for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
	{
		BuildModuleLine(ModuleIndex);
	}

	AveragedPropellantRatio.SetPeriod(1.0f);
	AveragedCrewRatio.SetPeriod(1.0f);
	AveragedEnergyRatio.SetPeriod(1.0f);
	AveragedPowerRatio.SetPeriod(1.0f);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuOperations::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNeutronTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	if (Spacecraft && GameState && PropellantSystem && CrewSystem)
	{
		AveragedPropellantRatio.Set(PropellantSystem->GetCurrentPropellantMass() / PropellantSystem->GetPropellantCapacity(), DeltaTime);
		AveragedCrewRatio.Set(
			CrewSystem->GetCurrentCrew() > 0 ? static_cast<float>(CrewSystem->GetTotalBusyCrew()) / CrewSystem->GetCurrentCrew() : 0.0f,
			DeltaTime);
		AveragedEnergyRatio.Set(PowerSystem->GetEnergyCapacity() > 0
									? static_cast<float>(PowerSystem->GetRemainingEnergy()) / PowerSystem->GetEnergyCapacity()
									: 0.0f,
			DeltaTime);
		AveragedPowerRatio.Set(
			PowerSystem->GetPowerRange() > 0
				? static_cast<float>(PowerSystem->GetCurrentPower() - PowerSystem->GetMinimumPower()) / PowerSystem->GetPowerRange()
				: 0.0f,
			DeltaTime);
	}
}

void SNovaMainMenuOperations::Show()
{
	SNeutronTabPanel::Show();

	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	NCHECK(Spacecraft);
	if (SpacecraftPawn->IsDocked())
	{
		ProcessingSystem->Load(*Spacecraft);
		PowerSystem->Load(*Spacecraft);
		CrewSystem->Load(*Spacecraft);
	}

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
		
						// Crew status
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(20, 0))
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.HAlign(HAlign_Center)
							.AutoHeight()
							[
								SNew(SRichTextBlock)
								.TextStyle(&Theme.HeadingFont)
								.Text(INVTEXT("<img src=\"/Text/Crew\"/>"))
								.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
								+ SRichTextBlock::ImageDecorator()
							]

							+ SVerticalBox::Slot()
							.HAlign(HAlign_Center)
							.AutoHeight()
							[
								SNew(SNeutronText)
								.TextStyle(&Theme.MainFont)
								.Text(FNeutronTextGetter::CreateLambda([=]() {
									return CrewSystem ? FText::FormatNamed(INVTEXT("{busy} / {total}"),
										TEXT("busy"), FText::AsNumber(CrewSystem->GetBusyCrew(ProcessingGroupIndex)),
										TEXT("total"), FText::AsNumber(CrewSystem->GetRequiredCrew(ProcessingGroupIndex))) : FText();
								}))
							]
						]
		
						// Power status
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.HAlign(HAlign_Center)
							.AutoHeight()
							[
								SNew(SRichTextBlock)
								.TextStyle(&Theme.HeadingFont)
								.Text(INVTEXT("<img src=\"/Text/Power\"/>"))
								.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
								+ SRichTextBlock::ImageDecorator()
							]

							+ SVerticalBox::Slot()
							.HAlign(HAlign_Center)
							.AutoHeight()
							[
								SNew(SNeutronText)
								.TextStyle(&Theme.MainFont)
								.Text(FNeutronTextGetter::CreateLambda([=]() {
									return ProcessingSystem ? FText::FormatNamed(INVTEXT("{busy} / {total} kW"),
										TEXT("busy"), FText::AsNumber(ProcessingSystem->GetPowerUsage(ProcessingGroupIndex)),
										TEXT("total"), FText::AsNumber(ProcessingSystem->GetRequiredPowerUsage(ProcessingGroupIndex))) : FText();
								}))
							]
						]
		
						// Production status
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(20, 0))
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
								if (ProcessingSystem)
								{
									// Resource inputs
									FString InputList;
									for (const UNovaResource* Input : ProcessingSystem->GetInputResources(ProcessingGroupIndex))
									{
										InputList += InputList.Len() ? TEXT(", ") : FString();
										InputList += Input->Name.ToString();
									}

									// Resource outputs
									FString OutputList;
									for (const UNovaResource* Output : ProcessingSystem->GetOutputResources(ProcessingGroupIndex))
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
								}

								return FText();
							})
							.AutoWrapText(true)
						]
					]
				]

				// Processing group activity toggle
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNeutronAssignNew(EnableButton, SNeutronButton)
					.Size("HalfButtonSize")
					.Text_Lambda([=]()
					{
						return ProcessingSystem && ProcessingSystem->IsProcessingGroupActive(ProcessingGroupIndex) ? LOCTEXT("StopProcessing", "Stop") : LOCTEXT("StartProcessing", "Start");
					})
					.HelpText_Lambda([=]()
					{
						if (ProcessingSystem == nullptr) return FText();
						
						auto Status = ProcessingSystem->GetProcessingGroupStatus(ProcessingGroupIndex);
						bool IsActive = ProcessingSystem->IsProcessingGroupActive(ProcessingGroupIndex);

						if (Status.Num() == 0)
						{
							return LOCTEXT("ProcessingNoneHelp", "This module group doesn't have processing modules");
						}
						else if (!IsActive && Status.Contains(ENovaSpacecraftProcessingSystemStatus::Docked))
						{
							return LOCTEXT("ProcessingDockedHelp", "Modules cannot be activated while docked");
						}
						else if (!IsActive && Status.Contains(ENovaSpacecraftProcessingSystemStatus::Blocked))
						{
							return LOCTEXT("ProcessingBlockedHelp", "This module group is blocked");
						}
						else
						{
							return LOCTEXT("ProcessingToggleHelp", "Toggle activity for this module group");
						}
					})
					.Enabled_Lambda([=]()
					{
						if (ProcessingSystem && CrewSystem)
						{
							auto Status = ProcessingSystem->GetProcessingGroupStatus(ProcessingGroupIndex);
							bool IsActive = ProcessingSystem->IsProcessingGroupActive(ProcessingGroupIndex);
							bool HasEnoughCrew = CrewSystem->GetRequiredCrew(ProcessingGroupIndex) <= CrewSystem->GetTotalAvailableCrew();

							return Status.Num() && HasEnoughCrew && (IsActive || (!Status.Contains(ENovaSpacecraftProcessingSystemStatus::Docked)
								&& !Status.Contains(ENovaSpacecraftProcessingSystemStatus::Blocked)));
						}
						else
						{
							return false;
						}
					})
					.OnClicked(FSimpleDelegate::CreateLambda([=]()
					{
						ProcessingSystem->SetProcessingGroupActive(ProcessingGroupIndex, !ProcessingSystem->IsProcessingGroupActive(ProcessingGroupIndex));
					}))
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
						NCHECK(Spacecraft);
						ModuleGroupsPanel->OpenModuleGroup(ProcessingSystem, *Spacecraft, ProcessingGroupIndex);
					}))
				]
			]
		];
		// clang-format on

		ModuleEquipmentButtons.Add(EnableButton);
		if (ProcessingGroupIndex == 0)
		{
			DefaultModuleButton = EnableButton;
		}

		// Add status icons
		for (int32 Index = 0; Index < ProcessingSystem->GetProcessingGroupStatus(ProcessingGroupIndex).Num(); Index++)
		{
			// clang-format off
			StatusBox->AddSlot()
			.AutoWidth()
			[
				SNew(SNeutronImage)
				.Image(FNeutronImageGetter::CreateLambda([=]() -> const FSlateBrush*
				{
					if (ProcessingSystem == nullptr) return nullptr;

					switch (ProcessingSystem->GetProcessingGroupStatus(ProcessingGroupIndex)[Index])
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
					if (ProcessingSystem == nullptr) return FLinearColor::Black;

					switch (ProcessingSystem->GetProcessingGroupStatus(ProcessingGroupIndex)[Index])
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
		.Padding(Theme.ContentPadding)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.InfoFont)
			.Text(LOCTEXT("NoModuleGroup", "This spacecraft doesn't have active module groups"))
		];
		// clang-format on
	}

	// Add mining rig if found
	bool  HasEquipment        = false;
	int32 MiningRigGroupIndex = ProcessingSystem->GetMiningRigIndex();
	MiningRigButton.Reset();
	if (MiningRigGroupIndex != INDEX_NONE)
	{
		HasEquipment                  = true;
		const FNovaModuleGroup& Group = Spacecraft->GetModuleGroups()[MiningRigGroupIndex];

		// clang-format off
		EquipmentBox->AddSlot()
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
					.Size("OperationsAltButtonSize")
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
					
						// Mining rig state breakdown
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(Theme.ContentPadding)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.HeadingFont)
							.Text(LOCTEXT("MiningRigTitle", "Mining rig"))
						]
					
						// Mining rig state breakdown
						+ SHorizontalBox::Slot()
						.Padding(Theme.ContentPadding)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.MainFont)
							.Text_Lambda([=]()
							{
								if (ProcessingSystem && ProcessingSystem->GetMiningRigIndex() != INDEX_NONE)
								{
									const UNovaResource* Resource = ProcessingSystem->GetCurrentMiningResource();

									if (ProcessingSystem->IsMiningRigActive() && Resource && ProcessingSystem->GetMiningRigStatus() == ENovaSpacecraftProcessingSystemStatus::Processing)
									{
										FNumberFormattingOptions Options;
										Options.MaximumFractionalDigits = 2;
										return FText::FormatNamed(LOCTEXT("MiningRigActiveFormat", "Currently extracting {resource} at a rate of {rate}T/s"),
											TEXT("resource"), Resource->Name,
											TEXT("rate"), FText::AsNumber(ProcessingSystem->GetCurrentMiningRate(), &Options));
									}
									else
									{
										return LOCTEXT("MiningRigInactive", "Mining rig is currently inactive");
									}

								}

								return FText();
							})
						]
					]
				]

				// Mining rig activity toggle
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNeutronAssignNew(MiningRigButton, SNeutronButton)
					.Size("HalfButtonSize")
					.Text_Lambda([=]()
					{
						return ProcessingSystem && ProcessingSystem->IsMiningRigActive() ?
							LOCTEXT("StopMining", "Stop") : LOCTEXT("StartMining", "Start");
					})
					.HelpText_Lambda([=]()
					{
						FText Help;
						if (ProcessingSystem && ProcessingSystem->CanMiningRigBeActive(&Help))
						{
							return LOCTEXT("MiningHelp", "Toggle activity for this mining rig");
						}
						else
						{
							return Help;
						}
					})
					.Enabled_Lambda([=]()
					{
						if (ProcessingSystem && ProcessingSystem->GetMiningRigIndex() != INDEX_NONE)
						{
							return ProcessingSystem->GetMiningRigStatus() != ENovaSpacecraftProcessingSystemStatus::Docked
								&& (ProcessingSystem->IsMiningRigActive() || ProcessingSystem->CanMiningRigBeActive());
						}
						return false;
					})
					.OnClicked(FSimpleDelegate::CreateLambda([=]()
					{
						ProcessingSystem->SetMiningRigActive(MiningRigButton->IsActive());
					}))
				]
			]
		];
		// clang-format on

		ModuleEquipmentButtons.Add(MiningRigButton);
		DefaultEquipmentButton = MiningRigButton;
	}

	// No equipment found
	if (!HasEquipment)
	{
		// clang-format off
		EquipmentBox->AddSlot()
		.Padding(Theme.ContentPadding)
		.VAlign(VAlign_Center)
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
	DefaultEquipmentButton.Reset();
	ModuleEquipmentButtons.Empty();
}

void SNovaMainMenuOperations::UpdateGameObjects()
{
	PC             = MenuManager.IsValid() ? MenuManager->GetPC<ANovaPlayerController>() : nullptr;
	GameState      = IsValid(PC) ? MenuManager->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	Spacecraft     = IsValid(PC) ? PC->GetSpacecraft() : nullptr;
	SpacecraftPawn = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
	CrewSystem     = IsValid(GameState) && Spacecraft ? GameState->GetSpacecraftSystem<UNovaSpacecraftCrewSystem>(Spacecraft) : nullptr;
	PropellantSystem =
		IsValid(GameState) && Spacecraft ? GameState->GetSpacecraftSystem<UNovaSpacecraftPropellantSystem>(Spacecraft) : nullptr;
	ProcessingSystem =
		IsValid(GameState) && Spacecraft ? GameState->GetSpacecraftSystem<UNovaSpacecraftProcessingSystem>(Spacecraft) : nullptr;
	PowerSystem = IsValid(GameState) && Spacecraft ? GameState->GetSpacecraftSystem<UNovaSpacecraftPowerSystem>(Spacecraft) : nullptr;

	if (SpacecraftPawn && SpacecraftPawn->IsDocked())
	{
		for (const TSharedPtr<SNeutronButton>& Button : ModuleEquipmentButtons)
		{
			Button->SetActive(false);
		}
	}
}

void SNovaMainMenuOperations::OnKeyPressed(const FKey& Key)
{
	if (MenuManager->GetMenu()->IsActionKey(FNeutronPlayerInput::MenuAltPrimary, Key))
	{
		if (DefaultModuleButton.IsValid())
		{
			GetMenu()->SetFocusedButton(DefaultModuleButton, true);
		}
		else if (DefaultEquipmentButton.IsValid())
		{
			GetMenu()->SetFocusedButton(DefaultEquipmentButton, true);
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
	return IsValid(Resource) ? FText::FormatNamed(LOCTEXT("ResourceHelp", "Buy {resource}"), TEXT("resource"), Resource->Name) : FText();
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

bool SNovaMainMenuOperations::IsBulkTradeEnabled() const
{
	return SpacecraftPawn && SpacecraftPawn->IsDocked() && !SpacecraftPawn->HasModifications();
}

TOptional<float> SNovaMainMenuOperations::GetPropellantRatio() const
{
	return AveragedPropellantRatio.Get();
}

FText SNovaMainMenuOperations::GetCrewText() const
{
	return CrewSystem
	         ? FText::FormatNamed(LOCTEXT("CrewDetailsFormat", "<img src=\"/Text/Crew\"/> {busy} / {total} crew busy"), TEXT("busy"),
				   FText::AsNumber(CrewSystem->GetTotalBusyCrew()), TEXT("total"), FText::AsNumber(CrewSystem->GetCurrentCrew()))
	         : FText();
}

TOptional<float> SNovaMainMenuOperations::GetCrewRatio() const
{
	return AveragedCrewRatio.Get();
}

FText SNovaMainMenuOperations::GetEnergyText() const
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 1;
	Options.MaximumFractionalDigits = 1;

	return PowerSystem ? FText::FormatNamed(INVTEXT("<img src=\"/Text/Power\"/> {used} / {total} kWh"), TEXT("used"),
							 FText::AsNumber(PowerSystem->GetRemainingEnergy(), &Options), TEXT("total"),
							 FText::AsNumber(PowerSystem->GetEnergyCapacity(), &Options))
	                   : FText();
}

TOptional<float> SNovaMainMenuOperations::GetEnergyRatio() const
{
	return AveragedEnergyRatio.Get();
}

FText SNovaMainMenuOperations::GetPowerText() const
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 1;
	Options.MaximumFractionalDigits = 1;

	return PowerSystem
	         ? FText::FormatNamed(INVTEXT("<img src=\"/Text/PowerProducer\"/>{production} kW <img src=\"/Text/PowerConsumer\"/>{usage} kW"),
				   TEXT("production"), FText::AsNumber(PowerSystem->GetCurrentProduction(), &Options), TEXT("usage"),
				   FText::AsNumber(PowerSystem->GetCurrentConsumption(), &Options))
	         : FText();
}

TOptional<float> SNovaMainMenuOperations::GetPowerRatio() const
{
	return AveragedPowerRatio.Get();
}

FText SNovaMainMenuOperations::GetPropellantText() const
{
	if (SpacecraftPawn && Spacecraft && PropellantSystem)
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

		// Generic
		else if (Desc)
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

		// Generic
		else if (Desc)
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

		// Generic
		else if (Desc)
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

		// Generic
		else if (Desc)
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

			float Amount   = Cargo.Amount;
			float Capacity = Compartment.GetCargoCapacity(ModuleIndex);

			FNumberFormattingOptions Options;
			Options.MaximumFractionalDigits = 0;

			return FText::FormatNamed(LOCTEXT("CargoAmountFormat", "<img src=\"/Text/Cargo\"/> {amount} T / {capacity} T"), TEXT("amount"),
				FText::AsNumber(Amount, &Options), TEXT("capacity"), FText::AsNumber(Capacity));
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
			return UNovaSpacecraftProcessingSystem::GetStatusText(ProcessingSystem->GetModuleStatus(CompartmentIndex, ModuleIndex));
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
		if (IsValid(PC) && IsValid(GameState) && IsValid(GameState->GetCurrentArea()))
		{
			TradingModalPanel->Trade(PC, GameState->GetCurrentArea(), ResourceListView->GetSelectedItem(), INDEX_NONE, INDEX_NONE);
		}
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
		if (IsValid(PC) && IsValid(GameState) && IsValid(GameState->GetCurrentArea()))
		{
			TradingModalPanel->Trade(PC, GameState->GetCurrentArea(), ResourceListView->GetSelectedItem(), INDEX_NONE, INDEX_NONE);
		}
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
		ModuleGroupsPanel->OpenModuleGroup(ProcessingSystem, *Spacecraft, CompartmentIndex, ModuleIndex);
	}
}

void SNovaMainMenuOperations::OnBuyResource()
{
	GenericModalPanel->Hide();

	TradingModalPanel->Trade(
		PC, GameState->GetCurrentArea(), ResourceListView->GetSelectedItem(), CurrentCompartmentIndex, CurrentModuleIndex);
}

#undef LOCTEXT_NAMESPACE
