#pragma once

#include "Modules/ModuleManager.h"
#include "ResonanceMaterialProfile.h"
#include "Widgets/Input/SComboBox.h"

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
    void ApplyModel(EResonanceModelType ModelType);
    FReply SyncFromSelection();
    FReply TriggerPreview();
    FReply PinReference();
    FReply SwapAndPreviewReference();
    FReply ClearReference();
    FReply SaveRecipeSlot(int32 SlotIndex);
    FReply RecallRecipeSlot(int32 SlotIndex);
    FReply RefreshMidiDevices();
    FReply ConnectSelectedMidiDevice();
    FReply DisconnectMidiDevice();
    FReply OpenDemoMap();
    FText GetSelectionText() const;
    FText GetExcitationText() const;
    FText GetResonanceText() const;
    FText GetPrimaryActionText() const;
    FText GetStatusText() const;
    FText GetComparisonText() const;
    FText GetRecipeSlotText(int32 SlotIndex) const;
    FText GetMidiStatusText() const;
    FText GetWwiseStatusText() const;
    bool HasRecipeSlot(int32 SlotIndex) const;
    bool ReadRecipeSlot(int32 SlotIndex, FName& OutPreset, EResonanceModelType& OutModel, float& OutEnergy, float& OutBrightness, float& OutSize) const;

    FName ActivePreset = TEXT("拉丝钢");
    EResonanceModelType ActiveModel = EResonanceModelType::ModalImpact;
    float PreviewEnergy = 0.78f;
    float PreviewBrightness = 0.58f;
    float PreviewSize = 0.5f;
    bool bHasReference = false;
    FName ReferencePreset = NAME_None;
    EResonanceModelType ReferenceModel = EResonanceModelType::ModalImpact;
    float ReferenceEnergy = 0.0f;
    float ReferenceBrightness = 0.0f;
    float ReferenceSize = 0.0f;
    TArray<TSharedPtr<FString>> MidiDeviceOptions;
    TArray<int32> MidiDeviceIds;
    TSharedPtr<FString> SelectedMidiDevice;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> MidiDeviceCombo;
    FText LastStatus;
};
