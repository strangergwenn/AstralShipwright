// Astral Shipwright - Gwennaël Arbona

using UnrealBuildTool;

public class AstralShipwright : ModuleRules
{
    public AstralShipwright(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.Add("AstralShipwright");
        PrivatePCHHeaderFile = "Nova.h";

        PublicDependencyModuleNames.AddRange(new string[] {

            "Neutron",

            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "AppFramework",

            "Slate",
            "SlateCore",
            "UMG",
            "MoviePlayer",
            "RHI",
            "RenderCore",
            "Niagara",

            "Json",
            "JsonUtilities",

            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "NetCore"
        });

        if (Target.Type == TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "UnrealEd"
            });
        }

        DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");

        AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
    }
}
