#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ResonanceMaterialProfile.generated.h"

UENUM(BlueprintType)
enum class EResonanceModelType : uint8
{
    ModalImpact UMETA(DisplayName="模态撞击体"),
    WaveguideString UMETA(DisplayName="数字波导弦")
};

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
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="声学模型")
    EResonanceModelType ModelType = EResonanceModelType::ModalImpact;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="材质")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="材质", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Hardness = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="材质", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Roughness = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="共振")
    TArray<FResonanceMode> Modes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="数字波导弦", meta=(ClampMin="0.90", ClampMax="0.99999", EditCondition="ModelType == EResonanceModelType::WaveguideString", EditConditionHides))
    float StringDecay = 0.9965f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="数字波导弦", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="ModelType == EResonanceModelType::WaveguideString", EditConditionHides))
    float StringDamping = 0.36f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="数字波导弦", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="ModelType == EResonanceModelType::WaveguideString", EditConditionHides))
    float BodyCoupling = 0.22f;
};
