// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuOperations.h"

#include "NovaMainMenu.h"

#include "Game/NovaGameState.h"
#include "Player/NovaPlayerController.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/System/NovaSpacecraftPropellantSystem.h"

#include "UI/Widgets/NovaTradingPanel.h"
#include "UI/Widgets/NovaTradableAssetItem.h"

#include "Nova.h"

#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronGameInstance.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/UI/Widgets/NeutronFadingWidget.h"
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
	, CurrentModuleIndexx(INDEX_NONE)
{}

void SNovaMainMenuOperations::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	MenuManager                    = InArgs._MenuManager;

	// Parent constructor
	SNeutronNavigationPanel::Construct(SNeutronNavigationPanel::FArguments().Menu(InArgs._Menu));

	// Local data
	TSharedPtr<SScrollBox> MainLayoutBox;

	// clang-format off
	ChildSlot
	.HAlign(HAlign_Center)
	[
		SAssignNew(MainLayoutBox, SScrollBox)
		.Style(&Theme.ScrollBoxStyle)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.AnimateWheelScrolling(true)
		
		// Bulk trading title
		+ SScrollBox::Slot()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.HeadingFont)
			.Text(LOCTEXT("BatchTrade", "Batch trading"))
		]

		// Bulk trading box
		+ SScrollBox::Slot()
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
				.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([=]()
				{
					return SpacecraftPawn && SpacecraftPawn->IsDocked() && !SpacecraftPawn->HasModifications();
				})))
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

		// Compartments title
		+ SScrollBox::Slot()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.HeadingFont)
			.Text(LOCTEXT("CompartmentsTitle", "Compartments"))
		]
	];

	// Module line generator
	auto BuildModuleLine = [&](int32 ModuleIndex)
	{
		TSharedPtr<SHorizontalBox> ModuleLineBox;
		MainLayoutBox->AddSlot()
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

						// Name
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(&Theme.MainMenuDarkBackground)
							.Padding(Theme.ContentPadding)
							[
								SNew(SNeutronText)
								.Text(FNeutronTextGetter::CreateSP(this, &SNovaMainMenuOperations::GetModuleText, CompartmentIndex, ModuleIndex))
								.TextStyle(&Theme.MainFont)
								.AutoWrapText(true)
							]
						]
					
						+ SVerticalBox::Slot()

						// Amount / capacity
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(&Theme.MainMenuDarkBackground)
							.Padding(Theme.ContentPadding)
							[
								SNew(SNeutronRichText)
								.Text(FNeutronTextGetter::CreateSP(this, &SNovaMainMenuOperations::GetModuleDetails, CompartmentIndex, ModuleIndex))
								.TextStyle(&Theme.MainFont)
								.AutoWrapText(true)
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
		UNovaSpacecraftPropellantSystem* PropellantSystem = GameState->GetSpacecraftSystem<UNovaSpacecraftPropellantSystem>(Spacecraft);
		NCHECK(PropellantSystem);
		AveragedPropellantRatio.Set(PropellantSystem->GetCurrentPropellantMass() / PropellantSystem->GetPropellantCapacity(), DeltaTime);
	}
}

void SNovaMainMenuOperations::Show()
{
	SNeutronTabPanel::Show();
}

void SNovaMainMenuOperations::Hide()
{
	SNeutronTabPanel::Hide();
}

void SNovaMainMenuOperations::UpdateGameObjects()
{
	PC             = MenuManager.IsValid() ? MenuManager->GetPC<ANovaPlayerController>() : nullptr;
	GameState      = IsValid(PC) ? MenuManager->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	Spacecraft     = IsValid(PC) ? PC->GetSpacecraft() : nullptr;
	SpacecraftPawn = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
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
		UNovaSpacecraftPropellantSystem* PropellantSystem = SpacecraftPawn->FindComponentByClass<UNovaSpacecraftPropellantSystem>();
		NCHECK(PropellantSystem);

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

bool SNovaMainMenuOperations::IsValidCompartment(int32 CompartmentIndex, int32 ModuleIndex) const
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
	}

	return false;
}

const FNovaCompartmentModule& SNovaMainMenuOperations::GetModule(int32 CompartmentIndex, int32 ModuleIndex) const
{
	return Spacecraft->Compartments[CompartmentIndex].Modules[ModuleIndex];
}

bool SNovaMainMenuOperations::IsModuleEnabled(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (IsValidCompartment(CompartmentIndex, ModuleIndex))
	{
		const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

		// Cargo
		if (Desc->IsA<UNovaCargoModuleDescription>())
		{
			const FNovaCompartment&     Compartment = Spacecraft->Compartments[CompartmentIndex];
			const FNovaSpacecraftCargo& Cargo       = Compartment.GetCargo(ModuleIndex);
			return (SpacecraftPawn->IsDocked() || IsValid(Cargo.Resource)) && !SpacecraftPawn->HasModifications();
		}

		// Propellant
		else if (Desc->IsA<UNovaPropellantModuleDescription>())
		{
			return SpacecraftPawn->IsDocked() && !SpacecraftPawn->HasModifications();
		}
	}

	return false;
}

FText SNovaMainMenuOperations::GetModuleHelpText(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (IsValidCompartment(CompartmentIndex, ModuleIndex))
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
			return LOCTEXT("TradePropellantHelp", "Trade propellant");
		}
	}

	return FText();
}

const FSlateBrush* SNovaMainMenuOperations::GetModuleImage(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (IsValidCompartment(CompartmentIndex, ModuleIndex))
	{
		const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

		// Cargo
		if (Desc->IsA<UNovaCargoModuleDescription>())
		{
			const FNovaCompartment&     Compartment = Spacecraft->Compartments[CompartmentIndex];
			const FNovaSpacecraftCargo& Cargo       = Compartment.GetCargo(ModuleIndex);
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
	}

	return &UNovaResource::GetEmpty()->AssetRender;
}

FText SNovaMainMenuOperations::GetModuleText(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (IsValidCompartment(CompartmentIndex, ModuleIndex))
	{
		const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

		// Cargo
		if (Desc->IsA<UNovaCargoModuleDescription>())
		{
			const FNovaCompartment&     Compartment = Spacecraft->Compartments[CompartmentIndex];
			const FNovaSpacecraftCargo& Cargo       = Compartment.GetCargo(ModuleIndex);
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
	}

	return FText();
}

FText SNovaMainMenuOperations::GetModuleDetails(int32 CompartmentIndex, int32 ModuleIndex) const
{
	if (IsValidCompartment(CompartmentIndex, ModuleIndex))
	{
		const UNovaModuleDescription* Desc = GetModule(CompartmentIndex, ModuleIndex).Description;

		// Cargo
		if (Desc->IsA<UNovaCargoModuleDescription>())
		{
			const FNovaCompartment&     Compartment = Spacecraft->Compartments[CompartmentIndex];
			const FNovaSpacecraftCargo& Cargo       = Compartment.GetCargo(ModuleIndex);
			int32                       Amount      = Cargo.Amount;
			int32                       Capacity    = Compartment.GetCargoCapacity(ModuleIndex);

			return FText::FormatNamed(LOCTEXT("CargoAmountFormat", "<img src=\"/Text/Cargo\"/> {amount} T / {capacity} T"), TEXT("amount"),
				FText::AsNumber(Amount), TEXT("capacity"), FText::AsNumber(Capacity));
		}

		// Propellant
		else if (Desc->IsA<UNovaPropellantModuleDescription>())
		{
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
	CurrentModuleIndexx     = INDEX_NONE;

	auto TradeResource = [this]()
	{
		TradingModalPanel->Trade(PC, GameState->GetCurrentArea(), ResourceListView->GetSelectedItem(), INDEX_NONE, INDEX_NONE);
	};

	// Fill the resource list
	ResourceList.Empty();
	for (const UNovaResource* Resource : Spacecraft->GetOwnedResources())
	{
		if (GameState->IsResourceSold(Resource))
		{
			ResourceList.Add(Resource);
		}
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
	CurrentModuleIndexx     = INDEX_NONE;

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
		ResourceList.Num() == 0 ? LOCTEXT("NoBulkSellResource", "No resources are available to buy at this station") : FText(),
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
		const FNovaCompartment&     Compartment = Spacecraft->Compartments[CompartmentIndex];
		const FNovaSpacecraftCargo& Cargo       = Compartment.GetCargo(ModuleIndex);

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
				CurrentModuleIndexx     = ModuleIndex;

				auto BuyResource = [this, CompartmentIndex, ModuleIndex]()
				{
					TradingModalPanel->Trade(
						PC, GameState->GetCurrentArea(), ResourceListView->GetSelectedItem(), CompartmentIndex, ModuleIndex);
				};

				//	Fill the resource list
				ResourceList = GameState->GetResourcesSold(Compartment.GetCargoType(ModuleIndex));
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
}

void SNovaMainMenuOperations::OnBuyResource()
{
	GenericModalPanel->Hide();

	TradingModalPanel->Trade(
		PC, GameState->GetCurrentArea(), ResourceListView->GetSelectedItem(), CurrentCompartmentIndex, CurrentModuleIndexx);
}

#undef LOCTEXT_NAMESPACE
