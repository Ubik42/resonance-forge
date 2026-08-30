#pragma once

#include "Components/SynthComponent.h"
#include "Containers/Queue.h"
#include "ResonanceMaterialProfile.h"
#include "ResonanceForgeSynthComponent.generated.h"

UCLASS(ClassGroup=Audio, BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="共振声源"))
class RESONANCEFORGERUNTIME_API UResonanceForgeSynthComponent final : public USynthComponent
{
    GENERATED_BODY()

public:
    UResonanceForgeSynthComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台")
    TObjectPtr<UResonanceMaterialProfile> MaterialProfile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台", meta=(ClampMin="0.1", ClampMax="4.0"))
    float GlobalDecayScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台", meta=(ClampMin="0.25", ClampMax="4.0"))
    float PitchScale = 1.0f;

    UFUNCTION(BlueprintCallable, Category="共振铸造台")
    void Strike(float Energy = 0.7f, float Brightness = 0.5f, int32 MidiNote = 60);

    UFUNCTION(BlueprintCallable, Category="共振铸造台")
    void LoadBuiltInPreset(FName PresetName);

    UFUNCTION(BlueprintPure, Category="共振铸造台")
    static TArray<FName> GetBuiltInPresetNames();

protected:
    virtual bool Init(int32& SampleRate) override;
    virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
    struct FStrikeEvent
    {
        float Energy;
        float Brightness;
        int32 MidiNote;
    };

    struct FActiveMode
    {
        float BaseFrequencyHz = 440.0f;
        float FrequencyHz = 440.0f;
        float Gain = 0.0f;
        float Phase = 0.0f;
        float Envelope = 0.0f;
        float DecayMultiplier = 0.999f;
    };

    void RebuildModes();
    void ApplyStrike(const FStrikeEvent& Event);

    TQueue<FStrikeEvent, EQueueMode::Mpsc> PendingStrikes;
    TArray<FActiveMode> ActiveModes;
    TArray<FResonanceMode> BuiltInModes;
    float RenderSampleRate = 48000.0f;
};
