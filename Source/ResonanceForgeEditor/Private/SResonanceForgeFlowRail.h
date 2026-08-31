#pragma once

#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_OneParam(FOnResonanceFlowStationSelected, int32)

class SResonanceForgeFlowRail final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceForgeFlowRail) {}
        SLATE_ATTRIBUTE(int32, ActiveStation)
        SLATE_ATTRIBUTE(FText, GuideText)
        SLATE_EVENT(FOnResonanceFlowStationSelected, OnStationSelected)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        ActiveStation = InArgs._ActiveStation;
        GuideText = InArgs._GuideText;
        OnStationSelected = InArgs._OnStationSelected;
        SetCanTick(false);
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(900.0f, 126.0f);
    }

    virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
        {
            return FReply::Unhandled();
        }

        const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
        const float Left = 58.0f;
        const float Right = FMath::Max(Left + 1.0f, Geometry.GetLocalSize().X - 58.0f);
        const int32 Station = FMath::Clamp(
            FMath::RoundToInt((Local.X - Left) / ((Right - Left) / 4.0f)), 0, 4);
        OnStationSelected.ExecuteIfBound(Station);
        return FReply::Handled();
    }

    virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override
    {
        return FCursorReply::Cursor(EMouseCursor::Hand);
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
        const float Left = 58.0f;
        const float Right = FMath::Max(Left + 1.0f, Size.X - 58.0f);
        const float RailY = 63.0f;
        const int32 Active = FMath::Clamp(ActiveStation.Get(0), 0, 4);
        const FLinearColor Copper(0.95f, 0.43f, 0.13f, 1.0f);
        const FLinearColor Mint(0.39f, 0.88f, 0.74f, 1.0f);
        const FLinearColor ColdMetal(0.18f, 0.24f, 0.24f, 1.0f);
        const FLinearColor Ink(0.010f, 0.014f, 0.014f, 1.0f);
        const FLinearColor Muted(0.52f, 0.57f, 0.54f, 1.0f);
        const FSlateBrush* WhiteBrush = FAppStyle::GetBrush(TEXT("WhiteBrush"));

        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId, Geometry.ToPaintGeometry(), WhiteBrush,
            ESlateDrawEffect::None, Ink);

        const FText Guide = GuideText.Get(FText::GetEmpty());
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 1,
            Geometry.ToPaintGeometry(FVector2f(Size.X - 36.0f, 24.0f), FSlateLayoutTransform(FVector2f(18.0f, 12.0f))),
            Guide, FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Mint);

        TArray<FVector2D> ColdRail = {FVector2D(Left, RailY), FVector2D(Right, RailY)};
        FSlateDrawElement::MakeLines(
            OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), ColdRail,
            ESlateDrawEffect::None, ColdMetal, true, 8.0f);
        FSlateDrawElement::MakeLines(
            OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(), ColdRail,
            ESlateDrawEffect::None, FLinearColor(0.05f, 0.07f, 0.07f, 1.0f), true, 2.0f);

        const float ActiveX = FMath::Lerp(Left, Right, static_cast<float>(Active) / 4.0f);
        TArray<FVector2D> HotRail = {FVector2D(Left, RailY), FVector2D(ActiveX, RailY)};
        FSlateDrawElement::MakeLines(
            OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(), HotRail,
            ESlateDrawEffect::None, Copper, true, 3.0f);

        static const TCHAR* StationLabels[] = {
            TEXT("01  取件"), TEXT("02  起振"), TEXT("03  塑形"), TEXT("04  监听"), TEXT("05  铸样")};
        static const TCHAR* StationHints[] = {
            TEXT("读取对象"), TEXT("敲击或演奏"), TEXT("调弦与共振"), TEXT("UE / Wwise"), TEXT("WAV + 铭牌")};

        for (int32 Index = 0; Index < 5; ++Index)
        {
            const float X = FMath::Lerp(Left, Right, static_cast<float>(Index) / 4.0f);
            const bool bCurrent = Index == Active;
            const bool bHeated = Index <= Active;
            const FLinearColor StationColor = bCurrent ? Mint : (bHeated ? Copper : ColdMetal);
            const float OuterSize = bCurrent ? 24.0f : 18.0f;
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 4,
                Geometry.ToPaintGeometry(
                    FVector2f(OuterSize, OuterSize),
                    FSlateLayoutTransform(FVector2f(X - OuterSize * 0.5f, RailY - OuterSize * 0.5f))),
                WhiteBrush, ESlateDrawEffect::None,
                bCurrent ? Mint.CopyWithNewOpacity(0.25f) : ColdMetal);
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 5,
                Geometry.ToPaintGeometry(
                    FVector2f(8.0f, 8.0f),
                    FSlateLayoutTransform(FVector2f(X - 4.0f, RailY - 4.0f))),
                WhiteBrush, ESlateDrawEffect::None, StationColor);

            const float LabelWidth = 116.0f;
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 6,
                Geometry.ToPaintGeometry(
                    FVector2f(LabelWidth, 18.0f),
                    FSlateLayoutTransform(FVector2f(X - LabelWidth * 0.5f, 82.0f))),
                StationLabels[Index], FAppStyle::GetFontStyle(TEXT("BoldFont")),
                ESlateDrawEffect::None, StationColor);
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 6,
                Geometry.ToPaintGeometry(
                    FVector2f(LabelWidth, 16.0f),
                    FSlateLayoutTransform(FVector2f(X - LabelWidth * 0.5f, 103.0f))),
                StationHints[Index], FAppStyle::GetFontStyle(TEXT("SmallFont")),
                ESlateDrawEffect::None, bCurrent ? FLinearColor::White : Muted);
        }
        return LayerId + 6;
    }

private:
    TAttribute<int32> ActiveStation;
    TAttribute<FText> GuideText;
    FOnResonanceFlowStationSelected OnStationSelected;
};
