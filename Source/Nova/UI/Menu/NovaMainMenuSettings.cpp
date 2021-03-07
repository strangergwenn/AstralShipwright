// Nova project - Gwennaël Arbona

#include "NovaMainMenuSettings.h"
#include "NovaMainMenu.h"

#include "Nova/Game/NovaGameUserSettings.h"

#include "Nova/Player/NovaGameViewportClient.h"
#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/UI/Widget/NovaButton.h"
#include "Nova/UI/Widget/NovaSlider.h"
#include "Nova/UI/Widget/NovaKeyBinding.h"
#include "Nova/UI/Widget/NovaModalPanel.h"

#include "Nova/Nova.h"

#include "GameFramework/InputSettings.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuSettings"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaMainMenuSettings::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot
	[
		SAssignNew(MainContainer, SScrollBox)
		.Style(&Theme.ScrollBoxStyle)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.AnimateWheelScrolling(true)

		+ SScrollBox::Slot()
		[
			SNew(SHorizontalBox)

			// Game settings panel
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.Padding(Theme.ContentPadding)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.SubtitleFont)
					.Text(LOCTEXT("Game", "Game settings"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.TextStyle(&Theme.MainFont)
					.Text(LOCTEXT("GameLanguage", "Game language"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaAssignNew(CultureListView, SNovaModalListView<TSharedPtr<FString>>)
					.Panel(this)
					.TitleText(LOCTEXT("GameLanguageTitle", "Game language"))
					.HelpText(LOCTEXT("GameLanguageHelp", "Choose a new language to use in the game"))
					.ItemsSource(&CultureList)
					.OnGenerateItem(this, &SNovaMainMenuSettings::GenerateCultureItem)
					.OnGenerateName(this, &SNovaMainMenuSettings::GetCultureName)
					.OnGenerateTooltip(this, &SNovaMainMenuSettings::GenerateCultureTooltip)
					.OnSelectionChanged(this, &SNovaMainMenuSettings::OnSelectedCultureChanged)
					.ButtonSize("DefaultButtonSize")
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(GameplayContainer, SVerticalBox)
				]
			]

			// Video settings panel
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.Padding(Theme.ContentPadding)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.SubtitleFont)
					.Text(LOCTEXT("Display", "Display settings"))
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
				]
			
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaNew(SNovaButton)
					.Text(LOCTEXT("ApplyResolution", "Apply video settings"))
					.HelpText(LOCTEXT("ApplyResolutionHelp", "Apply the currently selected resolution & screen mode"))
					.Action(FNovaPlayerInput::MenuPrimary)
					.OnClicked(this, &SNovaMainMenuSettings::OnApplyVideoSettings)
					.Enabled(this, &SNovaMainMenuSettings::IsApplyVideoSettingsEnabled)
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.TextStyle(&Theme.MainFont)
					.Text(LOCTEXT("VideoResolution", "Video resolution"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaAssignNew(ResolutionListView, SNovaModalListView<TSharedPtr<FScreenResolutionRHI>>)
					.TitleText(LOCTEXT("ChooseResolutionTitle", "Video resolution"))
					.HelpText(LOCTEXT("ChooseResolution", "Choose a new video resolution"))
					.Panel(this)
					.ItemsSource(&ResolutionList)
					.OnGenerateItem(this, &SNovaMainMenuSettings::GenerateResolutionItem)
					.OnGenerateName(this, &SNovaMainMenuSettings::GetResolutionName)
					.OnGenerateTooltip(this, &SNovaMainMenuSettings::GenerateResolutionTooltip)
					.OnSelectionChanged(this, &SNovaMainMenuSettings::OnSelectedResolutionChanged)
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
					.ButtonSize("DefaultButtonSize")
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaAssignNew(FullscreenButton, SNovaButton)
					.Toggle(true)
					.Text(LOCTEXT("Fullscreen", "Fullscreen"))
					.HelpText(LOCTEXT("FullscreenHelp", "Set the game to fullscreen or windowed"))
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
				]
				
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaAssignNew(VSyncButton, SNovaButton)
					.Toggle(true)
					.Text(LOCTEXT("VerticalSync", "V-Sync"))
					.HelpText(LOCTEXT("VerticalSyncHelp", "Enable V-Sync to prevent tearing artifacts"))
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
				]
				
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaAssignNew(HDRButton, SNovaButton)
					.Toggle(true)
					.Text(LOCTEXT("HDR", "HDR"))
					.HelpText(LOCTEXT("HDRHelp", "Enable HDR output if your system is compatible, requires fullscreen"))
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
					.Enabled(this, &SNovaMainMenuSettings::IsHDRSupported)
				]
			]

			// Graphics settings
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.Padding(Theme.ContentPadding)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.SubtitleFont)
					.Text(LOCTEXT("Graphics", "Quality settings"))
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(GraphicsContainer, SVerticalBox)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaAssignNew(RaytracedShadowsButton, SNovaButton)
					.Toggle(true)
					.Text(LOCTEXT("EnableRaytracedShadows", "Enable raytraced shadows"))
					.HelpText(LOCTEXT("EnableRaytracedShadowsHelp", "Enable the use of DXR hardware for raytraced shadows"))
					.OnClicked(this, &SNovaMainMenuSettings::OnRaytracedShadowsToggled)
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
					.Enabled(this, &SNovaMainMenuSettings::IsRaytracingSupported)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaAssignNew(RaytracedAOutton, SNovaButton)
					.Toggle(true)
					.Text(LOCTEXT("EnableRaytracedSAO", "Enable raytraced AO"))
					.HelpText(LOCTEXT("EnableRaytracedAOHelp", "Enable the use of DXR hardware for raytraced ambient occlusion"))
					.OnClicked(this, &SNovaMainMenuSettings::OnRaytracedAOToggled)
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
					.Enabled(this, &SNovaMainMenuSettings::IsRaytracingSupported)
				]

				/*+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaAssignNew(RaytracedReflectionsButton, SNovaButton)
					.Toggle(true)
					.Text(LOCTEXT("EnableRaytracedReflections", "Enable raytraced reflections"))
					.HelpText(LOCTEXT("EnableRaytracedReflectionsHelp", "Enable the use of DXR hardware for raytraced reflections"))
					.OnClicked(this, &SNovaMainMenuSettings::OnRaytracedReflectionsToggled)
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
					.Enabled(this, &SNovaMainMenuSettings::IsRaytracingSupported)
				]*/

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaAssignNew(SSGIButton, SNovaButton)
					.Toggle(true)
					.Text(LOCTEXT("EnableSSGI", "Enable SSGI"))
					.HelpText(LOCTEXT("EnableSSGIHelp", "Enable Screen-Space Global Illumination for improved lighting"))
					.OnClicked(this, &SNovaMainMenuSettings::OnSSGIToggled)
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaAssignNew(CinematicBloomButton, SNovaButton)
					.Toggle(true)
					.Text(LOCTEXT("EnableCinematicBloom", "Cinematic bloom"))
					.HelpText(LOCTEXT("EnableCinematicBloomHelp", "Enable convolution bloom for improved bloom quality"))
					.OnClicked(this, &SNovaMainMenuSettings::OnCinematicBloomToggled)
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
				]
			]

			// Key bindings panel
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.Padding(Theme.ContentPadding)
			[
				SAssignNew(BindingsContainer, SVerticalBox)
				.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
			]
		]
	];

	// Input mappings
	/*Bindings.Add(MakeShareable((new FNovaKeyBinding(LOCTEXT("MoveYPlus", "Move forward")))
		->Axis(FNovaPlayerInput::MoveX, 1.0f)
		->Default(EKeys::W)
	));
	Bindings.Add(MakeShareable((new FNovaKeyBinding(LOCTEXT("MoveXPlus", "Move left")))
		->Axis(FNovaPlayerInput::MoveY, -1.0f)
		->Default(EKeys::A)
	));
	Bindings.Add(MakeShareable((new FNovaKeyBinding(LOCTEXT("MoveYNeg", "Move backward")))
		->Axis(FNovaPlayerInput::MoveX, -1.0f)
		->Default(EKeys::S)
	));
	Bindings.Add(MakeShareable((new FNovaKeyBinding(LOCTEXT("MoveXNeg", "Move right")))
		->Axis(FNovaPlayerInput::MoveY, 1.0f)
		->Default(EKeys::D)
	));*/
	Bindings.Add(MakeShareable((new FNovaKeyBinding(LOCTEXT("MenuToggle", "Toggle menu / exit game")))
		->Action(FNovaPlayerInput::MenuToggle)
		->Default(EKeys::Escape)
	));
	Bindings.Add(MakeShareable((new FNovaKeyBinding(LOCTEXT("MenuNextTab", "Next tab")))
		->Action(FNovaPlayerInput::MenuNextTab)
		->Default(EKeys::PageDown)
	));
	Bindings.Add(MakeShareable((new FNovaKeyBinding(LOCTEXT("MenuPreviousTab", "Previous tab")))
		->Action(FNovaPlayerInput::MenuPreviousTab)
		->Default(EKeys::PageUp)
	));
	Bindings.Add(MakeShareable((new FNovaKeyBinding(LOCTEXT("MenuZoomIn", "Zoom in")))
		->Action(FNovaPlayerInput::MenuZoomIn)
		->Default(EKeys::MouseScrollUp)
	));
	Bindings.Add(MakeShareable((new FNovaKeyBinding(LOCTEXT("MenuZoomOut", "Zoom out")))
		->Action(FNovaPlayerInput::MenuZoomOut)
		->Default(EKeys::MouseScrollDown)
	));

	// Build culture list
	TArray<FString> CultureNames = FTextLocalizationManager::Get().GetLocalizedCultureNames(ELocalizationLoadFlags::Game);
	TArray<FCultureRef> CultureRefList = FInternationalization::Get().GetAvailableCultures(CultureNames, false);
	for (FCultureRef Culture : CultureRefList)
	{
		CultureList.Add(MakeShared<FString>(Culture->GetName()));
	}

	// Build gameplay options
	FOVSlider = AddSettingSlider(GameplayContainer, LOCTEXT("FOV", "Field of view ({value}°)"),
		LOCTEXT("FOVHelp", "Set vertical field of view in degrees"),
		FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnFOVChanged),
		80, 110, 5);

	if (!MenuManager->IsOnConsole())
	{
		// Build graphics settings
		ViewDistanceSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("ViewDistanceQuality", "View distance"),
			LOCTEXT("ViewDistanceQualityHelp", "Set the distance over which game objects will be rendered"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnViewDistanceChanged));
		ShadowSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("ShadowQuality", "Shadow quality"),
			LOCTEXT("ShadowQualityHelp", "Set the quality of shadows"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnShadowChanged));
		EffectsSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("EffectsQuality", "Effects quality"),
			LOCTEXT("EffectsQualityHelp", "Set the quality of rendering effects"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnEffectsChanged));
		PostProcessSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("PostProcessQuality", "Post-processing quality"),
			LOCTEXT("PostProcessQualityHelp", "Set the distance over which game objects will be rendered"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnPostProcessChanged));
		AntiAliasingSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("AntiAliasingQuality", "Anti-aliasing quality"),
			LOCTEXT("AntiAliasingQualityHelp", "Set the quality of the anti-aliasing process"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnAntiAliasingChanged));
		ScreenPercentageSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("ScreenPercentage", "Screen percentage ({value}%)"),
			LOCTEXT("ScreenPercentageHelp", "Render the game at a higher or lower resolution compared to the display"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnScreenPercentageChanged),
			50, 150, 10);

		// Build key bindings title
		BindingsContainer->ClearChildren();
		BindingsContainer->AddSlot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.SubtitleFont)
			.Text(LOCTEXT("KeyBindings", "Key bindings"))
		];

		// Build key bindings panel
		for (TSharedPtr<FNovaKeyBinding> Binding : Bindings)
		{
			BindingsContainer->AddSlot()
			.AutoHeight()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.TextStyle(&Theme.MainFont)
					.Text(Binding->GetBindingName())
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNovaNew(SNovaKeyBinding)
						.Binding(Binding)
						.OnKeyBindingChanged(this, &SNovaMainMenuSettings::OnKeyBindingChanged, Binding)
					]
				
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNovaNew(SNovaButton)
						.Size("SmallButtonSize")
						.HelpText(LOCTEXT("ResetBinding", "Reset this key binding to its default value"))
						.Icon(FNovaStyleSet::GetBrush("Icon/SB_Remove"))
						.OnClicked(this, &SNovaMainMenuSettings::OnKeyBindingReset, Binding)
					]
				]
			];
		}

		ModalPanel = Menu->CreateModalPanel(this);
	}
	// clang-format on
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuSettings::Show()
{
	SNovaTabPanel::Show();

	GameUserSettings = Cast<UNovaGameUserSettings>(GEngine->GetGameUserSettings());

	// Graphics settings
	if (!MenuManager->IsOnConsole())
	{
		ViewDistanceSlider->SetCurrentValue(GameUserSettings->GetViewDistanceQuality());
		ShadowSlider->SetCurrentValue(GameUserSettings->GetShadowQuality());
		EffectsSlider->SetCurrentValue(GameUserSettings->GetVisualEffectQuality());
		PostProcessSlider->SetCurrentValue(GameUserSettings->GetPostProcessingQuality());
		AntiAliasingSlider->SetCurrentValue(GameUserSettings->GetAntiAliasingQuality());
		ScreenPercentageSlider->SetCurrentValue(GameUserSettings->ScreenPercentage);

		RaytracedShadowsButton->SetActive(GameUserSettings->EnableRaytracedShadows);
		RaytracedAOutton->SetActive(GameUserSettings->EnableRaytracedAO);
		// RaytracedReflectionsButton->SetActive(GameUserSettings->EnableRaytracedReflections);
		SSGIButton->SetActive(GameUserSettings->EnableSSGI);
		CinematicBloomButton->SetActive(GameUserSettings->EnableCinematicBloom);

		UpdateResolutionList();
	}

	// Culture settings
	for (TSharedPtr<FString> Culture : CultureList)
	{
		if (*Culture == FInternationalization::Get().GetCurrentCulture()->GetName())
		{
			SelectedCulture = Culture;
		}
	}
	CultureListView->Refresh(CultureList.Find(SelectedCulture));

	// Gameplay options
	FOVSlider->SetCurrentValue(GameUserSettings->FOV);
}

void SNovaMainMenuSettings::Hide()
{
	ModalPanel->Cancel();

	SNovaTabPanel::Hide();
}

TSharedPtr<SNovaButton> SNovaMainMenuSettings::GetDefaultFocusButton() const
{
	if (MenuManager->IsOnConsole())
	{
		return CultureListView;
	}
	else
	{
		return ResolutionListView;
	}
}

/*----------------------------------------------------
    Input management
----------------------------------------------------*/

void SNovaMainMenuSettings::OnKeyBindingChanged(FKey PreviousKey, FKey NewKey, TSharedPtr<FNovaKeyBinding> Binding)
{
	TArray<TSharedPtr<FNovaKeyBinding>> BindConflicts;

	// Remove a key
	if (!NewKey.IsValid())
	{
		UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
		Binding->Save();
		InputSettings->SaveKeyMappings();
		return;
	}

	// Handle conflicts
	else if (IsAlreadyUsed(BindConflicts, NewKey, Binding))
	{
		FText ConflictKeys;

		for (TSharedPtr<FNovaKeyBinding> Bind : BindConflicts)
		{
			if (ConflictKeys.IsEmpty())
			{
				ConflictKeys = FText::Format(LOCTEXT("ConflictsKeysFirst", "'{0}'"), Bind->GetBindingName());
			}
			else
			{
				ConflictKeys = FText::Format(LOCTEXT("ConflictsKeysOthers", "{0}, '{1}'"), ConflictKeys, Bind->GetBindingName());
			}
		}

		FText ConflictDetailsText = FText::FormatNamed(LOCTEXT("ConflictDetailsText",
														   "'{original}' is already used for {new}. Do you want to assign this key to "
														   "'{binding}' only, ignore the conflict, or cancel the assignment?"),
			TEXT("original"), FText::FromString(NewKey.ToString()), TEXT("new"), ConflictKeys, TEXT("binding"), Binding->GetBindingName());

		ModalPanel->Show(LOCTEXT("ConfirmKeyConflict", "Key already in use"), ConflictDetailsText,
			FSimpleDelegate::CreateSP(this, &SNovaMainMenuSettings::ApplyNewBinding, Binding, true),
			FSimpleDelegate::CreateSP(this, &SNovaMainMenuSettings::ApplyNewBinding, Binding, false),
			FSimpleDelegate::CreateSP(this, &SNovaMainMenuSettings::CancelNewBinding, Binding, PreviousKey));
	}
	else
	{
		ApplyNewBinding(Binding, false);
	}
}

void SNovaMainMenuSettings::OnKeyBindingReset(TSharedPtr<FNovaKeyBinding> Binding)
{
	NCHECK(Binding.IsValid());
	Binding->ResetKey();
}

void SNovaMainMenuSettings::ApplyNewBinding(TSharedPtr<FNovaKeyBinding> BindingThatChanged, bool Replace)
{
	if (Replace)
	{
		FKey KeyToErase = BindingThatChanged->GetKey();
		for (TSharedPtr<FNovaKeyBinding> Bind : Bindings)
		{
			if (BindingThatChanged != Bind)
			{
				if (Bind->GetKey() == KeyToErase)
				{
					Bind->SetKey(FKey());
					Bind->Save();
				}
			}
		}
	}

	// Apply and save
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	BindingThatChanged->Save();
	InputSettings->SaveKeyMappings();

	// Update the game
	for (TObjectIterator<UPlayerInput> It; It; ++It)
	{
		It->ForceRebuildingKeyMaps(true);
	}
	Menu->UpdateKeyBindings();
}

void SNovaMainMenuSettings::CancelNewBinding(TSharedPtr<struct FNovaKeyBinding> BindingThatChanged, FKey PreviousKey)
{
	BindingThatChanged->SetKey(PreviousKey);
}

bool SNovaMainMenuSettings::IsAlreadyUsed(
	TArray<TSharedPtr<FNovaKeyBinding>>& BindConflicts, FKey Key, TSharedPtr<FNovaKeyBinding> ExcludeBinding)
{
	for (TSharedPtr<FNovaKeyBinding> Bind : Bindings)
	{
		if (Bind->GetKey() == Key && ExcludeBinding != Bind)
		{
			BindConflicts.Add(Bind);
		}
	}

	return BindConflicts.Num() > 0;
}

/*----------------------------------------------------
    Video management
----------------------------------------------------*/

TSharedPtr<SNovaSlider> SNovaMainMenuSettings::AddSettingSlider(TSharedPtr<SVerticalBox> Container, FText Text, FText HelpText,
	FOnFloatValueChanged Callback, float MinValue, float MaxValue, float ValueStep)
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	// clang-format off
	TSharedPtr<SNovaSlider> Slider = SNew(SNovaSlider)
		.Panel(this)
		.MinValue(MinValue)
		.MaxValue(MaxValue)
		.ValueStep(ValueStep)
		.HelpText(HelpText)
		.OnValueChanged(Callback);

	Container->AddSlot()
	.AutoHeight()
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		[
			SNew(STextBlock)
			.TextStyle(&Theme.MainFont)
			.Text_Lambda([=]()
			{
				return FText::FormatNamed(Text, TEXT("value"),
					FText::AsNumber(FMath::RoundToInt(Slider->GetCurrentValue())));
			})
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			Slider.ToSharedRef()
		]
	];
	// clang-format on

	return Slider;
}

void SNovaMainMenuSettings::UpdateResolutionList()
{
	// Initialize buttons
	LastConfirmedVideoResolution = GameUserSettings->GetLastConfirmedScreenResolution();
	LastConfirmedFullscreenMode  = GameUserSettings->GetLastConfirmedFullscreenMode();
	FullscreenButton->SetActive(LastConfirmedFullscreenMode != EWindowMode::Windowed);
	VSyncButton->SetActive(GameUserSettings->IsVSyncEnabled());
	HDRButton->SetActive(IsHDRSupported() && GameUserSettings->IsHDREnabled());

	// Get all resolution settings
	ResolutionList.Empty();
	SelectedResolution.Reset();
	FScreenResolutionArray Resolutions;
	FIntPoint              CurrentResolution = GEngine->GameViewport->Viewport->GetSizeXY();
	if (RHIGetAvailableResolutions(Resolutions, true))
	{
		for (const FScreenResolutionRHI& Resolution : Resolutions)
		{
			if (Resolution.Width >= 1280 && Resolution.Height >= 720)
			{
				TSharedPtr<FScreenResolutionRHI> NewResolution = MakeShareable(new FScreenResolutionRHI(Resolution));
				ResolutionList.Insert(NewResolution, 0);

				if (Resolution.Width == CurrentResolution.X && Resolution.Height == CurrentResolution.Y)
				{
					SelectedResolution = NewResolution;
				}
			}
		}
	}
	else
	{
		NERR("SNovaMainMenuSettings::Show : screen resolutions could not be obtained");
	}

	// Update list
	ResolutionListView->Refresh(ResolutionList.Find(SelectedResolution));
}

void SNovaMainMenuSettings::UpdateResolution(FIntPoint Resolution, bool Fullscreen)
{
	// Apply settings
	LastConfirmedVideoResolution = GameUserSettings->GetLastConfirmedScreenResolution();
	LastConfirmedFullscreenMode  = GameUserSettings->GetLastConfirmedFullscreenMode();
	GameUserSettings->SetScreenResolution(Resolution);
	GameUserSettings->SetFullscreenMode(Fullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed);
	GameUserSettings->ApplyResolutionSettings(true);

	NLOG("SNovaMainMenuSettings::UpdateResolution : new resolution is %s, fullscreen %d", *Resolution.ToString(),
		FullscreenButton->IsActive());

	// Confirm
	ModalPanel->Show(LOCTEXT("ConfirmResolution", "Confirm video settings ?"),
		LOCTEXT("ConfirmResolutionHelp", "Do you want to confirm and save the new video settings, or revert the changes ?"),
		FSimpleDelegate::CreateSP(this, &SNovaMainMenuSettings::ApplyVideoSettings), FSimpleDelegate(),
		FSimpleDelegate::CreateSP(this, &SNovaMainMenuSettings::RevertVideoSettings));
}

void SNovaMainMenuSettings::ApplyVideoSettings()
{
	NLOG("SNovaMainMenuSettings::ApplyVideoSettings");

	GameUserSettings->ConfirmVideoMode();
	GameUserSettings->SaveConfig();
	GEngine->SaveConfig();

	UpdateResolutionList();
}

void SNovaMainMenuSettings::RevertVideoSettings()
{
	NLOG("SNovaMainMenuSettings::RevertVideoSettings");

	GameUserSettings->SetScreenResolution(LastConfirmedVideoResolution);
	GameUserSettings->SetFullscreenMode(LastConfirmedFullscreenMode);
	GameUserSettings->ApplyResolutionSettings(true);

	GameUserSettings->ConfirmVideoMode();

	UpdateResolutionList();
}

/*----------------------------------------------------
    Callbacks (main)
----------------------------------------------------*/

EVisibility SNovaMainMenuSettings::GetPCVisibility() const
{
	return MenuManager->IsOnConsole() ? EVisibility::Collapsed : EVisibility::Visible;
}

bool SNovaMainMenuSettings::IsRaytracingSupported() const
{
	return IsRayTracingEnabled();
}

bool SNovaMainMenuSettings::IsHDRSupported() const
{
	return GameUserSettings->GetFullscreenMode() == EWindowMode::Fullscreen && GameUserSettings->SupportsHDRDisplayOutput() &&
		   IsHDRAllowed();
}

void SNovaMainMenuSettings::OnFOVChanged(float Value)
{
	GameUserSettings->FOV = Value;
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnViewDistanceChanged(float Value)
{
	GameUserSettings->SetViewDistanceQuality(Value);
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnShadowChanged(float Value)
{
	GameUserSettings->SetShadowQuality(Value);
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnEffectsChanged(float Value)
{
	GameUserSettings->SetVisualEffectQuality(Value);
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnPostProcessChanged(float Value)
{
	GameUserSettings->SetPostProcessingQuality(Value);
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnAntiAliasingChanged(float Value)
{
	GameUserSettings->SetAntiAliasingQuality(Value);
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnScreenPercentageChanged(float Value)
{
	GameUserSettings->ScreenPercentage = Value;
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnRaytracedShadowsToggled()
{
	GameUserSettings->EnableRaytracedShadows = RaytracedShadowsButton->IsActive();
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnRaytracedAOToggled()
{
	GameUserSettings->EnableRaytracedAO = RaytracedAOutton->IsActive();
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnRaytracedReflectionsToggled()
{
	GameUserSettings->EnableRaytracedReflections = RaytracedReflectionsButton->IsActive();
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnSSGIToggled()
{
	GameUserSettings->EnableSSGI = SSGIButton->IsActive();
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnCinematicBloomToggled()
{
	GameUserSettings->EnableCinematicBloom = CinematicBloomButton->IsActive();
	GameUserSettings->SaveSettings();
}

/*----------------------------------------------------
    Callbacks (culture)
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuSettings::GenerateCultureItem(TSharedPtr<FString> Culture)
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
			[
				SNew(SImage)
				.Image(this, &SNovaMainMenuSettings::GetCultureIcon, Culture)
			]
		]

		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.MainFont)
			.Text(GetCultureName(Culture))
		];
	// clang-format on
}

FText SNovaMainMenuSettings::GetCultureName(TSharedPtr<FString> Culture) const
{
	FCulturePtr CulturePtr = FInternationalization::Get().GetCulture(*Culture);
	FString     NativeName = CulturePtr->GetNativeName();
	return FText::FromString(NativeName.Left(1).ToUpper() + NativeName.RightChop(1));
}

const FSlateBrush* SNovaMainMenuSettings::GetCultureIcon(TSharedPtr<FString> Culture) const
{
	return FNovaStyleSet::GetBrush(*Culture == *SelectedCulture ? "Icon/SB_ListOn" : "Icon/SB_ListOff");
}

FText SNovaMainMenuSettings::GenerateCultureTooltip(TSharedPtr<FString> Culture)
{
	FCulturePtr CulturePtr   = FInternationalization::Get().GetCulture(*Culture);
	FString     NativeName   = CulturePtr->GetNativeName();
	FText       LanguageText = FText::FromString(NativeName.Left(1).ToUpper() + NativeName.RightChop(1));

	return FText::FormatNamed(LOCTEXT("CultureHelp", "Set the game language to {language}"), TEXT("language"), LanguageText);
}

void SNovaMainMenuSettings::OnSelectedCultureChanged(TSharedPtr<FString> Culture, int32 Index)
{
	NLOG("SNovaMainMenuSettings::OnSelectedCultureChanged : %s", **Culture.Get());
	SelectedCulture = Culture;

	OnApplyCultureSettings();
}

void SNovaMainMenuSettings::OnApplyCultureSettings()
{
	FInternationalization::Get().SetCurrentCulture(*SelectedCulture.Get());
	FTextLocalizationManager::Get().RefreshResources();

	GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), **SelectedCulture.Get(), GGameUserSettingsIni);
	GConfig->Flush(false, GEditorSettingsIni);
}

/*----------------------------------------------------
    Callbacks (video)
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuSettings::GenerateResolutionItem(TSharedPtr<FScreenResolutionRHI> Resolution)
{
	const FNovaMainTheme&   Theme       = FNovaStyleSet::GetMainTheme();
	const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();

	// clang-format off
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.Padding(ButtonTheme.IconPadding)
		.AutoWidth()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(this, &SNovaMainMenuSettings::GetResolutionIcon, Resolution)
			]
		]

		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.MainFont)
			.Text(GetResolutionName(Resolution))
		];
	// clang-format on
}

FText SNovaMainMenuSettings::GetResolutionName(TSharedPtr<struct FScreenResolutionRHI> Resolution) const
{
	return FText::FromString(FString::FromInt(Resolution->Width) + TEXT("x") + FString::FromInt(Resolution->Height));
}

const FSlateBrush* SNovaMainMenuSettings::GetResolutionIcon(TSharedPtr<struct FScreenResolutionRHI> Resolution) const
{
	return FNovaStyleSet::GetBrush(Resolution == SelectedResolution ? "Icon/SB_ListOn" : "Icon/SB_ListOff");
}

FText SNovaMainMenuSettings::GenerateResolutionTooltip(TSharedPtr<FScreenResolutionRHI> Resolution)
{
	FText ResolutionText = FText::FromString(FString::FromInt(Resolution->Width) + TEXT("x") + FString::FromInt(Resolution->Height));

	return FText::FormatNamed(LOCTEXT("ResolutionHelp", "Set the resolution to {resolution}"), TEXT("resolution"), ResolutionText);
}

void SNovaMainMenuSettings::OnSelectedResolutionChanged(TSharedPtr<FScreenResolutionRHI> Resolution, int32 Index)
{
	NLOG("SNovaMainMenuSettings::OnSelectedResolutionChanged : %dx%d", Resolution->Width, Resolution->Height);
	SelectedResolution = Resolution;
}

bool SNovaMainMenuSettings::IsApplyVideoSettingsEnabled() const
{
	bool IsFullscreen = LastConfirmedFullscreenMode != EWindowMode::Windowed;
	bool IsVsync      = GameUserSettings->IsVSyncEnabled();
	bool IsHDR        = GameUserSettings->IsHDREnabled();

	return ((SelectedResolution.IsValid() && (SelectedResolution->Width != LastConfirmedVideoResolution.X ||
												 SelectedResolution->Height != LastConfirmedVideoResolution.Y)) ||
			FullscreenButton->IsActive() != IsFullscreen || VSyncButton->IsActive() != IsVsync || HDRButton->IsActive() != IsHDR);
}

void SNovaMainMenuSettings::OnApplyVideoSettings()
{
	NLOG("SNovaMainMenuSettings::OnApplyVideoSettings");

	FIntPoint Resolution;
	NCHECK(SelectedResolution.IsValid());
	Resolution.X = SelectedResolution->Width;
	Resolution.Y = SelectedResolution->Height;

	UpdateResolution(Resolution, FullscreenButton->IsActive());

	GameUserSettings->SetVSyncEnabled(VSyncButton->IsActive());
	GameUserSettings->EnableHDRDisplayOutput(HDRButton->IsActive());
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

#undef LOCTEXT_NAMESPACE
