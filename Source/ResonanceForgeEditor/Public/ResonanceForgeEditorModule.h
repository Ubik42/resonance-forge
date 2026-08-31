#pragma once

#include "Modules/ModuleManager.h"
#include "ResonanceMaterialProfile.h"
#include "Widgets/Input/SComboBox.h"
#include "Containers/Ticker.h"

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
    void ApplyModalModes(bool bAudition, const FText& ChangeLabel);
    void ResetModalModesToPreset();
    void SelectModalMode(int32 ModeIndex);
    void ApplyStrikePosition(float NewPosition, bool bFinished);
    void AuditionCurrentSound(const FText& ChangeLabel);
    FReply SyncFromSelection();
    FReply TriggerPreview();
    FReply TriggerStrikePreset(float Energy, float Brightness, const FText& GestureName);
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
    bool PollLiveImpact(float DeltaSeconds);
    FReply OpenDemoMap();
    FText GetSelectionText() const;
    FText GetExcitationText() const;
    FText GetResonanceText() const;
    FText GetOutputRouteText() const;
    FText GetPrimaryActionText() const;
    FText GetStatusText() const;
    FText GetComparisonText() const;
    FText GetRecipeSlotText(int32 SlotIndex) const;
    FText GetMidiStatusText() const;
    FText GetWwiseStatusText() const;
    FText GetWwiseVolumeText() const;
    FText GetWwiseLowpassText() const;
    FText GetWwisePitchText() const;
    FText GetSelectedModeFrequencyText() const;
    FText GetSelectedModeGainText() const;
    FText GetSelectedModeDecayText() const;
    FText GetStrikePositionText() const;
    float GetLiveImpactGlow() const;
    bool HasRecipeSlot(int32 SlotIndex) const;
    bool ReadRecipeSlot(int32 SlotIndex, FName& OutPreset, EResonanceModelType& OutModel, float& OutEnergy, float& OutBrightness, float& OutSize, float& OutSustain, float& OutDamping, float& OutCoupling) const;

    FName ActivePreset = TEXT("拉丝钢");
    EResonanceModelType ActiveModel = EResonanceModelType::ModalImpact;
    float PreviewEnergy = 0.78f;
    float PreviewBrightness = 0.58f;
    float PreviewSize = 0.5f;
    float PreviewStrikePosition = 0.5f;
    float WaveguideSustain = 0.90f;
    float WaveguideDamping = 0.36f;
    float WaveguideCoupling = 0.22f;
    TArray<FResonanceMode> ActiveModes;
    int32 SelectedModeIndex = 0;
    bool bHasReference = false;
    FName ReferencePreset = NAME_None;
    EResonanceModelType ReferenceModel = EResonanceModelType::ModalImpact;
    float ReferenceEnergy = 0.0f;
    float ReferenceBrightness = 0.0f;
    float ReferenceSize = 0.0f;
    float ReferenceStrikePosition = 0.5f;
    float ReferenceSustain = 0.90f;
    float ReferenceDamping = 0.36f;
    float ReferenceCoupling = 0.22f;
    TArray<FResonanceMode> ReferenceModes;
    TArray<TSharedPtr<FString>> MidiDeviceOptions;
    TArray<int32> MidiDeviceIds;
    TSharedPtr<FString> SelectedMidiDevice;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> MidiDeviceCombo;
    TWeakPtr<SWidget> WorkbenchWidget;
    TSharedPtr<class SScrollBox> WorkbenchScrollBox;
    class IConsoleObject* CaptureConsoleCommand = nullptr;
    FTSTicker::FDelegateHandle LiveImpactTickerHandle;
    TWeakObjectPtr<class AResonanceForgeImpactInstrumentActor> ObservedImpactActor;
    int32 ObservedImpactSerial = INDEX_NONE;
    float LiveImpactPosition = 0.5f;
    float LiveImpactEnergy = 0.0f;
    float LiveImpactBrightness = 0.0f;
    double LiveImpactObservedSeconds = -1000.0;
    FString SharedRecipeName = TEXT("新声学配方");
    FText LastStatus;
};
