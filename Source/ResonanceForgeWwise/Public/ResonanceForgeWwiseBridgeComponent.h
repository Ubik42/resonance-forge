#pragma once

#include "Components/ActorComponent.h"
#include "ResonanceForgeWwiseBridgeComponent.generated.h"

class UAkAudioEvent;
class UAkRtpc;
class UResonanceForgeSynthComponent;

USTRUCT(BlueprintType)
struct RESONANCEFORGEWWISE_API FResonanceForgeImpactParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Energy = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Brightness = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ObjectSize = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台", meta=(ClampMin="0", ClampMax="127"))
    int32 MidiNote = 60;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台")
    FName MaterialPreset = TEXT("拉丝钢");
};

UCLASS(ClassGroup=Audio, BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="共振铸造台 Wwise 桥接器"))
class RESONANCEFORGEWWISE_API UResonanceForgeWwiseBridgeComponent final : public UActorComponent
{
    GENERATED_BODY()

public:
    UResonanceForgeWwiseBridgeComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|Wwise")
    TObjectPtr<UAkAudioEvent> SteelImpactEvent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|Wwise|材质路由")
    TObjectPtr<UAkAudioEvent> WoodImpactEvent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|Wwise|材质路由")
    TObjectPtr<UAkAudioEvent> GlassImpactEvent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|Wwise|Game Parameter")
    TObjectPtr<UAkRtpc> ImpactEnergyRtpc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|Wwise|Game Parameter")
    TObjectPtr<UAkRtpc> ImpactBrightnessRtpc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|Wwise|Game Parameter")
    TObjectPtr<UAkRtpc> ObjectSizeRtpc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|Wwise|Game Parameter", meta=(ClampMin="0", ClampMax="2000"))
    int32 RtpcInterpolationMs = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|监听")
    bool bLayerNativeSynth = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振铸造台|Wwise")
    bool bAutoBindGeneratedAssets = true;

    UFUNCTION(BlueprintCallable, Category="共振铸造台|Wwise")
    int32 TriggerImpact(const FResonanceForgeImpactParameters& Parameters, UResonanceForgeSynthComponent* NativeSynth = nullptr);

    UFUNCTION(BlueprintCallable, Category="共振铸造台|Wwise")
    bool AutoBindDemoAssets();

    UFUNCTION(BlueprintPure, Category="共振铸造台|Wwise")
    FString GetIntegrationStatus() const;

    UFUNCTION(BlueprintPure, Category="共振铸造台|Wwise")
    static float ToWwiseRtpc(float NormalizedValue);

protected:
    virtual void BeginPlay() override;
};
