#include "ResonanceForgeWwiseBridgeComponent.h"

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
    FResonanceForgeWwiseGeneratedAssetsTest,
    "ResonanceForge.Wwise.GeneratedAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResonanceForgeWwiseGeneratedAssetsTest::RunTest(const FString& Parameters)
{
    TestNotNull(TEXT("撞击 Event 已同步到 UE"), LoadObject<UAkAudioEvent>(nullptr,
        TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Metal.Play_RF_Impact_Metal")));
    TestNotNull(TEXT("能量 RTPC 已同步到 UE"), LoadObject<UAkRtpc>(nullptr,
        TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ImpactEnergy.RF_ImpactEnergy")));
    TestNotNull(TEXT("明亮度 RTPC 已同步到 UE"), LoadObject<UAkRtpc>(nullptr,
        TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ImpactBrightness.RF_ImpactBrightness")));
    TestNotNull(TEXT("尺寸 RTPC 已同步到 UE"), LoadObject<UAkRtpc>(nullptr,
        TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ObjectSize.RF_ObjectSize")));
    return true;
}

#endif
