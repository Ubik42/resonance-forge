#pragma once

#include "ResonanceForgeSynthComponent.h"
#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_TwoParams(FOnStrikeRailChanged, float, bool)

class SResonanceStrikeRail final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceStrikeRail) {}
        SLATE_ATTRIBUTE(float, StrikePosition)
        SLATE_ATTRIBUTE(float, LiveImpactPosition)
        SLATE_ATTRIBUTE(float, LiveImpactGlow)
        SLATE_ATTRIBUTE(int32, ModeCount)
        SLATE_EVENT(FOnStrikeRailChanged, OnPositionChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        StrikePosition = InArgs._StrikePosition;
        LiveImpactPosition = InArgs._LiveImpactPosition;
        LiveImpactGlow = InArgs._LiveImpactGlow;
        ModeCount = InArgs._ModeCount;
        OnPositionChanged = InArgs._OnPositionChanged;
        SetCanTick(false);
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(680.0f, 126.0f);
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
        const float Left = 28.0f;
        const float Right = Size.X - 28.0f;
        const float RailY = 43.0f;
        const float Position = FMath::Clamp(StrikePosition.Get(0.5f), 0.0f, 1.0f);
        const float Glow = FMath::Clamp(LiveImpactGlow.Get(0.0f), 0.0f, 1.0f);
        const float ReturnedPosition = FMath::Clamp(LiveImpactPosition.Get(Position), 0.0f, 1.0f);
        const float ResponsePosition = Glow > 0.01f ? ReturnedPosition : Position;
        const float MarkerX = FMath::Lerp(Left, Right, Position);
        const float ReturnedX = FMath::Lerp(Left, Right, ReturnedPosition);
        const FLinearColor Iron(0.28f, 0.39f, 0.40f, 1.0f);
        const FLinearColor Copper(1.0f, 0.55f, 0.20f, 1.0f);
        const FLinearColor Mint(0.43f, 0.82f, 0.75f, 1.0f);
        const FLinearColor Muted(0.43f, 0.50f, 0.51f, 0.86f);

        TArray<FVector2D> Line = {FVector2D(Left, RailY), FVector2D(Right, RailY)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Iron, true, 7.0f);
        for (const float Tick : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
        {
            const float X = FMath::Lerp(Left, Right, Tick);
            Line = {FVector2D(X, RailY - (Tick == 0.5f ? 13.0f : 8.0f)), FVector2D(X, RailY + 10.0f)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Tick == 0.5f ? Mint : Iron, true, 2.0f);
        }

        Line = {FVector2D(MarkerX, 10.0f), FVector2D(MarkerX, RailY + 15.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Copper, true, 3.0f);
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 3,
            AllottedGeometry.ToPaintGeometry(FVector2f(12.0f, 12.0f), FSlateLayoutTransform(FVector2f(MarkerX - 6.0f, 5.0f))),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Copper);

        if (Glow > 0.01f)
        {
            Line = {FVector2D(ReturnedX, RailY - 17.0f - Glow * 10.0f), FVector2D(ReturnedX, RailY + 17.0f + Glow * 10.0f)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Mint.CopyWithNewOpacity(0.35f + Glow * 0.65f), true, 2.0f + Glow * 4.0f);
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 4,
                AllottedGeometry.ToPaintGeometry(FVector2f(18.0f + Glow * 12.0f, 8.0f), FSlateLayoutTransform(FVector2f(ReturnedX - 9.0f - Glow * 6.0f, RailY - 4.0f))),
                FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Mint.CopyWithNewOpacity(0.20f + Glow * 0.45f));
        }

        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
            AllottedGeometry.ToPaintGeometry(FVector2f(60.0f, 18.0f), FSlateLayoutTransform(FVector2f(Left, 58.0f))),
            TEXT("近端"), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Muted);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
            AllottedGeometry.ToPaintGeometry(FVector2f(60.0f, 18.0f), FSlateLayoutTransform(FVector2f(Size.X * 0.5f - 17.0f, 58.0f))),
            TEXT("中央"), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Muted);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
            AllottedGeometry.ToPaintGeometry(FVector2f(60.0f, 18.0f), FSlateLayoutTransform(FVector2f(Right - 30.0f, 58.0f))),
            TEXT("远端"), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Muted);

        const int32 Count = FMath::Clamp(ModeCount.Get(0), 1, 16);
        const float ResponseY = 112.0f;
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const float X = FMath::Lerp(Left, Right, Count == 1 ? 0.5f : static_cast<float>(Index) / static_cast<float>(Count - 1));
            const float Response = UResonanceForgeSynthComponent::ComputeModeExcitation(Index, ResponsePosition);
            const float Height = 26.0f * Response * (1.0f + Glow * 0.55f);
            Line = {FVector2D(X, ResponseY), FVector2D(X, ResponseY - Height)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 5, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, FLinearColor::LerpUsingHSV(Iron, Mint, FMath::Clamp(Response + Glow * 0.35f, 0.0f, 1.0f)), true, 5.0f + Glow * 2.0f);
        }
        return LayerId + 4;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
        UpdatePosition(MyGeometry, MouseEvent, false);
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
    {
        if (HasMouseCapture())
        {
            UpdatePosition(MyGeometry, MouseEvent, false);
            return FReply::Handled();
        }
        return FReply::Unhandled();
    }

    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture())
        {
            UpdatePosition(MyGeometry, MouseEvent, true);
            return FReply::Handled().ReleaseMouseCapture();
        }
        return FReply::Unhandled();
    }

private:
    void UpdatePosition(const FGeometry& Geometry, const FPointerEvent& Event, const bool bFinished)
    {
        const float X = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()).X;
        const float Position = FMath::GetRangePct(28.0f, FMath::Max(29.0f, Geometry.GetLocalSize().X - 28.0f), X);
        OnPositionChanged.ExecuteIfBound(FMath::Clamp(Position, 0.0f, 1.0f), bFinished);
    }

    TAttribute<float> StrikePosition;
    TAttribute<float> LiveImpactPosition;
    TAttribute<float> LiveImpactGlow;
    TAttribute<int32> ModeCount;
    FOnStrikeRailChanged OnPositionChanged;
};
