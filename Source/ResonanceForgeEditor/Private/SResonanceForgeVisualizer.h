#pragma once

#include "Rendering/DrawElements.h"
#include "ResonanceMaterialProfile.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

class SResonanceForgeVisualizer final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceForgeVisualizer) {}
        SLATE_ATTRIBUTE(EResonanceModelType, ModelType)
        SLATE_ATTRIBUTE(FName, PresetName)
        SLATE_ATTRIBUTE(float, Energy)
        SLATE_ATTRIBUTE(float, Brightness)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        ModelType = InArgs._ModelType;
        PresetName = InArgs._PresetName;
        Energy = InArgs._Energy;
        Brightness = InArgs._Brightness;
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(520.0f, 210.0f);
    }

    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override
    {
        const FVector2D Size = AllottedGeometry.GetLocalSize();
        const FLinearColor Background(0.012f, 0.018f, 0.026f, 1.0f);
        const FLinearColor Grid(0.10f, 0.15f, 0.18f, 0.45f);
        const FLinearColor Accent = ModelType.Get(EResonanceModelType::ModalImpact) == EResonanceModelType::WaveguideString
            ? FLinearColor(0.98f, 0.46f, 0.12f, 1.0f)
            : FLinearColor(0.06f, 0.82f, 0.92f, 1.0f);

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
            ESlateDrawEffect::None,
            Background);

        for (int32 Index = 1; Index < 6; ++Index)
        {
            const float X = Size.X * static_cast<float>(Index) / 6.0f;
            TArray<FVector2f> Vertical = {{X, 0.0f}, {X, static_cast<float>(Size.Y)}};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Vertical, ESlateDrawEffect::None, Grid, true, 1.0f);
        }
        for (int32 Index = 1; Index < 4; ++Index)
        {
            const float Y = Size.Y * static_cast<float>(Index) / 4.0f;
            TArray<FVector2f> Horizontal = {{0.0f, Y}, {static_cast<float>(Size.X), Y}};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Horizontal, ESlateDrawEffect::None, Grid, true, 1.0f);
        }

        const float EnergyValue = FMath::Clamp(Energy.Get(0.75f), 0.0f, 1.0f);
        const float BrightnessValue = FMath::Clamp(Brightness.Get(0.55f), 0.0f, 1.0f);
        if (ModelType.Get(EResonanceModelType::ModalImpact) == EResonanceModelType::WaveguideString)
        {
            TArray<FVector2f> StringPoints;
            constexpr int32 SegmentCount = 96;
            StringPoints.Reserve(SegmentCount);
            for (int32 Index = 0; Index < SegmentCount; ++Index)
            {
                const float T = static_cast<float>(Index) / static_cast<float>(SegmentCount - 1);
                const float Envelope = FMath::Sin(PI * T);
                const float Harmonic = FMath::Sin(T * PI * FMath::Lerp(5.0f, 13.0f, BrightnessValue));
                const float Y = Size.Y * 0.5f - Harmonic * Envelope * Size.Y * 0.31f * EnergyValue;
                StringPoints.Add(FVector2f(T * Size.X, Y));
            }
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), StringPoints, ESlateDrawEffect::None, Accent, true, 2.2f);
        }
        else
        {
            const bool bWood = PresetName.Get(FName(TEXT("拉丝钢"))) == TEXT("硬木");
            const bool bGlass = PresetName.Get(FName(TEXT("拉丝钢"))) == TEXT("薄玻璃");
            const TArray<float> Peaks = bWood
                ? TArray<float>{0.12f, 0.22f, 0.36f, 0.52f, 0.70f, 0.86f}
                : bGlass
                    ? TArray<float>{0.18f, 0.32f, 0.47f, 0.62f, 0.76f, 0.88f, 0.96f}
                    : TArray<float>{0.10f, 0.20f, 0.33f, 0.47f, 0.61f, 0.74f, 0.86f, 0.95f};
            for (int32 Index = 0; Index < Peaks.Num(); ++Index)
            {
                const float NormalizedIndex = Peaks.Num() > 1 ? static_cast<float>(Index) / static_cast<float>(Peaks.Num() - 1) : 0.0f;
                const float Tilt = FMath::Lerp(1.0f - NormalizedIndex * 0.62f, 0.35f + NormalizedIndex * 0.65f, BrightnessValue);
                const float Height = Size.Y * 0.78f * EnergyValue * Tilt;
                const float X = Peaks[Index] * Size.X;
                TArray<FVector2f> Peak = {{X, static_cast<float>(Size.Y - 10.0f)}, {X, static_cast<float>(Size.Y - 10.0f - Height)}};
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Peak, ESlateDrawEffect::None, Accent, true, Index == 0 ? 3.0f : 1.7f);
            }
        }

        return LayerId + 2;
    }

private:
    TAttribute<EResonanceModelType> ModelType;
    TAttribute<FName> PresetName;
    TAttribute<float> Energy;
    TAttribute<float> Brightness;
};
