// Astral Shipwright - Gwennaël Arbona

using UnrealBuildTool;
using System.Collections.Generic;

public class AstralShipwrightTarget : TargetRules
{
    public AstralShipwrightTarget(TargetInfo Target) : base(Target)
    {
		DefaultBuildSettings = BuildSettingsVersion.V2;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        Type = TargetType.Game;
		bUsesSteam = true;
        ExtraModuleNames.Add("AstralShipwright");
    }
}
