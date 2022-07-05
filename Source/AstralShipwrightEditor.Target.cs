// Astral Shipwright - Gwennaël Arbona

using UnrealBuildTool;
using System.Collections.Generic;

public class AstralShipwrightEditorTarget : TargetRules
{
	public AstralShipwrightEditorTarget(TargetInfo Target) : base(Target)
    {
		DefaultBuildSettings = BuildSettingsVersion.V2;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		Type = TargetType.Editor;
        ExtraModuleNames.Add("AstralShipwright");
    }
}
