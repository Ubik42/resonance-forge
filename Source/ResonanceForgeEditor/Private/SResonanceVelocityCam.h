#pragma once

#include "Rendering/DrawElements.h"
#include "ResonanceForgeImpactInstrumentActor.h"
#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

class SResonanceVelocityCam final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceVelocityCam) {}
        SLATE_ATTRIBUTE(float, InputVelocity)
        SLATE_ATTRIBUTE(EResonanceVelocityCurve, Curve)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        InputVelocity = InArgs._InputVelocity;
        Curve = InArgs._Curve;
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(230.0f, 82.0f);
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
        const float Left = 18.0f;
        const float Right = Size.X - 14.0f;
        const float Top = 10.0f;
        const float Bottom = Size.Y - 18.0f;
        const FLinearColor Grid(0.29f, 0.34f, 0.32f, 0.65f);
        const FLinearColor Copper(0.96f, 0.49f, 0.18f, 1.0f);
        const FLinearColor Mint(0.40f, 0.88f, 0.75f, 1.0f);

        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None,
            FLinearColor(0.018f, 0.016f, 0.013f, 1.0f));

        TArray<FVector2D> Axes = {
            FVector2D(Left, Top), FVector2D(Left, Bottom), FVector2D(Right, Bottom)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), Axes, ESlateDrawEffect::None, Grid, true, 1.0f);

        const EResonanceVelocityCurve CurveType = Curve.Get(EResonanceVelocityCurve::Linear);
        TArray<FVector2D> CurvePoints;
        CurvePoints.Reserve(33);
        for (int32 Index = 0; Index <= 32; ++Index)
        {
            const float Input = static_cast<float>(Index) / 32.0f;
            const float Output = AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(Input, CurveType);
            CurvePoints.Add(FVector2D(FMath::Lerp(Left, Right, Input), FMath::Lerp(Bottom, Top, Output)));
        }
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(), CurvePoints, ESlateDrawEffect::None, Copper, true, 2.5f);

        const float Input = FMath::Clamp(InputVelocity.Get(0.0f), 0.0f, 1.0f);
        const float Output = AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(Input, CurveType);
        const float MarkerX = FMath::Lerp(Left, Right, Input);
        const float MarkerY = FMath::Lerp(Bottom, Top, Output);
        TArray<FVector2D> Guide = {FVector2D(MarkerX, Bottom), FVector2D(MarkerX, MarkerY), FVector2D(Left, MarkerY)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(), Guide, ESlateDrawEffect::None, Mint.CopyWithNewOpacity(0.52f), true, 1.0f);
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 4,
            Geometry.ToPaintGeometry(FVector2f(7.0f, 7.0f), FSlateLayoutTransform(FVector2f(MarkerX - 3.5f, MarkerY - 3.5f))),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Mint);

        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 5,
            Geometry.ToPaintGeometry(FVector2f(70.0f, 15.0f), FSlateLayoutTransform(FVector2f(Left, Bottom + 2.0f))),
            TEXT("输入力度"), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Grid);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 5,
            Geometry.ToPaintGeometry(FVector2f(70.0f, 15.0f), FSlateLayoutTransform(FVector2f(Left + 4.0f, Top - 2.0f))),
            TEXT("输出能量"), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Grid);
        return LayerId + 5;
    }

private:
    TAttribute<float> InputVelocity;
    TAttribute<EResonanceVelocityCurve> Curve;
};
