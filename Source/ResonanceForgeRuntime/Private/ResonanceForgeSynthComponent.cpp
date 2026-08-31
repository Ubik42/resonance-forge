#include "ResonanceForgeSynthComponent.h"

#include "Math/UnrealMathUtility.h"

// 数字波导弦的延迟线、噪声激励与环路低通结构参考 STK Plucked。
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
    RebuildModes();
    InitializeWaveguideVoices();
    return true;
}

void UResonanceForgeSynthComponent::Strike(const float Energy, const float Brightness, const int32 MidiNote)
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
        PitchScale
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

#if WITH_DEV_AUTOMATION_TESTS
bool UResonanceForgeSynthComponent::RenderWaveguideForTest(const int32 MidiNote, const int32 NumFrames, TArray<float>& OutSamples)
{
    RenderSampleRate = 48000.0f;
    RebuildModes();
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
    if (MaterialProfile)
    {
        SynthesisModel = MaterialProfile->ModelType;
        StringDecay = MaterialProfile->StringDecay;
        StringDamping = MaterialProfile->StringDamping;
        BodyCoupling = MaterialProfile->BodyCoupling;
    }
    RebuildModes();
}

void UResonanceForgeSynthComponent::LoadBuiltInPreset(const FName PresetName)
{
    BuiltInModes = GetBuiltInModes(PresetName);
    RebuildModes();
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

void UResonanceForgeSynthComponent::RebuildModes()
{
    const TArray<FResonanceMode>& SourceModes = MaterialProfile && !MaterialProfile->Modes.IsEmpty()
        ? MaterialProfile->Modes
        : BuiltInModes;

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
            : 1.0f;
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
    Target->Gain = FMath::Clamp(Event.Energy, 0.0f, 1.5f);
    Target->EnergyEstimate = Target->Gain;
    Target->bActive = true;

    float PreviousNoise = 0.0f;
    for (int32 Index = 0; Index < Target->DelaySamples; ++Index)
    {
        const float Noise = NextNoiseSample(Target->NoiseState);
        const float ShapedNoise = FMath::Lerp(0.5f * (Noise + PreviousNoise), Noise, Event.Brightness);
        Target->DelayBuffer[Index] = ShapedNoise * Target->Gain * 0.62f;
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
        const float Current = Voice.DelayBuffer[Voice.Cursor];
        const float Average = 0.5f * (Current + Voice.DelayBuffer[NextIndex]);
        Voice.LoopState = FMath::Lerp(Average, Voice.LoopState, Voice.Damping);
        Voice.DelayBuffer[Voice.Cursor] = Voice.LoopState * Voice.LoopGain;
        Voice.Cursor = NextIndex;
        Voice.EnergyEstimate = FMath::Lerp(Voice.EnergyEstimate, FMath::Abs(Current), 0.0025f) * 0.99996f;
        Output += Current;

        if (Voice.EnergyEstimate < 0.00008f)
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
