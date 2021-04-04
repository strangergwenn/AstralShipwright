// Nova project - Gwennaël Arbona

#include "NovaMainMenuAssembly.h"
#include "NovaMainMenu.h"

#include "Nova/Actor/NovaTurntablePawn.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Game/NovaGameTypes.h"

#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaMenuManager.h"

#include "Nova/UI/Widget/NovaSlider.h"
#include "Nova/UI/Widget/NovaModalPanel.h"
#include "Nova/UI/Widget/NovaKeyLabel.h"

#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"
#include "DrawDebugHelpers.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuAssembly"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuAssembly::SNovaMainMenuAssembly()
	: FadeDuration(ENovaUIConstants::FadeDurationShort)
	, CurrentFadeTime(ENovaUIConstants::FadeDurationShort)
	, IsCompartmentPanelVisible(false)
	, SelectedCompartmentIndex(INDEX_NONE)
	, EditedCompartmentIndex(INDEX_NONE)
{}

void SNovaMainMenuAssembly::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

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
			.Padding(Theme.ContentPadding)
			[
				SNew(SHorizontalBox)

				// Display options
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.TextStyle(&Theme.SubtitleFont)
						.Text(LOCTEXT("CurrentDisplayFilter", "Display options"))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.TextStyle(&Theme.MainFont)
						.Text(LOCTEXT("DisplayFilter", "Hull display filter"))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SNovaSlider)
						.Panel(this)
						.Value(static_cast<int32>(ENovaAssemblyDisplayFilter::All))
						.MaxValue(static_cast<int32>(ENovaAssemblyDisplayFilter::All))
						.HelpText(LOCTEXT("DisplayFilterHelp", "Change which parts of the assembly to display"))
						.OnValueChanged(this, &SNovaMainMenuAssembly::OnSelectedFilterChanged)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.TextStyle(&Theme.MainFont)
						.Text(LOCTEXT("HighlightTitle", "Compartment highlighting"))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaAssignNew(HighlightButton, SNovaButton)
						.Action(FNovaPlayerInput::MenuSecondary)
						.Text(LOCTEXT("Highlight", "Highlight selection"))
						.HelpText(LOCTEXT("HighlightHelp", "Toggle the highlighting of the current selection"))
						.OnClicked(this, &SNovaMainMenuAssembly::OnToggleHighlight)
						.Enabled(this, &SNovaMainMenuAssembly::IsToggleHighlightEnabled)
						.Toggle(true)
					]
				]

				// Main box
				+ SHorizontalBox::Slot()
				[
					SAssignNew(MenuBox, SVerticalBox)

					// Main assembly panel
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.Padding(0)
						.BorderImage(new FSlateNoResource)
						.ColorAndOpacity(this, &SNovaMainMenuAssembly::GetMainColor)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.TextStyle(&Theme.SubtitleFont)
								.Text(LOCTEXT("CompartmentTitle", "Compartment assembly"))
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.TextStyle(&Theme.MainFont)
								.Text(LOCTEXT("CompartmentSelection", "Compartments"))
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SAssignNew(CompartmentBox, SHorizontalBox)
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.TextStyle(&Theme.MainFont)
								.Text(LOCTEXT("CompartmentControls", "Compartment controls"))
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
									.TitleText(LOCTEXT("BuildCompartmentForward", "Add compartment forward"))
									.HelpText(LOCTEXT("BuildCompartmentForwardelp", "Pick the blueprint for the new compartment"))
									.OnSelfRefresh(SNovaCompartmentList::FNovaOnSelfRefresh::CreateLambda([&]()
										{
											int32 NewIndex = GetNewBuildIndex(true);
											return SNovaCompartmentList::SelfRefreshType(0, GetSpacecraftPawn()->GetCompatibleCompartments(NewIndex));
										}))
									.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateCompartmentItem)
									.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateCompartmentTooltip)
									.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnSelectedCompartmentChanged, true)
									.Enabled(this, &SNovaMainMenuAssembly::IsAddCompartmentEnabled, true)
									.ListButtonSize("LargeListButtonSize")
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNovaAssignNew(EditCompartmentButton, SNovaButton)
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

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNovaAssignNew(CompartmentListView, SNovaCompartmentList)
									.Panel(this)
									.TitleText(LOCTEXT("BuildCompartmentAft", "Add compartment aft"))
									.HelpText(LOCTEXT("BuildCompartmentAftHelp", "Pick the blueprint for the new compartment"))
									.OnSelfRefresh(SNovaCompartmentList::FNovaOnSelfRefresh::CreateLambda([&]()
										{
											int32 NewIndex = GetNewBuildIndex(false);
											return SNovaCompartmentList::SelfRefreshType(0, GetSpacecraftPawn()->GetCompatibleCompartments(NewIndex));
										}))
									.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateCompartmentItem)
									.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateCompartmentTooltip)
									.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnSelectedCompartmentChanged, false)
									.Enabled(this, &SNovaMainMenuAssembly::IsAddCompartmentEnabled, false)
									.ListButtonSize("LargeListButtonSize")
								]
							]
						]
					]
				
					// Compartment edition panel
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.Padding(0)
						.BorderImage(new FSlateNoResource)
						.ColorAndOpacity(this, &SNovaMainMenuAssembly::GetCompartmentColor)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.TextStyle(&Theme.SubtitleFont)
								.Text(LOCTEXT("CompartmentTitle", "Compartment details"))
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.VAlign(VAlign_Bottom)
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SAssignNew(ModuleBox, SVerticalBox)

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.TextStyle(&Theme.MainFont)
										.Text(LOCTEXT("ModuleTitle", "Modules"))
									]
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SAssignNew(EquipmentBox, SVerticalBox)

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.TextStyle(&Theme.MainFont)
										.Text(LOCTEXT("EquipmentTitle", "Equipments"))
									]
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SVerticalBox)

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.TextStyle(&Theme.MainFont)
										.Text(LOCTEXT("FeaturesTitle", "Compartment features"))
									]
								
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNovaAssignNew(HullTypeListView, SNovaHullTypeList)
										.Panel(this)
										.ItemsSource(&HullTypeList)
										.TitleText(LOCTEXT("HullListTitle", "Hull type"))
										.HelpText(LOCTEXT("HullListHelp", "Change the hull type for this compartment"))
										.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateHullTypeItem)
										.OnGenerateName(this, &SNovaMainMenuAssembly::GetHullTypeName)
										.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateHullTypeTooltip)
										.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnSelectedHullTypeChanged)
										.ListButtonSize("LargeListButtonSize")
									]
								]
							]
						]
					]
				]				

				+ SHorizontalBox::Slot()

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Bottom)
				.AutoWidth()
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("SaveSpacecraft", "Save spacecraft"))
						.HelpText(LOCTEXT("SaveSpacecraftHelp", "Save the current state of the spacecraft"))
						.OnClicked(this, &SNovaMainMenuAssembly::OnSaveSpacecraft)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Action(FNovaPlayerInput::MenuCancel)
						.Text(LOCTEXT("CompartmentBack", "Back to assembly"))
						.HelpText(LOCTEXT("CompartmentBackHelp", "Go back to the main assembly"))
						.OnClicked(this, &SNovaMainMenuAssembly::OnBackToAssembly)
					]
				]
			]
		]
	];

	// Build the modal panel 
	ModalPanel = Menu->CreateModalPanel(this);

	// Build compartment buttons statically
	CompartmentBox->AddSlot()
		.AutoWidth()
		[
			SNew(SNovaKeyLabel)
			.Key(this, &SNovaMainMenuAssembly::GetPreviousCompartmentKey)
		];
	for (int32 Index = 0; Index < ENovaConstants::MaxCompartmentCount; Index++)
	{
		CompartmentBox->AddSlot()
		.AutoWidth()
		[
			SNovaNew(SNovaButton)
			.Text(FText::AsNumber(Index + 1))
			.HelpText(LOCTEXT("SelectCompartmentHelp", "Select this compartment for editing"))
			.Icon(this, &SNovaMainMenuAssembly::GetCompartmentIcon, Index)
			.Enabled(this, &SNovaMainMenuAssembly::IsSelectCompartmentEnabled, Index)
			.OnClicked(this, &SNovaMainMenuAssembly::OnCompartmentSelected, Index)
			.Size("CompartmentButtonSize")
		];
	}
	CompartmentBox->AddSlot()
		.AutoWidth()
		[
			SNew(SNovaKeyLabel)
			.Key(this, &SNovaMainMenuAssembly::GetNextCompartmentKey)
		];

	// Build module buttons
	for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
	{
		ModuleBox->AddSlot()
			.AutoHeight()
			[
				SNovaAssignNew(ModuleListViews[ModuleIndex], SNovaModuleList)
				.Panel(this)
				.ItemsSource(&ModuleLists[ModuleIndex])
				.TitleText(LOCTEXT("ModuleListTitle", "Module"))
				.HelpText(LOCTEXT("ModuleListHelp", "Change the module for this slot"))
				.Enabled(this, &SNovaMainMenuAssembly::IsModuleListEnabled, ModuleIndex)
				.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateModuleItem)
				.OnGenerateName(this, &SNovaMainMenuAssembly::GetModuleName)
				.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateModuleTooltip)
				.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnSelectedModuleChanged, ModuleIndex)
				.ListButtonSize("LargeListButtonSize")
			];
	}

	// Build equipment buttons
	for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
	{
		EquipmentBox->AddSlot()
			.AutoHeight()
			[
				SNovaAssignNew(EquipmentListViews[EquipmentIndex], SNovaEquipmentList)
				.Panel(this)
				.ItemsSource(&EquipmentLists[EquipmentIndex])
				.TitleText(LOCTEXT("EquipmentListTitle", "Equipment"))
				.HelpText(LOCTEXT("EquipmentListHelp", "Change the equipment for this slot"))
				.Enabled(this, &SNovaMainMenuAssembly::IsEquipmentListEnabled, EquipmentIndex)
				.OnGenerateItem(this, &SNovaMainMenuAssembly::GenerateEquipmentItem)
				.OnGenerateName(this, &SNovaMainMenuAssembly::GetEquipmentName)
				.OnGenerateTooltip(this, &SNovaMainMenuAssembly::GenerateEquipmentTooltip)
				.OnSelectionChanged(this, &SNovaMainMenuAssembly::OnSelectedEquipmentChanged, EquipmentIndex)
				.ListButtonSize("LargeListButtonSize")
			];
	}

	// clang-format on

	// Default settings
	HighlightButton->SetActive(true);
	SetCompartmentPanelVisible(false);
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void SNovaMainMenuAssembly::Show()
{
	SNovaTabPanel::Show();

	// Reset the compartment view
	if (IsCompartmentPanelVisible)
	{
		EditedCompartmentIndex = INDEX_NONE;
		GetSpacecraftPawn()->SetDisplayFilter(GetSpacecraftPawn()->GetDisplayFilter(), INDEX_NONE);
		GetSpacecraftPawn()->SetHighlightCompartment(HighlightButton->IsActive() ? SelectedCompartmentIndex : INDEX_NONE);
	}

	// Reset compartment data
	SetSelectedCompartment(GetSpacecraftPawn()->GetCompartmentCount() > 0 ? 0 : INDEX_NONE);
	SetCompartmentPanelVisible(false);
}

void SNovaMainMenuAssembly::ZoomIn()
{
	if (SelectedCompartmentIndex != INDEX_NONE)
	{
		SetSelectedCompartment(FMath::Min(SelectedCompartmentIndex + 1, GetSpacecraftPawn()->GetCompartmentCount() - 1));
	}
}

void SNovaMainMenuAssembly::ZoomOut()
{
	if (SelectedCompartmentIndex != INDEX_NONE)
	{
		SetSelectedCompartment(FMath::Max(SelectedCompartmentIndex - 1, 0));
	}
}

bool SNovaMainMenuAssembly::Cancel()
{
	if (IsCompartmentPanelVisible)
	{
		EditedCompartmentIndex = INDEX_NONE;

		GetSpacecraftPawn()->SetDisplayFilter(GetSpacecraftPawn()->GetDisplayFilter(), INDEX_NONE);
		GetSpacecraftPawn()->SetHighlightCompartment(HighlightButton->IsActive() ? SelectedCompartmentIndex : INDEX_NONE);

		return true;
	}
	else
	{
		return SNovaTabPanel::Cancel();
	}
}

int32 GetCompartmentIndexAtPosition(ANovaPlayerController* PC, ANovaSpacecraftPawn* SpacecraftPawn, FVector2D Position)
{
	FVector WorldLocation, WorldDirection;

	if (PC->DeprojectScreenPositionToWorld(Position.X, Position.Y, WorldLocation, WorldDirection))
	{
		TArray<FHitResult>    TraceHits;
		FVector               TraceEndLocation = WorldLocation + WorldDirection * 10000;
		FCollisionQueryParams TraceParams(FName("AssemblyTrace"));

		PC->GetWorld()->LineTraceMultiByChannel(TraceHits, WorldLocation, TraceEndLocation, ECC_Camera, TraceParams);

		// DrawDebugLine(PC->GetWorld(), WorldLocation, TraceEndLocation, FColor::Red, true);

		for (const FHitResult& TraceHit : TraceHits)
		{
			if (TraceHit.bBlockingHit)
			{
				// DrawDebugPoint(PC->GetWorld(), TraceHit.Location, 4, FColor::Red, true);

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

void SNovaMainMenuAssembly::OnClicked(const FVector2D& Position)
{
	if (!IsCompartmentPanelVisible && Menu->IsActiveNavigationPanel(this))
	{
		int32 HitIndex = GetCompartmentIndexAtPosition(MenuManager->GetPC(), GetSpacecraftPawn(), Position);
		if (HitIndex != INDEX_NONE)
		{
			SetSelectedCompartment(HitIndex);
		}
	}
}

void SNovaMainMenuAssembly::OnDoubleClicked(const FVector2D& Position)
{
	if (!IsCompartmentPanelVisible && Menu->IsActiveNavigationPanel(this) && SelectedCompartmentIndex != INDEX_NONE)
	{
		int32 HitIndex = GetCompartmentIndexAtPosition(MenuManager->GetPC(), GetSpacecraftPawn(), Position);
		if (HitIndex == SelectedCompartmentIndex)
		{
			OnEditCompartment();
		}
	}
}

void SNovaMainMenuAssembly::HorizontalAnalogInput(float Value)
{
	if (GetSpacecraftPawn())
	{
		GetSpacecraftPawn()->PanInput(Value);
	}
}

void SNovaMainMenuAssembly::VerticalAnalogInput(float Value)
{
	if (GetSpacecraftPawn())
	{
		GetSpacecraftPawn()->TiltInput(Value);
	}
}

TSharedPtr<SNovaButton> SNovaMainMenuAssembly::GetDefaultFocusButton() const
{
	if (IsCompartmentPanelVisible && IsModuleListEnabled(0))
	{
		return ModuleListViews[0];
	}
	else if (IsCompartmentPanelVisible && IsEquipmentListEnabled(0))
	{
		return EquipmentListViews[0];
	}
	else if (CompartmentListView->IsButtonEnabled())
	{
		return CompartmentListView;
	}
	else if (EditCompartmentButton->IsButtonEnabled())
	{
		return EditCompartmentButton;
	}
	else
	{
		return SNovaTabPanel::GetDefaultFocusButton();
	}
}

void SNovaMainMenuAssembly::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update fade time
	bool WantCompartmentPanelVisible = EditedCompartmentIndex != INDEX_NONE;
	if (WantCompartmentPanelVisible != IsCompartmentPanelVisible)
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
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

ANovaSpacecraftPawn* SNovaMainMenuAssembly::GetSpacecraftPawn() const
{
	return MenuManager->GetPC() ? MenuManager->GetPC()->GetSpacecraftPawn() : nullptr;
}

int32 SNovaMainMenuAssembly::GetNewBuildIndex(bool Forward) const
{
	int32 NewIndex = Forward ? SelectedCompartmentIndex : SelectedCompartmentIndex + 1;
	return FMath::Clamp(NewIndex, 0, ENovaConstants::MaxCompartmentCount - 1);
}

void SNovaMainMenuAssembly::SetSelectedCompartment(int32 Index)
{
	SelectedCompartmentIndex = Index;

	GetSpacecraftPawn()->SetHighlightCompartment(HighlightButton->IsActive() ? SelectedCompartmentIndex : INDEX_NONE);
}

void SNovaMainMenuAssembly::SetCompartmentPanelVisible(bool Active)
{
	IsCompartmentPanelVisible = Active;

	if (IsCompartmentPanelVisible)
	{
		ANovaSpacecraftPawn*    SpacecraftPawn = GetSpacecraftPawn();
		const FNovaCompartment& Compartment    = SpacecraftPawn->GetCompartment(SelectedCompartmentIndex);

		// Refresh hull type list
		HullTypeList = Compartment.Description->GetSupportedHullTypes();
		HullTypeListView->Refresh(HullTypeList.Find(Compartment.HullType));

		// Refresh modules lists
		for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxEquipmentCount; ModuleIndex++)
		{
			ModuleLists[ModuleIndex] = SpacecraftPawn->GetCompatibleModules(SelectedCompartmentIndex, ModuleIndex);
			ModuleListViews[ModuleIndex]->Refresh(ModuleLists[ModuleIndex].Find(Compartment.Modules[ModuleIndex].Description));
		}

		// Refresh equipment lists
		for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
		{
			EquipmentLists[EquipmentIndex] = SpacecraftPawn->GetCompatibleEquipments(SelectedCompartmentIndex, EquipmentIndex);
			EquipmentListViews[EquipmentIndex]->Refresh(EquipmentLists[EquipmentIndex].Find(Compartment.Equipments[EquipmentIndex]));
		}
	}

	MenuBox->GetChildren()->GetChildAt(0)->SetVisibility(IsCompartmentPanelVisible ? EVisibility::Collapsed : EVisibility::Visible);
	MenuBox->GetChildren()->GetChildAt(1)->SetVisibility(IsCompartmentPanelVisible ? EVisibility::Visible : EVisibility::Collapsed);
	ResetNavigation();
}

/*----------------------------------------------------
    Compartment template list
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateCompartmentItem(const UNovaCompartmentDescription* Description) const
{
	return GenerateAssetItem(Description);
}

FText SNovaMainMenuAssembly::GenerateCompartmentTooltip(const UNovaCompartmentDescription* Description) const
{
	return FText::FormatNamed(LOCTEXT("CompartmentHelp", "Build a new {compartment}"), TEXT("compartment"), Description->Name);
}

/*----------------------------------------------------
    Compartment module list
----------------------------------------------------*/

bool SNovaMainMenuAssembly::IsModuleListEnabled(int32 ModuleIndex) const
{
	if (IsCompartmentPanelVisible)
	{
		ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
		if (IsValid(SpacecraftPawn))
		{
			const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(SelectedCompartmentIndex);
			return ModuleIndex < Compartment.Description->ModuleSlots.Num();
		}
	}

	return false;
}

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateModuleItem(const UNovaModuleDescription* Module) const
{
	return GenerateAssetItem(Module);
}

FText SNovaMainMenuAssembly::GetModuleName(const UNovaModuleDescription* Module) const
{
	return GetAssetName(Module);
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

bool SNovaMainMenuAssembly::IsEquipmentListEnabled(int32 EquipmentIndex) const
{
	if (IsCompartmentPanelVisible)
	{
		ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();

		if (IsValid(SpacecraftPawn))
		{
			const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(SelectedCompartmentIndex);
			return EquipmentIndex < Compartment.Description->EquipmentSlots.Num();
		}
	}

	return false;
}

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateEquipmentItem(const UNovaEquipmentDescription* Equipment) const
{
	return GenerateAssetItem(Equipment);
}

FText SNovaMainMenuAssembly::GetEquipmentName(const UNovaEquipmentDescription* Equipment) const
{
	return GetAssetName(Equipment);
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

TSharedRef<SWidget> SNovaMainMenuAssembly::GenerateAssetItem(const UNovaAssetDescription* Asset) const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	return SNew(SOverlay).Clipping(EWidgetClipping::ClipToBoundsAlways)

		 + SOverlay::Slot()[SNew(SScaleBox)[SNew(SImage).Image(Asset ? &Asset->AssetRender : new FSlateNoResource)]]

		 + SOverlay::Slot().Padding(Theme.ContentPadding)[SNew(STextBlock).TextStyle(&Theme.MainFont).Text(GetAssetName(Asset))];
}

FText SNovaMainMenuAssembly::GetAssetName(const UNovaAssetDescription* Asset) const
{
	return Asset ? Asset->Name : LOCTEXT("Empty", "Empty");
}

FLinearColor SNovaMainMenuAssembly::GetMainColor() const
{
	float Alpha = IsCompartmentPanelVisible
					? 0.0f
					: FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENovaUIConstants::EaseStandard);
	return FLinearColor(1, 1, 1, Alpha);
}

FLinearColor SNovaMainMenuAssembly::GetCompartmentColor() const
{
	float Alpha = IsCompartmentPanelVisible
					? FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENovaUIConstants::EaseStandard)
					: 0.0f;
	return FLinearColor(1, 1, 1, Alpha);
}

const FSlateBrush* SNovaMainMenuAssembly::GetCompartmentIcon(int32 Index) const
{
	return FNovaStyleSet::GetBrush(Index == SelectedCompartmentIndex ? "Icon/SB_ListOn" : "Icon/SB_ListOff");
}

bool SNovaMainMenuAssembly::IsSelectCompartmentEnabled(int32 Index) const
{
	NCHECK(Index >= 0 && Index < ENovaConstants::MaxCompartmentCount);
	return GetSpacecraftPawn() && Index >= 0 && Index < GetSpacecraftPawn()->GetCompartmentCount();
}

bool SNovaMainMenuAssembly::IsAddCompartmentEnabled(bool Forward) const
{
	if (GetSpacecraftPawn() == nullptr)
	{
		return false;
	}
	else if (GetSpacecraftPawn()->GetCompartmentCount() >= ENovaConstants::MaxCompartmentCount)
	{
		return false;
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

bool SNovaMainMenuAssembly::IsEditCompartmentEnabled() const
{
	return SelectedCompartmentIndex >= 0;
}

bool SNovaMainMenuAssembly::IsToggleHighlightEnabled() const
{
	return EditedCompartmentIndex == INDEX_NONE;
}

FKey SNovaMainMenuAssembly::GetPreviousCompartmentKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuZoomOut);
}

FKey SNovaMainMenuAssembly::GetNextCompartmentKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuZoomIn);
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuAssembly::OnEditCompartment()
{
	NCHECK(SelectedCompartmentIndex >= 0 && SelectedCompartmentIndex < ENovaConstants::MaxCompartmentCount);

	NLOG("SNovaMainMenuAssembly::OnEditCompartment : editing compartment at index %d", SelectedCompartmentIndex);

	EditedCompartmentIndex = SelectedCompartmentIndex;

	GetSpacecraftPawn()->SetDisplayFilter(GetSpacecraftPawn()->GetDisplayFilter(), EditedCompartmentIndex);
	GetSpacecraftPawn()->SetHighlightCompartment(INDEX_NONE);
}

void SNovaMainMenuAssembly::OnRemoveCompartment()
{
	NCHECK(SelectedCompartmentIndex >= 0 && SelectedCompartmentIndex < ENovaConstants::MaxCompartmentCount);

	ModalPanel->Show(LOCTEXT("ScrapCompartmentConfirm", "Scrap compartment"),
		LOCTEXT("ScrapCompartmentConfirmHelp", "Confirm the scrapping of this compartment"),
		FSimpleDelegate::CreateSP(this, &SNovaMainMenuAssembly::OnRemoveCompartmentConfirmed), FSimpleDelegate(), FSimpleDelegate());
}

void SNovaMainMenuAssembly::OnRemoveCompartmentConfirmed()
{
	NCHECK(SelectedCompartmentIndex >= 0 && SelectedCompartmentIndex < ENovaConstants::MaxCompartmentCount);

	NLOG("SNovaMainMenuAssembly::OnRemoveCompartment : removing compartment at index %d", SelectedCompartmentIndex);

	if (GetSpacecraftPawn()->RemoveCompartment(SelectedCompartmentIndex))
	{
		GetSpacecraftPawn()->RequestAssemblyUpdate();

		SelectedCompartmentIndex--;
		if (GetSpacecraftPawn()->GetCompartmentCount() > 0 && SelectedCompartmentIndex == INDEX_NONE)
		{
			SelectedCompartmentIndex = 0;
		}

		SetSelectedCompartment(SelectedCompartmentIndex);
	}
}

void SNovaMainMenuAssembly::OnSelectedCompartmentChanged(const UNovaCompartmentDescription* Compartment, int32 Index, bool Forward)
{
	int32 NewIndex = GetNewBuildIndex(Forward);

	NLOG("SNovaMainMenuAssembly::OnSelectedCompartmentChanged : adding new compartment at index %d ('%s')", NewIndex,
		*Compartment->Name.ToString());

	if (GetSpacecraftPawn()->InsertCompartment(FNovaCompartment(Compartment), NewIndex))
	{
		GetSpacecraftPawn()->RequestAssemblyUpdate();
		SetSelectedCompartment(NewIndex);
	}
}

void SNovaMainMenuAssembly::OnSelectedFilterChanged(float Value)
{
	GetSpacecraftPawn()->SetDisplayFilter(static_cast<ENovaAssemblyDisplayFilter>(Value), EditedCompartmentIndex);
}

void SNovaMainMenuAssembly::OnCompartmentSelected(int32 Index)
{
	NCHECK(Index >= 0 && Index < ENovaConstants::MaxCompartmentCount);
	SetSelectedCompartment(Index);
}

void SNovaMainMenuAssembly::OnSelectedModuleChanged(const UNovaModuleDescription* Module, int32 Index, int32 SlotIndex)
{
	NLOG("SNovaMainMenuAssembly::OnSelectedModuleChanged : adding new module at index %d, slot %d ('%s')", EditedCompartmentIndex,
		SlotIndex, Module ? *Module->Name.ToString() : TEXT("nullptr"));

	GetSpacecraftPawn()->GetCompartment(EditedCompartmentIndex).Modules[SlotIndex].Description = Module;
	GetSpacecraftPawn()->RequestAssemblyUpdate();
}

void SNovaMainMenuAssembly::OnSelectedEquipmentChanged(const UNovaEquipmentDescription* Equipment, int32 Index, int32 SlotIndex)
{
	NLOG("SNovaMainMenuAssembly::OnSelectedEquipmentChanged : adding new equipment at index %d, slot %d ('%s')", EditedCompartmentIndex,
		SlotIndex, Equipment ? *Equipment->Name.ToString() : TEXT("nullptr"));

	GetSpacecraftPawn()->GetCompartment(EditedCompartmentIndex).Equipments[SlotIndex] = Equipment;
	GetSpacecraftPawn()->RequestAssemblyUpdate();
}

void SNovaMainMenuAssembly::OnSelectedHullTypeChanged(ENovaHullType Type, int32 Index)
{
	NLOG("SNovaMainMenuAssembly::OnSelectedHullTypeChanged : setting new hull ('%d')", static_cast<int32>(Type));

	GetSpacecraftPawn()->GetCompartment(EditedCompartmentIndex).HullType = Type;
	GetSpacecraftPawn()->RequestAssemblyUpdate();
}

void SNovaMainMenuAssembly::OnSaveSpacecraft()
{
	GetSpacecraftPawn()->SaveAssembly();
}

void SNovaMainMenuAssembly::OnBackToAssembly()
{
	Cancel();
}

void SNovaMainMenuAssembly::OnToggleHighlight()
{
	GetSpacecraftPawn()->SetHighlightCompartment(HighlightButton->IsActive() ? SelectedCompartmentIndex : INDEX_NONE);
}

#undef LOCTEXT_NAMESPACE
