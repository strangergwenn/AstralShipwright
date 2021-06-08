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
#include "Nova/UI/Widget/NovaFadingWidget.h"
#include "Nova/UI/Widget/NovaModalPanel.h"

#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuInventory"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuInventory::SNovaMainMenuInventory() : PC(nullptr), SpacecraftPawn(nullptr), GameState(nullptr)
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
						SNew(SProgressBar)
						.Style(&Theme.ProgressBarStyle)
						.Percent(0.5f)
					]
				]
			
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNovaAssignNew(RefuelButton, SNovaButton)
					.Text(LOCTEXT("RefillPropellant", "Refuel"))
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
				if (IsValid(SpacecraftPawn) && Index >= 0 && Index < SpacecraftPawn->GetCompartmentCount())
				{
					return SpacecraftPawn->GetCompartment(Index).GetCargoCapacity(Type) > 0;
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
				//.HelpText() // TODO
				.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([=]()
				{
					return IsValidCompartment();
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
									const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(Index);
									const FNovaSpacecraftCargo& Cargo = Compartment.GetCargo(Type);
									if (IsValid(Cargo.Resource))
									{
										return &Cargo.Resource->AssetRender;
									}
								}

								return nullptr;
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
							.BorderImage(&Theme.MainMenuGenericBackground)
							.Padding(Theme.ContentPadding)
							[
								SNew(SNovaText)
								.Text(FNovaTextGetter::CreateLambda([=]() -> FText
								{
									if (IsValidCompartment())
									{
										const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(Index);
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
							]
						]
					
						+ SVerticalBox::Slot()

						// Amount / capacity
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(&Theme.MainMenuGenericBackground)
							.Padding(Theme.ContentPadding)
							[
								SNew(SNovaRichText)
								.Text(FNovaTextGetter::CreateLambda([=]() -> FText
								{
									if (IsValidCompartment())
									{
										const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(Index);
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
	.Horizontal(true);
	// clang-format on

	// Build the procedural cargo lines
	BuildCargoLine(LOCTEXT("GeneralCargoTitle", "General cargo"), ENovaResourceType::General);
	BuildCargoLine(LOCTEXT("BulkCargoTitle", "Bulk cargo"), ENovaResourceType::Bulk);
	BuildCargoLine(LOCTEXT("LiquidCargoTitle", "Liquid cargo"), ENovaResourceType::Liquid);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuInventory::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);
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
	SpacecraftPawn = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
	GameState      = IsValid(PC) ? MenuManager->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
}

TSharedPtr<SNovaButton> SNovaMainMenuInventory::GetDefaultFocusButton() const
{
	return RefuelButton;
}

/*----------------------------------------------------
    Resource list
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuInventory::GenerateResourceItem(const UNovaResource* Resource)
{
	const FNovaMainTheme&   Theme       = FNovaStyleSet::GetMainTheme();
	const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();

	// clang-format off
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.Padding(ButtonTheme.IconPadding)
		.AutoWidth()
		[
			SNew(SBorder)
			.BorderImage(new FSlateNoResource)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(0)
			[
				SNew(SImage)
				.Image(this, &SNovaMainMenuInventory::GetResourceIcon, Resource)
			]
		]

		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.MainFont)
			.Text(Resource->Name)
		];
	// clang-format on
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
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuInventory::OnTradeWithSlot(int32 Index, ENovaResourceType Type)
{
	NCHECK(IsValid(SpacecraftPawn));
	NCHECK(IsValid(GameState));
	NCHECK(IsValid(GameState->GetCurrentArea()));

	const FNovaCompartment&     Compartment = SpacecraftPawn->GetCompartment(Index);
	const FNovaSpacecraftCargo& Cargo       = Compartment.GetCargo(Type);

	// Valid resource in hold - allow trading it directly
	if (IsValid(Cargo.Resource))
	{
		TradingModalPanel->StartTrade(SpacecraftPawn, Cargo.Resource, Index);
	}

	// Cargo hold is empty, pick a resource to buy first
	else
	{
		TSharedPtr<SHorizontalBox> TradeBox;

		auto BuyResource = [=]()
		{
			TradingModalPanel->StartTrade(SpacecraftPawn, ResourceListView->GetSelectedItem(), Index);
		};

		// clang-format off
		SAssignNew(TradeBox, SHorizontalBox)

			+ SHorizontalBox::Slot()
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				ResourceListView.ToSharedRef()
			]

			+ SHorizontalBox::Slot();
		// clang-format on

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

#undef LOCTEXT_NAMESPACE
