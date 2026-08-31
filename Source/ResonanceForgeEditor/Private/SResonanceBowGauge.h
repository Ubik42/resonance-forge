#pragma once

#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

class SResonanceBowGauge final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceBowGauge) {}
        SLATE_ATTRIBUTE(float, BowSpeed)
        SLATE_ATTRIBUTE(float, BowPressure)
        SLATE_ATTRIBUTE(bool, IndependentPressure)
        SLATE_ATTRIBUTE(bool, Connected)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        BowSpeed = InArgs._BowSpeed;
        BowPressure = InArgs._BowPressure;
        IndependentPressure = InArgs._IndependentPressure;
        Connected = InArgs._Connected;
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(520.0f, 92.0f);
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
        const float Speed = FMath::Clamp(BowSpeed.Get(0.55f), 0.0f, 1.0f);
        const float Pressure = FMath::Clamp(BowPressure.Get(0.55f), 0.0f, 1.0f);
        const bool bIndependent = IndependentPressure.Get(false);
        const bool bConnected = Connected.Get(false);
        const FLinearColor Copper(0.96f, 0.49f, 0.18f, 1.0f);
        const FLinearColor Mint(0.40f, 0.88f, 0.75f, 1.0f);
        const FLinearColor Brass(0.72f, 0.53f, 0.29f, 1.0f);
        const FLinearColor Rail(0.31f, 0.27f, 0.21f, 0.86f);
        const FLinearColor Muted(0.59f, 0.61f, 0.57f, 1.0f);

        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None,
            FLinearColor(0.018f, 0.015f, 0.012f, 1.0f));

        const TArray<FVector2D> TopRail = {FVector2D(12.0f, 10.0f), FVector2D(Size.X - 12.0f, 10.0f)};
        const TArray<FVector2D> BottomRail = {FVector2D(12.0f, Size.Y - 10.0f), FVector2D(Size.X - 12.0f, Size.Y - 10.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), TopRail, ESlateDrawEffect::None, Rail, true, 1.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), BottomRail, ESlateDrawEffect::None, Rail, true, 1.0f);

        auto DrawRivet = [&](const FVector2D Position)
        {
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(FVector2f(5.0f, 5.0f), FSlateLayoutTransform(FVector2f(Position.X - 2.5f, Position.Y - 2.5f))),
                FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Brass);
        };
        DrawRivet(FVector2D(16.0f, 14.0f));
        DrawRivet(FVector2D(Size.X - 16.0f, 14.0f));
        DrawRivet(FVector2D(16.0f, Size.Y - 14.0f));
        DrawRivet(FVector2D(Size.X - 16.0f, Size.Y - 14.0f));

        auto DrawDial = [&](const FVector2D Center, const float Value, const FLinearColor Color, const FString& Label, const FString& ValueText)
        {
            constexpr float Radius = 29.0f;
            TArray<FVector2D> Arc;
            Arc.Reserve(25);
            for (int32 Index = 0; Index <= 24; ++Index)
            {
                const float Angle = FMath::Lerp(-2.42f, -0.72f, static_cast<float>(Index) / 24.0f);
                Arc.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
            }
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(), Arc, ESlateDrawEffect::None, Rail, true, 2.0f);
            for (int32 Tick = 0; Tick <= 6; ++Tick)
            {
                const float Angle = FMath::Lerp(-2.42f, -0.72f, static_cast<float>(Tick) / 6.0f);
                const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
                const TArray<FVector2D> TickLine = {Center + Direction * 25.0f, Center + Direction * 31.0f};
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(), TickLine, ESlateDrawEffect::None, Brass, true, 1.0f);
            }
            const float NeedleAngle = FMath::Lerp(-2.42f, -0.72f, Value);
            const FVector2D NeedleEnd = Center + FVector2D(FMath::Cos(NeedleAngle), FMath::Sin(NeedleAngle)) * 25.0f;
            const TArray<FVector2D> Needle = {Center, NeedleEnd};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, Geometry.ToPaintGeometry(), Needle, ESlateDrawEffect::None, Color, true, 3.0f);
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 5,
                Geometry.ToPaintGeometry(FVector2f(7.0f, 7.0f), FSlateLayoutTransform(FVector2f(Center.X - 3.5f, Center.Y - 3.5f))),
                FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color);
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 5,
                Geometry.ToPaintGeometry(FVector2f(70.0f, 18.0f), FSlateLayoutTransform(FVector2f(Center.X - 34.0f, 10.0f))),
                Label, FAppStyle::GetFontStyle(TEXT("NormalFont")), ESlateDrawEffect::None, Color);
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 5,
                Geometry.ToPaintGeometry(FVector2f(90.0f, 16.0f), FSlateLayoutTransform(FVector2f(Center.X - 40.0f, Size.Y - 27.0f))),
                ValueText, FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Muted);
        };

        const FVector2D SpeedCenter(92.0f, 55.0f);
        const FVector2D PressureCenter(Size.X - 92.0f, 55.0f);
        const FString PressureValueText = bIndependent
            ? FString::Printf(TEXT("AT  %d%%"), FMath::RoundToInt(Pressure * 100.0f))
            : FString::Printf(TEXT("跟随  %d%%"), FMath::RoundToInt(Pressure * 100.0f));
        DrawDial(SpeedCenter, Speed, Copper, TEXT("弓速"), FString::Printf(TEXT("CC1  %d%%"), FMath::RoundToInt(Speed * 100.0f)));
        DrawDial(PressureCenter, Pressure, Mint, TEXT("弓压"), PressureValueText);

        const float HairLeft = 151.0f;
        const float HairRight = Size.X - 151.0f;
        const float HairCenterY = 51.0f;
        const float HairSpread = FMath::Lerp(7.0f, 2.0f, Pressure);
        for (int32 HairIndex = -2; HairIndex <= 2; ++HairIndex)
        {
            const float Offset = HairIndex * HairSpread * 0.5f;
            const TArray<FVector2D> Hair = {
                FVector2D(HairLeft, HairCenterY + Offset),
                FVector2D(HairRight, HairCenterY - Offset + (Speed - 0.5f) * 5.0f)};
            FSlateDrawElement::MakeLines(
                OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(), Hair,
                ESlateDrawEffect::None, Brass.CopyWithNewOpacity(0.34f + Pressure * 0.11f), true, 1.0f);
        }
        const float ContactX = FMath::Lerp(HairLeft + 14.0f, HairRight - 14.0f, Speed);
        const TArray<FVector2D> Contact = {
            FVector2D(ContactX, HairCenterY - 14.0f), FVector2D(ContactX, HairCenterY + 14.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, Geometry.ToPaintGeometry(), Contact, ESlateDrawEffect::None, Copper, true, 2.0f);

        const FString RouteText = bConnected
            ? (bIndependent ? TEXT("双路分控") : TEXT("CC1 联动"))
            : TEXT("试听标尺");
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 5,
            Geometry.ToPaintGeometry(FVector2f(100.0f, 16.0f), FSlateLayoutTransform(FVector2f(Size.X * 0.5f - 46.0f, 12.0f))),
            RouteText, FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None,
            bConnected ? (bIndependent ? Mint : Copper) : Muted);

        return LayerId + 5;
    }

private:
    TAttribute<float> BowSpeed;
    TAttribute<float> BowPressure;
    TAttribute<bool> IndependentPressure;
    TAttribute<bool> Connected;
};
