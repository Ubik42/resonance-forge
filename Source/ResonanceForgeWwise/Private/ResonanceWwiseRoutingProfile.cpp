#include "ResonanceWwiseRoutingProfile.h"

#include "AkAudioEvent.h"
#include "AkRtpc.h"

bool UResonanceWwiseRoutingProfile::IsComplete() const
{
    return SteelImpactEvent && WoodImpactEvent && GlassImpactEvent
        && ImpactEnergyRtpc && ImpactBrightnessRtpc && ObjectSizeRtpc;
}

FString UResonanceWwiseRoutingProfile::GetMissingItemsText() const
{
    TArray<FString> Missing;
    if (!SteelImpactEvent) Missing.Add(TEXT("拉丝钢 Event"));
    if (!WoodImpactEvent) Missing.Add(TEXT("硬木 Event"));
    if (!GlassImpactEvent) Missing.Add(TEXT("薄玻璃 Event"));
    if (!ImpactEnergyRtpc) Missing.Add(TEXT("能量 RTPC"));
    if (!ImpactBrightnessRtpc) Missing.Add(TEXT("明亮度 RTPC"));
    if (!ObjectSizeRtpc) Missing.Add(TEXT("尺度 RTPC"));
    return FString::Join(Missing, TEXT("、"));
}
