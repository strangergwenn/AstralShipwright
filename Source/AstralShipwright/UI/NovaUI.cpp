// Astral Shipwright - Gwennaël Arbona

#include "NovaUI.h"
#include "Nova.h"

/*----------------------------------------------------
    Player input bindings
----------------------------------------------------*/

// Common actions
const FName FNovaPlayerInput::MenuToggle   = "MenuToggle";
const FName FNovaPlayerInput::MenuConfirm  = "MenuConfirm";
const FName FNovaPlayerInput::MenuCancel   = "MenuCancel";
const FName FNovaPlayerInput::MenuUp       = "MenuUp";
const FName FNovaPlayerInput::MenuDown     = "MenuDown";
const FName FNovaPlayerInput::MenuLeft     = "MenuLeft";
const FName FNovaPlayerInput::MenuRight    = "MenuRight";
const FName FNovaPlayerInput::MenuNext     = "MenuNext";
const FName FNovaPlayerInput::MenuPrevious = "MenuPrevious";

const FName FNovaPlayerInput::MenuMoveHorizontal   = "MenuMoveHorizontal";
const FName FNovaPlayerInput::MenuMoveVertical     = "MenuMoveVertical";
const FName FNovaPlayerInput::MenuAnalogHorizontal = "MenuAnalogHorizontal";
const FName FNovaPlayerInput::MenuAnalogVertical   = "MenuAnalogVertical";
const FName FNovaPlayerInput::MenuZoomIn           = "MenuZoomIn";
const FName FNovaPlayerInput::MenuZoomOut          = "MenuZoomOut";

const FName FNovaPlayerInput::MenuPrimary      = "MenuPrimary";
const FName FNovaPlayerInput::MenuSecondary    = "MenuSecondary";
const FName FNovaPlayerInput::MenuAltPrimary   = "MenuAltPrimary";
const FName FNovaPlayerInput::MenuAltSecondary = "MenuAltSecondary";
const FName FNovaPlayerInput::MenuNextTab      = "MenuNextTab";
const FName FNovaPlayerInput::MenuPreviousTab  = "MenuPreviousTab";
