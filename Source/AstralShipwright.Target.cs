// Astral Shipwright - Gwennaël Arbona

using UnrealBuildTool;
using System.Collections.Generic;

public class AstralShipwrightTarget : TargetRules
{
    public AstralShipwrightTarget(TargetInfo Target) : base(Target)
    {
		DefaultBuildSettings = BuildSettingsVersion.V2;
        Type = TargetType.Game;
		bUsesSteam = true;
        ExtraModuleNames.Add("AstralShipwright");
    }
}
