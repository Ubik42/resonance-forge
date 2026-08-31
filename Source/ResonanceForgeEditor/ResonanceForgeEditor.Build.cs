using UnrealBuildTool;

public class ResonanceForgeEditor : ModuleRules
{
    public ResonanceForgeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "UnrealEd", "LevelEditor", "Slate", "SlateCore",
            "ToolMenus", "Projects", "InputCore", "MIDIDevice", "AssetRegistry", "Json",
            "ResonanceForgeRuntime", "ResonanceForgeWwise"
        });
    }
}
