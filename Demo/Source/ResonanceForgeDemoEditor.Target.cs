using UnrealBuildTool;

public class ResonanceForgeDemoEditorTarget : TargetRules
{
    public ResonanceForgeDemoEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("ResonanceForgeDemo");
    }
}
