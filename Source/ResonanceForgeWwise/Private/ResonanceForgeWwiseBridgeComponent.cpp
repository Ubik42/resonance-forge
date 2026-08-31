#include "ResonanceForgeWwiseBridgeComponent.h"

#include "AkAudioEvent.h"
#include "AkGameplayStatics.h"
#include "AkRtpc.h"
#include "ResonanceForgeSynthComponent.h"
#include "ResonanceWwiseRoutingProfile.h"

UResonanceForgeWwiseBridgeComponent::UResonanceForgeWwiseBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UResonanceForgeWwiseBridgeComponent::BeginPlay()
{
    Super::BeginPlay();
    if (RoutingProfile)
    {
        ApplyRoutingProfile(RoutingProfile);
    }
    if (bAutoBindGeneratedAssets)
    {
        AutoBindDemoAssets();
    }
}

bool UResonanceForgeWwiseBridgeComponent::AutoBindDemoAssets()
{
    bUsingDemoFallback = false;
    if (RoutingProfile)
    {
        ApplyRoutingProfile(RoutingProfile);
        if (!RoutingProfile->bAllowDemoAssetFallback)
        {
            return IsRouteComplete();
        }
    }
    if (!SteelImpactEvent)
    {
        SteelImpactEvent = LoadObject<UAkAudioEvent>(nullptr, TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Steel.Play_RF_Impact_Steel"));
        bUsingDemoFallback |= SteelImpactEvent != nullptr;
    }
    if (!WoodImpactEvent)
    {
        WoodImpactEvent = LoadObject<UAkAudioEvent>(nullptr, TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Wood.Play_RF_Impact_Wood"));
        bUsingDemoFallback |= WoodImpactEvent != nullptr;
    }
    if (!GlassImpactEvent)
    {
        GlassImpactEvent = LoadObject<UAkAudioEvent>(nullptr, TEXT("/Game/WwiseAudio/Events/Default_Work_Unit/ResonanceForge/Play_RF_Impact_Glass.Play_RF_Impact_Glass"));
        bUsingDemoFallback |= GlassImpactEvent != nullptr;
    }
    if (!ImpactEnergyRtpc)
    {
        ImpactEnergyRtpc = LoadObject<UAkRtpc>(nullptr, TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ImpactEnergy.RF_ImpactEnergy"));
        bUsingDemoFallback |= ImpactEnergyRtpc != nullptr;
    }
    if (!ImpactBrightnessRtpc)
    {
        ImpactBrightnessRtpc = LoadObject<UAkRtpc>(nullptr, TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ImpactBrightness.RF_ImpactBrightness"));
        bUsingDemoFallback |= ImpactBrightnessRtpc != nullptr;
    }
    if (!ObjectSizeRtpc)
    {
        ObjectSizeRtpc = LoadObject<UAkRtpc>(nullptr, TEXT("/Game/WwiseAudio/Game_Parameters/Default_Work_Unit/RF_ObjectSize.RF_ObjectSize"));
        bUsingDemoFallback |= ObjectSizeRtpc != nullptr;
    }
    return IsRouteComplete();
}

bool UResonanceForgeWwiseBridgeComponent::ApplyRoutingProfile(UResonanceWwiseRoutingProfile* NewProfile)
{
    RoutingProfile = NewProfile;
    bUsingDemoFallback = false;
    if (!RoutingProfile)
    {
        return false;
    }
    SteelImpactEvent = RoutingProfile->SteelImpactEvent;
    WoodImpactEvent = RoutingProfile->WoodImpactEvent;
    GlassImpactEvent = RoutingProfile->GlassImpactEvent;
    ImpactEnergyRtpc = RoutingProfile->ImpactEnergyRtpc;
    ImpactBrightnessRtpc = RoutingProfile->ImpactBrightnessRtpc;
    ObjectSizeRtpc = RoutingProfile->ObjectSizeRtpc;
    RtpcInterpolationMs = FMath::Clamp(RoutingProfile->RtpcInterpolationMs, 0, 2000);
    return IsRouteComplete();
}

bool UResonanceForgeWwiseBridgeComponent::IsRouteComplete() const
{
    return SteelImpactEvent && WoodImpactEvent && GlassImpactEvent
        && ImpactEnergyRtpc && ImpactBrightnessRtpc && ObjectSizeRtpc;
}

UAkAudioEvent* UResonanceForgeWwiseBridgeComponent::GetRoutedEventForPreset(const FName MaterialPreset) const
{
    if (MaterialPreset == TEXT("硬木"))
    {
        return WoodImpactEvent ? WoodImpactEvent : SteelImpactEvent;
    }
    if (MaterialPreset == TEXT("薄玻璃"))
    {
        return GlassImpactEvent ? GlassImpactEvent : SteelImpactEvent;
    }
    return SteelImpactEvent;
}

FString UResonanceForgeWwiseBridgeComponent::GetRouteSourceName() const
{
    if (RoutingProfile)
    {
        const FString Name = RoutingProfile->DisplayName.IsEmpty()
            ? RoutingProfile->GetName()
            : RoutingProfile->DisplayName.ToString();
        return bUsingDemoFallback
            ? FString::Printf(TEXT("共享路由「%s」+ Demo 补位"), *Name)
            : FString::Printf(TEXT("共享路由「%s」"), *Name);
    }
    return bUsingDemoFallback ? TEXT("Demo 自动绑定") : TEXT("场景手工绑定");
}

float UResonanceForgeWwiseBridgeComponent::ToWwiseRtpc(const float NormalizedValue)
{
    return FMath::Clamp(NormalizedValue, 0.0f, 1.0f) * 100.0f;
}

void UResonanceForgeWwiseBridgeComponent::SetLiveBrightness(const float NormalizedBrightness)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }
    if (!ImpactBrightnessRtpc && bAutoBindGeneratedAssets)
    {
        AutoBindDemoAssets();
    }
    if (ImpactBrightnessRtpc)
    {
        UAkGameplayStatics::SetRTPCValue(
            ImpactBrightnessRtpc,
            ToWwiseRtpc(NormalizedBrightness),
            RtpcInterpolationMs,
            Owner);
    }
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

    UAkAudioEvent* RoutedEvent = GetRoutedEventForPreset(Parameters.MaterialPreset);

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
        return FString::Printf(TEXT("Wwise 路由已就绪 · %s"), *GetRouteSourceName());
    }
    return FString::Printf(TEXT("等待绑定：%s"), *FString::Join(MissingItems, TEXT("、")));
}
