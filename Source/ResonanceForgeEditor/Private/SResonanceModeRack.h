#pragma once

#include "ResonanceMaterialProfile.h"
#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_OneParam(FOnResonanceModeSelected, int32)

class SResonanceModeRack final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceModeRack) {}
        SLATE_ATTRIBUTE(TArray<FResonanceMode>, Modes)
        SLATE_ATTRIBUTE(int32, SelectedMode)
        SLATE_EVENT(FOnResonanceModeSelected, OnModeSelected)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        Modes = InArgs._Modes;
        SelectedMode = InArgs._SelectedMode;
        OnModeSelected = InArgs._OnModeSelected;
        SetCanTick(false);
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(680.0f, 170.0f);
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
        const float Baseline = Size.Y - 30.0f;
        const FLinearColor Grid(0.15f, 0.19f, 0.20f, 0.75f);
        const FLinearColor Quiet(0.37f, 0.48f, 0.49f, 0.85f);
        const FLinearColor Active(1.0f, 0.59f, 0.25f, 1.0f);
        const FLinearColor Tail(0.48f, 0.86f, 0.79f, 0.72f);

        TArray<FVector2D> Line = {FVector2D(16.0f, Baseline), FVector2D(Size.X - 16.0f, Baseline)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Grid, true, 1.0f);

        static const float Guides[] = {125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f};
        for (const float Hz : Guides)
        {
            const float X = FrequencyToX(Hz, Size.X);
            Line = {FVector2D(X, 12.0f), FVector2D(X, Baseline + 5.0f)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Grid * 0.72f, true, 1.0f);
            const FString Label = Hz >= 1000.0f
                ? FString::Printf(TEXT("%.0fk"), Hz / 1000.0f)
                : FString::Printf(TEXT("%.0f"), Hz);
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 1,
                AllottedGeometry.ToPaintGeometry(FVector2f(34.0f, 18.0f), FSlateLayoutTransform(FVector2f(X - 10.0f, Baseline + 8.0f))),
                Label, FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Quiet);
        }

        const TArray<FResonanceMode> CurrentModes = Modes.Get(TArray<FResonanceMode>());
        const int32 Selected = SelectedMode.Get(INDEX_NONE);
        for (int32 Index = 0; Index < CurrentModes.Num(); ++Index)
        {
            const FResonanceMode& Mode = CurrentModes[Index];
            const float X = FrequencyToX(Mode.FrequencyHz, Size.X);
            const float Height = FMath::Lerp(22.0f, Baseline - 20.0f, FMath::Clamp(Mode.Gain / 1.5f, 0.0f, 1.0f));
            const float TailWidth = FMath::Lerp(10.0f, 48.0f, FMath::Clamp(Mode.DecaySeconds / 3.0f, 0.0f, 1.0f));
            const FLinearColor Color = Index == Selected ? Active : Quiet;

            Line = {FVector2D(X, Baseline), FVector2D(X, Baseline - Height)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Color, true, Index == Selected ? 5.0f : 3.0f);
            Line = {FVector2D(X, Baseline - Height), FVector2D(FMath::Min(Size.X - 18.0f, X + TailWidth), Baseline - Height + 8.0f)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Index == Selected ? Active : Tail, true, 2.0f);
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 3,
                AllottedGeometry.ToPaintGeometry(FVector2f(8.0f, 8.0f), FSlateLayoutTransform(FVector2f(X - 4.0f, Baseline - Height - 4.0f))),
                FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color);
        }
        return LayerId + 4;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        {
            return FReply::Unhandled();
        }
        const TArray<FResonanceMode> CurrentModes = Modes.Get(TArray<FResonanceMode>());
        if (CurrentModes.IsEmpty())
        {
            return FReply::Handled();
        }
        const float MouseX = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X;
        int32 Nearest = 0;
        float NearestDistance = TNumericLimits<float>::Max();
        for (int32 Index = 0; Index < CurrentModes.Num(); ++Index)
        {
            const float Distance = FMath::Abs(FrequencyToX(CurrentModes[Index].FrequencyHz, MyGeometry.GetLocalSize().X) - MouseX);
            if (Distance < NearestDistance)
            {
                Nearest = Index;
                NearestDistance = Distance;
            }
        }
        OnModeSelected.ExecuteIfBound(Nearest);
        return FReply::Handled();
    }

private:
    static float FrequencyToX(const float FrequencyHz, const float Width)
    {
        const float Normalized = FMath::GetRangePct(
            FMath::Loge(100.0f), FMath::Loge(8000.0f),
            FMath::Loge(FMath::Clamp(FrequencyHz, 100.0f, 8000.0f)));
        return FMath::Lerp(20.0f, FMath::Max(21.0f, Width - 20.0f), Normalized);
    }

    TAttribute<TArray<FResonanceMode>> Modes;
    TAttribute<int32> SelectedMode;
    FOnResonanceModeSelected OnModeSelected;
};
