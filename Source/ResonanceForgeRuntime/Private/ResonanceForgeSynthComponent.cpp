#include "ResonanceForgeSynthComponent.h"

#include "Math/UnrealMathUtility.h"

// 数字波导弦的延迟线、噪声激励与环路低通结构参考 STK Plucked；
// 弓擦的速度差与非线性摩擦曲线参考 STK BowTable / Bowed 的建模思路。
// STK: Copyright (c) 1995-2023 Perry R. Cook and Gary P. Scavone，MIT 风格许可。
// 本工程采用面向 Unreal 音频线程、固定复音池与参数快照的独立实现。

namespace ResonanceForge
{
    static TArray<FResonanceMode> MakeModes(const TArray<float>& Frequencies, const float BaseDecay)
    {
        TArray<FResonanceMode> Result;
        Result.Reserve(Frequencies.Num());
        for (int32 Index = 0; Index < Frequencies.Num(); ++Index)
        {
            FResonanceMode& Mode = Result.AddDefaulted_GetRef();
            Mode.FrequencyHz = Frequencies[Index];
            Mode.Gain = 1.0f / FMath::Sqrt(static_cast<float>(Index + 1));
            Mode.DecaySeconds = BaseDecay / (1.0f + 0.11f * Index);
        }
        return Result;
    }
}

UResonanceForgeSynthComponent::UResonanceForgeSynthComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NumChannels = 2;
    bAutoActivate = true;
    LoadBuiltInPreset(TEXT("拉丝钢"));
}

bool UResonanceForgeSynthComponent::Init(int32& SampleRate)
{
    RenderSampleRate = static_cast<float>(SampleRate);
    RebuildModesFrom(GetEffectiveModes());
    InitializeWaveguideVoices();
    return true;
}

void UResonanceForgeSynthComponent::Strike(
    const float Energy,
    const float Brightness,
    const int32 MidiNote,
    const float StrikePosition)
{
    const UResonanceMaterialProfile* Profile = MaterialProfile;
    PendingStrikes.Enqueue({
        FMath::Clamp(Energy, 0.0f, 1.5f),
        FMath::Clamp(Brightness, 0.0f, 1.0f),
        FMath::Clamp(MidiNote, 0, 127),
        Profile ? Profile->ModelType : SynthesisModel,
        Profile ? Profile->StringDecay : StringDecay,
        Profile ? Profile->StringDamping : StringDamping,
        Profile ? Profile->BodyCoupling : BodyCoupling,
        Profile ? Profile->PickupPosition : PickupPosition,
        Profile ? Profile->ExcitationType : ExcitationType,
        PitchScale,
        FMath::Clamp(StrikePosition, 0.0f, 1.0f)
    });
}

TArray<FName> UResonanceForgeSynthComponent::GetBuiltInPresetNames()
{
    return {TEXT("拉丝钢"), TEXT("硬木"), TEXT("薄玻璃")};
}

TArray<EResonanceModelType> UResonanceForgeSynthComponent::GetSupportedModels()
{
    return {EResonanceModelType::ModalImpact, EResonanceModelType::WaveguideString};
}

TArray<FResonanceMode> UResonanceForgeSynthComponent::GetBuiltInModes(const FName PresetName)
{
    if (PresetName == TEXT("硬木"))
    {
        return ResonanceForge::MakeModes({172.0f, 286.0f, 451.0f, 708.0f, 1098.0f, 1710.0f}, 0.54f);
    }
    if (PresetName == TEXT("薄玻璃"))
    {
        return ResonanceForge::MakeModes({392.0f, 615.0f, 933.0f, 1417.0f, 2150.0f, 3270.0f, 4860.0f}, 1.35f);
    }
    return ResonanceForge::MakeModes({224.0f, 361.0f, 587.0f, 927.0f, 1438.0f, 2215.0f, 3390.0f, 5120.0f}, 1.05f);
}

int32 UResonanceForgeSynthComponent::ComputeWaveguideDelaySamples(const float FrequencyHz, const float SampleRate)
{
    const float SafeSampleRate = FMath::Max(8000.0f, SampleRate);
    const float SafeFrequency = FMath::Clamp(FrequencyHz, 20.0f, SafeSampleRate * 0.45f);
    return FMath::Max(2, FMath::RoundToInt(SafeSampleRate / SafeFrequency));
}

float UResonanceForgeSynthComponent::ComputeModeExcitation(const int32 ModeIndex, const float StrikePosition)
{
    const float Position = FMath::Clamp(StrikePosition, 0.02f, 0.98f);
    const float ModeShape = FMath::Abs(FMath::Sin(UE_PI * static_cast<float>(FMath::Max(0, ModeIndex) + 1) * Position));
    return FMath::Lerp(0.12f, 1.0f, ModeShape);
}

bool UResonanceForgeSynthComponent::RenderOfflinePreview(
    const TArray<FResonanceMode>& Modes,
    const EResonanceModelType ModelType,
    const float Energy,
    const float Brightness,
    const float ObjectSize,
    const float StrikePosition,
    const int32 MidiNote,
    const float InStringDecay,
    const float InStringDamping,
    const float InBodyCoupling,
    const float InPickupPosition,
    const EResonanceExcitationType InExcitationType,
    const float DurationSeconds,
    const int32 SampleRate,
    TArray<float>& OutInterleavedStereo)
{
    const int32 SafeSampleRate = FMath::Clamp(SampleRate, 8000, 192000);
    const int32 NumFrames = FMath::Clamp(FMath::RoundToInt(DurationSeconds * SafeSampleRate), 1, SafeSampleRate * 12);
    UResonanceForgeSynthComponent* OfflineSynth = NewObject<UResonanceForgeSynthComponent>(GetTransientPackage());
    if (!OfflineSynth)
    {
        return false;
    }

    OfflineSynth->RenderSampleRate = static_cast<float>(SafeSampleRate);
    OfflineSynth->SynthesisModel = ModelType;
    OfflineSynth->StringDecay = FMath::Clamp(InStringDecay, 0.90f, 0.99999f);
    OfflineSynth->StringDamping = FMath::Clamp(InStringDamping, 0.0f, 1.0f);
    OfflineSynth->BodyCoupling = FMath::Clamp(InBodyCoupling, 0.0f, 1.0f);
    OfflineSynth->PickupPosition = FMath::Clamp(InPickupPosition, 0.0f, 1.0f);
    OfflineSynth->ExcitationType = InExcitationType;
    OfflineSynth->PitchScale = FMath::Lerp(1.35f, 0.72f, FMath::Clamp(ObjectSize, 0.0f, 1.0f));
    OfflineSynth->CustomModes = Modes;
    OfflineSynth->RebuildModesFrom(Modes.IsEmpty() ? GetBuiltInModes(TEXT("拉丝钢")) : Modes);
    OfflineSynth->InitializeWaveguideVoices();
    OfflineSynth->Strike(Energy, Brightness, MidiNote, StrikePosition);

    OutInterleavedStereo.SetNumZeroed(NumFrames * OfflineSynth->NumChannels);
    const int32 Written = OfflineSynth->OnGenerateAudio(OutInterleavedStereo.GetData(), OutInterleavedStereo.Num());
    return Written == OutInterleavedStereo.Num() && !OutInterleavedStereo.IsEmpty();
}

#if WITH_DEV_AUTOMATION_TESTS
bool UResonanceForgeSynthComponent::RenderWaveguideForTest(const int32 MidiNote, const int32 NumFrames, TArray<float>& OutSamples)
{
    RenderSampleRate = 48000.0f;
    RebuildModesFrom(GetEffectiveModes());
    InitializeWaveguideVoices();
    SynthesisModel = EResonanceModelType::WaveguideString;
    Strike(0.9f, 0.68f, MidiNote);
    OutSamples.SetNumZeroed(FMath::Max(1, NumFrames) * NumChannels);
    OnGenerateAudio(OutSamples.GetData(), OutSamples.Num());
    return !OutSamples.IsEmpty();
}
#endif

void UResonanceForgeSynthComponent::SetSynthesisModel(const EResonanceModelType NewModel)
{
    SynthesisModel = NewModel;
}

void UResonanceForgeSynthComponent::ApplyMaterialProfile(UResonanceMaterialProfile* NewProfile)
{
    MaterialProfile = NewProfile;
    CustomModes.Reset();
    if (MaterialProfile)
    {
        SynthesisModel = MaterialProfile->ModelType;
        StringDecay = MaterialProfile->StringDecay;
        StringDamping = MaterialProfile->StringDamping;
        BodyCoupling = MaterialProfile->BodyCoupling;
        PickupPosition = MaterialProfile->PickupPosition;
        ExcitationType = MaterialProfile->ExcitationType;
    }
    RequestModeRebuild();
}

void UResonanceForgeSynthComponent::SetCustomModes(const TArray<FResonanceMode>& NewModes)
{
    CustomModes.Reset(FMath::Min(NewModes.Num(), 16));
    for (const FResonanceMode& Source : NewModes)
    {
        if (CustomModes.Num() >= 16)
        {
            break;
        }
        FResonanceMode& Mode = CustomModes.AddDefaulted_GetRef();
        Mode.FrequencyHz = FMath::Clamp(Source.FrequencyHz, 40.0f, 12000.0f);
        Mode.Gain = FMath::Clamp(Source.Gain, 0.0f, 1.5f);
        Mode.DecaySeconds = FMath::Clamp(Source.DecaySeconds, 0.03f, 8.0f);
    }
    RequestModeRebuild();
}

void UResonanceForgeSynthComponent::ClearCustomModes()
{
    CustomModes.Reset();
    RequestModeRebuild();
}

TArray<FResonanceMode> UResonanceForgeSynthComponent::GetEffectiveModes() const
{
    if (!CustomModes.IsEmpty())
    {
        return CustomModes;
    }
    if (MaterialProfile && !MaterialProfile->Modes.IsEmpty())
    {
        return MaterialProfile->Modes;
    }
    return BuiltInModes;
}

void UResonanceForgeSynthComponent::LoadBuiltInPreset(const FName PresetName)
{
    BuiltInModes = GetBuiltInModes(PresetName);
    RequestModeRebuild();
}

void UResonanceForgeSynthComponent::InitializeWaveguideVoices()
{
    const int32 VoiceCount = FMath::Clamp(MaxStringVoices, 1, 16);
    const int32 MaximumDelay = ComputeWaveguideDelaySamples(20.0f, RenderSampleRate) + 2;
    WaveguideVoices.SetNum(VoiceCount);
    for (int32 Index = 0; Index < VoiceCount; ++Index)
    {
        FWaveguideVoice& Voice = WaveguideVoices[Index];
        Voice.DelayBuffer.SetNumZeroed(MaximumDelay);
        Voice.NoiseState = 0x9E3779B9u ^ static_cast<uint32>((Index + 1) * 747796405u);
    }
}

void UResonanceForgeSynthComponent::RequestModeRebuild()
{
    const TArray<FResonanceMode> Snapshot = GetEffectiveModes();
    SynthCommand([this, Snapshot]
    {
        RebuildModesFrom(Snapshot);
    });
}

void UResonanceForgeSynthComponent::RebuildModesFrom(const TArray<FResonanceMode>& SourceModes)
{
    ActiveModes.Reset(SourceModes.Num());
    for (const FResonanceMode& Source : SourceModes)
    {
        FActiveMode& Mode = ActiveModes.AddDefaulted_GetRef();
        Mode.BaseFrequencyHz = FMath::Max(20.0f, Source.FrequencyHz);
        Mode.FrequencyHz = FMath::Max(20.0f, Source.FrequencyHz);
        Mode.Gain = Source.Gain;
        const float Decay = FMath::Max(0.015f, Source.DecaySeconds * GlobalDecayScale);
        Mode.DecayMultiplier = FMath::Exp(-1.0f / (Decay * RenderSampleRate));
    }
}

void UResonanceForgeSynthComponent::ApplyStrike(const FStrikeEvent& Event)
{
    if (Event.ModelType == EResonanceModelType::WaveguideString)
    {
        StartWaveguideVoice(Event);
    }

    const float NotePitch = FMath::Pow(2.0f, (Event.MidiNote - 60.0f) / 12.0f);
    for (int32 Index = 0; Index < ActiveModes.Num(); ++Index)
    {
        FActiveMode& Mode = ActiveModes[Index];
        const float NormalizedIndex = ActiveModes.Num() > 1
            ? static_cast<float>(Index) / static_cast<float>(ActiveModes.Num() - 1)
            : 0.0f;
        const float SpectralTilt = FMath::Lerp(1.35f - NormalizedIndex, 0.35f + NormalizedIndex, Event.Brightness);
        Mode.FrequencyHz = FMath::Clamp(Mode.BaseFrequencyHz * NotePitch * Event.PitchScale, 20.0f, RenderSampleRate * 0.45f);
        const float ExcitationGain = Event.ModelType == EResonanceModelType::WaveguideString
            ? Event.Coupling
            : ComputeModeExcitation(Index, Event.StrikePosition);
        Mode.Envelope = FMath::Min(2.0f, Mode.Envelope + Event.Energy * Mode.Gain * SpectralTilt * ExcitationGain);
        Mode.Phase += 0.37f * Index;
    }
}

float UResonanceForgeSynthComponent::NextNoiseSample(uint32& State)
{
    State ^= State << 13;
    State ^= State >> 17;
    State ^= State << 5;
    return static_cast<float>(State & 0x00FFFFFFu) / static_cast<float>(0x007FFFFFu) - 1.0f;
}

void UResonanceForgeSynthComponent::StartWaveguideVoice(const FStrikeEvent& Event)
{
    if (WaveguideVoices.IsEmpty())
    {
        InitializeWaveguideVoices();
    }

    FWaveguideVoice* Target = nullptr;
    for (FWaveguideVoice& Voice : WaveguideVoices)
    {
        if (!Voice.bActive)
        {
            Target = &Voice;
            break;
        }
        if (!Target || Voice.EnergyEstimate < Target->EnergyEstimate)
        {
            Target = &Voice;
        }
    }
    if (!Target || Target->DelayBuffer.IsEmpty())
    {
        return;
    }

    const float Frequency = 440.0f * FMath::Pow(2.0f, (Event.MidiNote - 69.0f) / 12.0f) * Event.PitchScale;
    Target->DelaySamples = FMath::Min(
        ComputeWaveguideDelaySamples(Frequency, RenderSampleRate),
        Target->DelayBuffer.Num() - 1);
    Target->Cursor = 0;
    Target->LoopState = 0.0f;
    Target->LoopGain = FMath::Clamp(Event.Decay, 0.90f, 0.99999f);
    Target->Damping = FMath::Clamp(Event.Damping + (1.0f - Event.Brightness) * 0.42f, 0.02f, 0.98f);
    const float PhysicalPickupPosition = FMath::Lerp(0.04f, 0.50f, FMath::Clamp(Event.Pickup, 0.0f, 1.0f));
    Target->PickupOffset = FMath::Clamp(FMath::RoundToInt(Target->DelaySamples * PhysicalPickupPosition), 1, Target->DelaySamples - 1);
    Target->Gain = FMath::Clamp(Event.Energy, 0.0f, 1.5f);
    Target->EnergyEstimate = Target->Gain;
    Target->ExcitationType = Event.ExcitationType;
    Target->BowOffset = FMath::Clamp(
        FMath::RoundToInt(Target->DelaySamples * FMath::Lerp(0.12f, 0.88f, Event.StrikePosition)),
        1,
        Target->DelaySamples - 1);
    Target->BowSamplesTotal = Event.ExcitationType == EResonanceExcitationType::Bow
        ? FMath::RoundToInt(RenderSampleRate * FMath::Lerp(0.85f, 3.20f, FMath::Clamp(Event.Energy, 0.0f, 1.0f)))
        : 0;
    Target->BowSamplesRemaining = Target->BowSamplesTotal;
    Target->BowVelocity = FMath::Lerp(0.035f, 0.22f, Event.Brightness);
    Target->BowPressure = FMath::Lerp(0.12f, 0.42f, FMath::Clamp(Event.Energy, 0.0f, 1.0f));
    Target->bActive = true;

    float PreviousNoise = 0.0f;
    float PreviousSmoothedNoise = 0.0f;
    const float ExcitationCenter = FMath::Lerp(0.12f, 0.88f, FMath::Clamp(Event.StrikePosition, 0.0f, 1.0f));
    const float HammerWidth = FMath::Lerp(0.085f, 0.030f, Event.Brightness);
    for (int32 Index = 0; Index < Target->DelaySamples; ++Index)
    {
        const float Noise = NextNoiseSample(Target->NoiseState);
        const float T = Target->DelaySamples > 1
            ? static_cast<float>(Index) / static_cast<float>(Target->DelaySamples - 1)
            : 0.0f;
        float Excitation = 0.0f;
        if (Event.ExcitationType == EResonanceExcitationType::Bow)
        {
            // 弓擦不是一次性位移：这里只放入极低电平的粗糙种子，持续能量在渲染环中注入。
            Excitation = Noise * FMath::Sin(PI * T) * 0.012f;
        }
        else if (Event.ExcitationType == EResonanceExcitationType::Finger)
        {
            const float SmoothedNoise = 0.50f * PreviousSmoothedNoise + 0.25f * (Noise + PreviousNoise);
            const float Distance = (T - ExcitationCenter) / 0.24f;
            const float TouchFocus = FMath::Exp(-0.5f * Distance * Distance);
            const float SpatialEnvelope = FMath::Sin(PI * T) * FMath::Lerp(0.38f, 1.0f, TouchFocus);
            Excitation = FMath::Lerp(SmoothedNoise, Noise, 0.10f + Event.Brightness * 0.28f)
                * SpatialEnvelope * 0.72f;
            PreviousSmoothedNoise = SmoothedNoise;
        }
        else if (Event.ExcitationType == EResonanceExcitationType::Hammer)
        {
            const float Distance = (T - ExcitationCenter) / FMath::Max(0.01f, HammerWidth);
            const float BipolarPulse = -Distance * FMath::Exp(-0.5f * Distance * Distance);
            Excitation = BipolarPulse * (0.78f + Event.Brightness * 0.26f) + Noise * 0.055f;
        }
        else
        {
            const float LeftSpan = FMath::Max(0.02f, ExcitationCenter);
            const float RightSpan = FMath::Max(0.02f, 1.0f - ExcitationCenter);
            const float PluckShape = T <= ExcitationCenter
                ? T / LeftSpan
                : (1.0f - T) / RightSpan;
            const float ShapedNoise = FMath::Lerp(0.5f * (Noise + PreviousNoise), Noise, Event.Brightness);
            Excitation = FMath::Clamp(PluckShape, 0.0f, 1.0f)
                * (0.40f + ShapedNoise * 0.32f);
        }
        Target->DelayBuffer[Index] = Excitation * Target->Gain;
        PreviousNoise = Noise;
    }
}

float UResonanceForgeSynthComponent::RenderWaveguideVoices()
{
    float Output = 0.0f;
    for (FWaveguideVoice& Voice : WaveguideVoices)
    {
        if (!Voice.bActive || Voice.DelaySamples < 2)
        {
            continue;
        }

        const int32 NextIndex = (Voice.Cursor + 1) % Voice.DelaySamples;
        if (Voice.ExcitationType == EResonanceExcitationType::Bow && Voice.BowSamplesRemaining > 0)
        {
            const int32 BowIndex = (Voice.Cursor + Voice.BowOffset) % Voice.DelaySamples;
            const int32 BowNeighbor = (BowIndex + Voice.DelaySamples - 1) % Voice.DelaySamples;
            const float StringVelocity = Voice.DelayBuffer[BowIndex] - Voice.DelayBuffer[BowNeighbor];
            const int32 ElapsedSamples = Voice.BowSamplesTotal - Voice.BowSamplesRemaining;
            const float Attack = FMath::Clamp(ElapsedSamples / FMath::Max(1.0f, RenderSampleRate * 0.025f), 0.0f, 1.0f);
            const float Release = FMath::Clamp(Voice.BowSamplesRemaining / FMath::Max(1.0f, RenderSampleRate * 0.14f), 0.0f, 1.0f);
            const float StrokeEnvelope = FMath::Min(
                Attack * Attack * (3.0f - 2.0f * Attack),
                Release * Release * (3.0f - 2.0f * Release));
            const float RelativeVelocity = Voice.BowVelocity * StrokeEnvelope - StringVelocity;
            const float FrictionTable = FMath::Clamp(
                FMath::Pow(FMath::Abs(RelativeVelocity) * 3.8f + 0.75f, -4.0f),
                0.0f,
                1.0f);
            const float Friction = RelativeVelocity * FrictionTable * Voice.BowPressure * StrokeEnvelope * 0.24f;
            Voice.DelayBuffer[BowIndex] = FMath::Clamp(Voice.DelayBuffer[BowIndex] + Friction, -1.8f, 1.8f);
            --Voice.BowSamplesRemaining;
        }
        const float Current = Voice.DelayBuffer[Voice.Cursor];
        const int32 PickupIndex = (Voice.Cursor + Voice.PickupOffset) % Voice.DelaySamples;
        const float PickupSample = Current - Voice.DelayBuffer[PickupIndex] * 0.72f;
        const float Average = 0.5f * (Current + Voice.DelayBuffer[NextIndex]);
        Voice.LoopState = FMath::Lerp(Average, Voice.LoopState, Voice.Damping);
        Voice.DelayBuffer[Voice.Cursor] = Voice.LoopState * Voice.LoopGain;
        Voice.Cursor = NextIndex;
        Voice.EnergyEstimate = FMath::Lerp(Voice.EnergyEstimate, FMath::Abs(Current), 0.0025f) * 0.99996f;
        if (Voice.BowSamplesRemaining > 0)
        {
            Voice.EnergyEstimate = FMath::Max(Voice.EnergyEstimate, 0.0015f);
        }
        Output += PickupSample * 1.25f;

        if (Voice.BowSamplesRemaining <= 0 && Voice.EnergyEstimate < 0.00008f)
        {
            Voice.bActive = false;
        }
    }
    return Output;
}

int32 UResonanceForgeSynthComponent::OnGenerateAudio(float* OutAudio, const int32 NumSamples)
{
    FStrikeEvent Event;
    while (PendingStrikes.Dequeue(Event))
    {
        ApplyStrike(Event);
    }

    for (int32 SampleIndex = 0; SampleIndex < NumSamples; SampleIndex += NumChannels)
    {
        float Mono = RenderWaveguideVoices() * 0.72f;
        for (FActiveMode& Mode : ActiveModes)
        {
            Mono += FMath::Sin(Mode.Phase) * Mode.Envelope;
            Mode.Phase = FMath::Fmod(Mode.Phase + UE_TWO_PI * Mode.FrequencyHz / RenderSampleRate, UE_TWO_PI);
            Mode.Envelope *= Mode.DecayMultiplier;
        }

        const float Output = FMath::Tanh(Mono * 0.22f);
        OutAudio[SampleIndex] = Output;
        if (SampleIndex + 1 < NumSamples)
        {
            OutAudio[SampleIndex + 1] = Output;
        }
    }
    return NumSamples;
}
