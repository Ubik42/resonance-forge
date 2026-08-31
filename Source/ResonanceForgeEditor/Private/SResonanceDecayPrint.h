#pragma once

#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

class SResonanceDecayPrint final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceDecayPrint) {}
        SLATE_ARGUMENT(const TArray<float>*, Envelope)
        SLATE_ATTRIBUTE(float, TailDb)
        SLATE_ATTRIBUTE(float, DurationSeconds)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        Envelope = InArgs._Envelope;
        TailDb = InArgs._TailDb;
        DurationSeconds = InArgs._DurationSeconds;
        SetCanTick(false);
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(720.0f, 92.0f);
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
        const float MidY = Size.Y * 0.46f;
        const FLinearColor Charcoal(0.018f, 0.024f, 0.024f, 1.0f);
        const FLinearColor Grid(0.23f, 0.29f, 0.28f, 0.42f);
        const FLinearColor Copper(0.96f, 0.50f, 0.18f, 1.0f);
        const FLinearColor Mint(0.43f, 0.90f, 0.77f, 1.0f);
        const FLinearColor Quiet(0.58f, 0.63f, 0.61f, 0.88f);
        const bool bSettled = TailDb.Get(-96.0f) <= -48.0f;
        const FLinearColor TailColor = bSettled ? Mint : Copper;

        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId,
            Geometry.ToPaintGeometry(),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Charcoal);

        const float TailStart = Size.X * 0.875f;
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 1,
            Geometry.ToPaintGeometry(FVector2f(Size.X - TailStart, Size.Y), FSlateLayoutTransform(FVector2f(TailStart, 0.0f))),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, TailColor.CopyWithNewOpacity(0.08f));

        for (int32 Tick = 0; Tick <= 4; ++Tick)
        {
            const float X = Size.X * static_cast<float>(Tick) / 4.0f;
            TArray<FVector2D> TickLine = {FVector2D(X, 8.0f), FVector2D(X, Size.Y - 20.0f)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(), TickLine, ESlateDrawEffect::None, Grid, true, 1.0f);
        }

        TArray<FVector2D> Baseline = {FVector2D(0.0f, MidY), FVector2D(Size.X, MidY)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(), Baseline, ESlateDrawEffect::None, Grid, true, 1.0f);

        if (Envelope && Envelope->Num() > 1)
        {
            TArray<FVector2D> Upper;
            TArray<FVector2D> Lower;
            Upper.Reserve(Envelope->Num());
            Lower.Reserve(Envelope->Num());
            for (int32 Index = 0; Index < Envelope->Num(); ++Index)
            {
                const float Alpha = static_cast<float>(Index) / static_cast<float>(Envelope->Num() - 1);
                const float Height = FMath::Sqrt(FMath::Clamp((*Envelope)[Index], 0.0f, 1.0f)) * (MidY - 9.0f);
                Upper.Add(FVector2D(Alpha * Size.X, MidY - Height));
                Lower.Add(FVector2D(Alpha * Size.X, MidY + Height));
            }
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(), Upper, ESlateDrawEffect::None, Copper.CopyWithNewOpacity(0.94f), true, 2.0f);
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(), Lower, ESlateDrawEffect::None, Copper.CopyWithNewOpacity(0.55f), true, 1.5f);
        }

        const float Duration = DurationSeconds.Get(0.0f);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 4,
            Geometry.ToPaintGeometry(FVector2f(80.0f, 16.0f), FSlateLayoutTransform(FVector2f(5.0f, Size.Y - 18.0f))),
            TEXT("0 秒"), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Quiet);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 4,
            Geometry.ToPaintGeometry(FVector2f(90.0f, 16.0f), FSlateLayoutTransform(FVector2f(Size.X * 0.5f - 16.0f, Size.Y - 18.0f))),
            FString::Printf(TEXT("%.1f 秒"), Duration * 0.5f), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Quiet);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 4,
            Geometry.ToPaintGeometry(FVector2f(90.0f, 16.0f), FSlateLayoutTransform(FVector2f(Size.X - 54.0f, Size.Y - 18.0f))),
            FString::Printf(TEXT("%.1f 秒"), Duration), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, TailColor);
        return LayerId + 5;
    }

private:
    const TArray<float>* Envelope = nullptr;
    TAttribute<float> TailDb;
    TAttribute<float> DurationSeconds;
};
