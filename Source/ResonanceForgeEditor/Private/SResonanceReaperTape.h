#pragma once

#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

class SResonanceReaperTape final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceReaperTape) {}
        SLATE_ATTRIBUTE(int32, SampleCount)
        SLATE_ATTRIBUTE(int32, SelectionMask)
        SLATE_ATTRIBUTE(bool, ProjectReady)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        SampleCount = InArgs._SampleCount;
        SelectionMask = InArgs._SelectionMask;
        ProjectReady = InArgs._ProjectReady;
        SetCanTick(false);
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(720.0f, 82.0f);
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
        const FSlateBrush* WhiteBrush = FAppStyle::GetBrush(TEXT("WhiteBrush"));
        const FLinearColor Ink(0.010f, 0.014f, 0.014f, 1.0f);
        const FLinearColor Cold(0.10f, 0.13f, 0.13f, 1.0f);
        const FLinearColor Rail(0.30f, 0.35f, 0.34f, 1.0f);
        const FLinearColor Copper(0.95f, 0.43f, 0.13f, 1.0f);
        const FLinearColor Mint(0.39f, 0.88f, 0.74f, 1.0f);
        const FLinearColor Steel(0.35f, 0.66f, 0.88f, 1.0f);
        const FLinearColor Muted(0.56f, 0.61f, 0.59f, 1.0f);
        const int32 Count = FMath::Clamp(SampleCount.Get(0), 0, 3);
        const int32 Mask = SelectionMask.Get(0) & 0x7;

        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId, Geometry.ToPaintGeometry(), WhiteBrush,
            ESlateDrawEffect::None, Ink);

        const float Left = 38.0f;
        const float Right = FMath::Max(Left + 1.0f, Size.X - 38.0f);
        const float RailY = 38.0f;
        TArray<FVector2D> Tape = {FVector2D(Left, RailY), FVector2D(Right, RailY)};
        FSlateDrawElement::MakeLines(
            OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), Tape,
            ESlateDrawEffect::None, Rail, true, 3.0f);

        static const TCHAR* Labels[] = {TEXT("更早"), TEXT("上一版"), TEXT("当前")};
        const FLinearColor Colors[] = {Steel, Copper, Mint};
        const float Gap = 8.0f;
        const float SegmentWidth = (Right - Left - Gap * 2.0f) / 3.0f;
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const float X = Left + Index * (SegmentWidth + Gap);
            const bool bFilled = (Mask & (1 << Index)) != 0;
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(
                    FVector2f(SegmentWidth, 22.0f),
                    FSlateLayoutTransform(FVector2f(X, RailY - 11.0f))),
                WhiteBrush, ESlateDrawEffect::None,
                bFilled ? Colors[Index].CopyWithNewOpacity(0.62f) : Cold);
            const FString SegmentLabel = bFilled
                ? FString(Labels[Index])
                : FString::Printf(TEXT("%s · 未收入"), Labels[Index]);
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 3,
                Geometry.ToPaintGeometry(
                    FVector2f(SegmentWidth, 16.0f),
                    FSlateLayoutTransform(FVector2f(X + 8.0f, RailY - 8.0f))),
                SegmentLabel, FAppStyle::GetFontStyle(TEXT("SmallFont")),
                ESlateDrawEffect::None, bFilled ? FLinearColor::White : Muted);
        }

        const FString Status = ProjectReady.Get(false)
            ? TEXT("RPP 已排带 · 48 kHz")
            : (Count > 0 ? FString::Printf(TEXT("已收入 %d 份铸样 · 等待排带"), Count) : TEXT("从铭牌架收入要比较的铸样"));
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 4,
            Geometry.ToPaintGeometry(
                FVector2f(Size.X - 76.0f, 16.0f),
                FSlateLayoutTransform(FVector2f(38.0f, Size.Y - 20.0f))),
            Status, FAppStyle::GetFontStyle(TEXT("SmallFont")),
            ESlateDrawEffect::None, ProjectReady.Get(false) ? Mint : Muted);
        return LayerId + 4;
    }

private:
    TAttribute<int32> SampleCount;
    TAttribute<int32> SelectionMask;
    TAttribute<bool> ProjectReady;
};
