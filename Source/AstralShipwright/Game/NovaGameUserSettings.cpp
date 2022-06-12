// Astral Shipwright - Gwennaël Arbona

#include "NovaGameUserSettings.h"
#include "Nova.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaGameUserSettings::UNovaGameUserSettings()
{}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	EnableCameraDegradation = true;
}
