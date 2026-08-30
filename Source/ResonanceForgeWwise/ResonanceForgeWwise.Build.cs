using UnrealBuildTool;

public class ResonanceForgeWwise : ModuleRules
{
    public ResonanceForgeWwise(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "MIDIDevice", "ResonanceForgeRuntime", "AkAudio"
        });
    }
}
