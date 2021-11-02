// Astral Shipwright - Gwennaël Arbona

#include "NovaGameUserSettings.h"
#include "Nova.h"

#include "Modules/ModuleManager.h"
#include "RenderCore.h"

#define HAS_DLSS PLATFORM_WINDOWS && 0

#if HAS_DLSS
#include "DLSS.h"
#endif    // HAS_DLSS

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaGameUserSettings::UNovaGameUserSettings()
{}

/*----------------------------------------------------
    Settings
----------------------------------------------------*/

void UNovaGameUserSettings::ApplyCustomGraphicsSettings()
{
#if HAS_DLSS
	// Toggle DLSS
	EnableDLSS                = EnableDLSS && IsDLSSSupported();
	IConsoleVariable* DLSSVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable"));
	if (DLSSVar)
	{
		DLSSVar->Set(EnableDLSS ? 1 : 0, ECVF_SetByConsole);
	}
#endif    // HAS_DLSS

	// Set screen percentage
	IConsoleVariable* ScreenPercentageVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	if (ScreenPercentageVar)
	{
		ScreenPercentageVar->Set(ScreenPercentage, ECVF_SetByConsole);
	}
}

bool UNovaGameUserSettings::IsHDRSupported() const
{
	return GetFullscreenMode() == EWindowMode::Fullscreen && SupportsHDRDisplayOutput() && IsHDRAllowed();
}

bool UNovaGameUserSettings::IsDLSSSupported() const
{
#if HAS_DLSS
	IDLSSModuleInterface* DLSSModule = &FModuleManager::LoadModuleChecked<IDLSSModuleInterface>(TEXT("DLSS"));
	if (DLSSModule)
	{
		return DLSSModule->QueryDLSSSupport() == EDLSSSupport::Supported;
	}
#endif    // HAS_DLSS

	return false;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	// System
	EnableCrashReports = true;

	// Gameplay
	MouseSensitivity   = 0.8f;
	GamepadSensitivity = 2.0f;
	FOV                = 90.0f;

	// Graphics
	EnableDLSS             = false;
	EnableLumen            = false;
	EnableRaytracedShadows = false;
	EnableRaytracedAO      = false;
	EnableCinematicBloom   = false;
	ScreenPercentage       = 100.0f;
}

void UNovaGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);

	ApplyCustomGraphicsSettings();
}
