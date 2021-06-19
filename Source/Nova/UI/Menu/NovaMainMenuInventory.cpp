// Nova project - Gwennaël Arbona

#include "NovaMainMenuInventory.h"

#include "NovaMainMenu.h"

#include "Nova/Game/NovaGameState.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/System/NovaSpacecraftPropellantSystem.h"

#include "Nova/System/NovaAssetManager.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/UI/Component/NovaTradingPanel.h"
#include "Nova/UI/Component/NovaTradableAssetItem.h"
#include "Nova/UI/Widget/NovaFadingWidget.h"
#include "Nova/UI/Widget/NovaModalPanel.h"

#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuInventory"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuInventory::SNovaMainMenuInventory()
	: PC(nullptr), GameState(nullptr), Spacecraft(nullptr), SpacecraftPawn(nullptr), CurrentCompartmentIndex(INDEX_NONE)
{}

void SNovaMainMenuInventory::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// Local data
	TSharedPtr<SVerticalBox> MainLayoutBox;

	// clang-format off
	ChildSlot
	.HAlign(HAlign_Center)
	[
		SAssignNew(MainLayoutBox, SVerticalBox)

		// Main box
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(Theme.ContentPadding)
		[
			SNew(SVerticalBox)
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(Theme.VerticalContentPadding)
			[
				SNew(STextBlock)
				.TextStyle(&Theme.SubtitleFont)
				.Text(LOCTEXT("Propellant", "Propellant"))
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				[
					SNew(SNovaButtonLayout)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.VerticalContentPadding)
						.HAlign(HAlign_Left)
						[
							SNew(SNovaRichText)
							.TextStyle(&Theme.MainFont)
							.Text(FNovaTextGetter::CreateSP(this, &SNovaMainMenuInventory::GetPropellantText))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SProgressBar)
							.Style(&Theme.ProgressBarStyle)
							.Percent(this, &SNovaMainMenuInventory::GetPropellantRatio)
						]

						+ SVerticalBox::Slot()
					]
				]
			
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNovaNew(SNovaButton)
					.Text(LOCTEXT("TradePropellant", "Trade propellant"))
					.HelpText(LOCTEXT("TradePropellantHelp", "Trade propellant with this station"))
					.Action(FNovaPlayerInput::MenuPrimary)
					.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([=]()
					{
						return SpacecraftPawn->IsDocked();
					})))
					.OnClicked(this, &SNovaMainMenuInventory::OnRefuelPropellant)
				]
			]
		]
	];

	// Cargo line generator
	auto BuildCargoLine = [&](FText Title, ENovaResourceType Type)
	{
		TSharedPtr<SHorizontalBox> CargoLineBox;

		MainLayoutBox->AddSlot()
		.AutoHeight()
		.Padding(Theme.ContentPadding)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(Theme.VerticalContentPadding)
			[
				SNew(STextBlock)
				.TextStyle(&Theme.SubtitleFont)
				.Text(Title)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(CargoLineBox, SHorizontalBox)
			]
		];

		for (int32 Index = 0; Index < ENovaConstants::MaxCompartmentCount; Index++)
		{
			auto IsValidCompartment = [=]()
			{
				const FNovaSpacecraft* Spacecraft = PC->GetSpacecraft();
				if (Spacecraft && Index >= 0 && Index < Spacecraft->Compartments.Num())
				{
					return Spacecraft->Compartments[Index].GetCargoCapacity(Type) > 0;
				}
				else
				{
					return false;
				}
			};

			CargoLineBox->AddSlot()
			.AutoWidth()
			[
				SNovaNew(SNovaButton)
				.Size("InventoryButtonSize")
				.HelpText(LOCTEXT("TradeSlotHelp", "Trade with this cargo slot"))
				.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([=]()
				{
					return IsValidCompartment() && SpacecraftPawn->IsDocked();
				})))
				.OnClicked(this, &SNovaMainMenuInventory::OnTradeWithSlot, Index, Type)
				.Content()
				[
					SNew(SOverlay)

					// Resource picture
					+ SOverlay::Slot()
					[
						SNew(SScaleBox)
						.Stretch(EStretch::ScaleToFill)
						[
							SNew(SNovaImage)
							.Image(FNovaImageGetter::CreateLambda([=]() -> const FSlateBrush*
							{
								if (IsValidCompartment())
								{
									const FNovaCompartment& Compartment = Spacecraft->Compartments[Index];
									const FNovaSpacecraftCargo& Cargo = Compartment.GetCargo(Type);
									if (IsValid(Cargo.Resource))
									{
										return &Cargo.Resource->AssetRender;
									}
								}

								return &UNovaResource::GetEmpty()->AssetRender;
							}))
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
								SNew(SNovaText)
								.Text(FNovaTextGetter::CreateLambda([=]() -> FText
								{
									if (IsValidCompartment())
									{
										const FNovaCompartment& Compartment = Spacecraft->Compartments[Index];
										const FNovaSpacecraftCargo& Cargo = Compartment.GetCargo(Type);
										if (IsValid(Cargo.Resource))
										{
											return Cargo.Resource->Name;
										}
										else
										{
											return LOCTEXT("EmptyCargo", "Empty");
										}
									}

									return FText();
								}))
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
								SNew(SNovaRichText)
								.Text(FNovaTextGetter::CreateLambda([=]() -> FText
								{
									if (IsValidCompartment())
									{
										const FNovaCompartment& Compartment = Spacecraft->Compartments[Index];
										const FNovaSpacecraftCargo& Cargo = Compartment.GetCargo(Type);
										int32 Amount = Cargo.Amount;
										int32 Capacity = Compartment.GetCargoCapacity(Type);

										return FText::FormatNamed(LOCTEXT("CargoAmountFormat", "<img src=\"/Text/Cargo\"/> {amount}T / {capacity}T"),
											TEXT("amount"), FText::AsNumber(Amount),
											TEXT("capacity"), FText::AsNumber(Capacity));
									}

									return FText();
								}))
								.TextStyle(&Theme.MainFont)
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
	SAssignNew(ResourceListView, SNovaListView<const UNovaResource*>)
	.Panel(GenericModalPanel.Get())
	.ItemsSource(&ResourceList)
	.OnGenerateItem(this, &SNovaMainMenuInventory::GenerateResourceItem)
	.OnGenerateTooltip(this, &SNovaMainMenuInventory::GenerateResourceTooltip)
	.OnSelectionDoubleClicked(this, &SNovaMainMenuInventory::OnBuyResource)
	.Horizontal(true)
	.ButtonSize("LargeListButtonSize");
	// clang-format on

	// Build the procedural cargo lines
	BuildCargoLine(LOCTEXT("GeneralCargoTitle", "General cargo"), ENovaResourceType::General);
	BuildCargoLine(LOCTEXT("BulkCargoTitle", "Bulk cargo"), ENovaResourceType::Bulk);
	BuildCargoLine(LOCTEXT("LiquidCargoTitle", "Liquid cargo"), ENovaResourceType::Liquid);

	AveragedPropellantRatio.SetPeriod(1.0f);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuInventory::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	UNovaSpacecraftPropellantSystem* PropellantSystem = SpacecraftPawn->FindComponentByClass<UNovaSpacecraftPropellantSystem>();
	NCHECK(PropellantSystem);

	AveragedPropellantRatio.Set(PropellantSystem->GetCurrentPropellantAmount() / PropellantSystem->GetTotalPropellantAmount(), DeltaTime);
}

void SNovaMainMenuInventory::Show()
{
	SNovaTabPanel::Show();
}

void SNovaMainMenuInventory::Hide()
{
	SNovaTabPanel::Hide();
}

void SNovaMainMenuInventory::UpdateGameObjects()
{
	PC             = MenuManager.IsValid() ? MenuManager->GetPC() : nullptr;
	GameState      = IsValid(PC) ? MenuManager->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	Spacecraft     = IsValid(PC) ? PC->GetSpacecraft() : nullptr;
	SpacecraftPawn = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
}

/*----------------------------------------------------
    Resource list
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuInventory::GenerateResourceItem(const UNovaResource* Resource)
{
	return SNew(SNovaTradableAssetItem).Asset(Resource).Dark(true);
}

const FSlateBrush* SNovaMainMenuInventory::GetResourceIcon(const UNovaResource* Resource) const
{
	return ResourceListView->GetSelectionIcon(Resource);
}

FText SNovaMainMenuInventory::GenerateResourceTooltip(const UNovaResource* Resource)
{
	return FText::FormatNamed(LOCTEXT("ResourceHelp", "Buy {resource}"), TEXT("resource"), Resource->Name);
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

TOptional<float> SNovaMainMenuInventory::GetPropellantRatio() const
{
	return AveragedPropellantRatio.Get();
}

FText SNovaMainMenuInventory::GetPropellantText() const
{
	UNovaSpacecraftPropellantSystem* PropellantSystem = SpacecraftPawn->FindComponentByClass<UNovaSpacecraftPropellantSystem>();
	NCHECK(PropellantSystem);

	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 0;

	return FText::FormatNamed(LOCTEXT("PropellantFormat", "<img src=\"/Text/Propellant\"/> {remaining}T out of {total}T"),
		TEXT("remaining"), FText::AsNumber(PropellantSystem->GetCurrentPropellantAmount(), &Options), TEXT("total"),
		FText::AsNumber(PropellantSystem->GetTotalPropellantAmount(), &Options));
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuInventory::OnRefuelPropellant()
{
	TradingModalPanel->StartTrade(PC, UNovaResource::GetPropellant(), INDEX_NONE);
}

void SNovaMainMenuInventory::OnTradeWithSlot(int32 Index, ENovaResourceType Type)
{
	NCHECK(IsValid(GameState));
	NCHECK(IsValid(GameState->GetCurrentArea()));

	const FNovaCompartment&     Compartment = Spacecraft->Compartments[Index];
	const FNovaSpacecraftCargo& Cargo       = Compartment.GetCargo(Type);

	// Valid resource in hold - allow trading it directly
	if (IsValid(Cargo.Resource))
	{
		TradingModalPanel->StartTrade(PC, Cargo.Resource, Index);
	}

	// Cargo hold is empty, pick a resource to buy first
	else
	{
		CurrentCompartmentIndex = Index;

		auto BuyResource = [=]()
		{
			TradingModalPanel->StartTrade(PC, ResourceListView->GetSelectedItem(), Index);
		};

		//	Fill the resource list
		ResourceList.Empty();
		for (const FNovaResourceSale& Sale : GameState->GetCurrentArea()->ResourcesSold)
		{
			ResourceList.Add(Sale.Resource);
		}
		ResourceListView->Refresh(0);

		// Proceed with the modal panel
		GenericModalPanel->Show(LOCTEXT("SelectResource", "Select resource"), FText(), FSimpleDelegate::CreateLambda(BuyResource),
			FSimpleDelegate(), FSimpleDelegate(), ResourceListView);
	}
}

void SNovaMainMenuInventory::OnBuyResource()
{
	NCHECK(CurrentCompartmentIndex != INDEX_NONE);

	GenericModalPanel->Hide();

	TradingModalPanel->StartTrade(PC, ResourceListView->GetSelectedItem(), CurrentCompartmentIndex);
}

#undef LOCTEXT_NAMESPACE
