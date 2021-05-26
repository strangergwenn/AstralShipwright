// Nova project - Gwennaël Arbona

#include "NovaGameUserSettings.h"
#include "Nova/Nova.h"

#include "Modules/ModuleManager.h"
#include "RenderCore.h"

#if PLATFORM_WINDOWS
#include "DLSS.h"
#endif    // PLATFORM_WINDOWS

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
	// Toggle DLSS
	EnableDLSS                = EnableDLSS && IsDLSSSupported();
	IConsoleVariable* DLSSVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable"));
	if (DLSSVar)
	{
		DLSSVar->Set(EnableDLSS ? 1 : 0, ECVF_SetByConsole);
	}
}

bool UNovaGameUserSettings::IsHDRSupported() const
{
	return GetFullscreenMode() == EWindowMode::Fullscreen && SupportsHDRDisplayOutput() && IsHDRAllowed();
}

bool UNovaGameUserSettings::IsDLSSSupported() const
{
#if PLATFORM_WINDOWS
	IDLSSModuleInterface* DLSSModule = &FModuleManager::LoadModuleChecked<IDLSSModuleInterface>(TEXT("DLSS"));
	if (DLSSModule)
	{
		return DLSSModule->QueryDLSSSupport() == EDLSSSupport::Supported;
	}
#endif    // PLATFORM_WINDOWS

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
