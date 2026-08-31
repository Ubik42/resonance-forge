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

    const TArray<FResonanceMode> SteelModes = UResonanceForgeSynthComponent::GetBuiltInModes(TEXT("拉丝钢"));
    const TArray<FResonanceMode> WoodModes = UResonanceForgeSynthComponent::GetBuiltInModes(TEXT("硬木"));
    const TArray<FResonanceMode> GlassModes = UResonanceForgeSynthComponent::GetBuiltInModes(TEXT("薄玻璃"));
    TestEqual(TEXT("共享配方可取得拉丝钢八个模态"), SteelModes.Num(), 8);
    TestEqual(TEXT("共享配方可取得硬木六个模态"), WoodModes.Num(), 6);
    TestEqual(TEXT("共享配方可取得薄玻璃七个模态"), GlassModes.Num(), 7);
    TestTrue(TEXT("三种材质拥有不同的第一共振频率"), !FMath::IsNearlyEqual(SteelModes[0].FrequencyHz, WoodModes[0].FrequencyHz));
    TestTrue(TEXT("中央敲击充分激励第一模态"), UResonanceForgeSynthComponent::ComputeModeExcitation(0, 0.5f) > 0.95f);
    TestTrue(TEXT("中央敲击落在第二模态节点附近"), UResonanceForgeSynthComponent::ComputeModeExcitation(1, 0.5f) < 0.15f);
    TestTrue(TEXT("偏置落点会重新激励第二模态"), UResonanceForgeSynthComponent::ComputeModeExcitation(1, 0.34f) > 0.75f);

    TestEqual(TEXT("A4 在 48 kHz 下的波导长度接近 109 个采样"),
        UResonanceForgeSynthComponent::ComputeWaveguideDelaySamples(440.0f, 48000.0f), 109);
    TestEqual(TEXT("非法高频仍保留至少两个采样的稳定延迟"),
        UResonanceForgeSynthComponent::ComputeWaveguideDelaySamples(100000.0f, 48000.0f), 2);

    UResonanceForgeSynthComponent* Synth = NewObject<UResonanceForgeSynthComponent>();
    UResonanceMaterialProfile* SharedProfile = NewObject<UResonanceMaterialProfile>();
    SharedProfile->ModelType = EResonanceModelType::WaveguideString;
    SharedProfile->Modes = WoodModes;
    SharedProfile->StringDecay = 0.991f;
    SharedProfile->StringDamping = 0.48f;
    SharedProfile->BodyCoupling = 0.31f;
    SharedProfile->PickupPosition = 0.74f;
    SharedProfile->ExcitationType = EResonanceExcitationType::Finger;
    Synth->ApplyMaterialProfile(SharedProfile);
    TestEqual(TEXT("共享资产能够切换合成模型"), Synth->SynthesisModel, EResonanceModelType::WaveguideString);
    TestTrue(TEXT("共享资产能够写入弦路阻尼"), FMath::IsNearlyEqual(Synth->StringDamping, 0.48f));
    TestTrue(TEXT("共享资产能够写入箱体耦合"), FMath::IsNearlyEqual(Synth->BodyCoupling, 0.31f));
    TestTrue(TEXT("共享资产能够写入拾音位置"), FMath::IsNearlyEqual(Synth->PickupPosition, 0.74f));
    TestEqual(TEXT("共享资产能够写入起振手势"), Synth->ExcitationType, EResonanceExcitationType::Finger);
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

    Synth->ExcitationType = EResonanceExcitationType::Bow;
    TArray<float> BowedSamples;
    TestTrue(TEXT("弓擦手势能够离线生成持续波导输出"), Synth->RenderWaveguideForTest(55, 96000, BowedSamples));
    double LateSquareSum = 0.0;
    int32 LateSampleCount = 0;
    bool bBowFinite = true;
    for (int32 Index = BowedSamples.Num() / 2; Index < BowedSamples.Num(); ++Index)
    {
        bBowFinite &= FMath::IsFinite(BowedSamples[Index]);
        LateSquareSum += static_cast<double>(BowedSamples[Index]) * BowedSamples[Index];
        ++LateSampleCount;
    }
    const float LateRms = LateSampleCount > 0
        ? FMath::Sqrt(static_cast<float>(LateSquareSum / LateSampleCount))
        : 0.0f;
    TestTrue(TEXT("弓擦输出保持有限值"), bBowFinite);
    TestTrue(TEXT("弓擦在一秒后仍保留持续能量"), LateRms > 0.001f);
    return true;
}

#endif
