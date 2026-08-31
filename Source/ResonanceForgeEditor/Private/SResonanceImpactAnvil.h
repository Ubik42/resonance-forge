#pragma once

#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

class SResonanceImpactAnvil final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceImpactAnvil) {}
        SLATE_ATTRIBUTE(float, MinimumImpulse)
        SLATE_ATTRIBUTE(float, Sensitivity)
        SLATE_ATTRIBUTE(float, LastImpulse)
        SLATE_ATTRIBUTE(float, LastRelativeSpeed)
        SLATE_ATTRIBUTE(float, LastEnergy)
        SLATE_ATTRIBUTE(float, LastBrightness)
        SLATE_ATTRIBUTE(bool, HasCollision)
        SLATE_ATTRIBUTE(bool, PassedThreshold)
        SLATE_ATTRIBUTE(FText, CalibrationName)
        SLATE_ARGUMENT(const TArray<float>*, SampleImpulses)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        MinimumImpulse = InArgs._MinimumImpulse;
        Sensitivity = InArgs._Sensitivity;
        LastImpulse = InArgs._LastImpulse;
        LastRelativeSpeed = InArgs._LastRelativeSpeed;
        LastEnergy = InArgs._LastEnergy;
        LastBrightness = InArgs._LastBrightness;
        HasCollision = InArgs._HasCollision;
        PassedThreshold = InArgs._PassedThreshold;
        CalibrationName = InArgs._CalibrationName;
        SampleImpulses = InArgs._SampleImpulses;
        SetCanTick(false);
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(900.0f, 158.0f);
    }

    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& Geometry,
        const FSlateRect& CullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& WidgetStyle,
        bool bParentEnabled) const override
    {
        const FVector2D Size = Geometry.GetLocalSize();
        const FSlateBrush* White = FAppStyle::GetBrush(TEXT("WhiteBrush"));
        const FLinearColor Coal(0.012f, 0.016f, 0.016f, 1.0f);
        const FLinearColor Iron(0.20f, 0.25f, 0.25f, 1.0f);
        const FLinearColor Hot(0.96f, 0.52f, 0.22f, 1.0f);
        const FLinearColor Cyan(0.43f, 0.90f, 0.77f, 1.0f);
        const FLinearColor Muted(0.58f, 0.63f, 0.61f, 1.0f);

        FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), White, ESlateDrawEffect::None, Coal);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
            Geometry.ToPaintGeometry(FVector2f(180.0f, 22.0f), FSlateLayoutTransform(FVector2f(18.0f, 12.0f))),
            FText::FromString(TEXT("冲量标定砧尺")), FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Hot);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
            Geometry.ToPaintGeometry(FVector2f(240.0f, 20.0f), FSlateLayoutTransform(FVector2f(190.0f, 13.0f))),
            CalibrationName.Get(FText::GetEmpty()), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Muted);

        const float Gate = FMath::Max(0.0f, MinimumImpulse.Get(0.0f));
        const float Gain = FMath::Max(0.000001f, Sensitivity.Get(0.00008f));
        const float FullImpulse = Gate + 1.0f / Gain;
        const float HalfImpulse = Gate + 0.5f / Gain;
        const float RailX0 = 38.0f;
        const float RailX1 = Size.X * 0.68f;
        const float RailY = 86.0f;
        auto ImpulseX = [RailX0, RailX1, FullImpulse](const float Impulse)
        {
            return FMath::Lerp(RailX0, RailX1, FMath::Clamp(Impulse / FMath::Max(1.0f, FullImpulse), 0.0f, 1.0f));
        };

        TArray<FVector2D> Rail = {FVector2D(RailX0, RailY), FVector2D(RailX1, RailY)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), Rail, ESlateDrawEffect::None, Iron, true, 5.0f);
        TArray<FVector2D> HotRail = {FVector2D(ImpulseX(Gate), RailY), FVector2D(RailX1, RailY)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(), HotRail, ESlateDrawEffect::None, Hot.CopyWithNewOpacity(0.62f), true, 5.0f);

        if (SampleImpulses)
        {
            for (int32 Index = 0; Index < SampleImpulses->Num(); ++Index)
            {
                const float Impulse = (*SampleImpulses)[Index];
                const float X = ImpulseX(Impulse);
                const float Top = RailY - 19.0f - static_cast<float>(Index % 3) * 4.0f;
                TArray<FVector2D> Stamp = {FVector2D(X, Top), FVector2D(X, RailY - 5.0f)};
                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId + 3,
                    Geometry.ToPaintGeometry(),
                    Stamp,
                    ESlateDrawEffect::None,
                    Impulse >= Gate ? Cyan.CopyWithNewOpacity(0.72f) : Hot.CopyWithNewOpacity(0.72f),
                    true,
                    2.0f);
            }
        }

        const float Ticks[] = {0.0f, Gate, HalfImpulse, FullImpulse};
        const FString Labels[] = {
            TEXT("静默"),
            FString::Printf(TEXT("门槛 %.0f"), Gate),
            FString::Printf(TEXT("半响 %.0f"), HalfImpulse),
            FString::Printf(TEXT("满响 %.0f"), FullImpulse)};
        for (int32 Index = 0; Index < 4; ++Index)
        {
            const float X = ImpulseX(Ticks[Index]);
            TArray<FVector2D> Tick = {FVector2D(X, RailY - 11.0f), FVector2D(X, RailY + 11.0f)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(), Tick, ESlateDrawEffect::None, Index == 1 ? Cyan : Muted, true, Index == 1 ? 3.0f : 1.0f);
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 3,
                Geometry.ToPaintGeometry(FVector2f(115.0f, 18.0f), FSlateLayoutTransform(FVector2f(X - (Index == 3 ? 78.0f : 18.0f), RailY + 16.0f))),
                FText::FromString(Labels[Index]), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Index == 1 ? Cyan : Muted);
        }

        if (HasCollision.Get(false))
        {
            const float Impact = LastImpulse.Get(0.0f);
            const float X = ImpulseX(Impact);
            const FLinearColor Marker = PassedThreshold.Get(false) ? Cyan : Hot;
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 4,
                Geometry.ToPaintGeometry(FVector2f(13.0f, 28.0f), FSlateLayoutTransform(FVector2f(X - 6.5f, RailY - 34.0f))),
                White, ESlateDrawEffect::None, Marker);
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4,
                Geometry.ToPaintGeometry(FVector2f(210.0f, 18.0f), FSlateLayoutTransform(FVector2f(FMath::Clamp(X - 70.0f, RailX0, RailX1 - 150.0f), 42.0f))),
                FText::FromString(FString::Printf(TEXT("最近撞击 %.0f · 能量 %.0f%%"), Impact, LastEnergy.Get(0.0f) * 100.0f)),
                FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Marker);
        }
        else
        {
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(FVector2f(260.0f, 18.0f), FSlateLayoutTransform(FVector2f(RailX0, 43.0f))),
                FText::FromString(TEXT("等待 PIE 碰撞 · 标尺先显示目标响度")), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Muted);
        }

        const float RightX = RailX1 + 42.0f;
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
            Geometry.ToPaintGeometry(FVector2f(Size.X - RightX - 14.0f, 20.0f), FSlateLayoutTransform(FVector2f(RightX, 48.0f))),
            FText::FromString(HasCollision.Get(false)
                ? FString::Printf(TEXT("相对速度 %.0f cm/s"), LastRelativeSpeed.Get(0.0f))
                : TEXT("速度火花")),
            FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Cyan);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
            Geometry.ToPaintGeometry(FVector2f(Size.X - RightX - 14.0f, 20.0f), FSlateLayoutTransform(FVector2f(RightX, 77.0f))),
            FText::FromString(HasCollision.Get(false)
                ? FString::Printf(TEXT("明亮度 %.0f%%"), LastBrightness.Get(0.0f) * 100.0f)
                : TEXT("越快越亮 · 不改变响度门槛")),
            FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Hot);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
            Geometry.ToPaintGeometry(FVector2f(Size.X - RightX - 14.0f, 32.0f), FSlateLayoutTransform(FVector2f(RightX, 108.0f))),
            FText::FromString(TEXT("冲量管响度，速度管音色")), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Muted);

        return LayerId + 4;
    }

private:
    TAttribute<float> MinimumImpulse;
    TAttribute<float> Sensitivity;
    TAttribute<float> LastImpulse;
    TAttribute<float> LastRelativeSpeed;
    TAttribute<float> LastEnergy;
    TAttribute<float> LastBrightness;
    TAttribute<bool> HasCollision;
    TAttribute<bool> PassedThreshold;
    TAttribute<FText> CalibrationName;
    const TArray<float>* SampleImpulses = nullptr;
};
