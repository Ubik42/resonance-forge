#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ResonanceForgeSynthComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResonanceForgePresetTest,
    "ResonanceForge.Runtime.BuiltInPresets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResonanceForgePresetTest::RunTest(const FString& Parameters)
{
    const TArray<FName> Presets = UResonanceForgeSynthComponent::GetBuiltInPresetNames();
    TestEqual(TEXT("首版提供三种可听出差异的材质"), Presets.Num(), 3);
    TestTrue(TEXT("包含拉丝钢"), Presets.Contains(TEXT("拉丝钢")));
    TestTrue(TEXT("包含硬木"), Presets.Contains(TEXT("硬木")));
    TestTrue(TEXT("包含薄玻璃"), Presets.Contains(TEXT("薄玻璃")));
    return true;
}

#endif
