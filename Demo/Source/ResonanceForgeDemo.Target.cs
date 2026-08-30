using UnrealBuildTool;

public class ResonanceForgeDemoTarget : TargetRules
{
    public ResonanceForgeDemoTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("ResonanceForgeDemo");
    }
}
