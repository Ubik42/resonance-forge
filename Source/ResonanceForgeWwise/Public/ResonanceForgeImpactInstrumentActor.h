#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "MIDIDeviceController.h"
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|撞击", meta=(ClampMin="0.0"))
    float MinimumImpulse = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|撞击", meta=(ClampMin="0.00001"))
    float ImpulseSensitivity = 0.00008f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|撞击", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ObjectSize = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|撞击", meta=(ClampMin="0.0"))
    float RetriggerCooldownSeconds = 0.06f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|键盘")
    bool bEnableKeyboardTrigger = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|键盘")
    FKey KeyboardTriggerKey = EKeys::One;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|MIDI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MidiBrightness = 0.55f;

    UPROPERTY(BlueprintAssignable, Category="共振铸造台|事件")
    FOnResonanceImpactTriggered OnImpactTriggered;

    UFUNCTION(BlueprintCallable, Category="共振铸造台")
    int32 TriggerInstrument(float Energy = 0.75f, float Brightness = 0.55f, int32 MidiNote = 60);

    UFUNCTION(BlueprintCallable, Category="共振铸造台|MIDI")
    bool ConnectMidiInput(int32 DeviceId);

    UFUNCTION(BlueprintCallable, Category="共振铸造台|MIDI")
    void DisconnectMidiInput();

    UFUNCTION(BlueprintPure, Category="共振铸造台|MIDI")
    bool IsMidiConnected() const;

    UFUNCTION(BlueprintPure, Category="共振铸造台|撞击")
    static float ComputeImpactEnergy(float ImpulseMagnitude, float MinimumRequiredImpulse, float Sensitivity);

    UFUNCTION(BlueprintPure, Category="共振铸造台|撞击")
    static float ComputeImpactBrightness(float RelativeSpeed);

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
