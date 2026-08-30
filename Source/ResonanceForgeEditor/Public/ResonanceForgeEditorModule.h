#pragma once

#include "Modules/ModuleManager.h"

class FResonanceForgeEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    TSharedRef<class SDockTab> SpawnWorkbench(const class FSpawnTabArgs& Args);
    class AResonanceForgeImpactInstrumentActor* ResolveInstrument() const;
    void ApplyPreset(const FName PresetName);
    FReply TriggerPreview();
    FReply OpenDemoMap();
    FText GetSelectionText() const;
    FText GetStatusText() const;

    FName ActivePreset = TEXT("拉丝钢");
    float PreviewEnergy = 0.78f;
    float PreviewBrightness = 0.58f;
    float PreviewSize = 0.5f;
    FText LastStatus;
};
