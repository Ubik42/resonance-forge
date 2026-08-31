#pragma once

#include "HAL/PlatformTime.h"
#include "Input/Reply.h"
#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_TwoParams(FOnResonanceBowStrokeBegin, float, float)
DECLARE_DELEGATE_FourParams(FOnResonanceBowStrokeChanged, float, float, float, float)
DECLARE_DELEGATE(FOnResonanceBowStrokeEnd)

class SResonanceBowStroke final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceBowStroke) {}
        SLATE_ATTRIBUTE(float, StrokePosition)
        SLATE_ATTRIBUTE(float, BowSpeed)
        SLATE_ATTRIBUTE(float, BowPressure)
        SLATE_ATTRIBUTE(float, BowDirection)
        SLATE_ATTRIBUTE(bool, StrokeActive)
        SLATE_EVENT(FOnResonanceBowStrokeBegin, OnStrokeBegin)
        SLATE_EVENT(FOnResonanceBowStrokeChanged, OnStrokeChanged)
        SLATE_EVENT(FOnResonanceBowStrokeEnd, OnStrokeEnd)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        StrokePosition = InArgs._StrokePosition;
        BowSpeed = InArgs._BowSpeed;
        BowPressure = InArgs._BowPressure;
        BowDirection = InArgs._BowDirection;
        StrokeActive = InArgs._StrokeActive;
        OnStrokeBegin = InArgs._OnStrokeBegin;
        OnStrokeChanged = InArgs._OnStrokeChanged;
        OnStrokeEnd = InArgs._OnStrokeEnd;
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(900.0f, 118.0f);
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
        const float Position = FMath::Clamp(StrokePosition.Get(0.28f), 0.0f, 1.0f);
        const float Speed = FMath::Clamp(BowSpeed.Get(0.55f), 0.0f, 1.0f);
        const float Pressure = FMath::Clamp(BowPressure.Get(0.55f), 0.0f, 1.0f);
        const float Direction = BowDirection.Get(1.0f) < 0.0f ? -1.0f : 1.0f;
        const bool bActive = StrokeActive.Get(false) || bDragging;
        const FLinearColor Coal(0.014f, 0.012f, 0.010f, 1.0f);
        const FLinearColor Iron(0.25f, 0.24f, 0.21f, 1.0f);
        const FLinearColor Brass(0.70f, 0.51f, 0.28f, 1.0f);
        const FLinearColor Copper(0.98f, 0.47f, 0.14f, 1.0f);
        const FLinearColor Mint(0.40f, 0.90f, 0.76f, 1.0f);
        const FLinearColor Muted(0.62f, 0.63f, 0.58f, 1.0f);

        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Coal);

        const float Left = 42.0f;
        const float Right = Size.X - 42.0f;
        const float TrackY = 64.0f;
        const TArray<FVector2D> UpperRail = {FVector2D(Left, TrackY - 17.0f), FVector2D(Right, TrackY - 17.0f)};
        const TArray<FVector2D> LowerRail = {FVector2D(Left, TrackY + 17.0f), FVector2D(Right, TrackY + 17.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), UpperRail, ESlateDrawEffect::None, Iron, true, 2.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), LowerRail, ESlateDrawEffect::None, Iron, true, 2.0f);

        for (int32 Tick = 0; Tick <= 12; ++Tick)
        {
            const float X = FMath::Lerp(Left, Right, static_cast<float>(Tick) / 12.0f);
            const float Height = Tick % 3 == 0 ? 8.0f : 4.0f;
            const TArray<FVector2D> TickLine = {FVector2D(X, TrackY - Height), FVector2D(X, TrackY + Height)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), TickLine, ESlateDrawEffect::None, Brass.CopyWithNewOpacity(0.42f), true, 1.0f);
        }

        for (int32 Index = 0; Index < StrokeTrail.Num(); ++Index)
        {
            const float Alpha = static_cast<float>(Index + 1) / FMath::Max(1, StrokeTrail.Num());
            const FVector2D Point = StrokeTrail[Index];
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(FVector2f(4.0f + Alpha * 3.0f, 4.0f + Alpha * 3.0f),
                    FSlateLayoutTransform(FVector2f(Point.X - 3.0f, Point.Y - 3.0f))),
                FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None,
                Copper.CopyWithNewOpacity(Alpha * 0.48f));
        }

        const float CarriageX = FMath::Lerp(Left, Right, Position);
        const float HairHalfLength = FMath::Lerp(74.0f, 116.0f, Speed);
        const float HairSpread = FMath::Lerp(8.0f, 2.5f, Pressure);
        for (int32 Hair = -2; Hair <= 2; ++Hair)
        {
            const float Offset = Hair * HairSpread * 0.5f;
            const TArray<FVector2D> BowHair = {
                FVector2D(FMath::Clamp(CarriageX - HairHalfLength, Left, Right), TrackY + Offset),
                FVector2D(FMath::Clamp(CarriageX + HairHalfLength, Left, Right), TrackY - Offset)};
            FSlateDrawElement::MakeLines(
                OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(), BowHair,
                ESlateDrawEffect::None, Brass.CopyWithNewOpacity(0.36f + Pressure * 0.34f), true, 1.2f);
        }

        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 4,
            Geometry.ToPaintGeometry(FVector2f(18.0f, 38.0f), FSlateLayoutTransform(FVector2f(CarriageX - 9.0f, TrackY - 19.0f))),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None,
            bActive ? Copper : Brass);
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 5,
            Geometry.ToPaintGeometry(FVector2f(6.0f, 6.0f), FSlateLayoutTransform(FVector2f(CarriageX - 3.0f, TrackY - 3.0f))),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Mint);

        const float ArrowX = CarriageX + Direction * 32.0f;
        const TArray<FVector2D> ArrowShaft = {FVector2D(CarriageX + Direction * 12.0f, TrackY - 28.0f), FVector2D(ArrowX, TrackY - 28.0f)};
        const TArray<FVector2D> ArrowHeadA = {FVector2D(ArrowX, TrackY - 28.0f), FVector2D(ArrowX - Direction * 8.0f, TrackY - 34.0f)};
        const TArray<FVector2D> ArrowHeadB = {FVector2D(ArrowX, TrackY - 28.0f), FVector2D(ArrowX - Direction * 8.0f, TrackY - 22.0f)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 5, Geometry.ToPaintGeometry(), ArrowShaft, ESlateDrawEffect::None, Copper, true, 2.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 5, Geometry.ToPaintGeometry(), ArrowHeadA, ESlateDrawEffect::None, Copper, true, 2.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 5, Geometry.ToPaintGeometry(), ArrowHeadB, ESlateDrawEffect::None, Copper, true, 2.0f);

        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 6,
            Geometry.ToPaintGeometry(FVector2f(160.0f, 22.0f), FSlateLayoutTransform(FVector2f(16.0f, 10.0f))),
            TEXT("弓行轨"), FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Brass);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 6,
            Geometry.ToPaintGeometry(FVector2f(330.0f, 18.0f), FSlateLayoutTransform(FVector2f(102.0f, 12.0f))),
            TEXT("按住弓座左右拉动 · 向下压得更重"), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Muted);
        const FString DirectionText = Direction > 0.0f ? TEXT("推弓  →") : TEXT("←  回弓");
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 6,
            Geometry.ToPaintGeometry(FVector2f(100.0f, 18.0f), FSlateLayoutTransform(FVector2f(Size.X - 286.0f, 12.0f))),
            DirectionText, FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Copper);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 6,
            Geometry.ToPaintGeometry(FVector2f(180.0f, 18.0f), FSlateLayoutTransform(FVector2f(Size.X - 174.0f, 12.0f))),
            FString::Printf(TEXT("弓速 %d%% · 弓压 %d%%"), FMath::RoundToInt(Speed * 100.0f), FMath::RoundToInt(Pressure * 100.0f)),
            FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Mint);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 6,
            Geometry.ToPaintGeometry(FVector2f(260.0f, 16.0f), FSlateLayoutTransform(FVector2f(Left, Size.Y - 24.0f))),
            bActive ? TEXT("弓毛正在咬弦") : TEXT("从任意位置起弓，松手进入收弓"),
            FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, bActive ? Copper : Muted);

        return LayerId + 6;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        {
            return FReply::Unhandled();
        }
        const FVector2D Local = Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        bDragging = true;
        LastPointer = Local;
        LastMoveSeconds = FPlatformTime::Seconds();
        StrokeTrail.Reset();
        StrokeTrail.Add(Local);
        const float Position = MapPosition(Geometry, Local.X);
        const float Pressure = MapPressure(Geometry, Local.Y);
        OnStrokeBegin.ExecuteIfBound(Position, Pressure);
        Invalidate(EInvalidateWidgetReason::Paint);
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override
    {
        if (!bDragging || !HasMouseCapture())
        {
            return FReply::Unhandled();
        }
        const FVector2D Local = Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        if (FVector2D::Distance(Local, LastPointer) > 0.35f)
        {
            ApplyPointer(Geometry, Local);
        }
        return FReply::Handled();
    }

    virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !bDragging)
        {
            return FReply::Unhandled();
        }
        const FVector2D Local = Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        if (FVector2D::Distance(Local, LastPointer) > 0.35f)
        {
            ApplyPointer(Geometry, Local);
        }
        bDragging = false;
        OnStrokeEnd.ExecuteIfBound();
        Invalidate(EInvalidateWidgetReason::Paint);
        return FReply::Handled().ReleaseMouseCapture();
    }

    virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override
    {
        if (bDragging)
        {
            bDragging = false;
            OnStrokeEnd.ExecuteIfBound();
        }
        SLeafWidget::OnMouseCaptureLost(CaptureLostEvent);
    }

    virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override
    {
        return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
    }

private:
    static float MapPosition(const FGeometry& Geometry, const float X)
    {
        return FMath::Clamp((X - 42.0f) / FMath::Max(1.0f, Geometry.GetLocalSize().X - 84.0f), 0.0f, 1.0f);
    }

    static float MapPressure(const FGeometry& Geometry, const float Y)
    {
        return FMath::Clamp((Y - 28.0f) / FMath::Max(1.0f, Geometry.GetLocalSize().Y - 48.0f), 0.0f, 1.0f);
    }

    void ApplyPointer(const FGeometry& Geometry, const FVector2D& Local)
    {
        const double Now = FPlatformTime::Seconds();
        const float DeltaX = Local.X - LastPointer.X;
        const float DeltaSeconds = FMath::Max(0.001f, static_cast<float>(Now - LastMoveSeconds));
        const float PixelsPerSecond = FMath::Abs(DeltaX) / DeltaSeconds;
        const float Speed = FMath::Clamp(PixelsPerSecond / FMath::Max(320.0f, Geometry.GetLocalSize().X * 1.15f), 0.05f, 1.0f);
        const float Direction = FMath::Abs(DeltaX) > 0.35f ? (DeltaX < 0.0f ? -1.0f : 1.0f) : BowDirection.Get(1.0f);
        OnStrokeChanged.ExecuteIfBound(MapPosition(Geometry, Local.X), Speed, MapPressure(Geometry, Local.Y), Direction);
        LastPointer = Local;
        LastMoveSeconds = Now;
        StrokeTrail.Add(Local);
        if (StrokeTrail.Num() > 18)
        {
            StrokeTrail.RemoveAt(0, StrokeTrail.Num() - 18, EAllowShrinking::No);
        }
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    TAttribute<float> StrokePosition;
    TAttribute<float> BowSpeed;
    TAttribute<float> BowPressure;
    TAttribute<float> BowDirection;
    TAttribute<bool> StrokeActive;
    FOnResonanceBowStrokeBegin OnStrokeBegin;
    FOnResonanceBowStrokeChanged OnStrokeChanged;
    FOnResonanceBowStrokeEnd OnStrokeEnd;
    TArray<FVector2D> StrokeTrail;
    FVector2D LastPointer = FVector2D::ZeroVector;
    double LastMoveSeconds = 0.0;
    bool bDragging = false;
};
