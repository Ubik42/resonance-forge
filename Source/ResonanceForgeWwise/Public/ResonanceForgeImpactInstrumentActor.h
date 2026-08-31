#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "MIDIDeviceController.h"
#include "ResonanceMaterialProfile.h"
#include "ResonanceForgeImpactInstrumentActor.generated.h"

class UResonanceForgeSynthComponent;
class UResonanceForgeWwiseBridgeComponent;
class UStaticMeshComponent;
class UMIDIDeviceController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FOnResonanceImpactTriggered,
    float, Energy,
    float, Brightness,
    float, ObjectSize,
    int32, MidiNote);

UENUM(BlueprintType)
enum class EResonanceForgeListenMode : uint8
{
    NativeOnly UMETA(DisplayName="原声炉"),
    WwiseOnly UMETA(DisplayName="Wwise 出口"),
    Layered UMETA(DisplayName="双路叠听")
};

UENUM(BlueprintType)
enum class EResonanceVelocityCurve : uint8
{
    SoftTouch UMETA(DisplayName="软触"),
    Linear UMETA(DisplayName="线性"),
    HeavyHand UMETA(DisplayName="重手")
};

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="共振铸造台撞击乐器"))
class RESONANCEFORGEWWISE_API AResonanceForgeImpactInstrumentActor final : public AActor
{
    GENERATED_BODY()

public:
    AResonanceForgeImpactInstrumentActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|组件")
    TObjectPtr<UStaticMeshComponent> InstrumentMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|组件")
    TObjectPtr<UResonanceForgeSynthComponent> NativeSynth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|组件")
    TObjectPtr<UResonanceForgeWwiseBridgeComponent> WwiseBridge;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|材质")
    FName ResonancePreset = TEXT("拉丝钢");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|声学模型")
    EResonanceModelType SynthesisModel = EResonanceModelType::ModalImpact;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|监听", meta=(DisplayName="监听闸门"))
    EResonanceForgeListenMode ListenMode = EResonanceForgeListenMode::Layered;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|撞击", meta=(ClampMin="0.0"))
    float MinimumImpulse = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|撞击", meta=(ClampMin="0.00001"))
    float ImpulseSensitivity = 0.00008f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|撞击", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ObjectSize = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|撞击", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ManualStrikePosition = 0.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|撞击")
    float LastStrikePosition = 0.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|撞击")
    float LastImpactEnergy = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|撞击")
    float LastImpactBrightness = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|撞击")
    int32 ImpactSerial = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|撞击")
    float LastImpactWorldSeconds = -1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|撞击", meta=(ClampMin="0.0"))
    float RetriggerCooldownSeconds = 0.06f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|键盘")
    bool bEnableKeyboardTrigger = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|键盘")
    FKey KeyboardTriggerKey = EKeys::One;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|MIDI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MidiBrightness = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|MIDI", meta=(ClampMin="0.0", ClampMax="1.0", DisplayName="弓压"))
    float MidiBowPressure = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|演奏", meta=(ClampMin="-1.0", ClampMax="1.0", DisplayName="弓向"))
    float BowDirection = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|MIDI")
    bool bHasMidiAftertouch = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|MIDI")
    int32 LastMidiPressure = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|MIDI", meta=(DisplayName="力度响应"))
    EResonanceVelocityCurve VelocityCurve = EResonanceVelocityCurve::Linear;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|MIDI")
    int32 LastMidiNote = -1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|MIDI")
    int32 LastMidiVelocity = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|MIDI")
    int32 LastMidiControl = -1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="共振铸造台|MIDI")
    int32 LastMidiControlValue = 0;

    UPROPERTY(BlueprintAssignable, Category="共振铸造台|事件")
    FOnResonanceImpactTriggered OnImpactTriggered;

    UFUNCTION(BlueprintCallable, Category="共振铸造台")
    int32 TriggerInstrument(float Energy = 0.75f, float Brightness = 0.55f, int32 MidiNote = 60,
        float StrikePosition = -1.0f, bool bHoldNativeNote = false, float BowPressureOverride = -1.0f,
        float BowDirectionOverride = 0.0f);

    UFUNCTION(BlueprintPure, Category="共振铸造台|撞击")
    float ComputeNormalizedStrikePosition(const FVector& WorldImpactPoint) const;

    UFUNCTION(BlueprintCallable, Category="共振铸造台|MIDI")
    bool ConnectMidiInput(int32 DeviceId);

    UFUNCTION(BlueprintCallable, Category="共振铸造台|MIDI")
    void DisconnectMidiInput();

    UFUNCTION(BlueprintPure, Category="共振铸造台|MIDI")
    bool IsMidiConnected() const;

    UFUNCTION(BlueprintPure, Category="共振铸造台|MIDI")
    FString GetConnectedMidiDeviceName() const;

    UFUNCTION(BlueprintPure, Category="共振铸造台|撞击")
    static float ComputeImpactEnergy(float ImpulseMagnitude, float MinimumRequiredImpulse, float Sensitivity);

    UFUNCTION(BlueprintPure, Category="共振铸造台|撞击")
    static float ComputeImpactBrightness(float RelativeSpeed);

    UFUNCTION(BlueprintPure, Category="共振铸造台|监听")
    static bool ListenModeIncludesNative(EResonanceForgeListenMode Mode);

    UFUNCTION(BlueprintPure, Category="共振铸造台|监听")
    static bool ListenModeIncludesWwise(EResonanceForgeListenMode Mode);

    UFUNCTION(BlueprintPure, Category="共振铸造台|MIDI")
    static float ShapePerformanceVelocity(float NormalizedVelocity, EResonanceVelocityCurve Curve);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    UFUNCTION()
    void HandleMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    void HandleMidiEvent(UMIDIDeviceController* Controller, int32 Timestamp, EMIDIEventType EventType,
        int32 Channel, int32 ControlId, int32 Velocity, int32 RawEventType);

    void HandleKeyboardTrigger();

    UPROPERTY(Transient)
    TObjectPtr<UMIDIDeviceController> MidiController;

    double LastTriggerTime = -1000.0;
};
