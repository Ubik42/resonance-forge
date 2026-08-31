#include "ResonanceForgeWwiseBridgeComponent.h"

#include "AkAudioEvent.h"
#include "AkGameplayStatics.h"
#include "AkRtpc.h"
#include "ResonanceForgeSynthComponent.h"

UResonanceForgeWwiseBridgeComponent::UResonanceForgeWwiseBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UResonanceForgeWwiseBridgeComponent::BeginPlay()
{
    Super::BeginPlay();
    if (bAutoBindGeneratedAssets)
    {
        AutoBindDemoAssets();
    }
}

bool UResonanceForgeWwiseBridgeComponent::AutoBindDemoAssets()
{
    if (!SteelImpactEvent)
    {
        SteelImpactEvent = LoadObject<UAkAudioEvent>(nullptr, TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Steel.Play_RF_Impact_Steel"));
    }
    if (!WoodImpactEvent)
    {
        WoodImpactEvent = LoadObject<UAkAudioEvent>(nullptr, TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Wood.Play_RF_Impact_Wood"));
    }
    if (!GlassImpactEvent)
    {
        GlassImpactEvent = LoadObject<UAkAudioEvent>(nullptr, TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Glass.Play_RF_Impact_Glass"));
    }
    if (!ImpactEnergyRtpc)
    {
        ImpactEnergyRtpc = LoadObject<UAkRtpc>(nullptr, TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ImpactEnergy.RF_ImpactEnergy"));
    }
    if (!ImpactBrightnessRtpc)
    {
        ImpactBrightnessRtpc = LoadObject<UAkRtpc>(nullptr, TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ImpactBrightness.RF_ImpactBrightness"));
    }
    if (!ObjectSizeRtpc)
    {
        ObjectSizeRtpc = LoadObject<UAkRtpc>(nullptr, TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ObjectSize.RF_ObjectSize"));
    }
    return SteelImpactEvent && WoodImpactEvent && GlassImpactEvent
        && ImpactEnergyRtpc && ImpactBrightnessRtpc && ObjectSizeRtpc;
}

float UResonanceForgeWwiseBridgeComponent::ToWwiseRtpc(const float NormalizedValue)
{
    return FMath::Clamp(NormalizedValue, 0.0f, 1.0f) * 100.0f;
}

int32 UResonanceForgeWwiseBridgeComponent::TriggerImpact(
    const FResonanceForgeImpactParameters& Parameters,
    UResonanceForgeSynthComponent* NativeSynth)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return 0;
    }

    if (bAutoBindGeneratedAssets && (!SteelImpactEvent || !WoodImpactEvent || !GlassImpactEvent
        || !ImpactEnergyRtpc || !ImpactBrightnessRtpc || !ObjectSizeRtpc))
    {
        AutoBindDemoAssets();
    }

    const float Energy = FMath::Clamp(Parameters.Energy, 0.0f, 1.0f);
    const float Brightness = FMath::Clamp(Parameters.Brightness, 0.0f, 1.0f);
    const float ObjectSize = FMath::Clamp(Parameters.ObjectSize, 0.0f, 1.0f);

    if (bLayerNativeSynth && NativeSynth)
    {
        NativeSynth->Strike(Energy, Brightness, FMath::Clamp(Parameters.MidiNote, 0, 127), Parameters.StrikePosition);
    }

    if (ImpactEnergyRtpc)
    {
        UAkGameplayStatics::SetRTPCValue(ImpactEnergyRtpc, ToWwiseRtpc(Energy), RtpcInterpolationMs, Owner);
    }
    if (ImpactBrightnessRtpc)
    {
        UAkGameplayStatics::SetRTPCValue(ImpactBrightnessRtpc, ToWwiseRtpc(Brightness), RtpcInterpolationMs, Owner);
    }
    if (ObjectSizeRtpc)
    {
        UAkGameplayStatics::SetRTPCValue(ObjectSizeRtpc, ToWwiseRtpc(ObjectSize), RtpcInterpolationMs, Owner);
    }

    UAkAudioEvent* RoutedEvent = SteelImpactEvent;
    if (Parameters.MaterialPreset == TEXT("硬木"))
    {
        RoutedEvent = WoodImpactEvent ? WoodImpactEvent : SteelImpactEvent;
    }
    else if (Parameters.MaterialPreset == TEXT("薄玻璃"))
    {
        RoutedEvent = GlassImpactEvent ? GlassImpactEvent : SteelImpactEvent;
    }

    if (!RoutedEvent)
    {
        return 0;
    }

    return UAkGameplayStatics::PostEvent(RoutedEvent, Owner, 0, FOnAkPostEventCallback(), false);
}

FString UResonanceForgeWwiseBridgeComponent::GetIntegrationStatus() const
{
    TArray<FString> MissingItems;
    if (!SteelImpactEvent) MissingItems.Add(TEXT("拉丝钢 Event"));
    if (!WoodImpactEvent) MissingItems.Add(TEXT("硬木 Event"));
    if (!GlassImpactEvent) MissingItems.Add(TEXT("薄玻璃 Event"));
    if (!ImpactEnergyRtpc) MissingItems.Add(TEXT("ImpactEnergy RTPC"));
    if (!ImpactBrightnessRtpc) MissingItems.Add(TEXT("ImpactBrightness RTPC"));
    if (!ObjectSizeRtpc) MissingItems.Add(TEXT("ObjectSize RTPC"));

    if (MissingItems.IsEmpty())
    {
        return TEXT("Wwise 桥接已就绪");
    }
    return FString::Printf(TEXT("等待绑定：%s"), *FString::Join(MissingItems, TEXT("、")));
}
