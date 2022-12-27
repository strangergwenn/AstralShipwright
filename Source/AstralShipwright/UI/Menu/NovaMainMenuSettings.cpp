// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuSettings.h"
#include "NovaMainMenu.h"

#include "Player/NovaPlayerController.h"
#include "Game/NovaGameUserSettings.h"
#include "Nova.h"

#include "Neutron/Player/NeutronGameViewportClient.h"
#include "Neutron/Settings/NeutronGameUserSettings.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/System/NeutronSoundManager.h"
#include "Neutron/UI/Widgets/NeutronButton.h"
#include "Neutron/UI/Widgets/NeutronSlider.h"
#include "Neutron/UI/Widgets/NeutronKeyBinding.h"
#include "Neutron/UI/Widgets/NeutronModalPanel.h"

#include "GameFramework/InputSettings.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuSettings"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaMainMenuSettings::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	MenuManager                    = InArgs._MenuManager;

	// Parent constructor
	SNeutronNavigationPanel::Construct(SNeutronNavigationPanel::FArguments().Menu(InArgs._Menu));

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
			.Padding(Theme.ContentPadding + FMargin(0, 20, 0, 0))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.HeadingFont)
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
					SNeutronAssignNew(CultureListView, SNeutronModalListView<TSharedPtr<FString>>)
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

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNeutronAssignNew(CrashReportButton, SNeutronButton)
					.Toggle(true)
					.Text(LOCTEXT("EnableCrashReport", "Enable crash reports"))
					.HelpText(LOCTEXT("EnableCrashReportHelp", "Send anonymous debugging information to Deimos Games when the game crashes"))
					.OnClicked(this, &SNovaMainMenuSettings::OnCrashReportToggled)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.HeadingFont)
					.Text(LOCTEXT("Sound", "Sound settings"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(SoundContainer, SVerticalBox)
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
					.TextStyle(&Theme.HeadingFont)
					.Text(LOCTEXT("Display", "Display settings"))
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
				]
			
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNeutronNew(SNeutronButton)
					.Text(LOCTEXT("ApplyResolution", "Apply video settings"))
					.HelpText(LOCTEXT("ApplyResolutionHelp", "Apply the currently selected resolution & screen mode"))
					.Action(FNeutronPlayerInput::MenuPrimary)
					.ActionFocusable(false)
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
					SNeutronAssignNew(ResolutionListView, SNeutronModalListView<TSharedPtr<FScreenResolutionRHI>>)
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
					SNeutronAssignNew(FullscreenButton, SNeutronButton)
					.Toggle(true)
					.Text(LOCTEXT("Fullscreen", "Fullscreen"))
					.HelpText(LOCTEXT("FullscreenHelp", "Set the game to fullscreen or windowed"))
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
				]
				
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNeutronAssignNew(VSyncButton, SNeutronButton)
					.Toggle(true)
					.Text(LOCTEXT("VerticalSync", "V-Sync"))
					.HelpText(LOCTEXT("VerticalSyncHelp", "Enable V-Sync to prevent tearing artifacts"))
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
				]
				
				/*+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNeutronAssignNew(HDRButton, SNeutronButton)
					.Toggle(true)
					.Text(LOCTEXT("HDR", "HDR"))
					.HelpText(LOCTEXT("HDRHelp", "Enable HDR output if your system is compatible, requires fullscreen"))
					.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
					.Enabled(this, &SNovaMainMenuSettings::IsHDRSupported)
				]*/

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ScalingContainer, SVerticalBox)
				]
			]

			// Graphics settings
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.Padding(Theme.ContentPadding)
			[
				SAssignNew(GraphicsContainer, SVerticalBox)
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
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuPrimary", "Primary action")))
		->Action(FNeutronPlayerInput::MenuPrimary)
		->Default(EKeys::E)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuSecondary", "Secondary action")))
		->Action(FNeutronPlayerInput::MenuSecondary)
		->Default(EKeys::Q)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuAltPrimary", "Primary option")))
		->Action(FNeutronPlayerInput::MenuAltPrimary)
		->Default(EKeys::RightMouseButton)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuAltSecondary", "Secondary option")))
		->Action(FNeutronPlayerInput::MenuAltSecondary)
		->Default(EKeys::W)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuToggle", "Exit menu / Exit game")))
		->Action(FNeutronPlayerInput::MenuToggle)
		->Default(EKeys::Escape)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuNextTab", "Next tab")))
		->Action(FNeutronPlayerInput::MenuNextTab)
		->Default(EKeys::PageDown)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuPreviousTab", "Previous tab")))
		->Action(FNeutronPlayerInput::MenuPreviousTab)
		->Default(EKeys::PageUp)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuNext", "Next item")))
		->Action(FNeutronPlayerInput::MenuNext)
		->Default(EKeys::D)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuPrevious", "Previous item")))
		->Action(FNeutronPlayerInput::MenuPrevious)
		->Default(EKeys::A)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuZoomIn", "Zoom in")))
		->Action(FNeutronPlayerInput::MenuZoomIn)
		->Default(EKeys::MouseScrollUp)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("MenuZoomOut", "Zoom out")))
		->Action(FNeutronPlayerInput::MenuZoomOut)
		->Default(EKeys::MouseScrollDown)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("Flight", "Flight")))
		->Action("Flight")
		->Default(EKeys::F1)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("Navigation", "Navigation")))
		->Action("Navigation")
		->Default(EKeys::F2)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("Operations", "Operations")))
		->Action("Operations")
		->Default(EKeys::F3)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("Assembly", "Assembly")))
		->Action("Assembly")
		->Default(EKeys::F4)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("Career", "Career")))
		->Action("Career")
		->Default(EKeys::F5)
	));
	Bindings.Add(MakeShareable((new FNeutronKeyBinding(LOCTEXT("Settings", "Settings")))
		->Action("Settings")
		->Default(EKeys::F10)
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
		LOCTEXT("FOVHelp", "Set the horizontal field of view in degrees"),
		FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnFOVChanged),
		80, 110, 5);
	
	// Build sound options
	MasterVolumeSlider = AddSettingSlider(SoundContainer, LOCTEXT("MasterVolume", "Master volume"),
		LOCTEXT("MasterVolumeHelp", "Set the master game volume"),
		FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnMasterVolumeChanged),
		0, 10, 1);
	UIVolumeSlider = AddSettingSlider(SoundContainer, LOCTEXT("UIVolume", "UI volume"),
		LOCTEXT("UIVolumeHelp", "Set the user interface volume"),
		FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnUIVolumeChanged),
		0, 10, 1);
	EffectsVolumeSlider = AddSettingSlider(SoundContainer, LOCTEXT("EffectsVolume", "Effects volume"),
		LOCTEXT("EffectsVolumeHelp", "Set the game effects volume"),
		FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnEffectsVolumeChanged),
		0, 10, 1);
	MusicVolumeSlider = AddSettingSlider(SoundContainer, LOCTEXT("MusicVolume", "Music volume"),
		LOCTEXT("MusicVolumeHelp", "Set the music volume"),
		FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnMusicVolumeChanged),
		0, 10, 1);

	if (!MenuManager->IsOnConsole())
	{		
		ScalingContainer->AddSlot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.HeadingFont)
			.Text(LOCTEXT("ResolutionScaling", "Resolution scaling"))
			.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
		];

		GraphicsContainer->AddSlot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.HeadingFont)
			.Text(LOCTEXT("Graphics", "Quality settings"))
			.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
		];

		// Build quality settings
		GlobalIlluminationSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("GlobalIlluminationQuality", "Global illumination quality"),
			LOCTEXT("GlobalIlluminationQualityHelp", "Set the quality of global illumination"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnGlobalIlluminationChanged));
		ShadowSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("ShadowQuality", "Shadow quality"),
			LOCTEXT("ShadowQualityHelp", "Set the quality of shadows"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnShadowChanged));
		EffectsSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("EffectsQuality", "Effects quality"),
			LOCTEXT("EffectsQualityHelp", "Set the quality of rendering effects"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnEffectsChanged));
		PostProcessSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("PostProcessQuality", "Post-processing quality"),
			LOCTEXT("PostProcessQualityHelp", "Set the quality of post-processing effects"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnPostProcessChanged));
		AntiAliasingSlider = AddSettingSlider(GraphicsContainer, LOCTEXT("AntiAliasingQuality", "Anti-aliasing quality"),
			LOCTEXT("AntiAliasingQualityHelp", "Set the quality of the anti-aliasing process"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnAntiAliasingChanged));
		ScreenPercentageSlider = AddSettingSlider(ScalingContainer, LOCTEXT("ResolutionScale", "Resolution scale ({value}%)"),
			LOCTEXT("ScreenPercentageHelp", "Render the game at a higher or lower resolution compared to the display"),
			FOnFloatValueChanged::CreateSP(this, &SNovaMainMenuSettings::OnScreenPercentageChanged),
			50, 150, 10, TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenuSettings::IsScreenPercentageSupported)));
		
		// Build additional options
		GraphicsContainer->AddSlot()
		.AutoHeight()
		[
			SNeutronAssignNew(CameraDegradationButton, SNeutronButton)
			.Toggle(true)
			.Text(LOCTEXT("EnableCameraDegradation", "Enable camera noise"))
			.HelpText(LOCTEXT("EnableCameraDegradationHelp", "Enable the post-processing layer simulating camera bombardment"))
			.OnClicked(this, &SNovaMainMenuSettings::OnSpaceFeelToggled)
		];
		GraphicsContainer->AddSlot()
		.AutoHeight()
		[
			SNeutronAssignNew(MotionBlurButton, SNeutronButton)
			.Toggle(true)
			.Text(LOCTEXT("EnableMotionBlur", "Enable motion blur"))
			.HelpText(LOCTEXT("EnableMotionBlurHelp", "Enable motion blur"))
			.OnClicked(this, &SNovaMainMenuSettings::OnMotionBlurToggled)
		];

		GraphicsContainer->AddSlot()
		.AutoHeight()
		[
			SNeutronAssignNew(NaniteButton, SNeutronButton)
			.Toggle(true)
			.Text(LOCTEXT("Nanite", "Enable Nanite"))
			.HelpText(LOCTEXT("NaniteHelp", "Enable high-density Nanite geometry"))
			.OnClicked(this, &SNovaMainMenuSettings::OnNaniteToggled)
			.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
			.Enabled(this, &SNovaMainMenuSettings::IsNaniteSupported)
		];

		GraphicsContainer->AddSlot()
		.AutoHeight()
		[
			SNeutronAssignNew(LumenButton, SNeutronButton)
			.Toggle(true)
			.Text(LOCTEXT("Lumen", "Enable Lumen"))
			.HelpText(LOCTEXT("LumenHelp", "Enable the high-quality Lumen lighting"))
			.OnClicked(this, &SNovaMainMenuSettings::OnLumenToggled)
			.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
			.Enabled(this, &SNovaMainMenuSettings::IsLumenSupported)
		];

		GraphicsContainer->AddSlot()
		.AutoHeight()
		[
			SNeutronAssignNew(LumenHWRTButton, SNeutronButton)
			.Toggle(true)
			.Text(LOCTEXT("LumenRayTracing", "Enable Lumen raytracing support"))
			.HelpText(LOCTEXT("LumenRayTracingHelp", "Enable hardware raytracing support for Lumen, improving quality on DXR hardware"))
			.OnClicked(this, &SNovaMainMenuSettings::OnLumenHWRTToggled)
			.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
			.Enabled(this, &SNovaMainMenuSettings::IsLumenHWRTSupported)
		];

		GraphicsContainer->AddSlot()
		.AutoHeight()
		[
			SNeutronAssignNew(VirtualShadowsButton, SNeutronButton)
			.Toggle(true)
			.Text(LOCTEXT("VirtualShadows", "Enable virtual shadows"))
			.HelpText(LOCTEXT("VirtualShadowsHelp", "Enable virtual shadow maps for enhanced shadows"))
			.OnClicked(this, &SNovaMainMenuSettings::OnVirtualShadowsToggled)
			.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
			.Enabled(this, &SNovaMainMenuSettings::IsVirtualShadowSupported)
		];
		
		ScalingContainer->AddSlot()
		.AutoHeight()
		[
			SNeutronAssignNew(TSRButton, SNeutronButton)
			.Toggle(true)
			.Text(LOCTEXT("TSR", "Enable TSR"))
			.HelpText(LOCTEXT("TSRHelp", "Enable temporal super resolution for improved screen percentage quality"))
			.OnClicked(this, &SNovaMainMenuSettings::OnTSRToggled)
			.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
			.Enabled(this, &SNovaMainMenuSettings::IsTSRSupported)
		];
		
		ScalingContainer->AddSlot()
		.AutoHeight()
		[
			SNeutronAssignNew(DLSSButton, SNeutronButton)
			.Toggle(true)
			.Text(LOCTEXT("DLSS", "Enable DLSS"))
			.HelpText(LOCTEXT("DLSSHelp", "Enable DLSS support to improve performance on NVIDIA RTX hardware"))
			.OnClicked(this, &SNovaMainMenuSettings::OnDLSSToggled)
			.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
			.Enabled(this, &SNovaMainMenuSettings::IsDLSSSupported)
		];
		
		GraphicsContainer->AddSlot()
		.AutoHeight()
		[
			SNeutronAssignNew(CinematicBloomButton, SNeutronButton)
			.Toggle(true)
			.Text(LOCTEXT("EnableCinematicBloom", "Cinematic bloom"))
			.HelpText(LOCTEXT("EnableCinematicBloomHelp", "Enable convolution bloom for improved bloom quality"))
			.OnClicked(this, &SNovaMainMenuSettings::OnCinematicBloomToggled)
			.Visibility(this, &SNovaMainMenuSettings::GetPCVisibility)
		];

		// Build key bindings title
		BindingsContainer->ClearChildren();
		BindingsContainer->AddSlot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.HeadingFont)
			.Text(LOCTEXT("KeyBindings", "Key bindings"))
		];

		// Build key bindings panel
		for (TSharedPtr<FNeutronKeyBinding> Binding : Bindings)
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
						SNeutronNew(SNeutronKeyBinding)
						.Binding(Binding)
						.OnKeyBindingChanged(this, &SNovaMainMenuSettings::OnKeyBindingChanged, Binding)
						.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Edit"))
					]
				
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNeutronNew(SNeutronButton)
						.Size("SmallButtonSize")
						.HelpText(LOCTEXT("ResetBinding", "Reset this key binding to its default value"))
						.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Remove"))
						.OnClicked(this, &SNovaMainMenuSettings::OnKeyBindingReset, Binding)
					]
				]
			];
		}

		ModalPanel = Menu->CreateModalPanel();
	}
	// clang-format on
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuSettings::Show()
{
	SNeutronTabPanel::Show();

	GameUserSettings = Cast<UNeutronGameUserSettings>(GEngine->GetGameUserSettings());

	// Graphics settings
	if (!MenuManager->IsOnConsole())
	{
		GlobalIlluminationSlider->SetCurrentValue(GameUserSettings->GetGlobalIlluminationQuality());
		ShadowSlider->SetCurrentValue(GameUserSettings->GetShadowQuality());
		EffectsSlider->SetCurrentValue(GameUserSettings->GetVisualEffectQuality());
		PostProcessSlider->SetCurrentValue(GameUserSettings->GetPostProcessingQuality());
		AntiAliasingSlider->SetCurrentValue(GameUserSettings->GetAntiAliasingQuality());
		ScreenPercentageSlider->SetCurrentValue(GameUserSettings->ScreenPercentage);

		TSRButton->SetActive(GameUserSettings->EnableTSR);
		DLSSButton->SetActive(GameUserSettings->EnableDLSS && GameUserSettings->IsDLSSSupported());
		CameraDegradationButton->SetActive(Cast<UNovaGameUserSettings>(GameUserSettings)->EnableCameraDegradation);
		MotionBlurButton->SetActive(Cast<UNovaGameUserSettings>(GameUserSettings)->MotionBlurAmount > 0);
		NaniteButton->SetActive(GameUserSettings->EnableNanite && GameUserSettings->IsNaniteSupported());
		LumenButton->SetActive(GameUserSettings->EnableLumen && GameUserSettings->IsLumenSupported());
		LumenHWRTButton->SetActive(GameUserSettings->EnableLumenHWRT && GameUserSettings->IsLumenHWRTSupported());
		VirtualShadowsButton->SetActive(GameUserSettings->EnableVirtualShadows && GameUserSettings->IsVirtualShadowSupported());
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
	CrashReportButton->SetActive(GameUserSettings->EnableCrashReports);

	// Sound options
	MasterVolumeSlider->SetCurrentValue(GameUserSettings->MasterVolume);
	UIVolumeSlider->SetCurrentValue(GameUserSettings->UIVolume);
	EffectsVolumeSlider->SetCurrentValue(GameUserSettings->EffectsVolume);
	MusicVolumeSlider->SetCurrentValue(GameUserSettings->MusicVolume);
}

TSharedPtr<SNeutronButton> SNovaMainMenuSettings::GetDefaultFocusButton() const
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

void SNovaMainMenuSettings::OnKeyBindingChanged(FKey PreviousKey, FKey NewKey, TSharedPtr<FNeutronKeyBinding> Binding)
{
	TArray<TSharedPtr<FNeutronKeyBinding>> BindConflicts;

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

		for (TSharedPtr<FNeutronKeyBinding> Bind : BindConflicts)
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

void SNovaMainMenuSettings::OnKeyBindingReset(TSharedPtr<FNeutronKeyBinding> Binding)
{
	NCHECK(Binding.IsValid());
	Binding->ResetKey();
}

void SNovaMainMenuSettings::ApplyNewBinding(TSharedPtr<FNeutronKeyBinding> BindingThatChanged, bool Replace)
{
	if (Replace)
	{
		FKey KeyToErase = BindingThatChanged->GetKey();
		for (TSharedPtr<FNeutronKeyBinding> Bind : Bindings)
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

void SNovaMainMenuSettings::CancelNewBinding(TSharedPtr<struct FNeutronKeyBinding> BindingThatChanged, FKey PreviousKey)
{
	BindingThatChanged->SetKey(PreviousKey);
}

bool SNovaMainMenuSettings::IsAlreadyUsed(
	TArray<TSharedPtr<FNeutronKeyBinding>>& BindConflicts, FKey Key, TSharedPtr<FNeutronKeyBinding> ExcludeBinding)
{
	for (TSharedPtr<FNeutronKeyBinding> Bind : Bindings)
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

TSharedPtr<SNeutronSlider> SNovaMainMenuSettings::AddSettingSlider(TSharedPtr<SVerticalBox> Container, FText Text, FText HelpText,
	FOnFloatValueChanged Callback, float MinValue, float MaxValue, float ValueStep, TAttribute<bool> Enabled)
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	// clang-format off
	TSharedPtr<SNeutronSlider> Slider = SNeutronNew(SNeutronSlider)
		.MinValue(MinValue)
		.MaxValue(MaxValue)
		.ValueStep(ValueStep)
		.HelpText(HelpText)
		.OnValueChanged(Callback)
		.Enabled(Enabled);

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
	// HDRButton->SetActive(IsHDRSupported() && GameUserSettings->IsHDREnabled());

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
	ResolutionListView->Refresh(SelectedResolution.IsValid() ? ResolutionList.Find(SelectedResolution) : INDEX_NONE);
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
	ModalPanel->Show(LOCTEXT("ConfirmResolution", "Confirm video settings"),
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

bool SNovaMainMenuSettings::IsScreenPercentageSupported() const
{
	return true;
}

bool SNovaMainMenuSettings::IsTSRSupported() const
{
	return !(IsDLSSSupported() && GameUserSettings->EnableDLSS);
}

bool SNovaMainMenuSettings::IsDLSSSupported() const
{
	return GameUserSettings->IsDLSSSupported();
}

bool SNovaMainMenuSettings::IsHDRSupported() const
{
	return GameUserSettings->IsHDRSupported();
}

bool SNovaMainMenuSettings::IsNaniteSupported() const
{
	return GameUserSettings->IsNaniteSupported();
}

bool SNovaMainMenuSettings::IsLumenSupported() const
{
	return GameUserSettings->IsLumenSupported();
}

bool SNovaMainMenuSettings::IsLumenHWRTSupported() const
{
	return GameUserSettings->IsLumenHWRTSupported();
}

bool SNovaMainMenuSettings::IsVirtualShadowSupported() const
{
	return GameUserSettings->IsVirtualShadowSupported();
}

void SNovaMainMenuSettings::OnFOVChanged(float Value)
{
	GameUserSettings->FOV = Value;
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnCrashReportToggled()
{
	GameUserSettings->EnableCrashReports = CrashReportButton->IsActive();
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnMasterVolumeChanged(float Value)
{
	GameUserSettings->MasterVolume = Value;

	UNeutronSoundManager::Get()->SetMasterVolume(Value);

	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnUIVolumeChanged(float Value)
{
	GameUserSettings->UIVolume = Value;

	UNeutronSoundManager::Get()->SetUIVolume(Value);

	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnEffectsVolumeChanged(float Value)
{
	GameUserSettings->EffectsVolume = Value;

	UNeutronSoundManager::Get()->SetEffectsVolume(Value);

	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnMusicVolumeChanged(float Value)
{
	GameUserSettings->MusicVolume = Value;

	UNeutronSoundManager::Get()->SetMusicVolume(Value);

	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnGlobalIlluminationChanged(float Value)
{
	GameUserSettings->SetGlobalIlluminationQuality(Value);
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
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnTSRToggled()
{
	GameUserSettings->EnableTSR = TSRButton->IsActive();
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnDLSSToggled()
{
	GameUserSettings->EnableDLSS = DLSSButton->IsActive();
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnSpaceFeelToggled()
{
	Cast<UNovaGameUserSettings>(GameUserSettings)->EnableCameraDegradation = CameraDegradationButton->IsActive();
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnMotionBlurToggled()
{
	Cast<UNovaGameUserSettings>(GameUserSettings)->MotionBlurAmount = MotionBlurButton->IsActive() ? 0.5f : 0.0f;
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnNaniteToggled()
{
	GameUserSettings->EnableNanite = NaniteButton->IsActive();
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnLumenToggled()
{
	GameUserSettings->EnableLumen = LumenButton->IsActive();
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnLumenHWRTToggled()
{
	GameUserSettings->EnableLumenHWRT = LumenHWRTButton->IsActive();
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

void SNovaMainMenuSettings::OnVirtualShadowsToggled()
{
	GameUserSettings->EnableVirtualShadows = VirtualShadowsButton->IsActive();
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
	const FNeutronMainTheme&   Theme       = FNeutronStyleSet::GetMainTheme();
	const FNeutronButtonTheme& ButtonTheme = FNeutronStyleSet::GetButtonTheme();

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
				.Image_Lambda([=]()
				{
					return CultureListView->GetSelectionIcon(Culture);
				})
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

	GameUserSettings->SaveSettings();
}

/*----------------------------------------------------
    Callbacks (video)
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuSettings::GenerateResolutionItem(TSharedPtr<FScreenResolutionRHI> Resolution)
{
	const FNeutronMainTheme&   Theme       = FNeutronStyleSet::GetMainTheme();
	const FNeutronButtonTheme& ButtonTheme = FNeutronStyleSet::GetButtonTheme();

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
				.Image_Lambda([=]()
				{
					return ResolutionListView->GetSelectionIcon(Resolution);
				})
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
	// bool IsHDR        = GameUserSettings->IsHDREnabled();

	return ((SelectedResolution.IsValid() && (SelectedResolution->Width != LastConfirmedVideoResolution.X ||
												 SelectedResolution->Height != LastConfirmedVideoResolution.Y)) ||
			FullscreenButton->IsActive() != IsFullscreen || VSyncButton->IsActive() != IsVsync /* || HDRButton->IsActive() != IsHDR*/);
}

void SNovaMainMenuSettings::OnApplyVideoSettings()
{
	NLOG("SNovaMainMenuSettings::OnApplyVideoSettings");

	FIntPoint Resolution;
	if (SelectedResolution.IsValid())
	{
		Resolution.X = SelectedResolution->Width;
		Resolution.Y = SelectedResolution->Height;
	}
	else
	{
		Resolution.X = LastConfirmedVideoResolution.X;
		Resolution.Y = LastConfirmedVideoResolution.Y;
	}

	UpdateResolution(Resolution, FullscreenButton->IsActive());

	GameUserSettings->SetVSyncEnabled(VSyncButton->IsActive());
	// GameUserSettings->EnableHDRDisplayOutput(HDRButton->IsActive());
	GameUserSettings->ApplySettings(false);
	GameUserSettings->SaveSettings();
}

#undef LOCTEXT_NAMESPACE
