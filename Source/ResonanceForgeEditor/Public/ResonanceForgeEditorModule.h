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
    void ApplyWaveguideParameters();
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
    FReply ForgeSharedRecipeAsset();
    FReply CaptureWorkbenchScreenshot();
    void QueueAutomatedCapture();
    bool CaptureWorkbenchImage(const FString& FileName);
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
    bool ReadRecipeSlot(int32 SlotIndex, FName& OutPreset, EResonanceModelType& OutModel, float& OutEnergy, float& OutBrightness, float& OutSize, float& OutSustain, float& OutDamping, float& OutCoupling) const;

    FName ActivePreset = TEXT("拉丝钢");
    EResonanceModelType ActiveModel = EResonanceModelType::ModalImpact;
    float PreviewEnergy = 0.78f;
    float PreviewBrightness = 0.58f;
    float PreviewSize = 0.5f;
    float WaveguideSustain = 0.90f;
    float WaveguideDamping = 0.36f;
    float WaveguideCoupling = 0.22f;
    bool bHasReference = false;
    FName ReferencePreset = NAME_None;
    EResonanceModelType ReferenceModel = EResonanceModelType::ModalImpact;
    float ReferenceEnergy = 0.0f;
    float ReferenceBrightness = 0.0f;
    float ReferenceSize = 0.0f;
    float ReferenceSustain = 0.90f;
    float ReferenceDamping = 0.36f;
    float ReferenceCoupling = 0.22f;
    TArray<TSharedPtr<FString>> MidiDeviceOptions;
    TArray<int32> MidiDeviceIds;
    TSharedPtr<FString> SelectedMidiDevice;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> MidiDeviceCombo;
    TWeakPtr<SWidget> WorkbenchWidget;
    TSharedPtr<class SScrollBox> WorkbenchScrollBox;
    class IConsoleObject* CaptureConsoleCommand = nullptr;
    FString SharedRecipeName = TEXT("新声学配方");
    FText LastStatus;
};
