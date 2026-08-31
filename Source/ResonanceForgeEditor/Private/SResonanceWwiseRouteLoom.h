#pragma once

#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

class SResonanceWwiseRouteLoom final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceWwiseRouteLoom) {}
        SLATE_ATTRIBUTE(bool, IsComplete)
        SLATE_ATTRIBUTE(FText, SourceName)
        SLATE_ATTRIBUTE(FText, SteelEvent)
        SLATE_ATTRIBUTE(FText, WoodEvent)
        SLATE_ATTRIBUTE(FText, GlassEvent)
        SLATE_ATTRIBUTE(FText, EnergyRtpc)
        SLATE_ATTRIBUTE(FText, BrightnessRtpc)
        SLATE_ATTRIBUTE(FText, SizeRtpc)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        IsComplete = InArgs._IsComplete;
        SourceName = InArgs._SourceName;
        SteelEvent = InArgs._SteelEvent;
        WoodEvent = InArgs._WoodEvent;
        GlassEvent = InArgs._GlassEvent;
        EnergyRtpc = InArgs._EnergyRtpc;
        BrightnessRtpc = InArgs._BrightnessRtpc;
        SizeRtpc = InArgs._SizeRtpc;
        SetCanTick(false);
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(900.0f, 166.0f);
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
        const FLinearColor Ink(0.008f, 0.018f, 0.020f, 1.0f);
        const FLinearColor Steel(0.35f, 0.66f, 0.88f, 1.0f);
        const FLinearColor Wood(0.96f, 0.52f, 0.22f, 1.0f);
        const FLinearColor Glass(0.43f, 0.90f, 0.77f, 1.0f);
        const FLinearColor Cold(0.18f, 0.25f, 0.26f, 1.0f);
        const FLinearColor Muted(0.56f, 0.62f, 0.62f, 1.0f);
        const FLinearColor Status = IsComplete.Get(false) ? Glass : Wood;
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), White, ESlateDrawEffect::None, Ink);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
            Geometry.ToPaintGeometry(FVector2f(190.0f, 20.0f), FSlateLayoutTransform(FVector2f(16.0f, 10.0f))),
            FText::FromString(TEXT("Wwise 路由织机")), FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Glass);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
            Geometry.ToPaintGeometry(FVector2f(Size.X - 235.0f, 20.0f), FSlateLayoutTransform(FVector2f(220.0f, 11.0f))),
            SourceName.Get(FText::GetEmpty()), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Status);

        const float Split = Size.X * 0.54f;
        TArray<FVector2D> Shuttle = {FVector2D(Split, 42.0f), FVector2D(Split, 146.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), Shuttle, ESlateDrawEffect::None, Cold, true, 3.0f);

        const FText Materials[] = {FText::FromString(TEXT("拉丝钢")), FText::FromString(TEXT("硬木")), FText::FromString(TEXT("薄玻璃"))};
        const TAttribute<FText>* Events[] = {&SteelEvent, &WoodEvent, &GlassEvent};
        const FLinearColor Colors[] = {Steel, Wood, Glass};
        const FText Controls[] = {FText::FromString(TEXT("能量")), FText::FromString(TEXT("明亮度")), FText::FromString(TEXT("尺度"))};
        const TAttribute<FText>* Rtpcs[] = {&EnergyRtpc, &BrightnessRtpc, &SizeRtpc};

        for (int32 Index = 0; Index < 3; ++Index)
        {
            const float Y = 58.0f + Index * 42.0f;
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(FVector2f(10.0f, 10.0f), FSlateLayoutTransform(FVector2f(18.0f, Y - 5.0f))),
                White, ESlateDrawEffect::None, Colors[Index]);
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(FVector2f(68.0f, 18.0f), FSlateLayoutTransform(FVector2f(38.0f, Y - 10.0f))),
                Materials[Index], FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Colors[Index]);
            TArray<FVector2D> EventThread = {FVector2D(101.0f, Y), FVector2D(Split - 14.0f, Y)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), EventThread, ESlateDrawEffect::None, Colors[Index].CopyWithNewOpacity(0.45f), true, 2.0f);
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(FVector2f(Split - 130.0f, 18.0f), FSlateLayoutTransform(FVector2f(112.0f, Y - 10.0f))),
                Events[Index]->Get(FText::GetEmpty()), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, FLinearColor::White);

            const float RightStart = Split + 22.0f;
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(FVector2f(72.0f, 18.0f), FSlateLayoutTransform(FVector2f(RightStart, Y - 10.0f))),
                Controls[Index], FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Colors[Index]);
            TArray<FVector2D> RtpcThread = {FVector2D(RightStart + 66.0f, Y), FVector2D(Size.X - 18.0f, Y)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), RtpcThread, ESlateDrawEffect::None, Colors[Index].CopyWithNewOpacity(0.34f), true, 2.0f);
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(FVector2f(Size.X - RightStart - 92.0f, 18.0f), FSlateLayoutTransform(FVector2f(RightStart + 78.0f, Y - 10.0f))),
                Rtpcs[Index]->Get(FText::GetEmpty()), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Muted);
        }
        return LayerId + 2;
    }

private:
    TAttribute<bool> IsComplete;
    TAttribute<FText> SourceName;
    TAttribute<FText> SteelEvent;
    TAttribute<FText> WoodEvent;
    TAttribute<FText> GlassEvent;
    TAttribute<FText> EnergyRtpc;
    TAttribute<FText> BrightnessRtpc;
    TAttribute<FText> SizeRtpc;
};
