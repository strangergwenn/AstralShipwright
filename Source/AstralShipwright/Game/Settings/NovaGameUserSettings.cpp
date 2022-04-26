// Astral Shipwright - Gwennaël Arbona

#include "NovaGameUserSettings.h"

#include "System/NovaGameInstance.h"
#include "System/NovaSoundManager.h"
#include "Nova.h"

#include "Modules/ModuleManager.h"
#include "RenderCore.h"

#define HAS_DLSS PLATFORM_WINDOWS && 1

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
	// Toggle TSR
	IConsoleVariable* TSRVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"));
	if (TSRVar)
	{
		TSRVar->Set(EnableTSR ? 4 : 2, ECVF_SetByConsole);
	}

	EnableDLSS = EnableDLSS && IsDLSSSupported();

#if HAS_DLSS

	// Toggle VRS
	IConsoleVariable* VRSVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VRS.Enable"));
	if (VRSVar)
	{
		VRSVar->Set(EnableDLSS ? 0 : 1, ECVF_SetByConsole);
	}

	// Toggle DLSS
	IConsoleVariable* DLSSVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable"));
	if (DLSSVar)
	{
		DLSSVar->Set(EnableDLSS ? 1 : 0, ECVF_SetByConsole);
	}
#endif    // HAS_DLSS

	// Toggle Nanite
	IConsoleVariable* NaniteVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite"));
	if (NaniteVar)
	{
		NaniteVar->Set(EnableNanite ? 1 : 0, ECVF_SetByConsole);
	}

	// Toggle Lumen HWRT
	IConsoleVariable* LumenHWRTVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing"));
	if (LumenHWRTVar)
	{
		LumenHWRTVar->Set(EnableLumenHWRT ? 1 : 0, ECVF_SetByConsole);
	}

	// Toggle virtual shadow maps
	IConsoleVariable* VirtualShadowsVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Enable"));
	if (VirtualShadowsVar)
	{
		VirtualShadowsVar->Set(EnableVirtualShadows ? 1 : 0, ECVF_SetByConsole);
	}

	// Set screen percentage
	IConsoleVariable* ScreenPercentageVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	if (ScreenPercentageVar)
	{
		ScreenPercentageVar->Set(EnableDLSS ? 100 : ScreenPercentage, ECVF_SetByConsole);
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

bool UNovaGameUserSettings::IsNaniteSupported() const
{
	return DoesRuntimeSupportNanite(GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel], true, false);
}

bool UNovaGameUserSettings::IsLumenSupported() const
{
	EShaderPlatform Platform = GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel];

	return FDataDrivenShaderPlatformInfo::GetSupportsLumenGI(Platform) && !IsForwardShadingEnabled(Platform);
}

bool UNovaGameUserSettings::IsLumenHWRTSupported() const
{
	EShaderPlatform Platform = GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel];

	return IsLumenSupported() && FDataDrivenShaderPlatformInfo::GetSupportsRayTracing(Platform);
}

bool UNovaGameUserSettings::IsVirtualShadowSupported() const
{
	return IsNaniteSupported();
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

	// Sound
	MasterVolume  = 10;
	MusicVolume   = 10;
	EffectsVolume = 10;

	// Graphics
	EnableTSR            = true;
	EnableDLSS           = false;
	EnableNanite         = true;
	EnableLumen          = true;
	EnableLumenHWRT      = false;
	EnableVirtualShadows = false;
	EnableCinematicBloom = false;
	ScreenPercentage     = 100.0f;
}

void UNovaGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);

	ApplyCustomGraphicsSettings();
}
