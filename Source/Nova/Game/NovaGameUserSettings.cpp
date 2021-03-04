// Nova project - Gwennaël Arbona

#include "NovaGameUserSettings.h"
#include "Nova/Nova.h"

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
	IConsoleVariable* SSGIVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SSGI.Enable"));
	NCHECK(SSGIVar);
	SSGIVar->Set(EnableSSGI ? 1 : 0, ECVF_SetByConsole);
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	// Gameplay
	MouseSensitivity   = 0.8f;
	GamepadSensitivity = 2.0f;
	FOV                = 90.0f;

	// Graphics
	EnableSSGI                 = false;
	EnableRaytracedReflections = false;
	EnableRaytracedShadows     = false;
	EnableRaytracedAO          = false;
	EnableCinematicBloom       = false;
	ScreenPercentage           = 100.0f;
}

void UNovaGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);

	ApplyCustomGraphicsSettings();
}
