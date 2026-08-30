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
    if (!ImpactEvent)
    {
        ImpactEvent = LoadObject<UAkAudioEvent>(nullptr, TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Metal.Play_RF_Impact_Metal"));
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
    return ImpactEvent && ImpactEnergyRtpc && ImpactBrightnessRtpc && ObjectSizeRtpc;
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

    if (bAutoBindGeneratedAssets && (!ImpactEvent || !ImpactEnergyRtpc || !ImpactBrightnessRtpc || !ObjectSizeRtpc))
    {
        AutoBindDemoAssets();
    }

    const float Energy = FMath::Clamp(Parameters.Energy, 0.0f, 1.0f);
    const float Brightness = FMath::Clamp(Parameters.Brightness, 0.0f, 1.0f);
    const float ObjectSize = FMath::Clamp(Parameters.ObjectSize, 0.0f, 1.0f);

    if (bLayerNativeSynth && NativeSynth)
    {
        NativeSynth->Strike(Energy, Brightness, FMath::Clamp(Parameters.MidiNote, 0, 127));
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

    if (!ImpactEvent)
    {
        return 0;
    }

    return UAkGameplayStatics::PostEvent(ImpactEvent, Owner, 0, FOnAkPostEventCallback(), false);
}

FString UResonanceForgeWwiseBridgeComponent::GetIntegrationStatus() const
{
    TArray<FString> MissingItems;
    if (!ImpactEvent) MissingItems.Add(TEXT("Impact Event"));
    if (!ImpactEnergyRtpc) MissingItems.Add(TEXT("ImpactEnergy RTPC"));
    if (!ImpactBrightnessRtpc) MissingItems.Add(TEXT("ImpactBrightness RTPC"));
    if (!ObjectSizeRtpc) MissingItems.Add(TEXT("ObjectSize RTPC"));

    if (MissingItems.IsEmpty())
    {
        return TEXT("Wwise 桥接已就绪");
    }
    return FString::Printf(TEXT("等待绑定：%s"), *FString::Join(MissingItems, TEXT("、")));
}
