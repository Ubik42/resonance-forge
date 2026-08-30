#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ResonanceMaterialProfile.generated.h"

USTRUCT(BlueprintType)
struct RESONANCEFORGERUNTIME_API FResonanceMode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振")
    float FrequencyHz = 440.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振")
    float Gain = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共振")
    float DecaySeconds = 0.8f;
};

UCLASS(BlueprintType)
class RESONANCEFORGERUNTIME_API UResonanceMaterialProfile final : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="材质")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="材质", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Hardness = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="材质", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Roughness = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="共振")
    TArray<FResonanceMode> Modes;
};
