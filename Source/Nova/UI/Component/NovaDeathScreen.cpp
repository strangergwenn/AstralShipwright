// Nova project - Gwennaël Arbona

#include "NovaDeathScreen.h"

#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/Nova.h"


#define LOCTEXT_NAMESPACE "SNovaDeathScreen"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

void SNovaDeathScreen::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager = InArgs._MenuManager;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FNovaStyleSet::GetBrush("Common/SB_Black"))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SNovaDeathScreen::GetDamageTitleText)
				.TextStyle(&Theme.TitleFont)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SNovaDeathScreen::GetDamageText)
				.TextStyle(&Theme.SubtitleFont)
			]
		]
	];

	// Settings
	DisplayDuration = 3.0f;

	// Initialize
	PlayerWaiting = false;
	Hide();
}


/*----------------------------------------------------
	Interaction
----------------------------------------------------*/

void SNovaDeathScreen::Show(ENovaDamageType Type, FSimpleDelegate Callback)
{
	SetVisibility(EVisibility::Visible);

	PlayerWaiting = true;
	CurrentDisplayTime = 0;
	CurrentDamageType = Type;
	OnFinished = Callback;
}

void SNovaDeathScreen::Hide()
{
	SetVisibility(EVisibility::Hidden);
}

void SNovaDeathScreen::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update display time
	if (PlayerWaiting && CurrentDisplayTime > DisplayDuration)
	{
		OnFinished.ExecuteIfBound();
		PlayerWaiting = false;
	}
	CurrentDisplayTime += DeltaTime;
}


/*----------------------------------------------------
	Content callbacks
----------------------------------------------------*/

FText SNovaDeathScreen::GetDamageTitleText() const
{
	FText ResultText;

	switch (CurrentDamageType)
	{
	case ENovaDamageType::None:
	case ENovaDamageType::Generic:    ResultText = LOCTEXT("NoneTitle", "Unplanned disassembly");   break;
	case ENovaDamageType::OutOfWorld: ResultText = LOCTEXT("OutOfWorldTitle", "Lost to the abyss"); break;
	case ENovaDamageType::Wipe:       ResultText = LOCTEXT("WipeTitle", "Disaster");                break;
	}

	return ResultText.ToUpper();
}

FText SNovaDeathScreen::GetDamageText() const
{
	FText ResultText;

	switch (CurrentDamageType)
	{
	case ENovaDamageType::None:
	case ENovaDamageType::Generic:    ResultText = LOCTEXT("None", "You lost your life force");                       break;
	case ENovaDamageType::OutOfWorld: ResultText = LOCTEXT("OutOfWorld", "You were lost in the depths of the world"); break;
	case ENovaDamageType::Wipe:       ResultText = LOCTEXT("Wipe", "This timeline has come to an end");               break;
	}

	return ResultText;
}


#undef LOCTEXT_NAMESPACE
