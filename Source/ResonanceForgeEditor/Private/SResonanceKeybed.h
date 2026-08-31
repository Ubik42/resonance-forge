#pragma once

#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_TwoParams(FOnResonanceKeyPlayed, int32, float)

class SResonanceKeybed final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceKeybed) {}
        SLATE_ATTRIBUTE(int32, LastNote)
        SLATE_ATTRIBUTE(float, LastVelocity)
        SLATE_ATTRIBUTE(float, NoteGlow)
        SLATE_EVENT(FOnResonanceKeyPlayed, OnNotePlayed)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        LastNote = InArgs._LastNote;
        LastVelocity = InArgs._LastVelocity;
        NoteGlow = InArgs._NoteGlow;
        OnNotePlayed = InArgs._OnNotePlayed;
        SetCanTick(false);
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(720.0f, 138.0f);
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
        static const TCHAR* Labels[] = {TEXT("C3"), TEXT("C#"), TEXT("D"), TEXT("D#"), TEXT("E"), TEXT("F"), TEXT("F#"), TEXT("G"), TEXT("G#"), TEXT("A"), TEXT("A#"), TEXT("B"), TEXT("C4")};
        static const bool Raised[] = {false, true, false, true, false, false, true, false, true, false, true, false, false};
        const FVector2D Size = Geometry.GetLocalSize();
        const float Gap = 5.0f;
        const float KeyWidth = (Size.X - Gap * 14.0f) / 13.0f;
        const int32 ActiveNote = LastNote.Get(INDEX_NONE);
        const float Velocity = FMath::Clamp(LastVelocity.Get(0.0f), 0.0f, 1.0f);
        const float Glow = FMath::Clamp(NoteGlow.Get(0.0f), 0.0f, 1.0f);
        const FLinearColor Steel(0.34f, 0.43f, 0.44f, 1.0f);
        const FLinearColor Charcoal(0.095f, 0.075f, 0.060f, 1.0f);
        const FLinearColor Copper(1.0f, 0.54f, 0.19f, 1.0f);
        const FLinearColor Muted(0.51f, 0.57f, 0.57f, 0.92f);

        for (int32 Index = 0; Index < 13; ++Index)
        {
            const float X = Gap + Index * (KeyWidth + Gap);
            const float Top = Raised[Index] ? 18.0f : 7.0f;
            const float Height = Raised[Index] ? Size.Y - 38.0f : Size.Y - 26.0f;
            const bool bActive = ActiveNote == 48 + Index;
            const FLinearColor Base = Raised[Index] ? Charcoal : Steel;
            const FLinearColor Color = bActive
                ? FLinearColor::LerpUsingHSV(Base, Copper, 0.48f + Glow * 0.52f)
                : Base;
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId,
                Geometry.ToPaintGeometry(FVector2f(KeyWidth, Height), FSlateLayoutTransform(FVector2f(X, Top))),
                FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color);

            const float HammerY = FMath::Lerp(Top + 12.0f, Top + Height - 13.0f, bActive ? Velocity : 0.18f);
            TArray<FVector2D> HammerLine = {FVector2D(X + 8.0f, HammerY), FVector2D(X + KeyWidth - 8.0f, HammerY)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), HammerLine, ESlateDrawEffect::None, bActive ? Copper : Muted.CopyWithNewOpacity(0.35f), true, bActive ? 3.0f + Glow * 2.0f : 1.0f);
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(FVector2f(KeyWidth, 18.0f), FSlateLayoutTransform(FVector2f(X + 4.0f, Size.Y - 20.0f))),
                Labels[Index], FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, bActive ? Copper : Muted);
        }
        return LayerId + 3;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
        LastDraggedNote = INDEX_NONE;
        PlayAtPointer(Geometry, Event);
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (HasMouseCapture())
        {
            PlayAtPointer(Geometry, Event);
            return FReply::Handled();
        }
        return FReply::Unhandled();
    }

    virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override
    {
        if (Event.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture())
        {
            LastDraggedNote = INDEX_NONE;
            return FReply::Handled().ReleaseMouseCapture();
        }
        return FReply::Unhandled();
    }

private:
    void PlayAtPointer(const FGeometry& Geometry, const FPointerEvent& Event)
    {
        const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
        const int32 NoteIndex = FMath::Clamp(FMath::FloorToInt(Local.X / FMath::Max(1.0f, Geometry.GetLocalSize().X) * 13.0f), 0, 12);
        if (NoteIndex == LastDraggedNote) return;
        LastDraggedNote = NoteIndex;
        const float Velocity = FMath::Lerp(0.20f, 1.0f, FMath::Clamp(Local.Y / FMath::Max(1.0f, Geometry.GetLocalSize().Y), 0.0f, 1.0f));
        OnNotePlayed.ExecuteIfBound(48 + NoteIndex, Velocity);
    }

    TAttribute<int32> LastNote;
    TAttribute<float> LastVelocity;
    TAttribute<float> NoteGlow;
    FOnResonanceKeyPlayed OnNotePlayed;
    int32 LastDraggedNote = INDEX_NONE;
};
