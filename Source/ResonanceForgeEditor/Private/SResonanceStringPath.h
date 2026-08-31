#pragma once

#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_TwoParams(FOnResonancePickupChanged, float, bool)
DECLARE_DELEGATE_TwoParams(FOnResonanceStringStrikeChanged, float, bool)

class SResonanceStringPath final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceStringPath) {}
        SLATE_ATTRIBUTE(float, PickupPosition)
        SLATE_ATTRIBUTE(float, StrikePosition)
        SLATE_ATTRIBUTE(float, Sustain)
        SLATE_ATTRIBUTE(float, Damping)
        SLATE_ATTRIBUTE(float, Coupling)
        SLATE_EVENT(FOnResonancePickupChanged, OnPickupChanged)
        SLATE_EVENT(FOnResonanceStringStrikeChanged, OnStrikeChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        PickupPosition = InArgs._PickupPosition;
        StrikePosition = InArgs._StrikePosition;
        Sustain = InArgs._Sustain;
        Damping = InArgs._Damping;
        Coupling = InArgs._Coupling;
        OnPickupChanged = InArgs._OnPickupChanged;
        OnStrikeChanged = InArgs._OnStrikeChanged;
        SetCanTick(false);
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(720.0f, 166.0f);
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
        const float Left = 56.0f;
        const float Right = Size.X - 56.0f;
        const float Center = (Left + Right) * 0.5f;
        const float StringY = 72.0f;
        const float Position = FMath::Clamp(PickupPosition.Get(0.35f), 0.0f, 1.0f);
        const float CurrentPhysical = FMath::Lerp(0.04f, 0.50f, Position);
        const float MarkerX = FMath::Lerp(Left, Right, CurrentPhysical);
        const float Strike = FMath::Clamp(StrikePosition.Get(0.5f), 0.0f, 1.0f);
        const float StrikePhysical = FMath::Lerp(0.12f, 0.88f, Strike);
        const float StrikeX = FMath::Lerp(Left, Right, StrikePhysical);
        const float SustainValue = FMath::Clamp(Sustain.Get(0.90f), 0.0f, 1.0f);
        const float DampingValue = FMath::Clamp(Damping.Get(0.36f), 0.0f, 1.0f);
        const float CouplingValue = FMath::Clamp(Coupling.Get(0.22f), 0.0f, 1.0f);
        const FLinearColor Iron(0.27f, 0.35f, 0.35f, 1.0f);
        const FLinearColor Copper(0.95f, 0.47f, 0.17f, 1.0f);
        const FLinearColor Cyan(0.15f, 0.78f, 0.82f, 1.0f);
        const FLinearColor Mint(0.43f, 0.86f, 0.72f, 1.0f);
        const FLinearColor Muted(0.52f, 0.58f, 0.56f, 0.92f);

        TArray<FVector2D> Baseline = {FVector2D(Left, StringY), FVector2D(Right, StringY)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Baseline, ESlateDrawEffect::None, Iron, true, 2.0f);

        TArray<FVector2D> Upper;
        TArray<FVector2D> Lower;
        Upper.Reserve(65);
        Lower.Reserve(65);
        const float EnvelopeHeight = FMath::Lerp(8.0f, 22.0f, SustainValue);
        for (int32 Index = 0; Index <= 64; ++Index)
        {
            const float T = static_cast<float>(Index) / 64.0f;
            const float X = FMath::Lerp(Left, Right, T);
            const float Fundamental = FMath::Sin(PI * T);
            const float HarmonicGrain = 1.0f + (1.0f - DampingValue) * 0.16f * FMath::Sin(PI * T * 6.0f);
            const float Height = EnvelopeHeight * Fundamental * HarmonicGrain;
            Upper.Add(FVector2D(X, StringY - Height));
            Lower.Add(FVector2D(X, StringY + Height));
        }
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), Upper, ESlateDrawEffect::None, Copper.CopyWithNewOpacity(0.72f), true, 2.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), Lower, ESlateDrawEffect::None, Copper.CopyWithNewOpacity(0.34f), true, 1.5f);

        TArray<FVector2D> Bridge = {
            FVector2D(Left - 9.0f, StringY + 25.0f),
            FVector2D(Left, StringY + 10.0f),
            FVector2D(Left + 9.0f, StringY + 25.0f),
            FVector2D(Left - 9.0f, StringY + 25.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(), Bridge, ESlateDrawEffect::None, Copper, true, 3.0f);

        TArray<FVector2D> CenterMark = {FVector2D(Center, StringY - 27.0f), FVector2D(Center, StringY + 27.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(), CenterMark, ESlateDrawEffect::None, Mint.CopyWithNewOpacity(0.55f), true, 1.5f);

        const float BodyWidth = 30.0f + CouplingValue * 34.0f;
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 2,
            Geometry.ToPaintGeometry(FVector2f(BodyWidth, 8.0f), FSlateLayoutTransform(FVector2f(Left - BodyWidth * 0.5f, StringY + 31.0f))),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Copper.CopyWithNewOpacity(0.18f + CouplingValue * 0.35f));

        TArray<FVector2D> StrikeStem = {FVector2D(StrikeX, StringY - 39.0f), FVector2D(StrikeX, StringY - 7.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(), StrikeStem, ESlateDrawEffect::None, Copper, true, 3.0f);
        TArray<FVector2D> StrikeHead = {
            FVector2D(StrikeX - 8.0f, StringY - 12.0f),
            FVector2D(StrikeX, StringY - 2.0f),
            FVector2D(StrikeX + 8.0f, StringY - 12.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, Geometry.ToPaintGeometry(), StrikeHead, ESlateDrawEffect::None, Copper, true, 3.0f);
        const FString StrikeText = FString::Printf(TEXT("起振锤  %d%%"), FMath::RoundToInt(Strike * 100.0f));
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 5,
            Geometry.ToPaintGeometry(FVector2f(120.0f, 20.0f), FSlateLayoutTransform(FVector2f(FMath::Clamp(StrikeX - 42.0f, Left, Right - 88.0f), 4.0f))),
            StrikeText, FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Copper);

        TArray<FVector2D> PickupStem = {FVector2D(MarkerX, StringY - 29.0f), FVector2D(MarkerX, StringY + 34.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(), PickupStem, ESlateDrawEffect::None, Cyan, true, 3.0f);
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 4,
            Geometry.ToPaintGeometry(FVector2f(24.0f, 12.0f), FSlateLayoutTransform(FVector2f(MarkerX - 12.0f, StringY + 29.0f))),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Cyan);

        const FString PickupText = FString::Printf(TEXT("拾音梭  %d%%"), FMath::RoundToInt(Position * 100.0f));
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 5,
            Geometry.ToPaintGeometry(FVector2f(120.0f, 20.0f), FSlateLayoutTransform(FVector2f(FMath::Clamp(MarkerX - 44.0f, Left, Right - 90.0f), 116.0f))),
            PickupText, FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Cyan);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 2,
            Geometry.ToPaintGeometry(FVector2f(120.0f, 18.0f), FSlateLayoutTransform(FVector2f(Left - 18.0f, 144.0f))),
            TEXT("琴桥 · 泛音更亮"), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Copper);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 2,
            Geometry.ToPaintGeometry(FVector2f(120.0f, 18.0f), FSlateLayoutTransform(FVector2f(Center - 38.0f, 144.0f))),
            TEXT("弦心 · 基频更稳"), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Mint);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 2,
            Geometry.ToPaintGeometry(FVector2f(150.0f, 18.0f), FSlateLayoutTransform(FVector2f(Right - 132.0f, 144.0f))),
            TEXT("右半弦 · 对称参照"), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Muted);
        return LayerId + 6;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
        const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
        const float Left = 56.0f;
        const float Right = Geometry.GetLocalSize().X - 56.0f;
        const float PickupX = FMath::Lerp(Left, Right, FMath::Lerp(0.04f, 0.50f, FMath::Clamp(PickupPosition.Get(0.35f), 0.0f, 1.0f)));
        const float StrikeX = FMath::Lerp(Left, Right, FMath::Lerp(0.12f, 0.88f, FMath::Clamp(StrikePosition.Get(0.5f), 0.0f, 1.0f)));
        const float PickupDistance = FVector2D::Distance(Local, FVector2D(PickupX, 108.0f));
        const float StrikeDistance = FVector2D::Distance(Local, FVector2D(StrikeX, 34.0f));
        ActiveHandle = StrikeDistance <= PickupDistance ? EStringHandle::Strike : EStringHandle::Pickup;
        UpdateHandle(Geometry, Event, false);
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (!HasMouseCapture()) return FReply::Unhandled();
        UpdateHandle(Geometry, Event, false);
        return FReply::Handled();
    }

    virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (Event.GetEffectingButton() != EKeys::LeftMouseButton || !HasMouseCapture()) return FReply::Unhandled();
        UpdateHandle(Geometry, Event, true);
        return FReply::Handled().ReleaseMouseCapture();
    }

private:
    enum class EStringHandle : uint8
    {
        Pickup,
        Strike
    };

    void UpdateHandle(const FGeometry& Geometry, const FPointerEvent& Event, const bool bFinished)
    {
        const float Left = 56.0f;
        const float Right = Geometry.GetLocalSize().X - 56.0f;
        const float X = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()).X;
        const float PhysicalPosition = FMath::Clamp(FMath::GetRangePct(Left, FMath::Max(Left + 1.0f, Right), X), 0.0f, 1.0f);
        if (ActiveHandle == EStringHandle::Strike)
        {
            const float Position = FMath::GetRangePct(0.12f, 0.88f, PhysicalPosition);
            OnStrikeChanged.ExecuteIfBound(FMath::Clamp(Position, 0.0f, 1.0f), bFinished);
        }
        else
        {
            const float Position = FMath::GetRangePct(0.04f, 0.50f, PhysicalPosition);
            OnPickupChanged.ExecuteIfBound(FMath::Clamp(Position, 0.0f, 1.0f), bFinished);
        }
    }

    TAttribute<float> PickupPosition;
    TAttribute<float> StrikePosition;
    TAttribute<float> Sustain;
    TAttribute<float> Damping;
    TAttribute<float> Coupling;
    FOnResonancePickupChanged OnPickupChanged;
    FOnResonanceStringStrikeChanged OnStrikeChanged;
    EStringHandle ActiveHandle = EStringHandle::Pickup;
};
