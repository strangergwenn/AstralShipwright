// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuAssembly.h"
#include "NovaMainMenu.h"

#include "Player/NovaPlayerController.h"
#include "Game/NovaGameTypes.h"
#include "Game/NovaGameState.h"

#include "Spacecraft/NovaSpacecraftPawn.h"

#include "UI/Widgets/NovaSpacecraftDatasheet.h"
#include "UI/Widgets/NovaTradableAssetItem.h"

#include "Nova.h"

#include "Neutron/Actor/NeutronTurntablePawn.h"
#include "Neutron/System/NeutronGameInstance.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/UI/Widgets/NeutronFadingWidget.h"
#include "Neutron/UI/Widgets/NeutronInfoText.h"
#include "Neutron/UI/Widgets/NeutronModalPanel.h"
#include "Neutron/UI/Widgets/NeutronKeyLabel.h"
#include "Neutron/UI/Widgets/NeutronSlider.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"
#include "DrawDebugHelpers.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuAssembly"

/*----------------------------------------------------
    Diff widget
----------------------------------------------------*/

class SNovaAssemblyModalPanel : public SNeutronModalPanel
{
public:

	void ReviewAndSaveSpacecraft(
		ANovaPlayerController* PC, ANovaSpacecraftPawn* SpacecraftPawn, TSharedPtr<SNeutronModalPanel> GenericModalPanel)
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
		TSharedRef<SNeutronTable> CostTable = SNew(SNeutronTable).Title(LOCTEXT("CostAnalysis", "Cost analysis"));
		CostTable->AddEntry(LOCTEXT("UpgradeCostValue", "Estimated total value"), GetPriceText(Cost.TotalCost));
		CostTable->AddEntry(LOCTEXT("UpgradeCostUpgrades", "New parts bought"), GetPriceText(Cost.UpgradeCost));
		CostTable->AddEntry(LOCTEXT("UpgradeCostResale", "Parts resold"), GetPriceText(Cost.ResaleGain));
		CostTable->AddEntry(LOCTEXT("UpgradeCostPaint", "Paint"), GetPriceText(Cost.PaintCost));
		CostTable->AddEntry(LOCTEXT("UpgradeCostTotal", "Total cost of changes"), GetPriceText(Cost.TotalChangeCost));

		// Validate cost
		if (!PC->CanAffordTransaction(-Cost.TotalChangeCost))
		{
			HasValidSpacecraft = false;
			DetailsText        = LOCTEXT("TooExpensive", "Changes to the current design are too expensive to afford");
		}

		// Reversal callback
		FSimpleDelegate ConfirmRevertChanges = FSimpleDelegate::CreateLambda(
			[=]()
			{
				SpacecraftPawn->RevertModifications();
				SpacecraftPawn->SetEditing(true);
			});

		// Reversal confirmation callback
		FSimpleDelegate ConfirmChanges;
		FSimpleDelegate RevertChanges;
		if (HasModifiedDesign)
		{
			ConfirmChanges = FSimpleDelegate::CreateLambda(
				[=]()
				{
					PC->ProcessTransaction(-Cost.TotalChangeCost);
					SpacecraftPawn->ApplyAssembly();
					SpacecraftPawn->SetEditing(true);
					PC->SaveGame();
				});

			RevertChanges = FSimpleDelegate::CreateLambda(
				[=]()
				{
					GenericModalPanel->Show(LOCTEXT("RevertChanges", "Revert changes"),
						LOCTEXT("RevertChangesHelp", "Confirm the reversal of all changes to the spacecraft"), ConfirmRevertChanges);
				});
		}

		// Prepare Slate data
		const FNeutronMainTheme&   Theme = FNeutronStyleSet::GetMainTheme();
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
					SNew(SNeutronInfoText)
					.Text(DetailsText)
					.Type(HasValidSpacecraft ? ENeutronInfoBoxType::Positive : ENeutronInfoBoxType::Negative)
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
	, GameState(nullptr)
	, DesiredPanelState(ENovaMainMenuAssemblyState::Assembly)
	, CurrentPanelState(ENovaMainMenuAssemblyState::Assembly)
	, FadeDuration(ENeutronUIConstants::FadeDurationShort)
	, CurrentFadeTime(ENeutronUIConstants::FadeDurationShort)
	, SelectedCompartmentIndex(INDEX_NONE)
	, EditedCompartmentIndex(INDEX_NONE)
	, SelectedModuleOrEquipmentIndex(INDEX_NONE)
{}

void SNovaMainMenuAssembly::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme&  Theme                        = FNeutronStyleSet::GetMainTheme();
	const FNeutronButtonSize& CompartmentButtonSize        = FNeutronStyleSet::GetButtonSize("CompartmentButtonSize");
	MenuManager                                            = InArgs._MenuManager;
	const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	int32 PanelHeight = 3 * FNeutronStyleSet::GetButtonSize().Height + FontMeasureService->Measure("Z", Theme.MainFont.Font).Y;

	// Parent constructor
	SNeutronNavigationPanel::Construct(SNeutronNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot
	.VAlign(VAlign_Bottom)
	[
		SNew(SBackgroundBlur)
		.BlurRadius(this, &SNeutronTabPanel::GetBlurRadius)
		.BlurStrength(this, &SNeutronTabPanel::GetBlurStrength)
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
								SNew(SNeutronKeyLabel)
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
									SNew(SNeutronRichText)
									.Text(FNeutronTextGetter::CreateSP(this, &SNovaMainMenuAssembly::GetCompartmentText))
									.TextStyle(&Theme.InfoFont)
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SNeutronKeyLabel)
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
								SNeutronAssignNew(SaveButton, SNeutronButton)
								.Size("DoubleButtonSize")
								.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Review"))
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
									SNeutronAssignNew(CompartmentList, SNovaCompartmentList)
									.Panel(this)
									.Action(FNeutronPlayerInput::MenuSecondary)
									.TitleText(LOCTEXT("BuildCompartment", "Insert compartment"))
									.HelpText(LOCTEXT("BuildCompartmentHelp", "Insert a new compartment forward of the selected one"))
									.OnSelfRefresh(SNovaCompartmentList::FNeutronOnSelfRefresh::CreateLambda([this]()
										{
											IsCurrentCompartmentForward = IsNextCompartmentForward;
											IsNextCompartmentForward = true;

											NLOG("IsCurrentCompartmentForward %d", IsCurrentCompartmentForward);

											int32 NewIndex = GetNewBuildIndex(IsCurrentCompartmentForward);
											return SNovaCompartmentList::SelfRefreshType(0, SpacecraftPawn->GetCompatibleCompartments(NewIndex));
										}))
									.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateCompartmentItem)
									.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateCompartmentTooltip)
									.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnAddCompartment)
									.Enabled(this, &SNovaMainMenuAssembly::IsAddCompartmentEnabled)
									.ListButtonSize("LargeListButtonSize")
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNeutronAssignNew(EditButton, SNeutronButton)
									.Action(FNeutronPlayerInput::MenuPrimary)
									.Text(LOCTEXT("EditCompartment", "Edit compartment"))
									.HelpText(LOCTEXT("EditCompartmentHelp", "Add modules and equipment to the selected compartment"))
									.Enabled(this, &SNovaMainMenuAssembly::IsEditCompartmentEnabled)
									.OnClicked(this, &SNovaMainMenuAssembly::OnEditCompartment)
								]
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNeutronNew(SNeutronButton)
									.Text(LOCTEXT("ScrapCompartment", "Scrap compartment"))
									.HelpText(LOCTEXT("ScrapCompartmentHelp", "Scrap the currently selected compartment"))
									.Enabled(this, &SNovaMainMenuAssembly::IsEditCompartmentEnabled)
									.OnClicked(this, &SNovaMainMenuAssembly::OnRemoveCompartment)
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNeutronNew(SNeutronButton)
									.Text(LOCTEXT("CustomizeSpacecraft", "Customize spacecraft"))
									.HelpText(LOCTEXT("CustomizeSpacecraftHelp", "Customize the spacecraft's paint scheme"))
									.OnClicked(this, &SNovaMainMenuAssembly::OnOpenCustomization)
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
								SNew(SNeutronKeyLabel)
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
											.Text(LOCTEXT("EquipmentTitle", "Equipment"))
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
									SNew(SNeutronRichText)
									.Text(FNeutronTextGetter::CreateSP(this, &SNovaMainMenuAssembly::GetModuleOrEquipmentText))
									.TextStyle(&Theme.InfoFont)
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SNeutronKeyLabel)
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
								SNeutronAssignNew(BackButton, SNeutronButton)
								.Action(FNeutronPlayerInput::MenuCancel)
								.Text(LOCTEXT("CompartmentBack", "Back to assembly"))
								.HelpText(LOCTEXT("CompartmentBackHelp", "Go back to the main assembly"))
								.OnClicked(this, &SNovaMainMenuAssembly::OnBackToAssembly)
								.Enabled(this, &SNovaMainMenuAssembly::IsBackToAssemblyEnabled)
							]
							
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNeutronAssignNew(ModuleListView, SNovaModuleList)
								.Panel(this)
								.Action(FNeutronPlayerInput::MenuPrimary)
								.ItemsSource(&ModuleList)
								.ListButtonSize("LargeListButtonSize")
								.TitleText(LOCTEXT("ModuleListTitle", "Module"))
								.HelpText(this, &SNovaMainMenuAssembly::GetModuleListHelpText)
								.Enabled(this, &SNovaMainMenuAssembly::IsModuleListEnabled)
								.Visibility(this, &SNovaMainMenuAssembly::GetModuleListVisibility)
								.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateModuleItem)
								.OnGenerateName(this, &SNovaMainMenuAssembly::GetModuleListTitle)
								.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateModuleTooltip)
								.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnSelectedModuleChanged)
							]
							
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNeutronAssignNew(EquipmentListView, SNovaEquipmentList)
								.Panel(this)
								.Action(FNeutronPlayerInput::MenuPrimary)
								.ItemsSource(&EquipmentList)
								.ListButtonSize("LargeListButtonSize")
								.TitleText(LOCTEXT("EquipmentListTitle", "Equipment"))
								.HelpText(this, &SNovaMainMenuAssembly::GetEquipmentListHelpText)
								.Enabled(this, &SNovaMainMenuAssembly::IsEquipmentListEnabled)
								.Visibility(this, &SNovaMainMenuAssembly::GetEquipmentListVisibility)
								.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateEquipmentItem)
								.OnGenerateName(this, &SNovaMainMenuAssembly::GetEquipmentListTitle)
								.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateEquipmentTooltip)
								.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnSelectedEquipmentChanged)
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNeutronAssignNew(HullTypeListView, SNovaHullTypeList)
								.Panel(this)
								.Action(FNeutronPlayerInput::MenuSecondary)
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
				
				// Customization panel
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.ContentPadding)
				[
					SNew(SBorder)
					.BorderImage(new FSlateNoResource)
					.ColorAndOpacity(this, &SNovaMainMenuAssembly::GetCustomizationColor)
					.Padding(0)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
		
						// Paint pickers
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
								.Text(LOCTEXT("ColorsTitle", "Colors"))
							]
			
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								
								SNew(SBox)
								.HeightOverride(PanelHeight)
								[
									SNew(SVerticalBox)

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
										.Padding(Theme.ContentPadding)
										.VAlign(VAlign_Center)
										[
											SNew(STextBlock)
											.TextStyle(&Theme.MainFont)
											.Text(LOCTEXT("StructuralPaintTitle", "Structural paint"))
										]

										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNeutronAssignNew(StructuralPaintListView, SNovaPaintList)
											.Panel(this)
											.ItemsSource(&PaintList)
											.TitleText(LOCTEXT("StructuralPaint", "Structural paint"))
											.HelpText(LOCTEXT("StructuralPaintHelp", "Repaint the spacecraft's internal structures"))
											.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateStructuralPaintItem)
											.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GeneratePaintTooltip)
											.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnStructuralPaintSelected)
											.ButtonSize("DefaultButtonSize")
											.ButtonContent()
											[
												GeneratePaintListButton(ENovaMainMenuAssemblyPaintType::Structural)
											]
										]
									]
			
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
										.Padding(Theme.ContentPadding)
										.VAlign(VAlign_Center)
										[
											SNew(STextBlock)
											.TextStyle(&Theme.MainFont)
											.Text(LOCTEXT("HullPaintitle", "Hull paint"))
										]

										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNeutronAssignNew(HullPaintListView, SNovaPaintList)
											.Panel(this)
											.ItemsSource(&PaintList)
											.TitleText(LOCTEXT("HullPaint", "Hull paint"))
											.HelpText(LOCTEXT("HullPaintHelp", "Repaint the spacecraft's hull"))
											.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateHullPaintItem)
											.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GeneratePaintTooltip)
											.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnHullPaintSelected)
											.ButtonSize("DefaultButtonSize")
											.ButtonContent()
											[
												GeneratePaintListButton(ENovaMainMenuAssemblyPaintType::Hull)
											]
										]
									]
			
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
										.Padding(Theme.ContentPadding)
										.VAlign(VAlign_Center)
										[
											SNew(STextBlock)
											.TextStyle(&Theme.MainFont)
											.Text(LOCTEXT("DetailPaintTitle", "Accent paint"))
										]

										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNeutronAssignNew(DetailPaintListView, SNovaPaintList)
											.Panel(this)
											.ItemsSource(&PaintList)
											.TitleText(LOCTEXT("DetailPaint", "Accent paint"))
											.HelpText(LOCTEXT("DetailPaintHelp", "Repaint the spacecraft's wires, covers and decals"))
											.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateDetailPaintItem)
											.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GeneratePaintTooltip)
											.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnDetailPaintSelected)
											.ButtonSize("DefaultButtonSize")
											.ButtonContent()
											[
												GeneratePaintListButton(ENovaMainMenuAssemblyPaintType::Detail)
											]
										]
									]
								]
							]
						]

						+ SHorizontalBox::Slot()

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
								.Text(LOCTEXT("EmblemTitle", "Emblem"))
							]
							
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNeutronAssignNew(EmblemListView, SNovaEmblemList)
								.Panel(this)
								.ItemsSource(&EmblemList)
								.TitleText(LOCTEXT("Emblem", "Emblem"))
								.HelpText(LOCTEXT("EmblemHelp", "Change the ship's emblem"))
								.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateEmblemItem)
								.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateEmblemTooltip)
								.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnEmblemSelected)
								.ListButtonSize("LargeListButtonSize")
								.ButtonSize("InventoryButtonSize")
								.ButtonContent()
								[
									GenerateEmblemListButton()
								]
							]
						]

						+ SHorizontalBox::Slot()
						
						// Customization details
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
								.Text(LOCTEXT("SurfaceTitle", "Surface detail"))
							]

							// Isolating paint
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNeutronAssignNew(EnableHullPaintButton, SNeutronButton)
								.Action(FNeutronPlayerInput::MenuPrimary)
								.Text(LOCTEXT("EnableHullPaint", "Isolating paint"))
								.HelpText(LOCTEXT("EnableHullPaintHelp", "Paint the spacecraft with a smooth surface finish"))
								.OnClicked(this, &SNovaMainMenuAssembly::OnEnableHullPaintToggled)
								.Toggle(true)
							]

							// Wear & tear title
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.TextStyle(&Theme.MainFont)
								.Text(LOCTEXT("DirtyIntensityTitle", "Wear & tear"))
							]

							// Wear & tear
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNeutronAssignNew(DirtyIntensity, SNeutronSlider)
								.ValueStep(0.1f)
								.OnValueChanged(this, &SNovaMainMenuAssembly::OnDirtyIntensityChanged)
							]
						]
						
						+ SHorizontalBox::Slot()
						
						// Customization details
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
								.Text(LOCTEXT("CustomizationControls", "Customization controls"))
							]

							// Back
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNeutronAssignNew(BackButton2, SNeutronButton)
								.Action(FNeutronPlayerInput::MenuCancel)
								.Text(LOCTEXT("CompartmentBack", "Back to assembly"))
								.HelpText(LOCTEXT("CompartmentBackHelp", "Go back to the main assembly"))
								.OnClicked(this, &SNovaMainMenuAssembly::OnBackToAssembly)
								.Enabled(this, &SNovaMainMenuAssembly::IsBackToAssemblyEnabled)
							]

							// Name title
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.TextStyle(&Theme.MainFont)
								.Text(LOCTEXT("NameTitle", "Spacecraft name"))
							]

							// Name box
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SNeutronButton) // No navigation
								.Focusable(false)
								.Content()
								[
									SNew(SBox)
									.Padding(FNeutronStyleSet::GetButtonTheme().IconPadding)
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
							SNeutronAssignNew(PhotoModeButton, SNeutronButton)
							.Action(FNeutronPlayerInput::MenuAltPrimary)
							.Text(LOCTEXT("PhotoMode", "Toggle photo mode"))
							.HelpText(LOCTEXT("PhotoModeHelp", "Hide the interface to show off your spacecraft"))
							.OnClicked(this, &SNovaMainMenuAssembly::OnEnterPhotoMode, FNeutronPlayerInput::MenuAltPrimary)
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
						.Padding(FMargin(5, 0))
						[
							SNeutronAssignNew(DisplayFilter, SNeutronSlider)
							.Action(FNeutronPlayerInput::MenuAltSecondary)
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
								SNew(SNeutronText)
								.Text(FNeutronTextGetter::CreateSP(this, &SNovaMainMenuAssembly::GetSelectedFilterText))
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
			SNew(SNeutronButton) // No navigation
			.Focusable(false)
			.Size("CompartmentButtonSize")
			.HelpText(LOCTEXT("SelectCompartmentHelp", "Select this compartment for editing"))
			.Enabled(this, &SNovaMainMenuAssembly::IsSelectCompartmentEnabled, Index)
			.OnClicked(FSimpleDelegate::CreateLambda([=]()
			{
				if (Index < SpacecraftPawn->GetCompartmentCount())
				{
					SetSelectedCompartment(Index);
				}
				else
				{
					SetSelectedCompartment(SpacecraftPawn->GetCompartmentCount() > 0 ? Index - 1 : INDEX_NONE);
					IsNextCompartmentForward = false;
					CompartmentList->Show();
				}
			}))
			.OnDoubleClicked(FSimpleDelegate::CreateLambda([=]()
			{
				if (Index < SpacecraftPawn->GetCompartmentCount())
				{
					SetSelectedCompartment(Index);
					OnEditCompartment();
				}
			}))
			.UserSizeCallback(FNeutronButtonUserSizeCallback::CreateLambda([=]
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
						SNew(SNeutronImage)
						.Image(FNeutronImageGetter::CreateLambda([=]() -> const FSlateBrush*
						{
							if (IsValid(SpacecraftPawn) && Index >= 0 && Index < SpacecraftPawn->GetCompartmentCount())
							{
								const FNovaCompartment& Compartment    = SpacecraftPawn->GetCompartment(Index);
								if (IsValid(Compartment.Description))
								{
									return &Compartment.Description->AssetRender;
								}
							}
							
							return &UNeutronAssetManager::Get()->GetDefaultAsset<UNovaCompartmentDescription>()->AssetRender;
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
			SNew(SNeutronButton) // No navigation
			.Focusable(false)
			.Size("CompartmentButtonSize")
			.HelpText(this, &SNovaMainMenuAssembly::GetModuleHelpText, ModuleIndex)
			.Enabled(this, &SNovaMainMenuAssembly::IsModuleEnabled, ModuleIndex)
			.OnClicked(this, &SNovaMainMenuAssembly::SetSelectedModuleOrEquipment, GetCommonIndexFromModule(ModuleIndex))
			.OnDoubleClicked(FSimpleDelegate::CreateLambda([=]()
			{
				SetSelectedModuleOrEquipment(GetCommonIndexFromModule(ModuleIndex));
				ModuleListView->Show();
			}))
			.UserSizeCallback(FNeutronButtonUserSizeCallback::CreateLambda([=]
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
						SNew(SNeutronImage)
						.Image(FNeutronImageGetter::CreateLambda([=]() -> const FSlateBrush*
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

							return &UNeutronAssetManager::Get()->GetDefaultAsset<UNovaModuleDescription>()->AssetRender;
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
						SNew(SNeutronText)
						.Text(FNeutronTextGetter::CreateLambda([=]() -> FText
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
			SNew(SNeutronButton) // No navigation
			.Focusable(false)
			.Size("CompartmentButtonSize")
			.HelpText(this, &SNovaMainMenuAssembly::GetEquipmentHelpText, EquipmentIndex)
			.Enabled(this, &SNovaMainMenuAssembly::IsEquipmentEnabled, EquipmentIndex)
			.OnClicked(this, &SNovaMainMenuAssembly::SetSelectedModuleOrEquipment, GetCommonIndexFromEquipment(EquipmentIndex))
			.OnDoubleClicked(FSimpleDelegate::CreateLambda([=]()
			{
				SetSelectedModuleOrEquipment(GetCommonIndexFromEquipment(EquipmentIndex));
				EquipmentListView->Show();
			}))
			.UserSizeCallback(FNeutronButtonUserSizeCallback::CreateLambda([=]
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
						SNew(SNeutronImage)
						.Image(FNeutronImageGetter::CreateLambda([=]() -> const FSlateBrush*
						{
							const FNovaCompartment* Compartment = GetEditedCompartment();
							if (Compartment)
							{
								const UNovaEquipmentDescription* Equipment = Compartment->Equipment[EquipmentIndex];
								if (IsValid(Equipment))
								{
									return &Equipment->AssetRender;
								}
							}
							
							return &UNeutronAssetManager::Get()->GetDefaultAsset<UNovaEquipmentDescription>()->AssetRender;
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
							SNew(SNeutronText)
							.Text(FNeutronTextGetter::CreateLambda([=]() -> FText
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
							SNew(SNeutronRichText)
							.Text(FNeutronTextGetter::CreateLambda([=]() -> FText
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

										return LOCTEXT("Paired", "<img src=\"/Text/Paired\"/>");
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

	// clang-format on

	// Default settings
	CompartmentAnimation.Initialize(FNeutronStyleSet::GetButtonTheme().AnimationDuration);
	ModuleEquipmentAnimation.Initialize(FNeutronStyleSet::GetButtonTheme().AnimationDuration);
	SetPanelState(ENovaMainMenuAssemblyState::Assembly);
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
	SNeutronTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update fade time
	if (CurrentPanelState != DesiredPanelState)
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
		SetPanelState(DesiredPanelState);
	}

	// Set the hovered compartment
	if (IsValid(SpacecraftPawn))
	{
		SelectedCompartmentIndex = FMath::Min(SelectedCompartmentIndex, SpacecraftPawn->GetCompartmentCount() - 1);

		int32 HighlightedCompartment = INDEX_NONE;
		int32 OutlinedCompartment    = INDEX_NONE;

		if (CurrentPanelState == ENovaMainMenuAssemblyState::Assembly && !PC->IsInPhotoMode() && Menu->IsActiveNavigationPanel(this) &&
			SpacecraftPawn->GetCompartmentCount() > 1)
		{
			if (!MenuManager->IsUsingGamepad())
			{
				FVector2D MousePosition = Menu->GetTickSpaceGeometry().AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
				HighlightedCompartment  = GetCompartmentIndexAtPosition(PC, SpacecraftPawn, MousePosition);
			}
			OutlinedCompartment = SelectedCompartmentIndex;
		}

		SpacecraftPawn->SetHighlightedCompartment(HighlightedCompartment);
		SpacecraftPawn->SetOutlinedCompartment(OutlinedCompartment);
	}

	CompartmentAnimation.Update(SelectedCompartmentIndex, DeltaTime);
	ModuleEquipmentAnimation.Update(SelectedModuleOrEquipmentIndex, DeltaTime);
}

void SNovaMainMenuAssembly::Show()
{
	SNeutronTabPanel::Show();

	DesiredPanelState        = ENovaMainMenuAssemblyState::Assembly;
	IsNextCompartmentForward = true;

	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->SetEditing(true);
		SpacecraftNameText->SetText(SpacecraftPawn->GetSpacecraftCopy().GetName());

		// Reset the compartment view
		if (IsCompartmentPanelVisible())
		{
			EditedCompartmentIndex = INDEX_NONE;

			SpacecraftPawn->SetDisplayFilter(SpacecraftPawn->GetDisplayFilter(), INDEX_NONE);
			SpacecraftPawn->SetOutlinedCompartment(SelectedCompartmentIndex);
		}

		// Reset compartment data
		SetSelectedCompartment(SpacecraftPawn->GetCompartmentCount() > 0 ? 0 : INDEX_NONE);
		SetPanelState(ENovaMainMenuAssemblyState::Assembly);
	}
}

void SNovaMainMenuAssembly::Hide()
{
	SNeutronTabPanel::Hide();

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
	PC             = MenuManager.IsValid() ? MenuManager->GetPC<ANovaPlayerController>() : nullptr;
	SpacecraftPawn = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
	GameState      = IsValid(PC) ? MenuManager->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
}

void SNovaMainMenuAssembly::Next()
{
	if (CurrentPanelState == ENovaMainMenuAssemblyState::Assembly)
	{
		if (SelectedCompartmentIndex != INDEX_NONE)
		{
			SetSelectedCompartment(FMath::Min(SelectedCompartmentIndex + 1, SpacecraftPawn->GetCompartmentCount() - 1));
		}
	}
	else if (CurrentPanelState == ENovaMainMenuAssemblyState::Compartment)
	{
		SetSelectedModuleOrEquipment(FMath::Min(SelectedModuleOrEquipmentIndex + 1, GetMaxCommonIndex()));
	}
}

void SNovaMainMenuAssembly::Previous()
{
	if (CurrentPanelState == ENovaMainMenuAssemblyState::Assembly)
	{
		if (SelectedCompartmentIndex != INDEX_NONE)
		{
			SetSelectedCompartment(FMath::Max(SelectedCompartmentIndex - 1, 0));
		}
	}
	else if (CurrentPanelState == ENovaMainMenuAssemblyState::Compartment)
	{
		SetSelectedModuleOrEquipment(FMath::Max(SelectedModuleOrEquipmentIndex - 1, 0));
	}
}

void SNovaMainMenuAssembly::ZoomIn()
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->ZoomIn();
	}
}

void SNovaMainMenuAssembly::ZoomOut()
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->ZoomOut();
	}
}

void SNovaMainMenuAssembly::OnClicked(const FVector2D& Position)
{
	if (CurrentPanelState == ENovaMainMenuAssemblyState::Assembly && Menu->IsActiveNavigationPanel(this))
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
	if (CurrentPanelState == ENovaMainMenuAssemblyState::Assembly && Menu->IsActiveNavigationPanel(this) &&
		SelectedCompartmentIndex != INDEX_NONE)
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

TSharedPtr<SNeutronButton> SNovaMainMenuAssembly::GetDefaultFocusButton() const
{
	if (CurrentPanelState == ENovaMainMenuAssemblyState::Assembly && SaveButton->IsButtonEnabled())
	{
		return SaveButton;
	}
	else if (CurrentPanelState == ENovaMainMenuAssemblyState::Compartment && BackButton->IsButtonEnabled())
	{
		return BackButton;
	}
	else if (CurrentPanelState == ENovaMainMenuAssemblyState::Customization && EnableHullPaintButton->IsButtonEnabled())
	{
		return StructuralPaintListView;
	}
	else
	{
		return nullptr;
	}
}

bool SNovaMainMenuAssembly::IsButtonActionAllowed(TSharedPtr<SNeutronButton> Button) const
{
	TArray<TSharedPtr<SNeutronButton>> AllowedButtons;

	AllowedButtons.Add(PhotoModeButton);
	AllowedButtons.Add(DisplayFilter);

	if (CurrentPanelState == ENovaMainMenuAssemblyState::Assembly)
	{
		AllowedButtons.Add(CompartmentList);
		AllowedButtons.Add(EditButton);
	}
	else if (CurrentPanelState == ENovaMainMenuAssemblyState::Compartment)
	{
		AllowedButtons.Add(ModuleListView);
		AllowedButtons.Add(EquipmentListView);
		AllowedButtons.Add(HullTypeListView);
		AllowedButtons.Add(BackButton);
	}
	else if (CurrentPanelState == ENovaMainMenuAssemblyState::Customization)
	{
		AllowedButtons.Add(EnableHullPaintButton);
		AllowedButtons.Add(BackButton2);
	}

	return AllowedButtons.Contains(Button);
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
}

void SNovaMainMenuAssembly::SetSelectedModuleOrEquipment(int32 Index)
{
	if (IsValid(SpacecraftPawn))
	{
		NCHECK(Index >= 0 && Index <= GetMaxCommonIndex());
		NCHECK(EditedCompartmentIndex >= 0 && EditedCompartmentIndex < SpacecraftPawn->GetCompartmentCount());
		const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(EditedCompartmentIndex);

		// Keep valid indices
		if (IsValidCommonIndex(Index, Compartment.Description))
		{
			SelectedModuleOrEquipmentIndex = Index;
		}

		// Skip over invalid modules & equipment
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
				else if (IsValidCommonIndex(SelectedModuleOrEquipmentIndex, Compartment.Description))
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
			EquipmentList        = SpacecraftPawn->GetCompatibleEquipment(EditedCompartmentIndex, EquipmentIndex);
			EquipmentListView->Refresh(EquipmentList.Find(Compartment.Equipment[EquipmentIndex]));
		}
	}
}

void SNovaMainMenuAssembly::SetPanelState(ENovaMainMenuAssemblyState State)
{
	CurrentPanelState = State;

	// Compartment sub-menu
	if (State == ENovaMainMenuAssemblyState::Compartment && IsValid(SpacecraftPawn))
	{
		const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(SelectedCompartmentIndex);

		HullTypeList = Compartment.Description->GetSupportedHulls();
		HullTypeListView->Refresh(HullTypeList.Find(Compartment.HullType));

		SetSelectedModuleOrEquipment(0);
	}

	// Customization
	else if (State == ENovaMainMenuAssemblyState::Customization && IsValid(SpacecraftPawn))
	{
		const FNovaSpacecraftCustomization& Customization = SpacecraftPawn->GetCustomization();

		PaintList  = UNeutronAssetManager::Get()->GetSortedAssets<UNovaPaintDescription>();
		EmblemList = UNeutronAssetManager::Get()->GetSortedAssets<UNovaEmblemDescription>();

		StructuralPaintListView->Refresh(PaintList.Find(Customization.StructuralPaint));
		HullPaintListView->Refresh(PaintList.Find(Customization.HullPaint));
		DetailPaintListView->Refresh(PaintList.Find(Customization.DetailPaint));

		EmblemListView->Refresh(EmblemList.Find(Customization.Emblem));

		EnableHullPaintButton->SetActive(Customization.EnableHullPaint);
		DirtyIntensity->SetCurrentValue(Customization.DirtyIntensity);
	}

	// Assembly sub-menu
	else
	{
		SelectedModuleOrEquipmentIndex = INDEX_NONE;
	}

	// Change visibility
	MenuBox->GetChildren()->GetChildAt(0)->SetVisibility(
		State == ENovaMainMenuAssemblyState::Assembly ? EVisibility::Visible : EVisibility::Collapsed);
	MenuBox->GetChildren()->GetChildAt(1)->SetVisibility(
		State == ENovaMainMenuAssemblyState::Compartment ? EVisibility::Visible : EVisibility::Collapsed);
	MenuBox->GetChildren()->GetChildAt(2)->SetVisibility(
		State == ENovaMainMenuAssemblyState::Customization ? EVisibility::Visible : EVisibility::Collapsed);

	// Update UI state
	ResetNavigation();
	SlatePrepass(FSlateApplicationBase::Get().GetApplicationScale());
}

/*----------------------------------------------------
    Module / equipment index helpers
----------------------------------------------------*/

bool SNovaMainMenuAssembly::IsValidCommonIndex(int32 Index, const UNovaCompartmentDescription* CompartmentDescription) const
{
	if (IsModuleIndex(Index))
	{
		if (GetModuleIndex(Index) >= 0 && GetModuleIndex(Index) < CompartmentDescription->ModuleSlots.Num())
		{
			return true;
		}
	}
	else
	{
		if (GetEquipmentIndex(Index) >= 0 && GetEquipmentIndex(Index) < CompartmentDescription->EquipmentSlots.Num())
		{
			return true;
		}
	}

	return false;
}

/*----------------------------------------------------
    Compartment template list
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateCompartmentItem(const UNovaCompartmentDescription* Description) const
{
	return SNew(SNovaTradableAssetItem).Asset(Description).GameState(GameState).NoPriceHint(true);
}

FText SNovaMainMenuAssembly::GenerateCompartmentTooltip(const UNovaCompartmentDescription* Description) const
{
	return FText::FormatNamed(LOCTEXT("CompartmentHelp", "Build a new {compartment}"), TEXT("compartment"), Description->Name);
}

/*----------------------------------------------------
    Compartment module list
----------------------------------------------------*/

bool SNovaMainMenuAssembly::IsModuleListEnabled() const
{
	return IsCompartmentPanelVisible() && IsModuleEnabled(GetSelectedModuleIndex());
}

EVisibility SNovaMainMenuAssembly::GetModuleListVisibility() const
{
	return IsModuleSelected() ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SNovaMainMenuAssembly::GetModuleListHelpText() const
{
	FText Help;

	if ((IsCompartmentPanelVisible() && IsModuleEnabled(GetSelectedModuleIndex(), &Help)) || Help.IsEmpty())
	{
		return LOCTEXT("ModuleListHelp", "Change the module for this slot");
	}

	return Help;
}

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateModuleItem(const UNovaModuleDescription* Module) const
{
	return SNew(SNovaTradableAssetItem)
	    .Asset(Module)
	    .DefaultAsset(UNeutronAssetManager::Get()->GetDefaultAsset<UNovaModuleDescription>())
	    .GameState(GameState)
	    .NoPriceHint(true)
	    .SelectionIcon(TAttribute<const FSlateBrush*>::Create(TAttribute<const FSlateBrush*>::FGetter::CreateLambda(
			[=]()
			{
				return ModuleListView->GetSelectionIcon(Module);
			})));
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

bool SNovaMainMenuAssembly::IsEquipmentListEnabled() const
{
	return IsCompartmentPanelVisible() && IsEquipmentEnabled(GetSelectedEquipmentIndex());
}

EVisibility SNovaMainMenuAssembly::GetEquipmentListVisibility() const
{
	return IsModuleSelected() ? EVisibility::Collapsed : EVisibility::Visible;
}

FText SNovaMainMenuAssembly::GetEquipmentListHelpText() const
{
	FText Help;

	if ((IsCompartmentPanelVisible() && IsEquipmentEnabled(GetSelectedModuleIndex(), &Help)) || Help.IsEmpty())
	{
		return LOCTEXT("EquipmentListHelp", "Change the equipment for this slot");
	}

	return Help;
}

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateEquipmentItem(const UNovaEquipmentDescription* Equipment) const
{
	return SNew(SNovaTradableAssetItem)
	    .Asset(Equipment)
	    .DefaultAsset(UNeutronAssetManager::Get()->GetDefaultAsset<UNovaEquipmentDescription>())
	    .GameState(GameState)
	    .NoPriceHint(true)
	    .SelectionIcon(TAttribute<const FSlateBrush*>::Create(TAttribute<const FSlateBrush*>::FGetter::CreateLambda(
			[=]()
			{
				return EquipmentListView->GetSelectionIcon(Equipment);
			})));
}

FText SNovaMainMenuAssembly::GetEquipmentListTitle(const UNovaEquipmentDescription* Equipment) const
{
	return FText::FormatNamed(
		LOCTEXT("EquipmentTitleFormat", "Change equipment ({equipment})"), TEXT("equipment"), GetAssetName(Equipment));
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

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateHullTypeItem(const UNovaHullDescription* Hull) const
{
	return SNew(SNovaTradableAssetItem)
	    .Asset(Hull)
	    .DefaultAsset(UNeutronAssetManager::Get()->GetDefaultAsset<UNovaHullDescription>())
	    .GameState(GameState)
	    .NoPriceHint(true)
	    .SelectionIcon(TAttribute<const FSlateBrush*>::Create(TAttribute<const FSlateBrush*>::FGetter::CreateLambda(
			[=]()
			{
				return HullTypeListView->GetSelectionIcon(Hull);
			})));
}

FText SNovaMainMenuAssembly::GetHullTypeListTitle(const UNovaHullDescription* Hull) const
{
	return FText::FormatNamed(LOCTEXT("HullTypeTitle", "Change hull type ({hull})"), TEXT("hull"), GetAssetName(Hull));
}

FText SNovaMainMenuAssembly::GenerateHullTypeTooltip(const UNovaHullDescription* Hull) const
{
	if (IsValid(Hull))
	{
		return FText::FormatNamed(LOCTEXT("HullTypeHelp", "Use {hull} for this compartment"), TEXT("hull"), GetAssetName(Hull));
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
	float Alpha = CurrentPanelState == ENovaMainMenuAssemblyState::Assembly
	                ? FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENeutronUIConstants::EaseStandard)
	                : 0.0f;
	return FLinearColor(1, 1, 1, Alpha);
}

FLinearColor SNovaMainMenuAssembly::GetCompartmentColor() const
{
	float Alpha = CurrentPanelState == ENovaMainMenuAssemblyState::Compartment
	                ? FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENeutronUIConstants::EaseStandard)
	                : 0.0f;
	return FLinearColor(1, 1, 1, Alpha);
}

FLinearColor SNovaMainMenuAssembly::GetCustomizationColor() const
{
	float Alpha = CurrentPanelState == ENovaMainMenuAssemblyState::Customization
	                ? FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENeutronUIConstants::EaseStandard)
	                : 0.0f;
	return FLinearColor(1, 1, 1, Alpha);
}

FText SNovaMainMenuAssembly::GetSelectedFilterText() const
{
	if (IsValid(SpacecraftPawn))
	{
		switch (SpacecraftPawn->GetDisplayFilter())
		{
			case ENovaAssemblyDisplayFilter::StructuresOnly:
				return LOCTEXT("StructuresOnly", "Structure only");
				break;
			case ENovaAssemblyDisplayFilter::StructureModules:
				return LOCTEXT("ModulesStructure", "Structure & modules");
				break;
			case ENovaAssemblyDisplayFilter::StructureModulesEquipment:
				return LOCTEXT("ModulesStructureEquipments", "Structure, modules & equipment");
				break;
			case ENovaAssemblyDisplayFilter::StructureModulesEquipmentWiring:
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

	if (IsValid(SpacecraftPawn) && Index >= 0)
	{
		if (Index < SpacecraftPawn->GetCompartmentCount())
		{
			return true;
		}
		else if (Index == SpacecraftPawn->GetCompartmentCount())
		{
			return Index == 0 || !SpacecraftPawn->GetCompartment(Index - 1).HasAftEquipment();
		}
	}

	return false;
}

bool SNovaMainMenuAssembly::IsAddCompartmentEnabled() const
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
	else if (IsValid(SpacecraftPawn->GetCompartment(SelectedCompartmentIndex).Description) &&
			 SpacecraftPawn->GetCompartment(SelectedCompartmentIndex).Description->IsForwardCompartment)
	{
		return false;
	}
	else
	{
		return SelectedCompartmentIndex != INDEX_NONE;
	}
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

bool SNovaMainMenuAssembly::IsBackToAssemblyEnabled() const
{
	return CurrentPanelState != ENovaMainMenuAssemblyState::Assembly;
}

FText SNovaMainMenuAssembly::GetModuleHelpText(int32 ModuleIndex) const
{
	FText Help;

	if (IsModuleEnabled(ModuleIndex, &Help) || Help.IsEmpty())
	{
		return FText::FormatNamed(LOCTEXT("ModuleSlotFormat", "Select module slot {slot}"), TEXT("slot"), FText::AsNumber(ModuleIndex + 1));
	}
	else
	{
		return Help;
	}
}

bool SNovaMainMenuAssembly::IsModuleEnabled(int32 ModuleIndex, FText* Help) const
{
	if (IsCompartmentPanelVisible() && IsValid(SpacecraftPawn))
	{
		const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(SelectedCompartmentIndex);

		if (ModuleIndex >= Compartment.Description->ModuleSlots.Num())
		{
			return false;
		}
		else if (IsValid(Compartment.Modules[ModuleIndex].Description) && Compartment.GetCurrentCargoMass() > 0 &&
				 Compartment.Modules[ModuleIndex].Description->IsA<UNovaCargoModuleDescription>())
		{
			if (Help)
			{
				*Help = LOCTEXT("ModuleHasCargo", "This module cannot be changed because it holds cargo");
			}
			return false;
		}
		else if (SpacecraftPawn->GetCompatibleModules(SelectedCompartmentIndex, ModuleIndex).Num() == 1)
		{
			if (Help)
			{
				*Help = LOCTEXT("ModuleHasNoChoice", "There is no available module for this slot");
			}
			return false;
		}

		return true;
	}

	return false;
}

FText SNovaMainMenuAssembly::GetEquipmentHelpText(int32 EquipmentIndex) const
{
	FText Help;

	if (IsEquipmentEnabled(EquipmentIndex, &Help) || Help.IsEmpty())
	{
		return FText::FormatNamed(
			LOCTEXT("EquipmentSlotFormat", "Select equipment slot {slot}"), TEXT("slot"), FText::AsNumber(EquipmentIndex + 1));
	}
	else
	{
		return Help;
	}
}

bool SNovaMainMenuAssembly::IsEquipmentEnabled(int32 EquipmentIndex, FText* Help) const
{
	if (IsCompartmentPanelVisible() && IsValid(SpacecraftPawn))
	{
		const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(SelectedCompartmentIndex);

		if (EquipmentIndex >= Compartment.Description->EquipmentSlots.Num())
		{
			return false;
		}
		else if (SpacecraftPawn->GetCompatibleEquipment(SelectedCompartmentIndex, EquipmentIndex).Num() == 1)
		{
			if (Help)
			{
				*Help = LOCTEXT("EquipmentHasNoChoice", "There is no available equipment for this slot");
			}
			return false;
		}

		return true;
	}

	return false;
}

FText SNovaMainMenuAssembly::GetModuleOrEquipmentText()
{
	if (IsValid(SpacecraftPawn) && EditedCompartmentIndex != INDEX_NONE)
	{
		const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(EditedCompartmentIndex);

		// A module is selected : get its description
		if (IsModuleSelected())
		{
			int32 ModuleIndex = GetSelectedModuleIndex();
			if (ModuleIndex < Compartment.Description->ModuleSlots.Num())
			{
				const UNovaModuleDescription* Module = Compartment.Modules[ModuleIndex].Description;
				return Module ? Module->GetInlineDescription() : LOCTEXT("EmptyModule", "<img src=\"/Text/Module\"/> Empty module slot");
			}
		}

		// An equipment is selected : get its description
		else
		{
			int32 EquipmentIndex = GetSelectedEquipmentIndex();
			if (EquipmentIndex < Compartment.Description->EquipmentSlots.Num())
			{
				const UNovaEquipmentDescription* Equipment     = Compartment.Equipment[EquipmentIndex];
				FText                            EquipmentText = Equipment ? Equipment->GetInlineDescription()
				                                                           : LOCTEXT("EmptyEquipment", "<img src=\"/Text/Equipment\"/> Empty equipment slot");

				// Append the pairing information
				TArray<const FNovaEquipmentSlot*> PairedSlots = Compartment.Description->GetGroupedEquipmentSlots(EquipmentIndex);
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

					EquipmentText =
						FText::FromString(EquipmentText.ToString() + " " +
										  FText::FormatNamed(LOCTEXT("PairingFormat", "<img src=\"/Text/Paired\"/> Paired with {slot}"),
											  TEXT("slot"), FText::FromString(PairedList))
											  .ToString());
				}

				return EquipmentText;
			}
		}
	}

	return FText();
}

FKey SNovaMainMenuAssembly::GetPreviousItemKey() const
{
	return MenuManager->GetFirstActionKey(FNeutronPlayerInput::MenuPrevious);
}

FKey SNovaMainMenuAssembly::GetNextItemKey() const
{
	return MenuManager->GetFirstActionKey(FNeutronPlayerInput::MenuNext);
}

/*----------------------------------------------------
    Paint lists callbacks
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuAssembly::GeneratePaintListButton(ENovaMainMenuAssemblyPaintType Type) const
{
	const FNeutronMainTheme&   Theme       = FNeutronStyleSet::GetMainTheme();
	const FNeutronButtonTheme& ButtonTheme = FNeutronStyleSet::GetButtonTheme();

	// Get the current paint style
	auto GetSelectedPaintItem = [this, Type]() -> const UNovaPaintDescription*
	{
		TSharedPtr<SNovaMainMenuAssembly::SNovaPaintList> List;
		switch (Type)
		{
			case ENovaMainMenuAssemblyPaintType::Structural:
				List = StructuralPaintListView;
				break;
			case ENovaMainMenuAssemblyPaintType::Hull:
				List = HullPaintListView;
				break;
			case ENovaMainMenuAssemblyPaintType::Detail:
				List = DetailPaintListView;
				break;
		}

		NCHECK(List.IsValid());
		const UNovaPaintDescription* Paint = List->GetSelectedItem();
		NCHECK(Paint);
		return Paint;
	};

	// clang-format off
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.Padding(ButtonTheme.IconPadding)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.MainFont)
			.Text_Lambda([=]()
			{
				return GetSelectedPaintItem()->Name;
			})
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SColorBlock)
			.Size(FVector2D(64, 16))
			.Color_Lambda([=]()
			{
				return GetSelectedPaintItem()->PaintColor;
			})
		];

	// clang-format on
}

TSharedRef<SWidget> GeneratePaintItem(const UNovaPaintDescription* Paint, const TSharedPtr<SNovaMainMenuAssembly::SNovaPaintList>& List)
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	// clang-format off
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image_Lambda([=]()
			{
				return List->GetSelectionIcon(Paint);
			})
		]

		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.MainFont)
			.Text(Paint->Name)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SColorBlock)
			.Color(Paint->PaintColor)
			.Size(FVector2D(64, 16))
		];

	// clang-format on
}

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateStructuralPaintItem(const UNovaPaintDescription* Paint) const
{
	return GeneratePaintItem(Paint, StructuralPaintListView);
}

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateHullPaintItem(const UNovaPaintDescription* Paint) const
{
	return GeneratePaintItem(Paint, HullPaintListView);
}

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateDetailPaintItem(const UNovaPaintDescription* Paint) const
{
	return GeneratePaintItem(Paint, DetailPaintListView);
}

FText SNovaMainMenuAssembly::GeneratePaintTooltip(const UNovaPaintDescription* Paint) const
{
	return Paint->Name;
}

/*----------------------------------------------------
    Emblem callbacks
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateEmblemListButton() const
{
	const FNeutronMainTheme&   Theme       = FNeutronStyleSet::GetMainTheme();
	const FNeutronButtonTheme& ButtonTheme = FNeutronStyleSet::GetButtonTheme();

	// clang-format off
	return SNew(SScaleBox)
		.Stretch(EStretch::ScaleToFill)
		[
			SNew(SImage)
			.Image_Lambda([=]()
			{
				return &EmblemListView->GetSelectedItem()->Brush;
			})
		];
	// clang-format on
}

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateEmblemItem(const UNovaEmblemDescription* Emblem) const
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	// clang-format off
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image_Lambda([=]()
			{
				return EmblemListView->GetSelectionIcon(Emblem);
			})
		]

		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.MainFont)
			.Text(Emblem->Name)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFill)
			[
				SNew(SImage)
				.Image(&Emblem->Brush)
			]
		];

	// clang-format on
}

FText SNovaMainMenuAssembly::GenerateEmblemTooltip(const UNovaEmblemDescription* Emblem) const
{
	return Emblem->Name;
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
		// Update spacecraft filtering
		SpacecraftPawn->SetDisplayFilter(SpacecraftPawn->GetDisplayFilter(), EditedCompartmentIndex);
		SpacecraftPawn->SetOutlinedCompartment(INDEX_NONE);

		// Find the first module or equipment that we can equip
		for (int32 CommonIndex = 0; CommonIndex <= GetMaxCommonIndex(); CommonIndex++)
		{
			if (IsValidCommonIndex(CommonIndex, SpacecraftPawn->GetCompartment(SelectedCompartmentIndex).Description))
			{
				SelectedModuleOrEquipmentIndex = CommonIndex;
				break;
			}
		}
	}

	DesiredPanelState = ENovaMainMenuAssemblyState::Compartment;
}

void SNovaMainMenuAssembly::OnRemoveCompartment()
{
	NCHECK(SelectedCompartmentIndex >= 0 && SelectedCompartmentIndex < ENovaConstants::MaxCompartmentCount);

	GenericModalPanel->Show(LOCTEXT("ScrapCompartmentConfirm", "Confirm the scrapping of this compartment"),
		LOCTEXT("ScrapCompartmentConfirmHelp", "All resources in the compartment's cargo hold will be lost"),
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

		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
		FSlateApplication::Get().PlaySound(Theme.DeleteSound);
	}
}

void SNovaMainMenuAssembly::OnAddCompartment(const UNovaCompartmentDescription* Compartment, int32 Index)
{
	int32 NewIndex = GetNewBuildIndex(IsCurrentCompartmentForward);

	NLOG("SNovaMainMenuAssembly::OnAddCompartment : adding new compartment at index %d ('%s')", NewIndex, *Compartment->Name.ToString());

	if (IsValid(SpacecraftPawn) && SpacecraftPawn->InsertCompartment(FNovaCompartment(Compartment), NewIndex))
	{
		SpacecraftPawn->RequestAssemblyUpdate();
		SetSelectedCompartment(NewIndex);
	}
}

/*----------------------------------------------------
    Display filters
----------------------------------------------------*/

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
    Modules & equipment
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
		SpacecraftPawn->GetCompartment(EditedCompartmentIndex).Equipment[SlotIndex] = Equipment;
		SpacecraftPawn->RequestAssemblyUpdate();
	}
}

void SNovaMainMenuAssembly::OnSelectedHullTypeChanged(const UNovaHullDescription* Hull, int32 Index)
{
	NLOG("SNovaMainMenuAssembly::OnSelectedHullTypeChanged : setting new hull ('%d')", *GetAssetName(Hull).ToString());

	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->GetCompartment(EditedCompartmentIndex).HullType = Hull;
		SpacecraftPawn->RequestAssemblyUpdate();
	}
}

/*----------------------------------------------------
    Customization
----------------------------------------------------*/

void SNovaMainMenuAssembly::OnOpenCustomization()
{
	NLOG("SNovaMainMenuAssembly::OnOpenCustomization");

	SpacecraftPawn->SetOutlinedCompartment(INDEX_NONE);

	DesiredPanelState = ENovaMainMenuAssemblyState::Customization;
}

void SNovaMainMenuAssembly::OnStructuralPaintSelected(const UNovaPaintDescription* Paint, int32 Index)
{
	NLOG("SNovaMainMenuAssembly::OnStructuralPaintSelected : %d", Index);

	FNovaSpacecraftCustomization Customization = SpacecraftPawn->GetCustomization();
	Customization.StructuralPaint              = Paint;
	SpacecraftPawn->UpdateCustomization(Customization);
}

void SNovaMainMenuAssembly::OnHullPaintSelected(const UNovaPaintDescription* Paint, int32 Index)
{
	NLOG("SNovaMainMenuAssembly::OnHullPaintSelected : %d", Index);

	FNovaSpacecraftCustomization Customization = SpacecraftPawn->GetCustomization();
	Customization.HullPaint                    = Paint;
	SpacecraftPawn->UpdateCustomization(Customization);
}

void SNovaMainMenuAssembly::OnDetailPaintSelected(const UNovaPaintDescription* Paint, int32 Index)
{
	NLOG("SNovaMainMenuAssembly::OnDetailPaintSelected : %d", Index);

	FNovaSpacecraftCustomization Customization = SpacecraftPawn->GetCustomization();
	Customization.DetailPaint                  = Paint;
	SpacecraftPawn->UpdateCustomization(Customization);
}

void SNovaMainMenuAssembly::OnEmblemSelected(const UNovaEmblemDescription* Emblem, int32 Index)
{
	NLOG("SNovaMainMenuAssembly::OnEmblemSelected : %d", Index);

	FNovaSpacecraftCustomization Customization = SpacecraftPawn->GetCustomization();
	Customization.Emblem                       = Emblem;
	SpacecraftPawn->UpdateCustomization(Customization);
}

void SNovaMainMenuAssembly::OnEnableHullPaintToggled()
{
	NLOG("SNovaMainMenuAssembly::OnEnableHullPaintToggled");

	FNovaSpacecraftCustomization Customization = SpacecraftPawn->GetCustomization();
	Customization.EnableHullPaint              = EnableHullPaintButton->IsActive();
	SpacecraftPawn->UpdateCustomization(Customization);
}

void SNovaMainMenuAssembly::OnDirtyIntensityChanged(float Value)
{
	FNovaSpacecraftCustomization Customization = SpacecraftPawn->GetCustomization();
	Customization.DirtyIntensity               = Value;
	SpacecraftPawn->UpdateCustomization(Customization);
}

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

void SNovaMainMenuAssembly::OnConfirmCustomization()
{
	NLOG("SNovaMainMenuAssembly::OnConfirmCustomization");

	FNovaSpacecraftCustomization Customization = SpacecraftPawn->GetCustomization();

	Customization.DirtyIntensity  = DirtyIntensity->GetCurrentValue();
	Customization.StructuralPaint = StructuralPaintListView->GetSelectedItem();
	Customization.HullPaint       = HullPaintListView->GetSelectedItem();
	Customization.DetailPaint     = DetailPaintListView->GetSelectedItem();
	Customization.EnableHullPaint = EnableHullPaintButton->IsActive();

	SpacecraftPawn->UpdateCustomization(Customization);

	OnBackToAssembly();
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
	if (CurrentPanelState != ENovaMainMenuAssemblyState::Assembly)
	{
		EditedCompartmentIndex = INDEX_NONE;
		DesiredPanelState      = ENovaMainMenuAssemblyState::Assembly;

		SpacecraftPawn->SetDisplayFilter(SpacecraftPawn->GetDisplayFilter(), INDEX_NONE);
		SpacecraftPawn->SetOutlinedCompartment(SelectedCompartmentIndex);
	}
}

#undef LOCTEXT_NAMESPACE
