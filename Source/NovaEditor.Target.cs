// Nova project - Gwennaël Arbona

using UnrealBuildTool;
using System.Collections.Generic;

public class NovaEditorTarget : TargetRules
{
	public NovaEditorTarget(TargetInfo Target) : base(Target)
    {
		DefaultBuildSettings = BuildSettingsVersion.V2;
		Type = TargetType.Editor;
        ExtraModuleNames.Add("Nova");
    }
}
