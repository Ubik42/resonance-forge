using UnrealBuildTool;

public class ResonanceForgeRuntime : ModuleRules
{
    public ResonanceForgeRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "AudioMixer"
        });
    }
}
