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

    const TArray<EResonanceModelType> Models = UResonanceForgeSynthComponent::GetSupportedModels();
    TestEqual(TEXT("短周期版本提供两种互补的物理声学模型"), Models.Num(), 2);
    TestTrue(TEXT("保留模态撞击体"), Models.Contains(EResonanceModelType::ModalImpact));
    TestTrue(TEXT("加入数字波导弦"), Models.Contains(EResonanceModelType::WaveguideString));

    TestEqual(TEXT("A4 在 48 kHz 下的波导长度接近 109 个采样"),
        UResonanceForgeSynthComponent::ComputeWaveguideDelaySamples(440.0f, 48000.0f), 109);
    TestEqual(TEXT("非法高频仍保留至少两个采样的稳定延迟"),
        UResonanceForgeSynthComponent::ComputeWaveguideDelaySamples(100000.0f, 48000.0f), 2);

    UResonanceForgeSynthComponent* Synth = NewObject<UResonanceForgeSynthComponent>();
    TArray<float> RenderedSamples;
    TestTrue(TEXT("数字波导弦能够离线生成测试缓冲"), Synth->RenderWaveguideForTest(60, 4096, RenderedSamples));
    float Peak = 0.0f;
    bool bAllFinite = true;
    for (const float Sample : RenderedSamples)
    {
        Peak = FMath::Max(Peak, FMath::Abs(Sample));
        bAllFinite &= FMath::IsFinite(Sample);
    }
    TestTrue(TEXT("数字波导输出全部为有限值"), bAllFinite);
    TestTrue(TEXT("数字波导输出包含可听能量"), Peak > 0.01f);
    TestTrue(TEXT("软限幅将输出约束在安全范围"), Peak <= 1.0f);
    return true;
}

#endif
