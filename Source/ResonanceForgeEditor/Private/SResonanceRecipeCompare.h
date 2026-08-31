#pragma once

#include "Rendering/DrawElements.h"
#include "ResonanceForgeImpactInstrumentActor.h"
#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

class SResonanceRecipeCompare final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceRecipeCompare) {}
        SLATE_ATTRIBUTE(bool, HasReference)
        SLATE_ATTRIBUTE(FName, CurrentPreset)
        SLATE_ATTRIBUTE(FName, ReferencePreset)
        SLATE_ATTRIBUTE(EResonanceModelType, CurrentModel)
        SLATE_ATTRIBUTE(EResonanceModelType, ReferenceModel)
        SLATE_ATTRIBUTE(EResonanceExcitationType, CurrentExcitation)
        SLATE_ATTRIBUTE(EResonanceExcitationType, ReferenceExcitation)
        SLATE_ATTRIBUTE(EResonanceVelocityCurve, CurrentVelocityCurve)
        SLATE_ATTRIBUTE(EResonanceVelocityCurve, ReferenceVelocityCurve)
        SLATE_ATTRIBUTE(int32, CurrentMidiNote)
        SLATE_ATTRIBUTE(int32, ReferenceMidiNote)
        SLATE_ATTRIBUTE(float, CurrentInputVelocity)
        SLATE_ATTRIBUTE(float, ReferenceInputVelocity)
        SLATE_ATTRIBUTE(float, CurrentBowSpeed)
        SLATE_ATTRIBUTE(float, ReferenceBowSpeed)
        SLATE_ATTRIBUTE(float, CurrentBowPressure)
        SLATE_ATTRIBUTE(float, ReferenceBowPressure)
        SLATE_ATTRIBUTE(float, CurrentBowDirection)
        SLATE_ATTRIBUTE(float, ReferenceBowDirection)
        SLATE_ATTRIBUTE(float, CurrentStrike)
        SLATE_ATTRIBUTE(float, ReferenceStrike)
        SLATE_ATTRIBUTE(float, CurrentPickup)
        SLATE_ATTRIBUTE(float, ReferencePickup)
        SLATE_ATTRIBUTE(float, CurrentSustain)
        SLATE_ATTRIBUTE(float, ReferenceSustain)
        SLATE_ATTRIBUTE(float, CurrentEnergy)
        SLATE_ATTRIBUTE(float, ReferenceEnergy)
        SLATE_ATTRIBUTE(float, CurrentBrightness)
        SLATE_ATTRIBUTE(float, ReferenceBrightness)
        SLATE_ATTRIBUTE(float, CurrentSize)
        SLATE_ATTRIBUTE(float, ReferenceSize)
        SLATE_ATTRIBUTE(int32, CurrentModeCount)
        SLATE_ATTRIBUTE(int32, ReferenceModeCount)
        SLATE_ATTRIBUTE(bool, ModesChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        HasReference = InArgs._HasReference;
        CurrentPreset = InArgs._CurrentPreset;
        ReferencePreset = InArgs._ReferencePreset;
        CurrentModel = InArgs._CurrentModel;
        ReferenceModel = InArgs._ReferenceModel;
        CurrentExcitation = InArgs._CurrentExcitation;
        ReferenceExcitation = InArgs._ReferenceExcitation;
        CurrentVelocityCurve = InArgs._CurrentVelocityCurve;
        ReferenceVelocityCurve = InArgs._ReferenceVelocityCurve;
        CurrentMidiNote = InArgs._CurrentMidiNote;
        ReferenceMidiNote = InArgs._ReferenceMidiNote;
        CurrentInputVelocity = InArgs._CurrentInputVelocity;
        ReferenceInputVelocity = InArgs._ReferenceInputVelocity;
        CurrentBowSpeed = InArgs._CurrentBowSpeed;
        ReferenceBowSpeed = InArgs._ReferenceBowSpeed;
        CurrentBowPressure = InArgs._CurrentBowPressure;
        ReferenceBowPressure = InArgs._ReferenceBowPressure;
        CurrentBowDirection = InArgs._CurrentBowDirection;
        ReferenceBowDirection = InArgs._ReferenceBowDirection;
        CurrentStrike = InArgs._CurrentStrike;
        ReferenceStrike = InArgs._ReferenceStrike;
        CurrentPickup = InArgs._CurrentPickup;
        ReferencePickup = InArgs._ReferencePickup;
        CurrentSustain = InArgs._CurrentSustain;
        ReferenceSustain = InArgs._ReferenceSustain;
        CurrentEnergy = InArgs._CurrentEnergy;
        ReferenceEnergy = InArgs._ReferenceEnergy;
        CurrentBrightness = InArgs._CurrentBrightness;
        ReferenceBrightness = InArgs._ReferenceBrightness;
        CurrentSize = InArgs._CurrentSize;
        ReferenceSize = InArgs._ReferenceSize;
        CurrentModeCount = InArgs._CurrentModeCount;
        ReferenceModeCount = InArgs._ReferenceModeCount;
        ModesChanged = InArgs._ModesChanged;
        SetCanTick(false);
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(760.0f, 148.0f);
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
        const FLinearColor Ink(0.012f, 0.014f, 0.013f, 1.0f);
        const FLinearColor Brass(0.96f, 0.58f, 0.25f, 1.0f);
        const FLinearColor Violet(0.68f, 0.46f, 0.86f, 1.0f);
        const FLinearColor Mint(0.44f, 0.91f, 0.79f, 1.0f);
        const FLinearColor Cold(0.24f, 0.28f, 0.27f, 1.0f);
        const FLinearColor Muted(0.58f, 0.62f, 0.59f, 1.0f);
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), White, ESlateDrawEffect::None, Ink);

        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 1,
            Geometry.ToPaintGeometry(FVector2f(180.0f, 20.0f), FSlateLayoutTransform(FVector2f(14.0f, 9.0f))),
            FText::FromString(TEXT("配方对照尺")), FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Brass);

        if (!HasReference.Get(false))
        {
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 1,
                Geometry.ToPaintGeometry(FVector2f(Size.X - 28.0f, 20.0f), FSlateLayoutTransform(FVector2f(14.0f, 38.0f))),
                FText::FromString(TEXT("先钉住当前配方，再改一处参数；这里会把两版声源沿同一把标尺摊开。")),
                FAppStyle::GetFontStyle(TEXT("NormalFont")), ESlateDrawEffect::None, Muted);
            TArray<FVector2D> EmptyRail = {FVector2D(16.0f, 92.0f), FVector2D(Size.X - 16.0f, 92.0f)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), EmptyRail, ESlateDrawEffect::None, Cold, true, 2.0f);
            return LayerId + 1;
        }

        const FName CurrentPresetValue = CurrentPreset.Get(NAME_None);
        const FName ReferencePresetValue = ReferencePreset.Get(NAME_None);
        const EResonanceModelType CurrentModelValue = CurrentModel.Get(EResonanceModelType::ModalImpact);
        const EResonanceModelType ReferenceModelValue = ReferenceModel.Get(EResonanceModelType::ModalImpact);
        const EResonanceExcitationType CurrentExcitationValue = CurrentExcitation.Get(EResonanceExcitationType::Pick);
        const EResonanceExcitationType ReferenceExcitationValue = ReferenceExcitation.Get(EResonanceExcitationType::Pick);
        const EResonanceVelocityCurve CurrentCurveValue = CurrentVelocityCurve.Get(EResonanceVelocityCurve::Linear);
        const EResonanceVelocityCurve ReferenceCurveValue = ReferenceVelocityCurve.Get(EResonanceVelocityCurve::Linear);

        const auto Percent = [](const float Value)
        {
            return FMath::RoundToInt(FMath::Clamp(Value, 0.0f, 1.0f) * 100.0f);
        };
        const auto ModelLabel = [](const EResonanceModelType Value)
        {
            return Value == EResonanceModelType::WaveguideString ? FString(TEXT("波导弦")) : FString(TEXT("模态体"));
        };
        const auto GestureLabel = [](const EResonanceExcitationType Value, const float Direction)
        {
            if (Value == EResonanceExcitationType::Finger) return FString(TEXT("指腹"));
            if (Value == EResonanceExcitationType::Hammer) return FString(TEXT("锤击"));
            if (Value == EResonanceExcitationType::Bow) return FString(Direction < 0.0f ? TEXT("弓擦·回弓") : TEXT("弓擦·推弓"));
            return FString(TEXT("拨片"));
        };
        const auto CurveLabel = [](const EResonanceVelocityCurve Value)
        {
            return Value == EResonanceVelocityCurve::SoftTouch ? FString(TEXT("软触"))
                : Value == EResonanceVelocityCurve::HeavyHand ? FString(TEXT("重手")) : FString(TEXT("线性"));
        };
        const auto NoteLabel = [](const int32 Note)
        {
            static const TCHAR* Names[] = {TEXT("C"), TEXT("C#"), TEXT("D"), TEXT("D#"), TEXT("E"), TEXT("F"), TEXT("F#"), TEXT("G"), TEXT("G#"), TEXT("A"), TEXT("A#"), TEXT("B")};
            const int32 Safe = FMath::Clamp(Note, 0, 127);
            return FString::Printf(TEXT("%s%d"), Names[Safe % 12], Safe / 12 - 1);
        };

        struct FColumn
        {
            FString Label;
            FString Reference;
            FString Current;
            bool bChanged = false;
        };
        TArray<FColumn> Columns;
        Columns.Reserve(6);
        Columns.Add({TEXT("声材"), ReferencePresetValue.ToString(), CurrentPresetValue.ToString(), ReferencePresetValue != CurrentPresetValue});
        Columns.Add({TEXT("构型"), ModelLabel(ReferenceModelValue), ModelLabel(CurrentModelValue), ReferenceModelValue != CurrentModelValue});
        Columns.Add({TEXT("起振"), GestureLabel(ReferenceExcitationValue, ReferenceBowDirection.Get(1.0f)), GestureLabel(CurrentExcitationValue, CurrentBowDirection.Get(1.0f)),
            ReferenceExcitationValue != CurrentExcitationValue || (CurrentExcitationValue == EResonanceExcitationType::Bow && ReferenceExcitationValue == EResonanceExcitationType::Bow && FMath::Sign(ReferenceBowDirection.Get(1.0f)) != FMath::Sign(CurrentBowDirection.Get(1.0f)))});
        Columns.Add({TEXT("演奏"), FString::Printf(TEXT("%s · %s · %d"), *NoteLabel(ReferenceMidiNote.Get(55)), *CurveLabel(ReferenceCurveValue), Percent(ReferenceInputVelocity.Get(0.76f))),
            FString::Printf(TEXT("%s · %s · %d"), *NoteLabel(CurrentMidiNote.Get(55)), *CurveLabel(CurrentCurveValue), Percent(CurrentInputVelocity.Get(0.76f))),
            ReferenceMidiNote.Get(55) != CurrentMidiNote.Get(55) || ReferenceCurveValue != CurrentCurveValue || !FMath::IsNearlyEqual(ReferenceInputVelocity.Get(0.76f), CurrentInputVelocity.Get(0.76f), 0.005f)});
        if (CurrentModelValue == EResonanceModelType::WaveguideString || ReferenceModelValue == EResonanceModelType::WaveguideString)
        {
            Columns.Add({TEXT("弓感 速/压"), FString::Printf(TEXT("%d / %d"), Percent(ReferenceBowSpeed.Get(0.58f)), Percent(ReferenceBowPressure.Get(0.55f))),
                FString::Printf(TEXT("%d / %d"), Percent(CurrentBowSpeed.Get(0.58f)), Percent(CurrentBowPressure.Get(0.55f))),
                !FMath::IsNearlyEqual(ReferenceBowSpeed.Get(0.58f), CurrentBowSpeed.Get(0.58f), 0.005f) || !FMath::IsNearlyEqual(ReferenceBowPressure.Get(0.55f), CurrentBowPressure.Get(0.55f), 0.005f)});
            Columns.Add({TEXT("弦床 入/拾/延"), FString::Printf(TEXT("%d / %d / %d"), Percent(ReferenceStrike.Get(0.5f)), Percent(ReferencePickup.Get(0.35f)), Percent(ReferenceSustain.Get(0.90f))),
                FString::Printf(TEXT("%d / %d / %d"), Percent(CurrentStrike.Get(0.5f)), Percent(CurrentPickup.Get(0.35f)), Percent(CurrentSustain.Get(0.90f))),
                !FMath::IsNearlyEqual(ReferenceStrike.Get(0.5f), CurrentStrike.Get(0.5f), 0.005f) || !FMath::IsNearlyEqual(ReferencePickup.Get(0.35f), CurrentPickup.Get(0.35f), 0.005f) || !FMath::IsNearlyEqual(ReferenceSustain.Get(0.90f), CurrentSustain.Get(0.90f), 0.005f)});
        }
        else
        {
            Columns.Add({TEXT("音色 能/亮/尺"), FString::Printf(TEXT("%d / %d / %d"), Percent(ReferenceEnergy.Get(0.78f)), Percent(ReferenceBrightness.Get(0.58f)), Percent(ReferenceSize.Get(0.5f))),
                FString::Printf(TEXT("%d / %d / %d"), Percent(CurrentEnergy.Get(0.78f)), Percent(CurrentBrightness.Get(0.58f)), Percent(CurrentSize.Get(0.5f))),
                !FMath::IsNearlyEqual(ReferenceEnergy.Get(0.78f), CurrentEnergy.Get(0.78f), 0.005f) || !FMath::IsNearlyEqual(ReferenceBrightness.Get(0.58f), CurrentBrightness.Get(0.58f), 0.005f) || !FMath::IsNearlyEqual(ReferenceSize.Get(0.5f), CurrentSize.Get(0.5f), 0.005f)});
            Columns.Add({TEXT("共振齿"), FString::Printf(TEXT("%d 根"), ReferenceModeCount.Get(0)), FString::Printf(TEXT("%d 根"), CurrentModeCount.Get(0)), ModesChanged.Get(false)});
        }

        int32 ChangedCount = 0;
        for (const FColumn& Column : Columns) ChangedCount += Column.bChanged ? 1 : 0;
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 1,
            Geometry.ToPaintGeometry(FVector2f(210.0f, 18.0f), FSlateLayoutTransform(FVector2f(Size.X - 225.0f, 11.0f))),
            FText::FromString(FString::Printf(TEXT("A 参考  ·  B 本炉  ·  %d 处改动"), ChangedCount)),
            FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Mint);

        const float Left = 62.0f;
        const float Right = FMath::Max(Left + 1.0f, Size.X - 20.0f);
        const float RailA = 69.0f;
        const float RailB = 113.0f;
        TArray<FVector2D> ReferenceRail = {FVector2D(Left, RailA), FVector2D(Right, RailA)};
        TArray<FVector2D> CurrentRail = {FVector2D(Left, RailB), FVector2D(Right, RailB)};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), ReferenceRail, ESlateDrawEffect::None, Violet.CopyWithNewOpacity(0.42f), true, 2.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), CurrentRail, ESlateDrawEffect::None, Brass.CopyWithNewOpacity(0.55f), true, 2.0f);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(FVector2f(28.0f, 18.0f), FSlateLayoutTransform(FVector2f(18.0f, RailA - 9.0f))), FText::FromString(TEXT("A")), FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Violet);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(FVector2f(28.0f, 18.0f), FSlateLayoutTransform(FVector2f(18.0f, RailB - 9.0f))), FText::FromString(TEXT("B")), FAppStyle::GetFontStyle(TEXT("BoldFont")), ESlateDrawEffect::None, Brass);

        const float Step = (Right - Left) / static_cast<float>(Columns.Num());
        for (int32 Index = 0; Index < Columns.Num(); ++Index)
        {
            const FColumn& Column = Columns[Index];
            const float X = Left + Step * (static_cast<float>(Index) + 0.5f);
            const float TextWidth = FMath::Max(72.0f, Step - 8.0f);
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 2,
                Geometry.ToPaintGeometry(FVector2f(TextWidth, 16.0f), FSlateLayoutTransform(FVector2f(X - TextWidth * 0.5f, 36.0f))),
                FText::FromString(Column.Label), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Column.bChanged ? Mint : Muted);
            if (Column.bChanged)
            {
                TArray<FVector2D> ChangeLine = {FVector2D(X, RailA), FVector2D(X, RailB)};
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(), ChangeLine, ESlateDrawEffect::None, Mint.CopyWithNewOpacity(0.55f), true, 1.0f);
            }
            const FLinearColor Marker = Column.bChanged ? Mint : Cold;
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(FVector2f(7.0f, 7.0f), FSlateLayoutTransform(FVector2f(X - 3.5f, RailA - 3.5f))), White, ESlateDrawEffect::None, Column.bChanged ? Violet : Marker);
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3, Geometry.ToPaintGeometry(FVector2f(7.0f, 7.0f), FSlateLayoutTransform(FVector2f(X - 3.5f, RailB - 3.5f))), White, ESlateDrawEffect::None, Column.bChanged ? Brass : Marker);
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4, Geometry.ToPaintGeometry(FVector2f(TextWidth, 16.0f), FSlateLayoutTransform(FVector2f(X - TextWidth * 0.5f, 75.0f))), FText::FromString(Column.Reference), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Violet.CopyWithNewOpacity(Column.bChanged ? 1.0f : 0.58f));
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4, Geometry.ToPaintGeometry(FVector2f(TextWidth, 16.0f), FSlateLayoutTransform(FVector2f(X - TextWidth * 0.5f, 119.0f))), FText::FromString(Column.Current), FAppStyle::GetFontStyle(TEXT("SmallFont")), ESlateDrawEffect::None, Brass.CopyWithNewOpacity(Column.bChanged ? 1.0f : 0.58f));
        }
        return LayerId + 4;
    }

private:
    TAttribute<bool> HasReference;
    TAttribute<FName> CurrentPreset;
    TAttribute<FName> ReferencePreset;
    TAttribute<EResonanceModelType> CurrentModel;
    TAttribute<EResonanceModelType> ReferenceModel;
    TAttribute<EResonanceExcitationType> CurrentExcitation;
    TAttribute<EResonanceExcitationType> ReferenceExcitation;
    TAttribute<EResonanceVelocityCurve> CurrentVelocityCurve;
    TAttribute<EResonanceVelocityCurve> ReferenceVelocityCurve;
    TAttribute<int32> CurrentMidiNote;
    TAttribute<int32> ReferenceMidiNote;
    TAttribute<float> CurrentInputVelocity;
    TAttribute<float> ReferenceInputVelocity;
    TAttribute<float> CurrentBowSpeed;
    TAttribute<float> ReferenceBowSpeed;
    TAttribute<float> CurrentBowPressure;
    TAttribute<float> ReferenceBowPressure;
    TAttribute<float> CurrentBowDirection;
    TAttribute<float> ReferenceBowDirection;
    TAttribute<float> CurrentStrike;
    TAttribute<float> ReferenceStrike;
    TAttribute<float> CurrentPickup;
    TAttribute<float> ReferencePickup;
    TAttribute<float> CurrentSustain;
    TAttribute<float> ReferenceSustain;
    TAttribute<float> CurrentEnergy;
    TAttribute<float> ReferenceEnergy;
    TAttribute<float> CurrentBrightness;
    TAttribute<float> ReferenceBrightness;
    TAttribute<float> CurrentSize;
    TAttribute<float> ReferenceSize;
    TAttribute<int32> CurrentModeCount;
    TAttribute<int32> ReferenceModeCount;
    TAttribute<bool> ModesChanged;
};
