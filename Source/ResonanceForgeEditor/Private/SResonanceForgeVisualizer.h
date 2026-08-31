#pragma once

#include "Rendering/DrawElements.h"
#include "ResonanceMaterialProfile.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

class SResonanceForgeVisualizer final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SResonanceForgeVisualizer) {}
        SLATE_ATTRIBUTE(EResonanceModelType, ModelType)
        SLATE_ATTRIBUTE(FName, PresetName)
        SLATE_ATTRIBUTE(float, Energy)
        SLATE_ATTRIBUTE(float, Brightness)
        SLATE_ATTRIBUTE(float, Size)
        SLATE_ATTRIBUTE(float, Sustain)
        SLATE_ATTRIBUTE(float, Damping)
        SLATE_ATTRIBUTE(float, Coupling)
        SLATE_ATTRIBUTE(bool, HasReference)
        SLATE_ATTRIBUTE(EResonanceModelType, ReferenceModelType)
        SLATE_ATTRIBUTE(FName, ReferencePresetName)
        SLATE_ATTRIBUTE(float, ReferenceEnergy)
        SLATE_ATTRIBUTE(float, ReferenceBrightness)
        SLATE_ATTRIBUTE(float, ReferenceSize)
        SLATE_ATTRIBUTE(float, ReferenceSustain)
        SLATE_ATTRIBUTE(float, ReferenceDamping)
        SLATE_ATTRIBUTE(float, ReferenceCoupling)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        ModelType = InArgs._ModelType;
        PresetName = InArgs._PresetName;
        Energy = InArgs._Energy;
        Brightness = InArgs._Brightness;
        SizeValue = InArgs._Size;
        Sustain = InArgs._Sustain;
        Damping = InArgs._Damping;
        Coupling = InArgs._Coupling;
        HasReference = InArgs._HasReference;
        ReferenceModelType = InArgs._ReferenceModelType;
        ReferencePresetName = InArgs._ReferencePresetName;
        ReferenceEnergy = InArgs._ReferenceEnergy;
        ReferenceBrightness = InArgs._ReferenceBrightness;
        ReferenceSize = InArgs._ReferenceSize;
        ReferenceSustain = InArgs._ReferenceSustain;
        ReferenceDamping = InArgs._ReferenceDamping;
        ReferenceCoupling = InArgs._ReferenceCoupling;
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(620.0f, 280.0f);
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
        const FLinearColor Background(0.010f, 0.014f, 0.018f, 1.0f);
        const FLinearColor Grid(0.18f, 0.20f, 0.16f, 0.26f);
        const FLinearColor Accent = ModelType.Get(EResonanceModelType::ModalImpact) == EResonanceModelType::WaveguideString
            ? FLinearColor(0.98f, 0.46f, 0.12f, 1.0f)
            : FLinearColor(0.06f, 0.82f, 0.92f, 1.0f);

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
            ESlateDrawEffect::None,
            Background);

        const FVector2f Center(static_cast<float>(Size.X * 0.5f), static_cast<float>(Size.Y * 0.51f));
        const float Radius = FMath::Min(Size.X * 0.34f, Size.Y * 0.42f);
        for (int32 Ring = 1; Ring <= 4; ++Ring)
        {
            TArray<FVector2f> Circle;
            constexpr int32 CircleSegments = 72;
            for (int32 Index = 0; Index <= CircleSegments; ++Index)
            {
                const float Angle = 2.0f * PI * static_cast<float>(Index) / static_cast<float>(CircleSegments);
                const float R = Radius * static_cast<float>(Ring) / 4.0f;
                Circle.Add(Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * R);
            }
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Circle, ESlateDrawEffect::None, Grid, true, 1.0f);
        }
        for (int32 Spoke = 0; Spoke < 12; ++Spoke)
        {
            const float Angle = 2.0f * PI * static_cast<float>(Spoke) / 12.0f;
            TArray<FVector2f> Line = {Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius * 0.18f, Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Grid, true, 1.0f);
        }

        const float EnergyValue = FMath::Clamp(Energy.Get(0.75f), 0.0f, 1.0f);
        const float BrightnessValue = FMath::Clamp(Brightness.Get(0.55f), 0.0f, 1.0f);
        const float ScaleValue = FMath::Clamp(SizeValue.Get(0.5f), 0.0f, 1.0f);

        auto DrawFingerprint = [&](EResonanceModelType InModel, FName InPreset, float InEnergy, float InBrightness, float InSize, float InSustain, float InDamping, float InCoupling, const FLinearColor& Color, float Thickness, int32 DrawLayer)
        {
            TArray<FVector2f> Shape;
            constexpr int32 SegmentCount = 120;
            Shape.Reserve(SegmentCount + 1);
            const bool bWood = InPreset == TEXT("硬木");
            const bool bGlass = InPreset == TEXT("薄玻璃");
            const float MaterialPhase = bWood ? 0.8f : (bGlass ? 2.1f : 0.0f);
            for (int32 Index = 0; Index <= SegmentCount; ++Index)
            {
                const float T = static_cast<float>(Index) / static_cast<float>(SegmentCount);
                const float Angle = T * 2.0f * PI - PI * 0.5f;
                float Texture = 0.0f;
                if (InModel == EResonanceModelType::WaveguideString)
                {
                    const float DetailFrequency = FMath::Lerp(8.0f, 3.0f, InDamping) + InBrightness * 2.0f;
                    Texture = FMath::Lerp(0.055f, 0.18f, InCoupling) * FMath::Sin(Angle * DetailFrequency);
                    Texture += FMath::Lerp(0.035f, 0.10f, InSustain) * FMath::Sin(Angle * 2.0f + 0.4f);
                }
                else
                {
                    const float Teeth = bGlass ? 9.0f : (bWood ? 5.0f : 7.0f);
                    Texture = 0.13f * FMath::Pow(FMath::Abs(FMath::Sin(Angle * Teeth + MaterialPhase)), 3.0f);
                    Texture += 0.07f * FMath::Sin(Angle * (2.0f + InBrightness * 3.0f) + MaterialPhase);
                }
                const float Base = InModel == EResonanceModelType::WaveguideString
                    ? 0.40f + InSize * 0.17f + InSustain * 0.13f
                    : 0.48f + InSize * 0.20f;
                const float Dynamic = Texture * FMath::Lerp(0.35f, 1.0f, InEnergy);
                const float R = Radius * FMath::Clamp(Base + Dynamic, 0.2f, 0.96f);
                Shape.Add(Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * R);
            }
            FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, AllottedGeometry.ToPaintGeometry(), Shape, ESlateDrawEffect::None, Color, true, Thickness);
        };

        if (HasReference.Get(false))
        {
            DrawFingerprint(ReferenceModelType.Get(EResonanceModelType::ModalImpact), ReferencePresetName.Get(NAME_None), ReferenceEnergy.Get(0.0f), ReferenceBrightness.Get(0.0f), ReferenceSize.Get(0.0f), ReferenceSustain.Get(0.90f), ReferenceDamping.Get(0.36f), ReferenceCoupling.Get(0.22f), FLinearColor(0.92f, 0.52f, 0.18f, 0.62f), 1.2f, LayerId + 2);
        }
        DrawFingerprint(ModelType.Get(EResonanceModelType::ModalImpact), PresetName.Get(TEXT("拉丝钢")), EnergyValue, BrightnessValue, ScaleValue, Sustain.Get(0.90f), Damping.Get(0.36f), Coupling.Get(0.22f), Accent, 2.6f, LayerId + 3);

        TArray<FVector2f> CoreLine = {{Center.X - Radius * 0.12f, Center.Y}, {Center.X + Radius * 0.12f, Center.Y}};
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), CoreLine, ESlateDrawEffect::None, FLinearColor(0.94f, 0.88f, 0.68f, 0.8f), true, 2.0f);

        return LayerId + 4;
    }

private:
    TAttribute<EResonanceModelType> ModelType;
    TAttribute<FName> PresetName;
    TAttribute<float> Energy;
    TAttribute<float> Brightness;
    TAttribute<float> SizeValue;
    TAttribute<float> Sustain;
    TAttribute<float> Damping;
    TAttribute<float> Coupling;
    TAttribute<bool> HasReference;
    TAttribute<EResonanceModelType> ReferenceModelType;
    TAttribute<FName> ReferencePresetName;
    TAttribute<float> ReferenceEnergy;
    TAttribute<float> ReferenceBrightness;
    TAttribute<float> ReferenceSize;
    TAttribute<float> ReferenceSustain;
    TAttribute<float> ReferenceDamping;
    TAttribute<float> ReferenceCoupling;
};
