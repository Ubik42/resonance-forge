#include "ResonanceForgeWwiseBridgeComponent.h"
#include "ResonanceForgeImpactInstrumentActor.h"

#include "AkAudioEvent.h"
#include "AkRtpc.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResonanceForgeWwiseRtpcMappingTest,
    "ResonanceForge.Wwise.RtpcMapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResonanceForgeWwiseRtpcMappingTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("0 映射到 0"), UResonanceForgeWwiseBridgeComponent::ToWwiseRtpc(0.0f), 0.0f);
    TestEqual(TEXT("0.5 映射到 50"), UResonanceForgeWwiseBridgeComponent::ToWwiseRtpc(0.5f), 50.0f);
    TestEqual(TEXT("1 映射到 100"), UResonanceForgeWwiseBridgeComponent::ToWwiseRtpc(1.0f), 100.0f);
    TestEqual(TEXT("输入会限制在有效范围"), UResonanceForgeWwiseBridgeComponent::ToWwiseRtpc(1.5f), 100.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResonanceForgeVelocityCurveTest,
    "ResonanceForge.Performance.VelocityCurve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResonanceForgeVelocityCurveTest::RunTest(const FString& Parameters)
{
    const float Input = 0.76f;
    const float Soft = AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(Input, EResonanceVelocityCurve::SoftTouch);
    const float Linear = AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(Input, EResonanceVelocityCurve::Linear);
    const float Heavy = AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(Input, EResonanceVelocityCurve::HeavyHand);
    TestTrue(TEXT("软触抬升同一输入"), Soft > Linear);
    TestTrue(TEXT("重手压低同一输入"), Heavy < Linear);
    TestTrue(TEXT("线性保持原始力度"), FMath::IsNearlyEqual(Linear, Input));
    TestEqual(TEXT("所有曲线保留静音端点"), AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(0.0f, EResonanceVelocityCurve::SoftTouch), 0.0f);
    TestEqual(TEXT("所有曲线保留满力度端点"), AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(1.0f, EResonanceVelocityCurve::HeavyHand), 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResonanceForgeWwiseGeneratedAssetsTest,
    "ResonanceForge.Wwise.GeneratedAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResonanceForgeWwiseGeneratedAssetsTest::RunTest(const FString& Parameters)
{
    TestNotNull(TEXT("拉丝钢 Event 已同步到 UE"), LoadObject<UAkAudioEvent>(nullptr,
        TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Steel.Play_RF_Impact_Steel")));
    TestNotNull(TEXT("硬木 Event 已同步到 UE"), LoadObject<UAkAudioEvent>(nullptr,
        TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Wood.Play_RF_Impact_Wood")));
    TestNotNull(TEXT("薄玻璃 Event 已同步到 UE"), LoadObject<UAkAudioEvent>(nullptr,
        TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Glass.Play_RF_Impact_Glass")));
    TestNotNull(TEXT("能量 RTPC 已同步到 UE"), LoadObject<UAkRtpc>(nullptr,
        TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ImpactEnergy.RF_ImpactEnergy")));
    TestNotNull(TEXT("明亮度 RTPC 已同步到 UE"), LoadObject<UAkRtpc>(nullptr,
        TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ImpactBrightness.RF_ImpactBrightness")));
    TestNotNull(TEXT("尺寸 RTPC 已同步到 UE"), LoadObject<UAkRtpc>(nullptr,
        TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ObjectSize.RF_ObjectSize")));
    return true;
}

#endif
