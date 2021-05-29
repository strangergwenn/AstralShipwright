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

#include "Nova/UI/Widget/NovaFadingWidget.h"

#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuInventory"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuInventory::SNovaMainMenuInventory() : PC(nullptr), SpacecraftPawn(nullptr)
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
					SNew(SProgressBar)
					.Style(&Theme.ProgressBarStyle)
					.Percent(0.5f)
				]
			
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNovaNew(SNovaButton)
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
				//.HelpText()
				.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([=]()
				{
					return IsValidCompartment();
				})))
				//.OnClicked(this, &SNovaMainMenuAssembly::OnCompartmentSelected, Index)
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
					.VAlign(VAlign_Top)
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
								SNew(SNovaText)
								.Text(FNovaTextGetter::CreateLambda([=]() -> FText
								{
									if (IsValidCompartment())
									{
										const FNovaCompartment& Compartment = SpacecraftPawn->GetCompartment(Index);
										const FNovaSpacecraftCargo& Cargo = Compartment.GetCargo(Type);
										int32 Amount = Cargo.Amount;
										int32 Capacity = Compartment.GetCargoCapacity(Type);

										return FText::FromString(FText::AsNumber(Amount).ToString() + " / " + FText::AsNumber(Capacity).ToString());
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

	// BUild the procedural cargo lines
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
}

TSharedPtr<SNovaButton> SNovaMainMenuInventory::GetDefaultFocusButton() const
{
	return nullptr;
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

#undef LOCTEXT_NAMESPACE
