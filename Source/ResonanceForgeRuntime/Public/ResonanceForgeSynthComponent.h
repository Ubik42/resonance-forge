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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|模态撞击体", meta=(TitleProperty="FrequencyHz"))
    TArray<FResonanceMode> CustomModes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|声学模型")
    EResonanceModelType SynthesisModel = EResonanceModelType::ModalImpact;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|数字波导弦", meta=(ClampMin="0.90", ClampMax="0.99999"))
    float StringDecay = 0.9965f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|数字波导弦", meta=(ClampMin="0.0", ClampMax="1.0"))
    float StringDamping = 0.36f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|数字波导弦", meta=(ClampMin="0.0", ClampMax="1.0"))
    float BodyCoupling = 0.22f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|数字波导弦", meta=(ClampMin="0.0", ClampMax="1.0", DisplayName="拾音位置"))
    float PickupPosition = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|数字波导弦", meta=(ClampMin="1", ClampMax="16"))
    int32 MaxStringVoices = 8;

    UFUNCTION(BlueprintCallable, Category="共振铸造台")
    void Strike(float Energy = 0.7f, float Brightness = 0.5f, int32 MidiNote = 60, float StrikePosition = 0.5f);

    UFUNCTION(BlueprintCallable, Category="共振铸造台")
    void LoadBuiltInPreset(FName PresetName);

    UFUNCTION(BlueprintCallable, Category="共振铸造台")
    void SetSynthesisModel(EResonanceModelType NewModel);

    UFUNCTION(BlueprintCallable, Category="共振铸造台")
    void ApplyMaterialProfile(UResonanceMaterialProfile* NewProfile);

    UFUNCTION(BlueprintCallable, Category="共振铸造台")
    void SetCustomModes(const TArray<FResonanceMode>& NewModes);

    UFUNCTION(BlueprintCallable, Category="共振铸造台")
    void ClearCustomModes();

    UFUNCTION(BlueprintPure, Category="共振铸造台")
    TArray<FResonanceMode> GetEffectiveModes() const;

    UFUNCTION(BlueprintPure, Category="共振铸造台")
    static TArray<FName> GetBuiltInPresetNames();

    UFUNCTION(BlueprintPure, Category="共振铸造台")
    static TArray<EResonanceModelType> GetSupportedModels();

    UFUNCTION(BlueprintPure, Category="共振铸造台")
    static TArray<FResonanceMode> GetBuiltInModes(FName PresetName);

    static int32 ComputeWaveguideDelaySamples(float FrequencyHz, float SampleRate);

    UFUNCTION(BlueprintPure, Category="共振铸造台")
    static float ComputeModeExcitation(int32 ModeIndex, float StrikePosition);

    static bool RenderOfflinePreview(
        const TArray<FResonanceMode>& Modes,
        EResonanceModelType ModelType,
        float Energy,
        float Brightness,
        float ObjectSize,
        float StrikePosition,
        int32 MidiNote,
        float StringDecay,
        float StringDamping,
        float BodyCoupling,
        float PickupPosition,
        float DurationSeconds,
        int32 SampleRate,
        TArray<float>& OutInterleavedStereo);

#if WITH_DEV_AUTOMATION_TESTS
    bool RenderWaveguideForTest(int32 MidiNote, int32 NumFrames, TArray<float>& OutSamples);
#endif

protected:
    virtual bool Init(int32& SampleRate) override;
    virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
    struct FStrikeEvent
    {
        float Energy;
        float Brightness;
        int32 MidiNote;
        EResonanceModelType ModelType;
        float Decay;
        float Damping;
        float Coupling;
        float Pickup;
        float PitchScale;
        float StrikePosition;
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

    struct FWaveguideVoice
    {
        TArray<float> DelayBuffer;
        int32 DelaySamples = 2;
        int32 Cursor = 0;
        uint32 NoiseState = 1;
        float LoopState = 0.0f;
        float LoopGain = 0.9965f;
        float Damping = 0.36f;
        int32 PickupOffset = 1;
        float Gain = 0.0f;
        float EnergyEstimate = 0.0f;
        bool bActive = false;
    };

    void RequestModeRebuild();
    void RebuildModesFrom(const TArray<FResonanceMode>& SourceModes);
    void ApplyStrike(const FStrikeEvent& Event);
    void InitializeWaveguideVoices();
    void StartWaveguideVoice(const FStrikeEvent& Event);
    float RenderWaveguideVoices();
    static float NextNoiseSample(uint32& State);

    TQueue<FStrikeEvent, EQueueMode::Mpsc> PendingStrikes;
    TArray<FActiveMode> ActiveModes;
    TArray<FResonanceMode> BuiltInModes;
    TArray<FWaveguideVoice> WaveguideVoices;
    float RenderSampleRate = 48000.0f;
};
