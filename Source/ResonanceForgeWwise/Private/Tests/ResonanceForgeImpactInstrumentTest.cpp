#include "ResonanceForgeImpactInstrumentActor.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResonanceForgeImpactMappingTest,
    "ResonanceForge.Physics.ImpactMapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResonanceForgeImpactMappingTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("低于阈值不触发"), AResonanceForgeImpactInstrumentActor::ComputeImpactEnergy(700.0f, 800.0f, 0.001f), 0.0f);
    TestEqual(TEXT("阈值后的冲量映射为能量"), AResonanceForgeImpactInstrumentActor::ComputeImpactEnergy(1300.0f, 800.0f, 0.001f), 0.5f);
    TestEqual(TEXT("能量上限为 1"), AResonanceForgeImpactInstrumentActor::ComputeImpactEnergy(4000.0f, 800.0f, 0.001f), 1.0f);
    TestTrue(TEXT("高速撞击更明亮"), AResonanceForgeImpactInstrumentActor::ComputeImpactBrightness(2000.0f) > AResonanceForgeImpactInstrumentActor::ComputeImpactBrightness(200.0f));
    return true;
}

#endif
