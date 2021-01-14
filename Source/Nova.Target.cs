// Nova project - Gwennaël Arbona

using UnrealBuildTool;
using System.Collections.Generic;

public class NovaTarget : TargetRules
{
    public NovaTarget(TargetInfo Target) : base(Target)
    {
		DefaultBuildSettings = BuildSettingsVersion.V2;
        Type = TargetType.Game;
		bUsesSteam = true;
        ExtraModuleNames.Add("Nova");
    }
}
