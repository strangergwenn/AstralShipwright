// Nova project - Gwennaël Arbona

#include "NovaMainMenuAssembly.h"
#include "NovaMainMenu.h"

#include "Nova/Actor/NovaTurntablePawn.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Game/NovaGameTypes.h"
#include "Nova/Game/NovaGameState.h"

#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaMenuManager.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"

#include "Nova/UI/Component/NovaSpacecraftDatasheet.h"
#include "Nova/UI/Component/NovaTradableAssetItem.h"
#include "Nova/UI/Widget/NovaFadingWidget.h"
#include "Nova/UI/Widget/NovaModalPanel.h"
#include "Nova/UI/Widget/NovaKeyLabel.h"
#include "Nova/UI/Widget/NovaSlider.h"

#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"
#include "DrawDebugHelpers.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuAssembly"

/*----------------------------------------------------
    Diff widget
----------------------------------------------------*/

class SNovaAssemblyModalPanel : public SNovaModalPanel
{
public:
	void ReviewAndSaveSpacecraft(
		ANovaPlayerController* PC, ANovaSpacecraftPawn* SpacecraftPawn, TSharedPtr<SNovaModalPanel> GenericModalPanel)
	{
		const FNovaSpacecraft* CurrentSpacecraft = PC->GetSpacecraft();
		NCHECK(CurrentSpacecraft);
		FNovaSpacecraft ModifiedSpacecraft = SpacecraftPawn->GetSpacecraftCopy();
		bool            HasModifiedDesign  = SpacecraftPawn->HasModifications();

		// Analyze changes
		HasValidSpacecraft = true;
		FText DetailsText  = LOCTEXT("NoChanges", "No changes were made to the current design");
		if (HasModifiedDesign)
		{
			HasValidSpacecraft = ModifiedSpacecraft.IsValid(&DetailsText);
		}

		// Build the cost table
		FNovaSpacecraftUpgradeCost Cost =
			ModifiedSpacecraft.GetUpgradeCost(PC->GetWorld()->GetGameState<ANovaGameState>(), CurrentSpacecraft);
		TSharedRef<SNovaTable> CostTable = SNew(SNovaTable).Title(LOCTEXT("CostAnalysis", "Cost analysis"));
		CostTable->AddEntry(LOCTEXT("UpgradeCostUpgrades", "New parts bought"), GetPriceText(Cost.UpgradeCosts));
		CostTable->AddEntry(LOCTEXT("UpgradeCostResale", "Parts resold"), GetPriceText(Cost.ResaleGains));
		CostTable->AddEntry(LOCTEXT("UpgradeCostDepreciation", "Parts depreciation"),
			FMath::RoundToInt(100 * ENovaConstants::ResaleDepreciation), -1, FText::FromString("%"));
		CostTable->AddEntry(LOCTEXT("UpgradeCostTotal", "Total cost of changes"), GetPriceText(Cost.TotalCost));

		// Validate cost
		if (!PC->CanAffordTransaction(-Cost.TotalCost))
		{
			HasValidSpacecraft = false;
			DetailsText        = LOCTEXT("TooExpensive", "Changes to the current design are too expensive to afford");
		}

		// Reversal callback
		FSimpleDelegate ConfirmRevertChanges = FSimpleDelegate::CreateLambda(
			[=]()
			{
				SpacecraftPawn->ResetSpacecraft();
			});

		// Reversal confirmation callback
		FSimpleDelegate ConfirmChanges;
		FSimpleDelegate RevertChanges;
		if (HasModifiedDesign)
		{
			ConfirmChanges = FSimpleDelegate::CreateLambda(
				[=]()
				{
					PC->ProcessTransaction(-Cost.TotalCost);
					SpacecraftPawn->ApplyAssembly();
					PC->GetGameInstance<UNovaGameInstance>()->SaveGame(PC);
				});

			RevertChanges = FSimpleDelegate::CreateLambda(
				[=]()
				{
					GenericModalPanel->Show(LOCTEXT("RevertChanges", "Revert changes"),
						LOCTEXT("RevertChangesHelp", "Confirm the reversal of all changes to the spacecraft"), ConfirmRevertChanges);
				});
		}

		// Prepare Slate data
		const FNovaMainTheme&      Theme = FNovaStyleSet::GetMainTheme();
		TSharedPtr<SHorizontalBox> SpacecraftDiffBox;

		// clang-format off
		SAssignNew(InternalWidget, SHorizontalBox)

			+ SHorizontalBox::Slot()
		
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SVerticalBox)

				// Changes box
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SNovaInfoText)
					.Text(DetailsText)
					.Type(HasValidSpacecraft ? ENovaInfoBoxType::Positive : ENovaInfoBoxType::Negative)
				]

				// Diff box
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				[
					SAssignNew(SpacecraftDiffBox, SHorizontalBox)
				
					// Left panel
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SNovaSpacecraftDatasheet)
						.Title(LOCTEXT("CurrentSpacecraftTitle", "Current design"))
						.TargetSpacecraft(*CurrentSpacecraft)
					]
				]

				// Cost table
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				[
					CostTable
				]
			]

			+ SHorizontalBox::Slot();

		// Add the modification panel if relevant
		if (HasModifiedDesign)
		{
			SpacecraftDiffBox->AddSlot()
				.AutoWidth()
				[
					SNew(SNovaSpacecraftDatasheet)
					.Title(LOCTEXT("ModifiedSpacecraftTitle", "Modified design"))
					.TargetSpacecraft(ModifiedSpacecraft)
					.ComparisonSpacecraft(CurrentSpacecraft)
				];
		}

		// clang-format on

		// Show the modal dialog
		Show(LOCTEXT("ReviewChangesConfirm", "Review changes"), FText(), ConfirmChanges, RevertChanges);
	}

protected:
	virtual bool IsConfirmEnabled() const
	{
		return HasValidSpacecraft;
	}

	virtual void UpdateButtons() override
	{
		ConfirmButton->SetText(LOCTEXT("ConfirmSpacecraft", "Apply changes"));
		ConfirmButton->SetHelpText(LOCTEXT("ConfirmSpacecraftHelp", "Apply the changes to the spacecraft's design"));
		DismissButton->SetText(LOCTEXT("ResetSpacecraft", "Revert changes"));
		DismissButton->SetHelpText(LOCTEXT("ResetSpacecraftHelp", "Revert all changes to the spacecraft's design"));
		CancelButton->SetText(LOCTEXT("CancelSpacecraft", "Close"));
		CancelButton->SetHelpText(LOCTEXT("CancelSpacecraftHelp", "Close without doing anything"));
	}

protected:
	bool HasValidSpacecraft;
};

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuAssembly::SNovaMainMenuAssembly()
	: PC(nullptr)
	, SpacecraftPawn(nullptr)
	, FadeDuration(ENovaUIConstants::FadeDurationShort)
	, CurrentFadeTime(ENovaUIConstants::FadeDurationShort)
	, CompartmentPanelVisible(false)
	, SelectedCompartmentIndex(INDEX_NONE)
	, EditedCompartmentIndex(INDEX_NONE)
	, SelectedModuleOrEquipmentIndex(INDEX_NONE)
{}

void SNovaMainMenuAssembly::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme&  Theme                 = FNovaStyleSet::GetMainTheme();
	const FNovaButtonSize& CompartmentButtonSize = FNovaStyleSet::GetButtonSize("CompartmentButtonSize");
	MenuManager                                  = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot
	.VAlign(VAlign_Bottom)
	[
		SNew(SBackgroundBlur)
		.BlurRadius(this, &SNovaTabPanel::GetBlurRadius)
		.BlurStrength(this, &SNovaTabPanel::GetBlurStrength)
		.bApplyAlphaToBlur(true)
		.Padding(0)
		[
			SNew(SBorder)
			.BorderImage(&Theme.MainMenuBackground)
			.Padding(0)
			[
				SAssignNew(MenuBox, SVerticalBox)

				// Main compartment panel
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.ContentPadding)
				[
					SNew(SBorder)
					.BorderImage(new FSlateNoResource)
					.ColorAndOpacity(this, &SNovaMainMenuAssembly::GetMainColor)
					.Padding(0)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
		
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SNovaKeyLabel)
								.Key(this, &SNovaMainMenuAssembly::GetPreviousItemKey)
							]

							// Compartment list
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(Theme.VerticalContentPadding)
								[
									SNew(STextBlock)
									.TextStyle(&Theme.HeadingFont)
									.Text(LOCTEXT("CompartmentTitle", "Compartments"))
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(CompartmentBox, SHorizontalBox)
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								.VAlign(VAlign_Center)
								[
									SNew(SNovaRichText)
									.Text(FNovaTextGetter::CreateSP(this, &SNovaMainMenuAssembly::GetCompartmentText))
									.TextStyle(&Theme.InfoFont)
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SNovaKeyLabel)
								.Key(this, &SNovaMainMenuAssembly::GetNextItemKey)
							]
						]

						+ SHorizontalBox::Slot()
						
						// Assembly controls
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(Theme.VerticalContentPadding)
							[
								SNew(STextBlock)
								.TextStyle(&Theme.HeadingFont)
								.Text(LOCTEXT("AssemblyControls", "Assembly controls"))
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNovaAssignNew(SaveButton, SNovaButton)
								.Size("DoubleButtonSize")
								.Icon(FNovaStyleSet::GetBrush("Icon/SB_Review"))
								.Text(LOCTEXT("ReviewSpacecraft", "Review & save spacecraft"))
								.HelpText(LOCTEXT("ReviewSpacecraftHelp", "Review the spacecraft design and changes made to it before saving"))
								.OnClicked(this, &SNovaMainMenuAssembly::OnReviewSpacecraft)
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNovaNew(SNovaCompartmentList)
									.Panel(this)
									.Action(FNovaPlayerInput::MenuSecondary)
									.TitleText(LOCTEXT("BuildCompartmentForward", "Add compartment forward"))
									.HelpText(LOCTEXT("BuildCompartmentForwardelp", "Pick the blueprint for the new compartment"))
									.OnSelfRefresh(SNovaCompartmentList::FNovaOnSelfRefresh::CreateLambda([&]()
										{
											int32 NewIndex = GetNewBuildIndex(true);
											return SNovaCompartmentList::SelfRefreshType(0, SpacecraftPawn->GetCompatibleCompartments(NewIndex));
										}))
									.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateCompartmentItem)
									.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateCompartmentTooltip)
									.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnAddCompartment, true)
									.Enabled(this, &SNovaMainMenuAssembly::IsAddCompartmentEnabled, true)
									.ListButtonSize("LargeListButtonSize")
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNovaNew(SNovaCompartmentList)
									.Panel(this)
									.TitleText(LOCTEXT("BuildCompartmentAft", "Add compartment aft"))
									.HelpText(LOCTEXT("BuildCompartmentAftHelp", "Pick the blueprint for the new compartment"))
									.OnSelfRefresh(SNovaCompartmentList::FNovaOnSelfRefresh::CreateLambda([&]()
										{
											int32 NewIndex = GetNewBuildIndex(false);
											return SNovaCompartmentList::SelfRefreshType(0, SpacecraftPawn->GetCompatibleCompartments(NewIndex));
										}))
									.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateCompartmentItem)
									.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateCompartmentTooltip)
									.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnAddCompartment, false)
									.Enabled(this, &SNovaMainMenuAssembly::IsAddCompartmentEnabled, false)
									.ListButtonSize("LargeListButtonSize")
								]
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNovaNew(SNovaButton)
									.Action(FNovaPlayerInput::MenuPrimary)
									.Text(LOCTEXT("EditCompartment", "Edit compartment"))
									.HelpText(LOCTEXT("EditCompartmentHelp", "Add modules and equipments to the selected compartment"))
									.Enabled(this, &SNovaMainMenuAssembly::IsEditCompartmentEnabled)
									.OnClicked(this, &SNovaMainMenuAssembly::OnEditCompartment)
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNovaNew(SNovaButton)
									.Text(LOCTEXT("ScrapCompartment", "Scrap compartment"))
									.HelpText(LOCTEXT("ScrapCompartmentHelp", "Scrap the currently selected compartment"))
									.Enabled(this, &SNovaMainMenuAssembly::IsEditCompartmentEnabled)
									.OnClicked(this, &SNovaMainMenuAssembly::OnRemoveCompartment)
								]
							]
						]

						+ SHorizontalBox::Slot()
					]
				]
				
				// Compartment edition panel
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.ContentPadding)
				[
					SNew(SBorder)
					.BorderImage(new FSlateNoResource)
					.ColorAndOpacity(this, &SNovaMainMenuAssembly::GetCompartmentColor)
					.Padding(0)
					[
						SNew(SHorizontalBox)
						
						+ SHorizontalBox::Slot()

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SHorizontalBox)
								
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SNovaKeyLabel)
								.Key(this, &SNovaMainMenuAssembly::GetPreviousItemKey)
							]
				
							// Compartment selection
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SHorizontalBox)

									// Module list
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SVerticalBox)

										+ SVerticalBox::Slot()
										.AutoHeight()
										.Padding(Theme.VerticalContentPadding)
										[
											SNew(STextBlock)
											.TextStyle(&Theme.HeadingFont)
											.Text(LOCTEXT("ModulesTitle", "Modules"))
										]

										+ SVerticalBox::Slot()
										.AutoHeight()
										[
											SAssignNew(ModuleBox, SHorizontalBox)
										]
									]

									// Equipment list
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SVerticalBox)

										+ SVerticalBox::Slot()
										.AutoHeight()
										.Padding(Theme.VerticalContentPadding)
										[
											SNew(STextBlock)
											.TextStyle(&Theme.HeadingFont)
											.Text(LOCTEXT("EquipmentsTitle", "Equipments"))
										]

										+ SVerticalBox::Slot()
										.AutoHeight()
										[
											SAssignNew(EquipmentBox, SHorizontalBox)
										]
									]
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SNovaRichText)
									.Text(FNovaTextGetter::CreateSP(this, &SNovaMainMenuAssembly::GetModuleOrEquipmentText))
									.TextStyle(&Theme.InfoFont)
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SNovaKeyLabel)
								.Key(this, &SNovaMainMenuAssembly::GetNextItemKey)
							]
						]
						
						+ SHorizontalBox::Slot()

						// Compartment controls
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(Theme.VerticalContentPadding)
							[
								SNew(STextBlock)
								.TextStyle(&Theme.HeadingFont)
								.Text(LOCTEXT("CompartmentControls", "Compartment controls"))
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNovaNew(SNovaButton)
								.Action(FNovaPlayerInput::MenuCancel)
								.Text(LOCTEXT("CompartmentBack", "Back to assembly"))
								.HelpText(LOCTEXT("CompartmentBackHelp", "Go back to the main assembly"))
								.OnClicked(this, &SNovaMainMenuAssembly::OnBackToAssembly)
								.Enabled(this, &SNovaMainMenuAssembly::IsBackToAssemblyEnabled)
							]
							
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNovaAssignNew(ModuleListView, SNovaModuleList)
								.Panel(this)
								.Action(FNovaPlayerInput::MenuPrimary)
								.ItemsSource(&ModuleList)
								.ListButtonSize("LargeListButtonSize")
								.TitleText(LOCTEXT("ModuleListTitle", "Module"))
								.HelpText(LOCTEXT("ModuleListHelp", "Change the module for this slot"))
								.Enabled(this, &SNovaMainMenuAssembly::IsCompartmentPanelVisible)
								.Visibility(this, &SNovaMainMenuAssembly::GetModuleListVisibility)
								.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateModuleItem)
								.OnGenerateName(this, &SNovaMainMenuAssembly::GetModuleListTitle)
								.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateModuleTooltip)
								.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnSelectedModuleChanged)
							]
							
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNovaAssignNew(EquipmentListView, SNovaEquipmentList)
								.Panel(this)
								.Action(FNovaPlayerInput::MenuPrimary)
								.ItemsSource(&EquipmentList)
								.ListButtonSize("LargeListButtonSize")
								.TitleText(LOCTEXT("EquipmentListTitle", "Equipment"))
								.HelpText(LOCTEXT("EquipmentListHelp", "Change the equipment for this slot"))
								.Enabled(this, &SNovaMainMenuAssembly::IsCompartmentPanelVisible)
								.Visibility(this, &SNovaMainMenuAssembly::GetEquipmentListVisibility)
								.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateEquipmentItem)
								.OnGenerateName(this, &SNovaMainMenuAssembly::GetEquipmentListTitle)
								.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateEquipmentTooltip)
								.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnSelectedEquipmentChanged)
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNovaAssignNew(HullTypeListView, SNovaHullTypeList)
								.Panel(this)
								.Action(FNovaPlayerInput::MenuSecondary)
								.ItemsSource(&HullTypeList)
								.ListButtonSize("LargeListButtonSize")
								.TitleText(LOCTEXT("HullListTitle", "Hull type"))
								.HelpText(LOCTEXT("HullListHelp", "Change the hull type for this compartment"))
								.Enabled(this, &SNovaMainMenuAssembly::IsCompartmentPanelVisible)
								.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateHullTypeItem)
								.OnGenerateName(this, &SNovaMainMenuAssembly::GetHullTypeListTitle)
								.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateHullTypeTooltip)
								.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnSelectedHullTypeChanged)
							]
						]

						+ SHorizontalBox::Slot()
					]
				]

				// Display options panel
				+ SVerticalBox::Slot()
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuGenericBorder)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNovaNew(SNovaButton)
							.Action(FNovaPlayerInput::MenuAltPrimary)
							.Text(LOCTEXT("PhotoMode", "Toggle photo mode"))
							.HelpText(LOCTEXT("PhotoModeHelp", "Hide the interface to show off your spacecraft"))
							.OnClicked(this, &SNovaMainMenuAssembly::OnEnterPhotoMode, FNovaPlayerInput::MenuAltPrimary)
						]

						+ SHorizontalBox::Slot()

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SNovaButton) // No navigation
							.Focusable(false)
							.Content()
							[
								SNew(SBox)
								.Padding(FNovaStyleSet::GetButtonTheme().IconPadding)
								.VAlign(VAlign_Center)
								[
									SAssignNew(SpacecraftNameText, SEditableText)
									.HintText(LOCTEXT("SpacecraftHint", "Spacecraft name"))
									.Font(Theme.MainFont.Font)
									.ColorAndOpacity(Theme.MainFont.ColorAndOpacity)
									.OnTextChanged(this, &SNovaMainMenuAssembly::OnSpacecraftNameChanged)
								]
							]
						]

						+ SHorizontalBox::Slot()

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("DisplayFilter", "Display filter"))
							.TextStyle(&Theme.MainFont)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNovaAssignNew(DisplayFilter, SNovaSlider)
							.Action(FNovaPlayerInput::MenuAltSecondary)
							.Value(static_cast<int32>(ENovaAssemblyDisplayFilter::All))
							.MaxValue(static_cast<int32>(ENovaAssemblyDisplayFilter::All))
							.HelpText(LOCTEXT("DisplayFilterHelp", "Change which parts of the assembly to display"))
							.OnValueChanged(this, &SNovaMainMenuAssembly::OnSelectedFilterChanged)
						]

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(250)
							[
								SNew(SNovaText)
								.Text(FNovaTextGetter::CreateSP(this, &SNovaMainMenuAssembly::GetSelectedFilterText))
								.TextStyle(&Theme.MainFont)
								.WrapTextAt(250)
							]
						]

						+ SHorizontalBox::Slot()
					]
				]
			]
		]
	];

	// Build the modal panels
	GenericModalPanel = Menu->CreateModalPanel();
	AssemblyModalPanel = Menu->CreateModalPanel<SNovaAssemblyModalPanel>();

	// Build compartment buttons
	for (int32 Index = 0; Index < ENovaConstants::MaxCompartmentCount; Index++)
	{
		CompartmentBox->AddSlot()
		.AutoWidth()
		[
			SNew(SNovaButton) // No navigation
			.Focusable(false)
			.Size("CompartmentButtonSize")
			.HelpText(LOCTEXT("SelectCompartmentHelp", "Select this compartment for editing"))
			.Enabled(this, &SNovaMainMenuAssembly::IsSelectCompartmentEnabled, Index)
			.OnClicked(this, &SNovaMainMenuAssembly::OnCompartmentSelected, Index)
			.OnDoubleClicked(FSimpleDelegate::CreateLambda([=]()
			{
				OnCompartmentSelected(Index);
				OnEditCompartment();
			}))
			.UserSizeCallback(FNovaButtonUserSizeCallback::CreateLambda([=]
			{
				return CompartmentAnimation.GetAlpha(Index);
			}))
			.Content()
			[
				SNew(SOverlay)

				// Compartment picture
				+ SOverlay::Slot()
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFill)
					[
						SNew(SNovaImage)
						.Image(FNovaImageGetter::CreateLambda([=]() -> const FSlateBrush*
						{
							if (IsValid(SpacecraftPawn) && Index >= 0 && Index < SpacecraftPawn->GetCompartmentCount())
							{
								const FNovaCompartment& Compartment    = SpacecraftPawn->GetCompartment(Index);
								if (IsValid(Compartment.Description))
								{
									return &Compartment.Description->AssetRender;
								}
							}

							return nullptr;
						}))
					]
				]

				// Compartment index
				+ SOverlay::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuGenericBackground)
					.Padding(Theme.ContentPadding)
					[
						SNew(STextBlock)
						.Text(FText::AsNumber(Index + 1))
						.TextStyle(&Theme.MainFont)
					]
				]
			]
		];
	}

	// Get the currently edited compartment
	auto GetEditedCompartment = [this]() -> const FNovaCompartment*
	{
		if (EditedCompartmentIndex != INDEX_NONE && IsValid(SpacecraftPawn))
		{
			return &SpacecraftPawn->GetCompartment(EditedCompartmentIndex);
		}
		else
		{
			return nullptr;
		}
	};

	// Build module buttons
	for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
	{
		ModuleBox->AddSlot()
		.AutoWidth()
		[
			SNew(SNovaButton) // No navigation
			.Focusable(false)
			.Size("CompartmentButtonSize")
			.HelpText(FText::FormatNamed(LOCTEXT("ModuleSlotFormat", "Select module slot {slot}"), TEXT("slot"), FText::AsNumber(ModuleIndex + 1)))
			.Enabled(this, &SNovaMainMenuAssembly::IsModuleEnabled, ModuleIndex)
			.OnClicked(this, &SNovaMainMenuAssembly::SetSelectedModuleOrEquipment, GetCommonIndexFromModule(ModuleIndex))
			.OnDoubleClicked(FSimpleDelegate::CreateLambda([=]()
			{
				SetSelectedModuleOrEquipment(GetCommonIndexFromModule(ModuleIndex));
				ModuleListView->Show();
			}))
			.UserSizeCallback(FNovaButtonUserSizeCallback::CreateLambda([=]
			{
				return ModuleEquipmentAnimation.GetAlpha(GetCommonIndexFromModule(ModuleIndex));
			}))
			.Content()
			[
				SNew(SOverlay)

				// Module picture
				+ SOverlay::Slot()
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFill)
					[
						SNew(SNovaImage)
						.Image(FNovaImageGetter::CreateLambda([=]() -> const FSlateBrush*
						{
							const FNovaCompartment* Compartment = GetEditedCompartment();
							if (Compartment)
							{
								const FNovaCompartmentModule& Module = Compartment->Modules[ModuleIndex];
								if (IsValid(Module.Description))
								{
									return &Module.Description->AssetRender;
								}
							}

							return nullptr;
						}))
					]
				]

				// Module slot name
				+ SOverlay::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuGenericBackground)
					.Padding(Theme.ContentPadding)
					[
						SNew(SNovaText)
						.Text(FNovaTextGetter::CreateLambda([=]() -> FText
						{
							const FNovaCompartment* Compartment = GetEditedCompartment();
							if (Compartment && IsValid(Compartment->Description) && ModuleIndex < Compartment->Description->ModuleSlots.Num())
							{
								const FNovaModuleSlot& Slot = Compartment->Description->ModuleSlots[ModuleIndex];
								return Slot.DisplayName;
							}
						
							return FText();
						}))
						.TextStyle(&Theme.MainFont)
						.AutoWrapText(true)
					]
				]
			]
		];
	}

	// Build equipment buttons
	for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
	{
		EquipmentBox->AddSlot()
		.AutoWidth()
		[
			SNew(SNovaButton) // No navigation
			.Focusable(false)
			.Size("CompartmentButtonSize")
			.HelpText(FText::FormatNamed(LOCTEXT("EquipmentSlotFormat", "Select equipment slot {slot}"), TEXT("slot"), FText::AsNumber(EquipmentIndex + 1)))
			.Enabled(this, &SNovaMainMenuAssembly::IsEquipmentEnabled, EquipmentIndex)
			.OnClicked(this, &SNovaMainMenuAssembly::SetSelectedModuleOrEquipment, GetCommonIndexFromEquipment(EquipmentIndex))
			.OnDoubleClicked(FSimpleDelegate::CreateLambda([=]()
			{
				SetSelectedModuleOrEquipment(GetCommonIndexFromEquipment(EquipmentIndex));
				EquipmentListView->Show();
			}))
			.UserSizeCallback(FNovaButtonUserSizeCallback::CreateLambda([=]
			{
				return ModuleEquipmentAnimation.GetAlpha(GetCommonIndexFromEquipment(EquipmentIndex));
			}))
			.Content()
			[
				SNew(SOverlay)

				// Equipment picture
				+ SOverlay::Slot()
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFill)
					[
						SNew(SNovaImage)
						.Image(FNovaImageGetter::CreateLambda([=]() -> const FSlateBrush*
						{
							const FNovaCompartment* Compartment = GetEditedCompartment();
							if (Compartment)
							{
								const UNovaEquipmentDescription* Equipment = Compartment->Equipments[EquipmentIndex];
								if (IsValid(Equipment))
								{
									return &Equipment->AssetRender;
								}
							}

							return nullptr;
						}))
					]
				]

				// Equipment slot name and pairing information
				+ SOverlay::Slot()
				[
					SNew(SVerticalBox)

					// Name
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(&Theme.MainMenuGenericBackground)
						.Padding(Theme.ContentPadding)
						[
							SNew(SNovaText)
							.Text(FNovaTextGetter::CreateLambda([=]() -> FText
							{
								const FNovaCompartment* Compartment = GetEditedCompartment();
								if (Compartment && IsValid(Compartment->Description) && EquipmentIndex < Compartment->Description->EquipmentSlots.Num())
								{
									const FNovaEquipmentSlot& Slot = Compartment->Description->EquipmentSlots[EquipmentIndex];
									return Slot.DisplayName;
								}
							
								return FText();
							}))
							.TextStyle(&Theme.MainFont)
							.AutoWrapText(true)
						]
					]

					+ SVerticalBox::Slot()

					// Pairing
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(&Theme.MainMenuGenericBackground)
						.Padding(Theme.ContentPadding)
						.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([=]()
						{
							const FNovaCompartment* Compartment = GetEditedCompartment();
							if (Compartment && IsValid(Compartment->Description) && EquipmentIndex < Compartment->Description->EquipmentSlots.Num())
							{
								TArray<const FNovaEquipmentSlot*> PairedSlots = Compartment->Description->GetGroupedEquipmentSlots(EquipmentIndex);
								if (PairedSlots.Num())
								{
									return EVisibility::Visible;
								}
							}

							return EVisibility::Collapsed;
						})))
						[
							SNew(SNovaRichText)
							.Text(FNovaTextGetter::CreateLambda([=]() -> FText
							{
								const FNovaCompartment* Compartment = GetEditedCompartment();
								if (Compartment && IsValid(Compartment->Description) && EquipmentIndex < Compartment->Description->EquipmentSlots.Num())
								{
									TArray<const FNovaEquipmentSlot*> PairedSlots = Compartment->Description->GetGroupedEquipmentSlots(EquipmentIndex);
									if (PairedSlots.Num())
									{
										FString PairedList;
										for (const FNovaEquipmentSlot* PairedSlot : PairedSlots)
										{
											if (PairedList.Len())
											{
												PairedList += TEXT(", ");
											}
											PairedList += PairedSlot->DisplayName.ToString();
										}

										return FText::FormatNamed(LOCTEXT("PairingFormat", "<img src=\"/Text/Paired\"/> Paired with {slot}"), TEXT("slot"), FText::FromString(PairedList));
									}
								}
							
								return FText();
							}))
							.TextStyle(&Theme.MainFont)
							.WrapTextAt(1.9f * FNovaStyleSet::GetButtonSize("CompartmentButtonSize").Width)
							.AutoWrapText(false)
						]
					]
				]
			]
		];
	}

	// clang-format on

	// Default settings
	CompartmentAnimation.Initialize(FNovaStyleSet::GetButtonTheme().AnimationDuration, ENovaUIConstants::EaseLight);
	ModuleEquipmentAnimation.Initialize(FNovaStyleSet::GetButtonTheme().AnimationDuration, ENovaUIConstants::EaseLight);
	SetCompartmentPanelVisible(false);
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

int32 GetCompartmentIndexAtPosition(ANovaPlayerController* PC, ANovaSpacecraftPawn* SpacecraftPawn, FVector2D Position)
{
	FVector WorldLocation, WorldDirection;

	if (PC->DeprojectScreenPositionToWorld(Position.X, Position.Y, WorldLocation, WorldDirection))
	{
		TArray<FHitResult>    TraceHits;
		FVector               TraceEndLocation = WorldLocation + WorldDirection * 10000;
		FCollisionQueryParams TraceParams(FName("AssemblyTrace"));

		PC->GetWorld()->LineTraceMultiByChannel(TraceHits, WorldLocation, TraceEndLocation, ECC_Camera, TraceParams);

		for (const FHitResult& TraceHit : TraceHits)
		{
			if (TraceHit.bBlockingHit)
			{
				int32 HitIndex = SpacecraftPawn->GetCompartmentIndexByPrimitive(TraceHit.GetComponent());
				if (HitIndex != INDEX_NONE)
				{
					return HitIndex;
				}
			}
		}
	}

	return INDEX_NONE;
}

void SNovaMainMenuAssembly::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update fade time
	bool WantCompartmentPanelVisible = EditedCompartmentIndex != INDEX_NONE;
	if (WantCompartmentPanelVisible != CompartmentPanelVisible)
	{
		CurrentFadeTime -= DeltaTime;
	}
	else
	{
		CurrentFadeTime += DeltaTime;
	}
	CurrentFadeTime = FMath::Clamp(CurrentFadeTime, 0.0f, FadeDuration);

	// Update desired panel
	if (CurrentFadeTime <= 0)
	{
		SetCompartmentPanelVisible(WantCompartmentPanelVisible);
	}

	// Set the hovered compartment
	if (IsValid(SpacecraftPawn))
	{
		int32 HighlightedCompartment = INDEX_NONE;
		if (!CompartmentPanelVisible && !MenuManager->IsUsingGamepad() && !PC->IsInPhotoMode() && Menu->IsActiveNavigationPanel(this))
		{
			FVector2D MousePosition = Menu->GetTickSpaceGeometry().AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
			HighlightedCompartment  = GetCompartmentIndexAtPosition(PC, SpacecraftPawn, MousePosition);
		}
		SpacecraftPawn->SetHighlightedCompartment(HighlightedCompartment);
	}

	CompartmentAnimation.Update(SelectedCompartmentIndex, DeltaTime);
	ModuleEquipmentAnimation.Update(SelectedModuleOrEquipmentIndex, DeltaTime);
}

void SNovaMainMenuAssembly::Show()
{
	SNovaTabPanel::Show();

	if (IsValid(SpacecraftPawn))
	{
		SpacecraftNameText->SetText(SpacecraftPawn->GetSpacecraftCopy().GetName());

		// Reset the compartment view
		if (CompartmentPanelVisible)
		{
			EditedCompartmentIndex = INDEX_NONE;

			SpacecraftPawn->SetDisplayFilter(SpacecraftPawn->GetDisplayFilter(), INDEX_NONE);
			SpacecraftPawn->SetOutlinedCompartment(SelectedCompartmentIndex);
		}

		// Reset compartment data
		SetSelectedCompartment(SpacecraftPawn->GetCompartmentCount() > 0 ? 0 : INDEX_NONE);
		SetCompartmentPanelVisible(false);
	}
}

void SNovaMainMenuAssembly::Hide()
{
	SNovaTabPanel::Hide();

	// Reset the pawn
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->SetDisplayFilter(ENovaAssemblyDisplayFilter::All, INDEX_NONE);
		SpacecraftPawn->SetOutlinedCompartment(INDEX_NONE);
		SpacecraftPawn->SetHighlightedCompartment(INDEX_NONE);
	}

	DisplayFilter->SetCurrentValue(static_cast<float>(ENovaAssemblyDisplayFilter::All));
}

void SNovaMainMenuAssembly::UpdateGameObjects()
{
	PC             = MenuManager.IsValid() ? MenuManager->GetPC() : nullptr;
	SpacecraftPawn = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
}

void SNovaMainMenuAssembly::Next()
{
	if (!CompartmentPanelVisible)
	{
		if (SelectedCompartmentIndex != INDEX_NONE)
		{
			SetSelectedCompartment(FMath::Min(SelectedCompartmentIndex + 1, SpacecraftPawn->GetCompartmentCount() - 1));
		}
	}
	else
	{
		SetSelectedModuleOrEquipment(FMath::Min(SelectedModuleOrEquipmentIndex + 1, GetMaxCommonIndex()));
	}
}

void SNovaMainMenuAssembly::Previous()
{
	if (!CompartmentPanelVisible)
	{
		if (SelectedCompartmentIndex != INDEX_NONE)
		{
			SetSelectedCompartment(FMath::Max(SelectedCompartmentIndex - 1, 0));
		}
	}
	else
	{
		SetSelectedModuleOrEquipment(FMath::Max(SelectedModuleOrEquipmentIndex - 1, 0));
	}
}

void SNovaMainMenuAssembly::OnClicked(const FVector2D& Position)
{
	if (!CompartmentPanelVisible && Menu->IsActiveNavigationPanel(this))
	{
		int32 HitIndex = GetCompartmentIndexAtPosition(PC, SpacecraftPawn, Position);
		if (HitIndex != INDEX_NONE)
		{
			SetSelectedCompartment(HitIndex);
		}
	}
}

void SNovaMainMenuAssembly::OnDoubleClicked(const FVector2D& Position)
{
	if (!CompartmentPanelVisible && Menu->IsActiveNavigationPanel(this) && SelectedCompartmentIndex != INDEX_NONE)
	{
		int32 HitIndex = GetCompartmentIndexAtPosition(PC, SpacecraftPawn, Position);
		if (HitIndex == SelectedCompartmentIndex)
		{
			OnEditCompartment();
		}
	}
}

void SNovaMainMenuAssembly::HorizontalAnalogInput(float Value)
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->PanInput(Value);
	}
}

void SNovaMainMenuAssembly::VerticalAnalogInput(float Value)
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->TiltInput(Value);
	}
}

TSharedPtr<SNovaButton> SNovaMainMenuAssembly::GetDefaultFocusButton() const
{
	if (!CompartmentPanelVisible && SaveButton->IsButtonEnabled())
	{
		return SaveButton;
	}
	else
	{
		return nullptr;
	}
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

int32 SNovaMainMenuAssembly::GetNewBuildIndex(bool Forward) const
{
	int32 NewIndex = Forward ? SelectedCompartmentIndex : SelectedCompartmentIndex + 1;
	return FMath::Clamp(NewIndex, 0, ENovaConstants::MaxCompartmentCount - 1);
}

void SNovaMainMenuAssembly::SetSelectedCompartment(int32 Index)
{
	SelectedCompartmentIndex = Index;

	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->SetOutlinedCompartment(SelectedCompartmentIndex);
	}
}

void SNovaMainMenuAssembly::SetSelectedModuleOrEquipment(int32 Index)
{
	if (IsValid(SpacecraftPawn))
	{
		NCHECK(Index >= 0 && Index <= GetMaxCommonIndex());
		NCHECK(EditedCompartmentIndex >= 0 && EditedCompartmentIndex < SpacecraftPawn->GetCompartmentCount());
		const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(EditedCompartmentIndex);

		// Check that a common index maps to a valid module or equipment
		auto IsValidIndex = [&](int32 CommonIndex)
		{
			if (IsModuleIndex(CommonIndex))
			{
				if (GetModuleIndex(CommonIndex) >= 0 && GetModuleIndex(CommonIndex) < Compartment.Description->ModuleSlots.Num())
				{
					return true;
				}
			}
			else
			{
				if (GetEquipmentIndex(CommonIndex) >= 0 && GetEquipmentIndex(CommonIndex) < Compartment.Description->EquipmentSlots.Num())
				{
					return true;
				}
			}

			return false;
		};

		// Keep valid indices
		if (IsValidIndex(Index))
		{
			SelectedModuleOrEquipmentIndex = Index;
		}

		// Skip over invalid modules & equipments
		else if (SelectedModuleOrEquipmentIndex != INDEX_NONE)
		{
			int32 Step          = FMath::Sign(Index - SelectedModuleOrEquipmentIndex);
			int32 OriginalIndex = SelectedModuleOrEquipmentIndex;

			while (true)
			{
				SelectedModuleOrEquipmentIndex += Step;
				if (SelectedModuleOrEquipmentIndex < 0 || SelectedModuleOrEquipmentIndex >= GetMaxCommonIndex())
				{
					SelectedModuleOrEquipmentIndex = OriginalIndex;
					break;
				}
				else if (IsValidIndex(SelectedModuleOrEquipmentIndex))
				{
					break;
				}
			}
		}
		else
		{
			SelectedModuleOrEquipmentIndex = Index;
		}
		NCHECK(SelectedModuleOrEquipmentIndex >= 0 && SelectedModuleOrEquipmentIndex <= GetMaxCommonIndex());

		// Refresh modules list
		if (IsModuleSelected())
		{
			int32 ModuleIndex = GetSelectedModuleIndex();
			ModuleList        = SpacecraftPawn->GetCompatibleModules(EditedCompartmentIndex, ModuleIndex);
			ModuleListView->Refresh(ModuleList.Find(Compartment.Modules[ModuleIndex].Description));
		}

		// Refresh equipment list
		else
		{
			int32 EquipmentIndex = GetSelectedEquipmentIndex();
			EquipmentList        = SpacecraftPawn->GetCompatibleEquipments(EditedCompartmentIndex, EquipmentIndex);
			EquipmentListView->Refresh(EquipmentList.Find(Compartment.Equipments[EquipmentIndex]));
		}
	}
}

void SNovaMainMenuAssembly::SetCompartmentPanelVisible(bool Active)
{
	CompartmentPanelVisible = Active;

	// Refresh the hull type list
	if (CompartmentPanelVisible && IsValid(SpacecraftPawn))
	{
		const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(SelectedCompartmentIndex);

		HullTypeList = Compartment.Description->GetSupportedHullTypes();
		HullTypeListView->Refresh(HullTypeList.Find(Compartment.HullType));

		SetSelectedModuleOrEquipment(0);
	}
	else
	{
		SelectedModuleOrEquipmentIndex = INDEX_NONE;
	}

	// Change visibility
	MenuBox->GetChildren()->GetChildAt(0)->SetVisibility(CompartmentPanelVisible ? EVisibility::Collapsed : EVisibility::Visible);
	MenuBox->GetChildren()->GetChildAt(1)->SetVisibility(CompartmentPanelVisible ? EVisibility::Visible : EVisibility::Collapsed);

	// Update UI state
	ResetNavigation();
	SlatePrepass(FSlateApplicationBase::Get().GetApplicationScale());
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->ResetZoom();
	}
}

/*----------------------------------------------------
    Compartment template list
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateCompartmentItem(const UNovaCompartmentDescription* Description) const
{
	return SNew(SNovaTradableAssetItem).Asset(Description);
}

FText SNovaMainMenuAssembly::GenerateCompartmentTooltip(const UNovaCompartmentDescription* Description) const
{
	return FText::FormatNamed(LOCTEXT("CompartmentHelp", "Build a new {compartment}"), TEXT("compartment"), Description->Name);
}

/*----------------------------------------------------
    Compartment module list
----------------------------------------------------*/

EVisibility SNovaMainMenuAssembly::GetModuleListVisibility() const
{
	return (IsModuleSelected() && IsModuleEnabled(GetSelectedModuleIndex())) ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateModuleItem(const UNovaModuleDescription* Module) const
{
	return SNew(SNovaTradableAssetItem).Asset(Module);
}

FText SNovaMainMenuAssembly::GetModuleListTitle(const UNovaModuleDescription* Module) const
{
	return FText::FormatNamed(LOCTEXT("ModuleTitle", "Change module ({module})"), TEXT("module"), GetAssetName(Module));
}

FText SNovaMainMenuAssembly::GenerateModuleTooltip(const UNovaModuleDescription* Module) const
{
	if (Module)
	{
		return FText::FormatNamed(LOCTEXT("ModuleHelp", "Add {module} to this compartment"), TEXT("module"), Module->Name);
	}
	else
	{
		return LOCTEXT("RemoveModule", "Remove this module");
	}
}

/*----------------------------------------------------
    Compartment equipment list
----------------------------------------------------*/

EVisibility SNovaMainMenuAssembly::GetEquipmentListVisibility() const
{
	return (!IsModuleSelected() && IsEquipmentEnabled(GetSelectedEquipmentIndex())) ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateEquipmentItem(const UNovaEquipmentDescription* Equipment) const
{
	return SNew(SNovaTradableAssetItem).Asset(Equipment);
}

FText SNovaMainMenuAssembly::GetEquipmentListTitle(const UNovaEquipmentDescription* Equipment) const
{
	return FText::FormatNamed(LOCTEXT("EquipmentTitle", "Change equipment ({equipment})"), TEXT("equipment"), GetAssetName(Equipment));
}

FText SNovaMainMenuAssembly::GenerateEquipmentTooltip(const UNovaEquipmentDescription* Equipment) const
{
	if (Equipment)
	{
		return FText::FormatNamed(LOCTEXT("EquipmentHelp", "Add {equipment} to this compartment"), TEXT("equipment"), Equipment->Name);
	}
	else
	{
		return LOCTEXT("RemoveEquipment", "Remove this equipment");
	}
}

/*----------------------------------------------------
    Compartment hull type list
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateHullTypeItem(ENovaHullType Type) const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	return SNew(SOverlay).Clipping(EWidgetClipping::ClipToBoundsAlways)

		 + SOverlay::Slot()[SNew(SScaleBox)[
			   // TODO : image background for various hull types
			   SNew(SImage).Image(new FSlateNoResource)]]

		 + SOverlay::Slot().Padding(Theme.ContentPadding)[SNew(STextBlock).TextStyle(&Theme.MainFont).Text(GetHullTypeName(Type))];
}

FText SNovaMainMenuAssembly::GetHullTypeListTitle(ENovaHullType Type) const
{
	return FText::FormatNamed(LOCTEXT("HullTypeTitle", "Change hull type ({hull})"), TEXT("hull"), GetHullTypeName(Type));
}

FText SNovaMainMenuAssembly::GetHullTypeName(ENovaHullType Type) const
{
	switch (Type)
	{
		default:
		case ENovaHullType::None:
			return LOCTEXT("ENovaHullTypeNone", "No hull");
		case ENovaHullType::PlasticFabric:
			return LOCTEXT("ENovaHullTypePlasticFabric", "Plastic isolation");
		case ENovaHullType::MetalFabric:
			return LOCTEXT("ENovaHullTypeMetalFabric", "Metallic isolation");
	}
}

FText SNovaMainMenuAssembly::GenerateHullTypeTooltip(ENovaHullType Type) const
{
	if (Type != ENovaHullType::None)
	{
		return FText::FormatNamed(LOCTEXT("HullTypeHelp", "Use {hull} for this compartment"), TEXT("hull"), GetHullTypeName(Type));
	}
	else
	{
		return LOCTEXT("RemoveHull", "Don't use any hull for this compartment");
	}
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

FText SNovaMainMenuAssembly::GetAssetName(const UNovaTradableAssetDescription* Asset) const
{
	return Asset ? Asset->Name : LOCTEXT("Empty", "Empty");
}

FLinearColor SNovaMainMenuAssembly::GetMainColor() const
{
	float Alpha =
		CompartmentPanelVisible ? 0.0f : FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENovaUIConstants::EaseStandard);
	return FLinearColor(1, 1, 1, Alpha);
}

FLinearColor SNovaMainMenuAssembly::GetCompartmentColor() const
{
	float Alpha =
		CompartmentPanelVisible ? FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENovaUIConstants::EaseStandard) : 0.0f;
	return FLinearColor(1, 1, 1, Alpha);
}

FText SNovaMainMenuAssembly::GetSelectedFilterText() const
{
	if (IsValid(SpacecraftPawn))
	{
		switch (SpacecraftPawn->GetDisplayFilter())
		{
			case ENovaAssemblyDisplayFilter::ModulesOnly:
				return LOCTEXT("ModulesOnly", "Modules only");
				break;
			case ENovaAssemblyDisplayFilter::ModulesStructure:
				return LOCTEXT("ModulesStructure", "Modules & structure");
				break;
			case ENovaAssemblyDisplayFilter::ModulesStructureEquipments:
				return LOCTEXT("ModulesStructureEquipments", "Modules, structure, equipments");
				break;
			case ENovaAssemblyDisplayFilter::ModulesStructureEquipmentsWiring:
				return LOCTEXT("ModulesStructureEquipmentsWiring", "All internal systems");
				break;
			case ENovaAssemblyDisplayFilter::All:
			default:
				return LOCTEXT("FilterAll", "Full spacecraft");
				break;
		}
	}

	return FText();
}

bool SNovaMainMenuAssembly::IsSelectCompartmentEnabled(int32 Index) const
{
	NCHECK(Index >= 0 && Index < ENovaConstants::MaxCompartmentCount);
	return IsValid(SpacecraftPawn) && Index >= 0 && Index < SpacecraftPawn->GetCompartmentCount();
}

bool SNovaMainMenuAssembly::IsAddCompartmentEnabled(bool Forward) const
{
	if (!IsValid(SpacecraftPawn))
	{
		return false;
	}
	else if (SpacecraftPawn->GetCompartmentCount() >= ENovaConstants::MaxCompartmentCount)
	{
		return false;
	}
	else if (SpacecraftPawn->GetCompartmentCount() == 0)
	{
		return true;
	}
	else if (Forward)
	{
		return SelectedCompartmentIndex != INDEX_NONE;
	}
	else
	{
		return true;
	}
}

bool SNovaMainMenuAssembly::IsBackToAssemblyEnabled() const
{
	return EditedCompartmentIndex != INDEX_NONE;
}

bool SNovaMainMenuAssembly::IsEditCompartmentEnabled() const
{
	return SelectedCompartmentIndex >= 0;
}

FText SNovaMainMenuAssembly::GetCompartmentText()
{
	if (IsValid(SpacecraftPawn) && SelectedCompartmentIndex != INDEX_NONE)
	{
		return SpacecraftPawn->GetCompartmentMetrics(SelectedCompartmentIndex).GetInlineDescription();
	}

	return FText();
}

bool SNovaMainMenuAssembly::IsModuleEnabled(int32 ModuleIndex) const
{
	if (CompartmentPanelVisible && IsValid(SpacecraftPawn))
	{
		const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(SelectedCompartmentIndex);
		return ModuleIndex < Compartment.Description->ModuleSlots.Num();
	}

	return false;
}

bool SNovaMainMenuAssembly::IsEquipmentEnabled(int32 EquipmentIndex) const
{
	if (CompartmentPanelVisible && IsValid(SpacecraftPawn))
	{
		const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(SelectedCompartmentIndex);
		return EquipmentIndex < Compartment.Description->EquipmentSlots.Num();
	}

	return false;
}

FText SNovaMainMenuAssembly::GetModuleOrEquipmentText()
{
	if (IsValid(SpacecraftPawn) && EditedCompartmentIndex != INDEX_NONE)
	{
		const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(EditedCompartmentIndex);

		if (IsModuleSelected())
		{
			int32 ModuleIndex = GetSelectedModuleIndex();
			if (ModuleIndex < Compartment.Description->ModuleSlots.Num())
			{
				const UNovaModuleDescription* Module = Compartment.Modules[ModuleIndex].Description;
				return Module ? Module->GetInlineDescription() : LOCTEXT("EmptyModule", "Empty module slot");
			}
		}
		else
		{
			int32 EquipmentIndex = GetSelectedEquipmentIndex();
			if (EquipmentIndex < Compartment.Description->EquipmentSlots.Num())
			{
				const UNovaEquipmentDescription* Equipment = Compartment.Equipments[EquipmentIndex];
				return Equipment ? Equipment->GetInlineDescription() : LOCTEXT("EmptyEquipment", "Empty equipment slot");
			}
		}
	}

	return FText();
}

FKey SNovaMainMenuAssembly::GetPreviousItemKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuPrevious);
}

FKey SNovaMainMenuAssembly::GetNextItemKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuNext);
}

/*----------------------------------------------------
    Compartment callbacks
----------------------------------------------------*/

void SNovaMainMenuAssembly::OnEditCompartment()
{
	NCHECK(SelectedCompartmentIndex >= 0 && SelectedCompartmentIndex < ENovaConstants::MaxCompartmentCount);

	NLOG("SNovaMainMenuAssembly::OnEditCompartment : editing compartment at index %d", SelectedCompartmentIndex);

	EditedCompartmentIndex = SelectedCompartmentIndex;

	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->SetDisplayFilter(SpacecraftPawn->GetDisplayFilter(), EditedCompartmentIndex);
		SpacecraftPawn->SetOutlinedCompartment(INDEX_NONE);
	}
}

void SNovaMainMenuAssembly::OnRemoveCompartment()
{
	NCHECK(SelectedCompartmentIndex >= 0 && SelectedCompartmentIndex < ENovaConstants::MaxCompartmentCount);

	GenericModalPanel->Show(LOCTEXT("ScrapCompartmentConfirm", "Scrap compartment"),
		LOCTEXT("ScrapCompartmentConfirmHelp", "Confirm the scrapping of this compartment"),
		FSimpleDelegate::CreateSP(this, &SNovaMainMenuAssembly::OnRemoveCompartmentConfirmed));
}

void SNovaMainMenuAssembly::OnRemoveCompartmentConfirmed()
{
	NCHECK(SelectedCompartmentIndex >= 0 && SelectedCompartmentIndex < ENovaConstants::MaxCompartmentCount);

	NLOG("SNovaMainMenuAssembly::OnRemoveCompartment : removing compartment at index %d", SelectedCompartmentIndex);

	if (IsValid(SpacecraftPawn) && SpacecraftPawn->RemoveCompartment(SelectedCompartmentIndex))
	{
		SpacecraftPawn->RequestAssemblyUpdate();

		SelectedCompartmentIndex--;
		if (SpacecraftPawn->GetCompartmentCount() > 0 && SelectedCompartmentIndex == INDEX_NONE)
		{
			SelectedCompartmentIndex = 0;
		}

		SetSelectedCompartment(SelectedCompartmentIndex);
	}
}

void SNovaMainMenuAssembly::OnAddCompartment(const UNovaCompartmentDescription* Compartment, int32 Index, bool Forward)
{
	int32 NewIndex = GetNewBuildIndex(Forward);

	NLOG("SNovaMainMenuAssembly::OnAddCompartment : adding new compartment at index %d ('%s')", NewIndex, *Compartment->Name.ToString());

	if (IsValid(SpacecraftPawn) && SpacecraftPawn->InsertCompartment(FNovaCompartment(Compartment), NewIndex))
	{
		SpacecraftPawn->RequestAssemblyUpdate();
		SetSelectedCompartment(NewIndex);
	}
}

void SNovaMainMenuAssembly::OnCompartmentSelected(int32 Index)
{
	NCHECK(Index >= 0 && Index < ENovaConstants::MaxCompartmentCount);
	SetSelectedCompartment(Index);
}

/*----------------------------------------------------
    Display filters
----------------------------------------------------*/

void SNovaMainMenuAssembly::OnSpacecraftNameChanged(const FText& InText)
{
	constexpr int32 MaxSpacecraftNameLength = 25;

	FString SpacecraftName = InText.ToString().Left(MaxSpacecraftNameLength);
	if (InText.ToString().Len() > MaxSpacecraftNameLength)
	{
		SpacecraftNameText->SetText(FText::FromString(SpacecraftName));
	}

	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->RenameSpacecraft(SpacecraftName);
	}
}

void SNovaMainMenuAssembly::OnEnterPhotoMode(FName ActionName)
{
	if (IsValid(PC))
	{
		PC->EnterPhotoMode(ActionName);
	}
}

void SNovaMainMenuAssembly::OnSelectedFilterChanged(float Value)
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->SetDisplayFilter(static_cast<ENovaAssemblyDisplayFilter>(Value), EditedCompartmentIndex);
	}
}

/*----------------------------------------------------
    Modules & equipments
----------------------------------------------------*/

void SNovaMainMenuAssembly::OnSelectedModuleChanged(const UNovaModuleDescription* Module, int32 Index)
{
	int32 SlotIndex = GetSelectedModuleIndex();

	NLOG("SNovaMainMenuAssembly::OnSelectedModuleChanged : adding new module at index %d, slot %d ('%s')", EditedCompartmentIndex,
		SlotIndex, Module ? *Module->Name.ToString() : TEXT("nullptr"));

	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->GetCompartment(EditedCompartmentIndex).Modules[SlotIndex].Description = Module;
		SpacecraftPawn->RequestAssemblyUpdate();
	}
}

void SNovaMainMenuAssembly::OnSelectedEquipmentChanged(const UNovaEquipmentDescription* Equipment, int32 Index)
{
	int32 SlotIndex = GetSelectedEquipmentIndex();

	NLOG("SNovaMainMenuAssembly::OnSelectedEquipmentChanged : adding new equipment at index %d, slot %d ('%s')", EditedCompartmentIndex,
		SlotIndex, Equipment ? *Equipment->Name.ToString() : TEXT("nullptr"));

	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->GetCompartment(EditedCompartmentIndex).Equipments[SlotIndex] = Equipment;
		SpacecraftPawn->RequestAssemblyUpdate();
	}
}

void SNovaMainMenuAssembly::OnSelectedHullTypeChanged(ENovaHullType Type, int32 Index)
{
	NLOG("SNovaMainMenuAssembly::OnSelectedHullTypeChanged : setting new hull ('%d')", static_cast<int32>(Type));

	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->GetCompartment(EditedCompartmentIndex).HullType = Type;
		SpacecraftPawn->RequestAssemblyUpdate();
	}
}

/*----------------------------------------------------
    Save & exit
----------------------------------------------------*/

void SNovaMainMenuAssembly::OnReviewSpacecraft()
{
	if (IsValid(PC) && IsValid(SpacecraftPawn))
	{
		AssemblyModalPanel->ReviewAndSaveSpacecraft(PC, SpacecraftPawn, GenericModalPanel);
	}
}

void SNovaMainMenuAssembly::OnBackToAssembly()
{
	if (CompartmentPanelVisible)
	{
		EditedCompartmentIndex = INDEX_NONE;

		SpacecraftPawn->SetDisplayFilter(SpacecraftPawn->GetDisplayFilter(), INDEX_NONE);
		SpacecraftPawn->SetOutlinedCompartment(SelectedCompartmentIndex);
	}
}

#undef LOCTEXT_NAMESPACE
