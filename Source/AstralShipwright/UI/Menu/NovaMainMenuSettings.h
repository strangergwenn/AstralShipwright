// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronTabView.h"
#include "Neutron/UI/Widgets/NeutronModalListView.h"

class SNovaMainMenuSettings
	: public SNeutronTabPanel
	, public INeutronGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuSettings)
	{}

	SLATE_ARGUMENT(class SNeutronMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNeutronMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Show() override;

	TSharedPtr<class SNeutronButton> GetDefaultFocusButton() const override;

	/*----------------------------------------------------
	    Input management
	----------------------------------------------------*/

	void OnKeyBindingChanged(FKey PreviousKey, FKey NewKey, TSharedPtr<struct FNeutronKeyBinding> Binding);
	void OnKeyBindingReset(TSharedPtr<struct FNeutronKeyBinding> Binding);

	void ApplyNewBinding(TSharedPtr<struct FNeutronKeyBinding> BindingThatChanged, bool Replace);
	void CancelNewBinding(TSharedPtr<struct FNeutronKeyBinding> BindingThatChanged, FKey PreviousKey);
	bool IsAlreadyUsed(
		TArray<TSharedPtr<struct FNeutronKeyBinding>>& BindConflicts, FKey Key, TSharedPtr<FNeutronKeyBinding> ExcludeBinding);

	/*----------------------------------------------------
	    Video management
	----------------------------------------------------*/

	TSharedPtr<class SNeutronSlider> AddSettingSlider(TSharedPtr<class SVerticalBox> Container, FText Text, FText HelpText,
		FOnFloatValueChanged Callback, float MinValue = 0, float MaxValue = 4, float ValueStep = 1, TAttribute<bool> Enabled = true);

	void UpdateResolutionList();

	void UpdateResolution(FIntPoint Resolution, bool Fullscreen);

	void ApplyVideoSettings();
	void RevertVideoSettings();

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:

	// Platform support getters
	EVisibility GetPCVisibility() const;
	bool        IsScreenPercentageSupported() const;
	bool        IsTSRSupported() const;
	bool        IsDLSSSupported() const;
	bool        IsHDRSupported() const;
	bool        IsNaniteSupported() const;
	bool        IsLumenSupported() const;
	bool        IsLumenHWRTSupported() const;
	bool        IsVirtualShadowSupported() const;

	// Gameplay settings callbacks
	void OnFOVChanged(float Value);
	void OnCrashReportToggled();

	// Sound settings callbacks
	void OnMasterVolumeChanged(float Value);
	void OnUIVolumeChanged(float Value);
	void OnEffectsVolumeChanged(float Value);
	void OnMusicVolumeChanged(float Value);

	// Graphics settings callbacks
	void OnGlobalIlluminationChanged(float Value);
	void OnShadowChanged(float Value);
	void OnEffectsChanged(float Value);
	void OnPostProcessChanged(float Value);
	void OnAntiAliasingChanged(float Value);
	void OnScreenPercentageChanged(float Value);

	// Graphics settings callbacks
	void OnTSRToggled();
	void OnDLSSToggled();
	void OnSpaceFeelToggled();
	void OnNaniteToggled();
	void OnLumenToggled();
	void OnLumenHWRTToggled();
	void OnVirtualShadowsToggled();
	void OnCinematicBloomToggled();

	// Culture list
	TSharedRef<SWidget> GenerateCultureItem(TSharedPtr<FString> Culture);
	FText               GetCultureName(TSharedPtr<FString> Culture) const;
	FText               GenerateCultureTooltip(TSharedPtr<FString> Culture);
	void                OnSelectedCultureChanged(TSharedPtr<FString> Culture, int32 Index);

	// Culture settings
	void OnApplyCultureSettings();

	// Resolution list
	TSharedRef<SWidget> GenerateResolutionItem(TSharedPtr<struct FScreenResolutionRHI> Resolution);
	FText               GetResolutionName(TSharedPtr<struct FScreenResolutionRHI> Resolution) const;
	FText               GenerateResolutionTooltip(TSharedPtr<struct FScreenResolutionRHI> Resolution);
	void                OnSelectedResolutionChanged(TSharedPtr<struct FScreenResolutionRHI> Resolution, int32 Index);

	// Video settings
	bool IsApplyVideoSettingsEnabled() const;
	void OnApplyVideoSettings();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Menu widgets
	TWeakObjectPtr<UNeutronMenuManager>  MenuManager;
	TSharedPtr<class SVerticalBox>       BindingsContainer;
	TSharedPtr<class SNeutronModalPanel> ModalPanel;

	// Data
	class UNeutronGameUserSettings*                 GameUserSettings;
	TArray<TSharedPtr<FString>>                     CultureList;
	TSharedPtr<FString>                             SelectedCulture;
	TArray<TSharedPtr<struct FScreenResolutionRHI>> ResolutionList;
	TSharedPtr<struct FScreenResolutionRHI>         SelectedResolution;
	TArray<TSharedPtr<struct FNeutronKeyBinding>>   Bindings;
	FIntPoint                                       LastConfirmedVideoResolution;
	EWindowMode::Type                               LastConfirmedFullscreenMode;

	// Game settings widgets
	TSharedPtr<SVerticalBox>                               GameplayContainer;
	TSharedPtr<SNeutronModalListView<TSharedPtr<FString>>> CultureListView;
	TSharedPtr<class SNeutronSlider>                       FOVSlider;
	TSharedPtr<class SNeutronButton>                       CrashReportButton;

	// Sound settings widgets
	TSharedPtr<SVerticalBox>         SoundContainer;
	TSharedPtr<class SNeutronSlider> MasterVolumeSlider;
	TSharedPtr<class SNeutronSlider> UIVolumeSlider;
	TSharedPtr<class SNeutronSlider> EffectsVolumeSlider;
	TSharedPtr<class SNeutronSlider> MusicVolumeSlider;

	// Display settings widgets
	TSharedPtr<SNeutronModalListView<TSharedPtr<struct FScreenResolutionRHI>>> ResolutionListView;
	TSharedPtr<class SNeutronButton>                                           FullscreenButton;
	TSharedPtr<class SNeutronButton>                                           VSyncButton;
	TSharedPtr<class SNeutronButton>                                           HDRButton;

	// Graphics settings widgets
	TSharedPtr<SVerticalBox>         ScalingContainer;
	TSharedPtr<SVerticalBox>         GraphicsContainer;
	TSharedPtr<class SNeutronSlider> GlobalIlluminationSlider;
	TSharedPtr<class SNeutronSlider> ShadowSlider;
	TSharedPtr<class SNeutronSlider> EffectsSlider;
	TSharedPtr<class SNeutronSlider> PostProcessSlider;
	TSharedPtr<class SNeutronSlider> AntiAliasingSlider;
	TSharedPtr<class SNeutronSlider> ScreenPercentageSlider;

	// Graphics settings widgets
	TSharedPtr<class SNeutronButton> CameraDegradationButton;
	TSharedPtr<class SNeutronButton> DLSSButton;
	TSharedPtr<class SNeutronButton> TSRButton;
	TSharedPtr<class SNeutronButton> NaniteButton;
	TSharedPtr<class SNeutronButton> LumenButton;
	TSharedPtr<class SNeutronButton> LumenHWRTButton;
	TSharedPtr<class SNeutronButton> VirtualShadowsButton;
	TSharedPtr<class SNeutronButton> CinematicBloomButton;
};
