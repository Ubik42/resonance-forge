#include "ResonanceForgeSynthComponent.h"

#include "Math/UnrealMathUtility.h"

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
    return true;
}

void UResonanceForgeSynthComponent::Strike(const float Energy, const float Brightness, const int32 MidiNote)
{
    PendingStrikes.Enqueue({
        FMath::Clamp(Energy, 0.0f, 1.5f),
        FMath::Clamp(Brightness, 0.0f, 1.0f),
        FMath::Clamp(MidiNote, 0, 127)
    });
}

TArray<FName> UResonanceForgeSynthComponent::GetBuiltInPresetNames()
{
    return {TEXT("拉丝钢"), TEXT("硬木"), TEXT("薄玻璃")};
}

void UResonanceForgeSynthComponent::LoadBuiltInPreset(const FName PresetName)
{
    if (PresetName == TEXT("硬木"))
    {
        BuiltInModes = ResonanceForge::MakeModes({172.0f, 286.0f, 451.0f, 708.0f, 1098.0f, 1710.0f}, 0.54f);
    }
    else if (PresetName == TEXT("薄玻璃"))
    {
        BuiltInModes = ResonanceForge::MakeModes({392.0f, 615.0f, 933.0f, 1417.0f, 2150.0f, 3270.0f, 4860.0f}, 1.35f);
    }
    else
    {
        BuiltInModes = ResonanceForge::MakeModes({224.0f, 361.0f, 587.0f, 927.0f, 1438.0f, 2215.0f, 3390.0f, 5120.0f}, 1.05f);
    }
    RebuildModes();
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
    const float NotePitch = FMath::Pow(2.0f, (Event.MidiNote - 60.0f) / 12.0f);
    for (int32 Index = 0; Index < ActiveModes.Num(); ++Index)
    {
        FActiveMode& Mode = ActiveModes[Index];
        const float NormalizedIndex = ActiveModes.Num() > 1
            ? static_cast<float>(Index) / static_cast<float>(ActiveModes.Num() - 1)
            : 0.0f;
        const float SpectralTilt = FMath::Lerp(1.35f - NormalizedIndex, 0.35f + NormalizedIndex, Event.Brightness);
        Mode.FrequencyHz = FMath::Clamp(Mode.BaseFrequencyHz * NotePitch * PitchScale, 20.0f, RenderSampleRate * 0.45f);
        Mode.Envelope = FMath::Min(2.0f, Mode.Envelope + Event.Energy * Mode.Gain * SpectralTilt);
        Mode.Phase += 0.37f * Index;
    }
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
        float Mono = 0.0f;
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
