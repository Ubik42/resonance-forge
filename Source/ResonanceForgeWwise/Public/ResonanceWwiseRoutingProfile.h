#pragma once

#include "Engine/DataAsset.h"
#include "ResonanceWwiseRoutingProfile.generated.h"

class UAkAudioEvent;
class UAkRtpc;

UCLASS(BlueprintType)
class RESONANCEFORGEWWISE_API UResonanceWwiseRoutingProfile final : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路由")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="材质 Event")
    TObjectPtr<UAkAudioEvent> SteelImpactEvent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="材质 Event")
    TObjectPtr<UAkAudioEvent> WoodImpactEvent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="材质 Event")
    TObjectPtr<UAkAudioEvent> GlassImpactEvent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Game Parameter")
    TObjectPtr<UAkRtpc> ImpactEnergyRtpc;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Game Parameter")
    TObjectPtr<UAkRtpc> ImpactBrightnessRtpc;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Game Parameter")
    TObjectPtr<UAkRtpc> ObjectSizeRtpc;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Game Parameter", meta=(ClampMin="0", ClampMax="2000"))
    int32 RtpcInterpolationMs = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="回退")
    bool bAllowDemoAssetFallback = true;

    UFUNCTION(BlueprintPure, Category="共振铸造台|Wwise")
    bool IsComplete() const;

    UFUNCTION(BlueprintPure, Category="共振铸造台|Wwise")
    FString GetMissingItemsText() const;
};
