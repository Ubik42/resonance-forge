#include "ResonanceForgeEditorModule.h"
#include "SResonanceForgeVisualizer.h"
#include "SResonanceModeRack.h"
#include "SResonanceStrikeRail.h"
#include "SResonanceStringPath.h"
#include "SResonanceKeybed.h"
#include "SResonanceVelocityCam.h"
#include "SResonanceDecayPrint.h"
#include "SResonanceForgeFlowRail.h"

#include "Editor.h"
#include "Audio.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Engine/Selection.h"
#include "Framework/Docking/TabManager.h"
#include "Materials/MaterialInterface.h"
#include "MIDIDeviceManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "../../ResonanceForgeWwise/Public/ResonanceForgeImpactInstrumentActor.h"
#include "../../ResonanceForgeWwise/Public/ResonanceForgeWwiseBridgeComponent.h"
#include "ResonanceForgeSynthComponent.h"
#include "Styling/AppStyle.h"
#include "LevelEditorSubsystem.h"
#include "ToolMenus.h"
#include "UObject/SavePackage.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace ResonanceForgeEditor
{
    static const FName TabName(TEXT("ResonanceForgeWorkbench"));
    static const FLinearColor Cyan(0.10f, 0.80f, 0.86f, 1.0f);
    static const FLinearColor Steel(0.35f, 0.66f, 0.88f, 1.0f);
    static const FLinearColor Wood(0.94f, 0.48f, 0.17f, 1.0f);
    static const FLinearColor Glass(0.43f, 0.90f, 0.77f, 1.0f);
    static const FLinearColor Panel(0.015f, 0.019f, 0.021f, 0.98f);
    static const FLinearColor PanelRaised(0.035f, 0.041f, 0.040f, 1.0f);
    static const FLinearColor Muted(0.63f, 0.66f, 0.62f, 1.0f);
    static constexpr float WaveguideDecayMin = 0.9700f;
    static constexpr float WaveguideDecayMax = 0.9995f;

    float SmoothCurve(const float Alpha)
    {
        const float T = FMath::Clamp(Alpha, 0.0f, 1.0f);
        return T * T * (3.0f - 2.0f * T);
    }

    FString GetWwiseEventName(const FName PresetName)
    {
        if (PresetName == TEXT("硬木"))
        {
            return TEXT("Play_RF_Impact_Wood");
        }
        if (PresetName == TEXT("薄玻璃"))
        {
            return TEXT("Play_RF_Impact_Glass");
        }
        return TEXT("Play_RF_Impact_Steel");
    }

    TSharedRef<SWidget> WorkspaceTitle(const FText& Title, const FText& Detail)
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
            [SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted).AutoWrapText(true)];
    }

}

void FResonanceForgeEditorModule::StartupModule()
{
    LastStatus = NSLOCTEXT("ResonanceForge", "Ready", "已就绪 · 选择场景中的共振体开始工作");
    LiveImpactTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateRaw(this, &FResonanceForgeEditorModule::PollLiveImpact), 0.04f);
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        ResonanceForgeEditor::TabName,
        FOnSpawnTab::CreateRaw(this, &FResonanceForgeEditorModule::SpawnWorkbench))
        .SetDisplayName(NSLOCTEXT("ResonanceForge", "TabTitle", "共振铸造台"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FResonanceForgeEditorModule::RegisterMenus));

    CaptureConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("ResonanceForge.CaptureWorkbench"),
        TEXT("打开共振铸造台并将 Slate 工作台导出为 docs/images/resonance-forge-workbench.png"),
        FConsoleCommandDelegate::CreateRaw(this, &FResonanceForgeEditorModule::QueueAutomatedCapture),
        ECVF_Default);
}

void FResonanceForgeEditorModule::ShutdownModule()
{
    if (LiveImpactTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(LiveImpactTickerHandle);
        LiveImpactTickerHandle.Reset();
    }
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    if (CaptureConsoleCommand)
    {
        IConsoleManager::Get().UnregisterConsoleObject(CaptureConsoleCommand);
        CaptureConsoleCommand = nullptr;
    }
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ResonanceForgeEditor::TabName);
}

void FResonanceForgeEditorModule::QueueAutomatedCapture()
{
    OpenDemoMap();
    if (GEditor)
    {
        UWorld* World = GEditor->GetEditorWorldContext().World();
        GEditor->SelectNone(false, true, false);
        if (World)
        {
            for (TActorIterator<AResonanceForgeImpactInstrumentActor> It(World); It; ++It)
            {
                if (It->GetActorLabel() == TEXT("RF_04_数字波导弦"))
                {
                    GEditor->SelectActor(*It, true, true, true);
                    break;
                }
            }
        }
    }
    SyncFromSelection();
    if (ActiveModel == EResonanceModelType::WaveguideString)
    {
        PinReference();
        WaveguideSustain = 0.72f;
        WaveguideDamping = 0.52f;
        WaveguideCoupling = 0.26f;
        WaveguidePickup = 0.82f;
        WaveguideExcitation = EResonanceExcitationType::Bow;
        VelocityCurve = EResonanceVelocityCurve::SoftTouch;
        if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
        {
            Instrument->VelocityCurve = VelocityCurve;
        }
        ApplyStrikePosition(0.34f, false);
        SampleExportName = TEXT("RF_Bow_G3");
        ApplyWaveguideParameters();
        TriggerKeybedNote(55, 0.76f);
    }
    FGlobalTabmanager::Get()->TryInvokeTab(ResonanceForgeEditor::TabName);

    FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this](float)
        {
            CaptureWorkbenchScreenshot();
            if (WorkbenchScrollBox.IsValid())
            {
                WorkbenchScrollBox->SetScrollOffset(790.0f);
            }
            FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateLambda([this](float)
                {
                    CaptureWorkbenchImage(TEXT("resonance-forge-keybed.png"));
                    ExportCurrentSample();
                    const FString BowLabelPath = FPaths::ChangeExtension(LastSampleExportPath, TEXT("rfrecipe.json"));
                    WaveguideExcitation = EResonanceExcitationType::Hammer;
                    ApplyWaveguideParameters();
                    SampleExportName = TEXT("RF_Hammer_G3");
                    ExportCurrentSample();
                    ReforgeSampleLabelFromPath(BowLabelPath);
                    SampleExportName = TEXT("RF_Bow_G3");
                    if (WorkbenchScrollBox.IsValid())
                    {
                        WorkbenchScrollBox->ScrollToEnd();
                    }
                    FTSTicker::GetCoreTicker().AddTicker(
                        FTickerDelegate::CreateLambda([this](float)
                        {
                            CaptureWorkbenchImage(TEXT("resonance-forge-workbench-details.png"));
                            ApplyModel(EResonanceModelType::ModalImpact);
                            ApplyPreset(TEXT("硬木"));
                            ApplyStrikePosition(0.34f, false);
                            SelectedModeIndex = FMath::Min(2, ActiveModes.Num() - 1);
                            if (ActiveModes.IsValidIndex(SelectedModeIndex))
                            {
                                ActiveModes[SelectedModeIndex].FrequencyHz *= 1.16f;
                                ActiveModes[SelectedModeIndex].Gain = 1.18f;
                                ActiveModes[SelectedModeIndex].DecaySeconds *= 1.42f;
                                ApplyModalModes(false, FText::GetEmpty());
                            }
                            AuditionCurrentSound(NSLOCTEXT("ResonanceForge", "CaptureOffsetStrike", "偏置落点试敲"));
                            ClearReference();
                            if (WorkbenchScrollBox.IsValid())
                            {
                                WorkbenchScrollBox->SetScrollOffset(920.0f);
                            }
                            FTSTicker::GetCoreTicker().AddTicker(
                                FTickerDelegate::CreateLambda([this](float)
                                {
                                    CaptureWorkbenchImage(TEXT("resonance-forge-mode-rack.png"));
                                    if (FParse::Param(FCommandLine::Get(), TEXT("ResonanceForgeCaptureAndExit")))
                                    {
                                        FPlatformMisc::RequestExit(false);
                                    }
                                    return false;
                                }),
                                1.0f);
                            return false;
                        }),
                        1.0f);
                    return false;
                }),
                1.0f);
            return false;
        }),
        3.0f);
}

FReply FResonanceForgeEditorModule::CaptureWorkbenchScreenshot()
{
    CaptureWorkbenchImage(TEXT("resonance-forge-workbench.png"));
    return FReply::Handled();
}

bool FResonanceForgeEditorModule::CaptureWorkbenchImage(const FString& FileName)
{
    const TSharedPtr<SWidget> Widget = WorkbenchWidget.Pin();
    if (!Widget.IsValid() || !FSlateApplication::IsInitialized())
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "CaptureUnavailable", "工作台截图失败 · 请先让插件面板保持可见");
        return false;
    }

    TArray<FColor> Pixels;
    FIntVector ImageSize;
    if (!FSlateApplication::Get().TakeScreenshot(Widget.ToSharedRef(), Pixels, ImageSize) || Pixels.IsEmpty())
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "CaptureFailed", "工作台截图失败 · 当前 Slate 窗口尚未完成绘制");
        return false;
    }

    TArray64<uint8> PngData;
    FImageUtils::PNGCompressImageArray(
        ImageSize.X,
        ImageSize.Y,
        TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()),
        PngData);
    const FString OutputPath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), TEXT("docs"), TEXT("images"), FileName));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true);
    const bool bSaved = FFileHelper::SaveArrayToFile(PngData, *OutputPath);
    LastStatus = bSaved
        ? FText::Format(
            NSLOCTEXT("ResonanceForge", "CaptureSaved", "工作台截图已导出 · docs/images/{0}"),
            FText::FromString(FileName))
        : NSLOCTEXT("ResonanceForge", "CaptureWriteFailed", "工作台截图生成成功，但 PNG 写入失败");
    return bSaved;
}

void FResonanceForgeEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Window"));
    FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("音频工具"));
    Section.AddMenuEntry(
        TEXT("OpenResonanceForge"),
        NSLOCTEXT("ResonanceForge", "MenuLabel", "共振铸造台 · 材质声源工作台"),
        NSLOCTEXT("ResonanceForge", "MenuTooltip", "打开物理材质、模态合成与 Wwise 联动控制台"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("LevelEditor.Tabs.Audio")),
        FUIAction(FExecuteAction::CreateLambda([]
        {
            FGlobalTabmanager::Get()->TryInvokeTab(ResonanceForgeEditor::TabName);
        })));
}

AResonanceForgeImpactInstrumentActor* FResonanceForgeEditorModule::ResolveInstrument() const
{
    if (!GEditor)
    {
        return nullptr;
    }

    if (GEditor->PlayWorld)
    {
        for (TActorIterator<AResonanceForgeImpactInstrumentActor> It(GEditor->PlayWorld.Get()); It; ++It)
        {
            return *It;
        }
    }

    if (USelection* Selection = GEditor->GetSelectedActors())
    {
        for (FSelectionIterator It(*Selection); It; ++It)
        {
            if (AResonanceForgeImpactInstrumentActor* Instrument = Cast<AResonanceForgeImpactInstrumentActor>(*It))
            {
                return Instrument;
            }
        }
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (World)
    {
        for (TActorIterator<AResonanceForgeImpactInstrumentActor> It(World); It; ++It)
        {
            return *It;
        }
    }
    return nullptr;
}

bool FResonanceForgeEditorModule::PollLiveImpact(float)
{
    UWorld* World = nullptr;
    if (GEditor)
    {
        World = GEditor->PlayWorld ? GEditor->PlayWorld.Get() : GEditor->GetEditorWorldContext().World();
    }
    AResonanceForgeImpactInstrumentActor* Instrument = nullptr;
    if (World)
    {
        for (TActorIterator<AResonanceForgeImpactInstrumentActor> It(World); It; ++It)
        {
            if (!Instrument || It->LastImpactWorldSeconds > Instrument->LastImpactWorldSeconds)
            {
                Instrument = *It;
            }
        }
    }
    if (!Instrument)
    {
        ObservedImpactActor.Reset();
        ObservedImpactSerial = INDEX_NONE;
        return true;
    }
    if (ObservedImpactActor.Get() != Instrument)
    {
        ObservedImpactActor = Instrument;
        ObservedImpactSerial = INDEX_NONE;
    }
    if (Instrument->ImpactSerial > 0 && Instrument->ImpactSerial != ObservedImpactSerial)
    {
        SetFlowStation(1);
        ObservedImpactSerial = Instrument->ImpactSerial;
        LiveImpactPosition = Instrument->LastStrikePosition;
        LiveImpactEnergy = Instrument->LastImpactEnergy;
        LiveImpactBrightness = Instrument->LastImpactBrightness;
        LiveImpactObservedSeconds = FPlatformTime::Seconds();
        PreviewStrikePosition = LiveImpactPosition;
        if (FPlatformTime::Seconds() - LastSampleReforgedSeconds > 2.0)
        {
            LastStatus = FText::Format(
                NSLOCTEXT("ResonanceForge", "LiveImpactReturned", "触发回传 · 落点 {0}% / 能量 {1}% / 明亮度 {2}%"),
                FText::AsNumber(FMath::RoundToInt(LiveImpactPosition * 100.0f)),
                FText::AsNumber(FMath::RoundToInt(LiveImpactEnergy * 100.0f)),
                FText::AsNumber(FMath::RoundToInt(LiveImpactBrightness * 100.0f)));
        }
    }
    return true;
}

void FResonanceForgeEditorModule::ApplyPreset(const FName PresetName)
{
    SetFlowStation(2);
    ActivePreset = PresetName;
    ActiveModes = UResonanceForgeSynthComponent::GetBuiltInModes(PresetName);
    SelectedModeIndex = FMath::Clamp(SelectedModeIndex, 0, FMath::Max(0, ActiveModes.Num() - 1));
    AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument();
    if (!Instrument)
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "NoInstrument", "未找到共振体 · 请先打开演示地图或选择一个共振体");
        return;
    }

    static const TMap<FName, FString> MaterialPaths = {
        {TEXT("拉丝钢"), TEXT("/Game/ResonanceForge/Demo/Materials/MI_RF_Steel.MI_RF_Steel")},
        {TEXT("硬木"), TEXT("/Game/ResonanceForge/Demo/Materials/MI_RF_Wood.MI_RF_Wood")},
        {TEXT("薄玻璃"), TEXT("/Game/ResonanceForge/Demo/Materials/MI_RF_Glass.MI_RF_Glass")}
    };

    Instrument->Modify();
    Instrument->ResonancePreset = PresetName;
    Instrument->NativeSynth->ApplyMaterialProfile(nullptr);
    if (const FString* MaterialPath = MaterialPaths.Find(PresetName))
    {
        if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, **MaterialPath))
        {
            Instrument->InstrumentMesh->SetMaterial(0, Material);
        }
    }
    Instrument->RerunConstructionScripts();
    Instrument->NativeSynth->SetCustomModes(ActiveModes);
    Instrument->MarkPackageDirty();
    LastStatus = FText::Format(NSLOCTEXT("ResonanceForge", "Applied", "已应用「{0}」· 视觉材质与共振预设同步"), FText::FromName(PresetName));
}

void FResonanceForgeEditorModule::ApplyModel(const EResonanceModelType ModelType)
{
    SetFlowStation(2);
    ActiveModel = ModelType;
    AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument();
    if (!Instrument)
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "NoInstrumentForModel", "还没有可演奏对象，请先打开试听场景");
        return;
    }

    Instrument->Modify();
    Instrument->SynthesisModel = ModelType;
    Instrument->NativeSynth->ApplyMaterialProfile(nullptr);
    Instrument->NativeSynth->SetSynthesisModel(ModelType);
    Instrument->NativeSynth->SetCustomModes(ActiveModes);
    ApplyWaveguideParameters();
    Instrument->MarkPackageDirty();
    LastStatus = ModelType == EResonanceModelType::WaveguideString
        ? NSLOCTEXT("ResonanceForge", "WaveguideSelected", "已切换到数字波导弦，使用 MIDI 或试听按钮演奏音高")
        : NSLOCTEXT("ResonanceForge", "ModalSelected", "已切换到模态撞击体，场景碰撞将激励材质共振");
}

void FResonanceForgeEditorModule::ApplyWaveguideParameters()
{
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        if (Instrument->NativeSynth->MaterialProfile)
        {
            Instrument->NativeSynth->ApplyMaterialProfile(nullptr);
            Instrument->NativeSynth->SetSynthesisModel(ActiveModel);
            Instrument->NativeSynth->SetCustomModes(ActiveModes);
        }
        Instrument->NativeSynth->StringDecay = FMath::Lerp(
            ResonanceForgeEditor::WaveguideDecayMin,
            ResonanceForgeEditor::WaveguideDecayMax,
            FMath::Clamp(WaveguideSustain, 0.0f, 1.0f));
        Instrument->NativeSynth->StringDamping = FMath::Clamp(WaveguideDamping, 0.0f, 1.0f);
        Instrument->NativeSynth->BodyCoupling = FMath::Clamp(WaveguideCoupling, 0.0f, 1.0f);
        Instrument->NativeSynth->PickupPosition = FMath::Clamp(WaveguidePickup, 0.0f, 1.0f);
        Instrument->NativeSynth->ExcitationType = WaveguideExcitation;
    }
}

void FResonanceForgeEditorModule::ApplyWaveguidePickup(const float NewPosition, const bool bFinished)
{
    SetFlowStation(2);
    WaveguidePickup = FMath::Clamp(NewPosition, 0.0f, 1.0f);
    ApplyWaveguideParameters();
    if (bFinished)
    {
        AuditionCurrentSound(NSLOCTEXT("ResonanceForge", "PickupPositionAudition", "拾音位置调整完成"));
    }
}

FReply FResonanceForgeEditorModule::SetWaveguideExcitation(const EResonanceExcitationType NewType)
{
    SetFlowStation(1);
    WaveguideExcitation = NewType;
    ApplyWaveguideParameters();
    AuditionCurrentSound(FText::Format(
        NSLOCTEXT("ResonanceForge", "ExcitationGestureAudition", "起振手势「{0}」"),
        GetWaveguideExcitationText()));
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::SetVelocityCurve(const EResonanceVelocityCurve NewCurve)
{
    SetFlowStation(1);
    VelocityCurve = NewCurve;
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->Modify();
        Instrument->VelocityCurve = VelocityCurve;
        Instrument->MarkPackageDirty();
    }
    TriggerKeybedNote(LastKeybedNote, LastKeybedVelocity > 0.0f ? LastKeybedVelocity : 0.65f);
    return FReply::Handled();
}

void FResonanceForgeEditorModule::ApplyModalModes(const bool bAudition, const FText& ChangeLabel)
{
    SetFlowStation(2);
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->Modify();
        Instrument->NativeSynth->SetCustomModes(ActiveModes);
        Instrument->MarkPackageDirty();
        if (bAudition)
        {
            AuditionCurrentSound(ChangeLabel);
        }
    }
}

void FResonanceForgeEditorModule::ResetModalModesToPreset()
{
    ActiveModes = UResonanceForgeSynthComponent::GetBuiltInModes(ActivePreset);
    SelectedModeIndex = 0;
    ApplyModalModes(true, NSLOCTEXT("ResonanceForge", "ModesResetAudition", "共振齿列已归炉"));
}

void FResonanceForgeEditorModule::SelectModalMode(const int32 ModeIndex)
{
    if (ActiveModes.IsValidIndex(ModeIndex))
    {
        SetFlowStation(2);
        SelectedModeIndex = ModeIndex;
        LastStatus = FText::Format(
            NSLOCTEXT("ResonanceForge", "ModeSelected", "已夹住第 {0} 根共振齿 · 调整频率、重量或余响后松手试听"),
            FText::AsNumber(ModeIndex + 1));
    }
}

void FResonanceForgeEditorModule::ApplyStrikePosition(const float NewPosition, const bool bFinished)
{
    SetFlowStation(1);
    PreviewStrikePosition = FMath::Clamp(NewPosition, 0.0f, 1.0f);
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->Modify();
        Instrument->ManualStrikePosition = PreviewStrikePosition;
        Instrument->LastStrikePosition = PreviewStrikePosition;
        Instrument->MarkPackageDirty();
    }
    if (bFinished)
    {
        AuditionCurrentSound(ActiveModel == EResonanceModelType::WaveguideString
            ? NSLOCTEXT("ResonanceForge", "StringStrikePositionAudition", "弦上起振位置调整完成")
            : NSLOCTEXT("ResonanceForge", "StrikePositionAudition", "敲击落点调整完成"));
    }
}

void FResonanceForgeEditorModule::TriggerKeybedNote(const int32 MidiNote, const float Velocity)
{
    SetFlowStation(1);
    const int32 SafeNote = FMath::Clamp(MidiNote, 0, 127);
    const float SafeVelocity = FMath::Clamp(Velocity, 0.0f, 1.0f);
    LastKeybedNote = SafeNote;
    LastKeybedVelocity = SafeVelocity;
    LastKeybedPlayedSeconds = FPlatformTime::Seconds();
    const float ShapedVelocity = AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(SafeVelocity, VelocityCurve);
    PreviewEnergy = ShapedVelocity;
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->ObjectSize = PreviewSize;
        Instrument->VelocityCurve = VelocityCurve;
        ApplyWaveguideParameters();
        Instrument->ListenMode = ListenMode;
        Instrument->TriggerInstrument(ShapedVelocity, PreviewBrightness, SafeNote, PreviewStrikePosition);
        LastStatus = FText::Format(
            NSLOCTEXT("ResonanceForge", "KeybedPlayed", "试音键床 · Note {0} / 输入 {1}% → 能量 {2}% · {3}"),
            FText::AsNumber(SafeNote),
            FText::AsNumber(FMath::RoundToInt(SafeVelocity * 100.0f)),
            FText::AsNumber(FMath::RoundToInt(ShapedVelocity * 100.0f)),
            GetVelocityCurveText());
    }
    else
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "KeybedNoInstrument", "试音键床等待共振体 · 请先打开试听场景");
    }
}

void FResonanceForgeEditorModule::AuditionCurrentSound(const FText& ChangeLabel)
{
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->ObjectSize = PreviewSize;
        ApplyWaveguideParameters();
        Instrument->ListenMode = ListenMode;
        Instrument->TriggerInstrument(PreviewEnergy, PreviewBrightness, 60, PreviewStrikePosition);
        LastStatus = FText::Format(
            NSLOCTEXT("ResonanceForge", "AuditionedChange", "{0} · 已试听  能量 {1}% / 明亮度 {2}% / 尺度 {3}% · {4}"),
            ChangeLabel,
            FText::AsNumber(FMath::RoundToInt(PreviewEnergy * 100.0f)),
            FText::AsNumber(FMath::RoundToInt(PreviewBrightness * 100.0f)),
            FText::AsNumber(FMath::RoundToInt(PreviewSize * 100.0f)),
            GetListenModeText());
    }
    else
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "AuditionMissing", "无法试听 · 请先打开试听场景并选择一个共振体");
    }
}

FReply FResonanceForgeEditorModule::SyncFromSelection()
{
    if (const AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        SetFlowStation(0);
        const UResonanceMaterialProfile* SharedProfile = Instrument->NativeSynth ? Instrument->NativeSynth->MaterialProfile : nullptr;
        ActiveModel = SharedProfile ? SharedProfile->ModelType : Instrument->SynthesisModel;
        ActivePreset = SharedProfile ? SharedProfile->SourcePreset : Instrument->ResonancePreset;
        PreviewSize = Instrument->ObjectSize;
        VelocityCurve = Instrument->VelocityCurve;
        ListenMode = Instrument->ListenMode;
        PreviewStrikePosition = Instrument->LastStrikePosition;
        if (Instrument->NativeSynth)
        {
            WaveguideSustain = FMath::GetRangePct(
                ResonanceForgeEditor::WaveguideDecayMin,
                ResonanceForgeEditor::WaveguideDecayMax,
                Instrument->NativeSynth->StringDecay);
            WaveguideDamping = Instrument->NativeSynth->StringDamping;
            WaveguideCoupling = Instrument->NativeSynth->BodyCoupling;
            WaveguidePickup = Instrument->NativeSynth->PickupPosition;
            WaveguideExcitation = Instrument->NativeSynth->ExcitationType;
            ActiveModes = Instrument->NativeSynth->GetEffectiveModes();
            SelectedModeIndex = FMath::Clamp(SelectedModeIndex, 0, FMath::Max(0, ActiveModes.Num() - 1));
        }
        LastStatus = FText::Format(
            NSLOCTEXT("ResonanceForge", "SelectionSynced", "已读取「{0}」· 现在可以调整模型并试听"),
            FText::FromString(Instrument->GetActorLabel()));
        if (Instrument->WwiseBridge)
        {
            Instrument->WwiseBridge->AutoBindDemoAssets();
        }
    }
    else
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "SelectionSyncEmpty", "当前选择不是共振体 · 请在场景中选择带有 Resonance Forge 的对象");
    }
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::TriggerPreview()
{
    SetFlowStation(1);
    AuditionCurrentSound(NSLOCTEXT("ResonanceForge", "CurrentVoice", "当前声纹"));
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::SetListenMode(const EResonanceForgeListenMode NewMode)
{
    SetFlowStation(3);
    ListenMode = NewMode;
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->Modify();
        Instrument->ListenMode = ListenMode;
        Instrument->MarkPackageDirty();
        AuditionCurrentSound(NSLOCTEXT("ResonanceForge", "ListenGateAudition", "监听闸门已切换"));
    }
    else
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "ListenGateMissing", "监听闸门等待共振体 · 请先打开试听场景");
    }
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::TriggerStrikePreset(
    const float Energy,
    const float Brightness,
    const FText& GestureName)
{
    SetFlowStation(1);
    PreviewEnergy = FMath::Clamp(Energy, 0.0f, 1.0f);
    PreviewBrightness = FMath::Clamp(Brightness, 0.0f, 1.0f);

    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->ObjectSize = PreviewSize;
        ApplyWaveguideParameters();
        Instrument->ListenMode = ListenMode;
        Instrument->TriggerInstrument(PreviewEnergy, PreviewBrightness, 60, PreviewStrikePosition);
        LastStatus = FText::Format(
            NSLOCTEXT("ResonanceForge", "StrikePresetTriggered", "{0}已触发 · {1} / {2} / {3} · {4}"),
            GestureName,
            GetWwiseVolumeText(),
            GetWwiseLowpassText(),
            GetWwisePitchText(),
            GetListenModeText());
    }
    else
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "StrikePresetMissing", "无法试听锤击标尺 · 请先打开试听场景");
    }
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::PinReference()
{
    bHasReference = true;
    ReferencePreset = ActivePreset;
    ReferenceModel = ActiveModel;
    ReferenceEnergy = PreviewEnergy;
    ReferenceBrightness = PreviewBrightness;
    ReferenceSize = PreviewSize;
    ReferenceStrikePosition = PreviewStrikePosition;
    ReferenceSustain = WaveguideSustain;
    ReferenceDamping = WaveguideDamping;
    ReferenceCoupling = WaveguideCoupling;
    ReferencePickup = WaveguidePickup;
    ReferenceExcitation = WaveguideExcitation;
    ReferenceModes = ActiveModes;
    LastStatus = NSLOCTEXT("ResonanceForge", "ReferencePinned", "参考声纹已钉住 · 继续换材质或调整参数，紫色轮廓会保留用于比较");
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::SwapAndPreviewReference()
{
    if (!bHasReference)
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "ReferenceSwapMissing", "还没有参考声纹 · 先钉住当前版本");
        return FReply::Handled();
    }

    const FName PreviousPreset = ActivePreset;
    const EResonanceModelType PreviousModel = ActiveModel;
    const float PreviousEnergy = PreviewEnergy;
    const float PreviousBrightness = PreviewBrightness;
    const float PreviousSize = PreviewSize;
    const float PreviousStrikePosition = PreviewStrikePosition;
    const float PreviousSustain = WaveguideSustain;
    const float PreviousDamping = WaveguideDamping;
    const float PreviousCoupling = WaveguideCoupling;
    const float PreviousPickup = WaveguidePickup;
    const EResonanceExcitationType PreviousExcitation = WaveguideExcitation;
    const TArray<FResonanceMode> PreviousModes = ActiveModes;

    ActivePreset = ReferencePreset;
    ActiveModel = ReferenceModel;
    PreviewEnergy = ReferenceEnergy;
    PreviewBrightness = ReferenceBrightness;
    PreviewSize = ReferenceSize;
    PreviewStrikePosition = ReferenceStrikePosition;
    WaveguideSustain = ReferenceSustain;
    WaveguideDamping = ReferenceDamping;
    WaveguideCoupling = ReferenceCoupling;
    WaveguidePickup = ReferencePickup;
    WaveguideExcitation = ReferenceExcitation;
    ActiveModes = ReferenceModes;

    ReferencePreset = PreviousPreset;
    ReferenceModel = PreviousModel;
    ReferenceEnergy = PreviousEnergy;
    ReferenceBrightness = PreviousBrightness;
    ReferenceSize = PreviousSize;
    ReferenceStrikePosition = PreviousStrikePosition;
    ReferenceSustain = PreviousSustain;
    ReferenceDamping = PreviousDamping;
    ReferenceCoupling = PreviousCoupling;
    ReferencePickup = PreviousPickup;
    ReferenceExcitation = PreviousExcitation;
    ReferenceModes = PreviousModes;

    const TArray<FResonanceMode> DesiredModes = ActiveModes;
    ApplyModel(ActiveModel);
    ApplyPreset(ActivePreset);
    ActiveModes = DesiredModes;
    ApplyModalModes(false, FText::GetEmpty());
    ApplyStrikePosition(PreviewStrikePosition, false);
    TriggerPreview();
    LastStatus = FText::Format(
        NSLOCTEXT("ResonanceForge", "ReferenceSwapped", "正在试听「{0}」· 再按一次即可切回另一版"),
        FText::FromName(ActivePreset));
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::ClearReference()
{
    bHasReference = false;
    LastStatus = NSLOCTEXT("ResonanceForge", "ReferenceCleared", "参考声纹已清除");
    return FReply::Handled();
}

bool FResonanceForgeEditorModule::ReadRecipeSlot(
    const int32 SlotIndex,
    FName& OutPreset,
    EResonanceModelType& OutModel,
    float& OutEnergy,
    float& OutBrightness,
    float& OutSize,
    float& OutSustain,
    float& OutDamping,
    float& OutCoupling,
    float& OutPickup,
    EResonanceExcitationType& OutExcitation,
    EResonanceVelocityCurve& OutVelocityCurve) const
{
    if (!GConfig || SlotIndex < 0 || SlotIndex > 2)
    {
        return false;
    }

    const FString Section(TEXT("ResonanceForge.UserRecipes"));
    const FString Prefix = FString::Printf(TEXT("Slot%d"), SlotIndex + 1);
    bool bValid = false;
    if (!GConfig->GetBool(*Section, *(Prefix + TEXT(".Valid")), bValid, GEditorPerProjectIni) || !bValid)
    {
        return false;
    }

    FString PresetString;
    int32 ModelValue = 0;
    int32 ExcitationValue = static_cast<int32>(EResonanceExcitationType::Pick);
    int32 VelocityCurveValue = static_cast<int32>(EResonanceVelocityCurve::Linear);
    GConfig->GetString(*Section, *(Prefix + TEXT(".Preset")), PresetString, GEditorPerProjectIni);
    GConfig->GetInt(*Section, *(Prefix + TEXT(".Model")), ModelValue, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Energy")), OutEnergy, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Brightness")), OutBrightness, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Size")), OutSize, GEditorPerProjectIni);
    OutSustain = 0.90f;
    OutDamping = 0.36f;
    OutCoupling = 0.22f;
    OutPickup = 0.35f;
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Sustain")), OutSustain, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Damping")), OutDamping, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Coupling")), OutCoupling, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Pickup")), OutPickup, GEditorPerProjectIni);
    GConfig->GetInt(*Section, *(Prefix + TEXT(".Excitation")), ExcitationValue, GEditorPerProjectIni);
    GConfig->GetInt(*Section, *(Prefix + TEXT(".VelocityCurve")), VelocityCurveValue, GEditorPerProjectIni);
    OutPreset = PresetString.IsEmpty() ? FName(TEXT("拉丝钢")) : FName(*PresetString);
    OutModel = ModelValue == static_cast<int32>(EResonanceModelType::WaveguideString)
        ? EResonanceModelType::WaveguideString
        : EResonanceModelType::ModalImpact;
    OutEnergy = FMath::Clamp(OutEnergy, 0.0f, 1.0f);
    OutBrightness = FMath::Clamp(OutBrightness, 0.0f, 1.0f);
    OutSize = FMath::Clamp(OutSize, 0.0f, 1.0f);
    OutSustain = FMath::Clamp(OutSustain, 0.0f, 1.0f);
    OutDamping = FMath::Clamp(OutDamping, 0.0f, 1.0f);
    OutCoupling = FMath::Clamp(OutCoupling, 0.0f, 1.0f);
    OutPickup = FMath::Clamp(OutPickup, 0.0f, 1.0f);
    OutExcitation = ExcitationValue == static_cast<int32>(EResonanceExcitationType::Finger)
        ? EResonanceExcitationType::Finger
        : ExcitationValue == static_cast<int32>(EResonanceExcitationType::Hammer)
            ? EResonanceExcitationType::Hammer
            : ExcitationValue == static_cast<int32>(EResonanceExcitationType::Bow)
                ? EResonanceExcitationType::Bow
                : EResonanceExcitationType::Pick;
    OutVelocityCurve = VelocityCurveValue == static_cast<int32>(EResonanceVelocityCurve::SoftTouch)
        ? EResonanceVelocityCurve::SoftTouch
        : VelocityCurveValue == static_cast<int32>(EResonanceVelocityCurve::HeavyHand)
            ? EResonanceVelocityCurve::HeavyHand
            : EResonanceVelocityCurve::Linear;
    return true;
}

bool FResonanceForgeEditorModule::HasRecipeSlot(const int32 SlotIndex) const
{
    FName Preset;
    EResonanceModelType Model = EResonanceModelType::ModalImpact;
    float Energy = 0.0f;
    float Brightness = 0.0f;
    float Size = 0.0f;
    float Sustain = 0.0f;
    float Damping = 0.0f;
    float Coupling = 0.0f;
    float Pickup = 0.0f;
    EResonanceExcitationType Excitation = EResonanceExcitationType::Pick;
    EResonanceVelocityCurve SavedVelocityCurve = EResonanceVelocityCurve::Linear;
    return ReadRecipeSlot(SlotIndex, Preset, Model, Energy, Brightness, Size, Sustain, Damping, Coupling, Pickup, Excitation, SavedVelocityCurve);
}

FText FResonanceForgeEditorModule::GetRecipeSlotText(const int32 SlotIndex) const
{
    static const TCHAR* SlotNames[] = {TEXT("甲槽"), TEXT("乙槽"), TEXT("丙槽")};
    FName Preset;
    EResonanceModelType Model = EResonanceModelType::ModalImpact;
    float Energy = 0.0f;
    float Brightness = 0.0f;
    float Size = 0.0f;
    float Sustain = 0.0f;
    float Damping = 0.0f;
    float Coupling = 0.0f;
    float Pickup = 0.0f;
    EResonanceExcitationType Excitation = EResonanceExcitationType::Pick;
    EResonanceVelocityCurve SavedVelocityCurve = EResonanceVelocityCurve::Linear;
    if (!ReadRecipeSlot(SlotIndex, Preset, Model, Energy, Brightness, Size, Sustain, Damping, Coupling, Pickup, Excitation, SavedVelocityCurve))
    {
        return FText::Format(NSLOCTEXT("ResonanceForge", "EmptyRecipeSlot", "{0} · 空"), FText::FromString(SlotNames[SlotIndex]));
    }

    const FText ModelText = Model == EResonanceModelType::WaveguideString
        ? NSLOCTEXT("ResonanceForge", "RecipeWaveguide", "波导弦")
        : NSLOCTEXT("ResonanceForge", "RecipeModal", "撞击体");
    return FText::Format(
        NSLOCTEXT("ResonanceForge", "FilledRecipeSlot", "{0} · {1} / {2}"),
        FText::FromString(SlotNames[SlotIndex]), FText::FromName(Preset), ModelText);
}

FReply FResonanceForgeEditorModule::SaveRecipeSlot(const int32 SlotIndex)
{
    if (!GConfig || SlotIndex < 0 || SlotIndex > 2)
    {
        return FReply::Handled();
    }

    const FString Section(TEXT("ResonanceForge.UserRecipes"));
    const FString Prefix = FString::Printf(TEXT("Slot%d"), SlotIndex + 1);
    GConfig->SetBool(*Section, *(Prefix + TEXT(".Valid")), true, GEditorPerProjectIni);
    GConfig->SetString(*Section, *(Prefix + TEXT(".Preset")), *ActivePreset.ToString(), GEditorPerProjectIni);
    GConfig->SetInt(*Section, *(Prefix + TEXT(".Model")), static_cast<int32>(ActiveModel), GEditorPerProjectIni);
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".Energy")), PreviewEnergy, GEditorPerProjectIni);
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".Brightness")), PreviewBrightness, GEditorPerProjectIni);
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".Size")), PreviewSize, GEditorPerProjectIni);
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".StrikePosition")), PreviewStrikePosition, GEditorPerProjectIni);
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".Sustain")), WaveguideSustain, GEditorPerProjectIni);
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".Damping")), WaveguideDamping, GEditorPerProjectIni);
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".Coupling")), WaveguideCoupling, GEditorPerProjectIni);
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".Pickup")), WaveguidePickup, GEditorPerProjectIni);
    GConfig->SetInt(*Section, *(Prefix + TEXT(".Excitation")), static_cast<int32>(WaveguideExcitation), GEditorPerProjectIni);
    GConfig->SetInt(*Section, *(Prefix + TEXT(".VelocityCurve")), static_cast<int32>(VelocityCurve), GEditorPerProjectIni);
    GConfig->Flush(false, GEditorPerProjectIni);
    LastStatus = FText::Format(
        NSLOCTEXT("ResonanceForge", "RecipeSaved", "已把当前声音存入{0} · 仅保存在本机工程设置中"),
        FText::FromString(SlotIndex == 0 ? TEXT("甲槽") : (SlotIndex == 1 ? TEXT("乙槽") : TEXT("丙槽"))));
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::RecallRecipeSlot(const int32 SlotIndex)
{
    FName Preset;
    EResonanceModelType Model = EResonanceModelType::ModalImpact;
    float Energy = 0.0f;
    float Brightness = 0.0f;
    float Size = 0.0f;
    float Sustain = 0.0f;
    float Damping = 0.0f;
    float Coupling = 0.0f;
    float Pickup = 0.0f;
    EResonanceExcitationType Excitation = EResonanceExcitationType::Pick;
    EResonanceVelocityCurve SavedVelocityCurve = EResonanceVelocityCurve::Linear;
    if (!ReadRecipeSlot(SlotIndex, Preset, Model, Energy, Brightness, Size, Sustain, Damping, Coupling, Pickup, Excitation, SavedVelocityCurve))
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "RecipeSlotEmpty", "这个配方槽还是空的 · 先把当前声音存进去");
        return FReply::Handled();
    }

    ActiveModel = Model;
    ActivePreset = Preset;
    PreviewEnergy = Energy;
    PreviewBrightness = Brightness;
    PreviewSize = Size;
    WaveguideSustain = Sustain;
    WaveguideDamping = Damping;
    WaveguideCoupling = Coupling;
    WaveguidePickup = Pickup;
    WaveguideExcitation = Excitation;
    VelocityCurve = SavedVelocityCurve;
    const FString RecipeSection(TEXT("ResonanceForge.UserRecipes"));
    const FString RecipePrefix = FString::Printf(TEXT("Slot%d"), SlotIndex + 1);
    PreviewStrikePosition = 0.5f;
    GConfig->GetFloat(*RecipeSection, *(RecipePrefix + TEXT(".StrikePosition")), PreviewStrikePosition, GEditorPerProjectIni);
    PreviewStrikePosition = FMath::Clamp(PreviewStrikePosition, 0.0f, 1.0f);
    ApplyModel(ActiveModel);
    ApplyPreset(ActivePreset);
    ApplyWaveguideParameters();
    ApplyStrikePosition(PreviewStrikePosition, false);
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->ObjectSize = PreviewSize;
        Instrument->VelocityCurve = VelocityCurve;
        Instrument->MarkPackageDirty();
    }
    LastStatus = FText::Format(
        NSLOCTEXT("ResonanceForge", "RecipeRecalled", "已召回{0} · 模型、材质与演奏参数已同步到当前对象"),
        FText::FromString(SlotIndex == 0 ? TEXT("甲槽") : (SlotIndex == 1 ? TEXT("乙槽") : TEXT("丙槽"))));
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::RefreshMidiDevices()
{
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->DisconnectMidiInput();
    }

    TArray<FMIDIDeviceInfo> InputDevices;
    TArray<FMIDIDeviceInfo> OutputDevices;
    UMIDIDeviceManager::FindAllMIDIDeviceInfo(InputDevices, OutputDevices);
    MidiDeviceOptions.Reset();
    MidiDeviceIds.Reset();
    SelectedMidiDevice.Reset();

    int32 PreferredIndex = INDEX_NONE;
    for (const FMIDIDeviceInfo& Device : InputDevices)
    {
        FString Label = FString::Printf(TEXT("%s · ID %d"), *Device.DeviceName, Device.DeviceID);
        if (Device.bIsAlreadyInUse)
        {
            Label += TEXT(" · 正在占用");
        }
        MidiDeviceOptions.Add(MakeShared<FString>(MoveTemp(Label)));
        MidiDeviceIds.Add(Device.DeviceID);
        if (PreferredIndex == INDEX_NONE && (Device.bIsDefaultDevice || !Device.bIsAlreadyInUse))
        {
            PreferredIndex = MidiDeviceOptions.Num() - 1;
        }
    }

    if (PreferredIndex == INDEX_NONE && !MidiDeviceOptions.IsEmpty())
    {
        PreferredIndex = 0;
    }
    if (PreferredIndex != INDEX_NONE)
    {
        SelectedMidiDevice = MidiDeviceOptions[PreferredIndex];
    }
    if (MidiDeviceCombo.IsValid())
    {
        MidiDeviceCombo->RefreshOptions();
        MidiDeviceCombo->SetSelectedItem(SelectedMidiDevice);
    }

    LastStatus = MidiDeviceOptions.IsEmpty()
        ? NSLOCTEXT("ResonanceForge", "NoMidiDevices", "没有发现 MIDI 输入设备 · 连接设备后点击刷新")
        : FText::Format(NSLOCTEXT("ResonanceForge", "MidiDevicesFound", "发现 {0} 个 MIDI 输入设备 · 已选择可用设备"), FText::AsNumber(MidiDeviceOptions.Num()));
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::ConnectSelectedMidiDevice()
{
    const int32 OptionIndex = MidiDeviceOptions.IndexOfByKey(SelectedMidiDevice);
    AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument();
    if (!Instrument)
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "MidiNoInstrument", "无法连接 MIDI · 请先打开试听场景并选择一个共振体");
        return FReply::Handled();
    }
    if (!MidiDeviceIds.IsValidIndex(OptionIndex))
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "MidiNoSelection", "还没有可连接的 MIDI 输入设备");
        return FReply::Handled();
    }

    const bool bConnected = Instrument->ConnectMidiInput(MidiDeviceIds[OptionIndex]);
    LastStatus = bConnected
        ? NSLOCTEXT("ResonanceForge", "MidiConnected", "MIDI 已连接 · Note On 起音，弓擦松键收弓，CC1 实时推拉弓压")
        : NSLOCTEXT("ResonanceForge", "MidiConnectFailed", "MIDI 连接失败 · 设备可能正被其他程序占用");
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::DisconnectMidiDevice()
{
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->DisconnectMidiInput();
    }
    LastStatus = NSLOCTEXT("ResonanceForge", "MidiDisconnected", "MIDI 已断开");
    return FReply::Handled();
}

FText FResonanceForgeEditorModule::GetMidiStatusText() const
{
    const AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument();
    if (!Instrument || !Instrument->IsMidiConnected())
    {
        return MidiDeviceOptions.IsEmpty()
            ? NSLOCTEXT("ResonanceForge", "MidiWaiting", "等待 MIDI 输入设备")
            : NSLOCTEXT("ResonanceForge", "MidiReadyToConnect", "设备已发现 · 连接后按键即可演奏");
    }

    if (Instrument->LastMidiNote >= 0)
    {
        const int32 ShapedEnergy = FMath::RoundToInt(
            AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(
                Instrument->LastMidiVelocity / 127.0f,
                Instrument->VelocityCurve) * 127.0f);
        return FText::Format(
            NSLOCTEXT("ResonanceForge", "MidiLiveActivity", "{0} · Note {1} / Velocity {2} → Energy {3} · 弓压 CC1 {4}"),
            FText::FromString(Instrument->GetConnectedMidiDeviceName()),
            FText::AsNumber(Instrument->LastMidiNote),
            FText::AsNumber(Instrument->LastMidiVelocity),
            FText::AsNumber(ShapedEnergy),
            FText::AsNumber(Instrument->LastMidiControlValue));
    }
    return FText::Format(
        NSLOCTEXT("ResonanceForge", "MidiConnectedWaiting", "{0} · 已连接，等待演奏"),
        FText::FromString(Instrument->GetConnectedMidiDeviceName()));
}

FText FResonanceForgeEditorModule::GetWwiseStatusText() const
{
    const AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument();
    if (!Instrument || !Instrument->WwiseBridge)
    {
        return NSLOCTEXT("ResonanceForge", "WwiseWaitingForObject", "Wwise 等待场景对象");
    }

    const FString Status = Instrument->WwiseBridge->GetIntegrationStatus();
    return Status == TEXT("Wwise 桥接已就绪")
        ? NSLOCTEXT("ResonanceForge", "WwiseReady", "Wwise 已就绪 · 3 材质 Event / 3 RTPC")
        : FText::FromString(Status);
}

FText FResonanceForgeEditorModule::GetWwiseVolumeText() const
{
    const float Energy = FMath::Clamp(PreviewEnergy, 0.0f, 1.0f);
    float VolumeDb = 0.0f;
    if (Energy <= 0.20f)
    {
        const float T = Energy / 0.20f;
        VolumeDb = FMath::Lerp(-24.0f, -12.0f, T * T);
    }
    else if (Energy <= 0.55f)
    {
        VolumeDb = FMath::Lerp(-12.0f, -4.0f, ResonanceForgeEditor::SmoothCurve((Energy - 0.20f) / 0.35f));
    }
    else
    {
        VolumeDb = FMath::Lerp(-4.0f, 0.0f, FMath::Sqrt((Energy - 0.55f) / 0.45f));
    }
    return FText::FromString(FString::Printf(TEXT("响度约 %.1f dB"), VolumeDb));
}

FText FResonanceForgeEditorModule::GetWwiseLowpassText() const
{
    const float Brightness = FMath::Clamp(PreviewBrightness, 0.0f, 1.0f);
    float Lowpass = 0.0f;
    if (Brightness <= 0.45f)
    {
        const float T = Brightness / 0.45f;
        Lowpass = FMath::Lerp(82.0f, 34.0f, FMath::Pow(T, 0.25f));
    }
    else
    {
        Lowpass = FMath::Lerp(34.0f, 0.0f, ResonanceForgeEditor::SmoothCurve((Brightness - 0.45f) / 0.55f));
    }
    return FText::FromString(FString::Printf(TEXT("低通约 %.0f / 100"), Lowpass));
}

FText FResonanceForgeEditorModule::GetWwisePitchText() const
{
    const float Size = FMath::Clamp(PreviewSize, 0.0f, 1.0f);
    const float PitchCent = Size <= 0.50f
        ? FMath::Lerp(420.0f, 0.0f, ResonanceForgeEditor::SmoothCurve(Size / 0.50f))
        : FMath::Lerp(0.0f, -520.0f, ResonanceForgeEditor::SmoothCurve((Size - 0.50f) / 0.50f));
    return FText::FromString(FString::Printf(TEXT("移调约 %+.0f cent"), PitchCent));
}

FText FResonanceForgeEditorModule::GetListenModeText() const
{
    switch (ListenMode)
    {
    case EResonanceForgeListenMode::NativeOnly:
        return NSLOCTEXT("ResonanceForge", "ListenNativeText", "原声炉");
    case EResonanceForgeListenMode::WwiseOnly:
        return NSLOCTEXT("ResonanceForge", "ListenWwiseText", "Wwise 出口");
    default:
        return NSLOCTEXT("ResonanceForge", "ListenLayeredText", "双路叠听");
    }
}

FText FResonanceForgeEditorModule::GetWaveguideExcitationText() const
{
    switch (WaveguideExcitation)
    {
    case EResonanceExcitationType::Finger:
        return NSLOCTEXT("ResonanceForge", "ExcitationFingerText", "指腹");
    case EResonanceExcitationType::Hammer:
        return NSLOCTEXT("ResonanceForge", "ExcitationHammerText", "锤击");
    case EResonanceExcitationType::Bow:
        return NSLOCTEXT("ResonanceForge", "ExcitationBowText", "弓擦");
    default:
        return NSLOCTEXT("ResonanceForge", "ExcitationPickText", "拨片");
    }
}

FText FResonanceForgeEditorModule::GetVelocityCurveText() const
{
    switch (VelocityCurve)
    {
    case EResonanceVelocityCurve::SoftTouch:
        return NSLOCTEXT("ResonanceForge", "VelocitySoftText", "软触");
    case EResonanceVelocityCurve::HeavyHand:
        return NSLOCTEXT("ResonanceForge", "VelocityHeavyText", "重手");
    default:
        return NSLOCTEXT("ResonanceForge", "VelocityLinearText", "线性");
    }
}

FText FResonanceForgeEditorModule::GetVelocityMappingText() const
{
    const float Input = FMath::Clamp(LastKeybedVelocity, 0.0f, 1.0f);
    const float Output = AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(Input, VelocityCurve);
    return FText::Format(
        NSLOCTEXT("ResonanceForge", "VelocityMappingReading", "输入 {0}%  →  输出 {1}%"),
        FText::AsNumber(FMath::RoundToInt(Input * 100.0f)),
        FText::AsNumber(FMath::RoundToInt(Output * 100.0f)));
}

FText FResonanceForgeEditorModule::GetSelectedModeFrequencyText() const
{
    return ActiveModes.IsValidIndex(SelectedModeIndex)
        ? FText::FromString(FString::Printf(TEXT("%.0f Hz"), ActiveModes[SelectedModeIndex].FrequencyHz))
        : FText::FromString(TEXT("—"));
}

FText FResonanceForgeEditorModule::GetSelectedModeGainText() const
{
    return ActiveModes.IsValidIndex(SelectedModeIndex)
        ? FText::FromString(FString::Printf(TEXT("%.2f"), ActiveModes[SelectedModeIndex].Gain))
        : FText::FromString(TEXT("—"));
}

FText FResonanceForgeEditorModule::GetSelectedModeDecayText() const
{
    return ActiveModes.IsValidIndex(SelectedModeIndex)
        ? FText::FromString(FString::Printf(TEXT("%.2f 秒"), ActiveModes[SelectedModeIndex].DecaySeconds))
        : FText::FromString(TEXT("—"));
}

FText FResonanceForgeEditorModule::GetStrikePositionText() const
{
    const int32 Percent = FMath::RoundToInt(FMath::Clamp(PreviewStrikePosition, 0.0f, 1.0f) * 100.0f);
    const FText Region = Percent < 38
        ? NSLOCTEXT("ResonanceForge", "StrikeNear", "近端")
        : Percent > 62
            ? NSLOCTEXT("ResonanceForge", "StrikeFar", "远端")
            : NSLOCTEXT("ResonanceForge", "StrikeCenter", "中央");
    return GetLiveImpactGlow() > 0.02f
        ? FText::Format(NSLOCTEXT("ResonanceForge", "StrikePositionReturned", "触发回传 · {0}%"), FText::AsNumber(FMath::RoundToInt(LiveImpactPosition * 100.0f)))
        : FText::Format(NSLOCTEXT("ResonanceForge", "StrikePositionReading", "{0} · {1}%"), Region, FText::AsNumber(Percent));
}

float FResonanceForgeEditorModule::GetLiveImpactGlow() const
{
    const double Age = FPlatformTime::Seconds() - LiveImpactObservedSeconds;
    if (Age < 0.0 || Age > 5.0)
    {
        return 0.0f;
    }
    return FMath::Clamp(LiveImpactEnergy * FMath::Exp(-static_cast<float>(Age) / 1.45f), 0.0f, 1.0f);
}

float FResonanceForgeEditorModule::GetKeybedGlow() const
{
    const double Age = FPlatformTime::Seconds() - LastKeybedPlayedSeconds;
    if (Age < 0.0 || Age > 6.0)
    {
        return 0.0f;
    }
    return FMath::Clamp(LastKeybedVelocity * FMath::Exp(-static_cast<float>(Age) / 2.2f), 0.0f, 1.0f);
}

FText FResonanceForgeEditorModule::GetKeybedStatusText() const
{
    return LastKeybedVelocity > 0.0f
        ? FText::Format(
            NSLOCTEXT("ResonanceForge", "KeybedLastNote", "最近试音 · Note {0} / 输入 {1}% → 能量 {2}% / 明亮度 {3}%"),
            FText::AsNumber(LastKeybedNote),
            FText::AsNumber(FMath::RoundToInt(LastKeybedVelocity * 100.0f)),
            FText::AsNumber(FMath::RoundToInt(PreviewEnergy * 100.0f)),
            FText::AsNumber(FMath::RoundToInt(PreviewBrightness * 100.0f)))
        : NSLOCTEXT("ResonanceForge", "KeybedReady", "无需 MIDI 设备 · 点击或横向拖过锤键即可演奏");
}

FReply FResonanceForgeEditorModule::ExportCurrentSample()
{
    FString SafeName = ObjectTools::SanitizeObjectName(SampleExportName.TrimStartAndEnd());
    if (SafeName.IsEmpty())
    {
        SafeName = TEXT("RF_ForgedSample");
    }

    const FString ExportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ResonanceForge"), TEXT("Exports"));
    IFileManager::Get().MakeDirectory(*ExportDirectory, true);
    FString ExportPath = FPaths::Combine(ExportDirectory, SafeName + TEXT(".wav"));
    for (int32 Suffix = 2; FPaths::FileExists(ExportPath); ++Suffix)
    {
        ExportPath = FPaths::Combine(ExportDirectory, FString::Printf(TEXT("%s_%02d.wav"), *SafeName, Suffix));
    }

    TArray<float> Samples;
    const float StringDecay = FMath::Lerp(
        ResonanceForgeEditor::WaveguideDecayMin,
        ResonanceForgeEditor::WaveguideDecayMax,
        FMath::Clamp(WaveguideSustain, 0.0f, 1.0f));
    const TArray<FResonanceMode> RenderedModes = ActiveModes.IsEmpty()
        ? UResonanceForgeSynthComponent::GetBuiltInModes(ActivePreset)
        : ActiveModes;
    const bool bRendered = UResonanceForgeSynthComponent::RenderOfflinePreview(
        RenderedModes,
        ActiveModel,
        PreviewEnergy,
        PreviewBrightness,
        PreviewSize,
        PreviewStrikePosition,
        LastKeybedNote,
        StringDecay,
        WaveguideDamping,
        WaveguideCoupling,
        WaveguidePickup,
        WaveguideExcitation,
        SampleExportDurationSeconds,
        48000,
        Samples);
    if (!bRendered)
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "SampleRenderFailed", "铸样失败 · 当前物理声源没有生成有效音频");
        return FReply::Handled();
    }

    constexpr int32 EnvelopeBins = 180;
    const int32 NumFrames = Samples.Num() / 2;
    LastSampleEnvelope.SetNumZeroed(EnvelopeBins);
    float RenderPeak = 0.0f;
    for (const float Sample : Samples)
    {
        RenderPeak = FMath::Max(RenderPeak, FMath::Abs(Sample));
    }
    for (int32 Bin = 0; Bin < EnvelopeBins; ++Bin)
    {
        const int32 FirstFrame = Bin * NumFrames / EnvelopeBins;
        const int32 LastFrame = FMath::Max(FirstFrame + 1, (Bin + 1) * NumFrames / EnvelopeBins);
        float BinPeak = 0.0f;
        for (int32 Frame = FirstFrame; Frame < LastFrame && Frame < NumFrames; ++Frame)
        {
            BinPeak = FMath::Max(BinPeak, FMath::Abs(Samples[Frame * 2]));
            BinPeak = FMath::Max(BinPeak, FMath::Abs(Samples[Frame * 2 + 1]));
        }
        LastSampleEnvelope[Bin] = RenderPeak > KINDA_SMALL_NUMBER ? BinPeak / RenderPeak : 0.0f;
    }

    const int32 TailFrames = FMath::Min(NumFrames, 4800);
    double TailSquareSum = 0.0;
    for (int32 Frame = NumFrames - TailFrames; Frame < NumFrames; ++Frame)
    {
        const float Left = Samples[Frame * 2];
        const float Right = Samples[Frame * 2 + 1];
        TailSquareSum += (static_cast<double>(Left) * Left + static_cast<double>(Right) * Right) * 0.5;
    }
    const float TailRms = TailFrames > 0 ? FMath::Sqrt(static_cast<float>(TailSquareSum / TailFrames)) : 0.0f;
    LastSampleTailDb = 20.0f * FMath::LogX(10.0f, FMath::Max(TailRms / FMath::Max(RenderPeak, KINDA_SMALL_NUMBER), 0.000001f));
    LastSampleDurationSeconds = SampleExportDurationSeconds;

    float Peak = 0.0f;
    for (const float Sample : Samples)
    {
        Peak = FMath::Max(Peak, FMath::Abs(Sample));
    }
    const float Gain = Peak > 0.98f ? 0.98f / Peak : 1.0f;
    TArray<int16> Pcm16;
    Pcm16.SetNumUninitialized(Samples.Num());
    for (int32 Index = 0; Index < Samples.Num(); ++Index)
    {
        Pcm16[Index] = static_cast<int16>(FMath::RoundToInt(FMath::Clamp(Samples[Index] * Gain, -1.0f, 1.0f) * 32767.0f));
    }

    TArray<uint8> WaveData;
    SerializeWaveFile(
        WaveData,
        reinterpret_cast<const uint8*>(Pcm16.GetData()),
        Pcm16.Num() * sizeof(int16),
        2,
        48000);
    if (WaveData.IsEmpty())
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "SampleSerializeFailed", "铸样失败 · 无法序列化 WAV 数据");
        return FReply::Handled();
    }

    const FString LabelPath = FPaths::ChangeExtension(ExportPath, TEXT("rfrecipe.json"));
    const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("schema"), TEXT("resonance-forge/sample-label/v1"));

    const TSharedRef<FJsonObject> Generator = MakeShared<FJsonObject>();
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ResonanceForge"));
    Generator->SetStringField(TEXT("pluginVersion"), Plugin.IsValid() ? Plugin->GetDescriptor().VersionName : TEXT("unknown"));
    Generator->SetStringField(TEXT("unrealVersion"), FEngineVersion::Current().ToString());
    Generator->SetStringField(TEXT("generatedAtUtc"), FDateTime::UtcNow().ToIso8601());
    Root->SetObjectField(TEXT("generator"), Generator);

    const TSharedRef<FJsonObject> Audio = MakeShared<FJsonObject>();
    Audio->SetStringField(TEXT("file"), FPaths::GetCleanFilename(ExportPath));
    Audio->SetNumberField(TEXT("durationSeconds"), SampleExportDurationSeconds);
    Audio->SetNumberField(TEXT("sampleRate"), 48000);
    Audio->SetNumberField(TEXT("channels"), 2);
    Audio->SetNumberField(TEXT("bitDepth"), 16);
    Audio->SetNumberField(TEXT("tailRelativeDb"), LastSampleTailDb);
    Audio->SetBoolField(TEXT("tailSettled"), LastSampleTailDb <= -48.0f);
    TArray<TSharedPtr<FJsonValue>> EnvelopeValues;
    EnvelopeValues.Reserve(LastSampleEnvelope.Num());
    for (const float EnvelopePeak : LastSampleEnvelope)
    {
        EnvelopeValues.Add(MakeShared<FJsonValueNumber>(EnvelopePeak));
    }
    Audio->SetArrayField(TEXT("envelopePeaks"), EnvelopeValues);
    Root->SetObjectField(TEXT("audio"), Audio);

    const TSharedRef<FJsonObject> Source = MakeShared<FJsonObject>();
    Source->SetStringField(TEXT("preset"), ActivePreset.ToString());
    Source->SetStringField(TEXT("model"), ActiveModel == EResonanceModelType::WaveguideString ? TEXT("WaveguideString") : TEXT("ModalImpact"));
    Source->SetNumberField(TEXT("energy"), PreviewEnergy);
    Source->SetNumberField(TEXT("brightness"), PreviewBrightness);
    Source->SetNumberField(TEXT("objectSize"), PreviewSize);
    Source->SetNumberField(TEXT("strikePosition"), PreviewStrikePosition);
    Source->SetNumberField(TEXT("midiNote"), LastKeybedNote);
    Source->SetNumberField(TEXT("velocity"), LastKeybedVelocity);
    Root->SetObjectField(TEXT("source"), Source);

    const TSharedRef<FJsonObject> Performance = MakeShared<FJsonObject>();
    Performance->SetStringField(TEXT("velocityCurve"),
        VelocityCurve == EResonanceVelocityCurve::SoftTouch ? TEXT("SoftTouch")
        : VelocityCurve == EResonanceVelocityCurve::HeavyHand ? TEXT("HeavyHand")
        : TEXT("Linear"));
    Performance->SetNumberField(TEXT("inputVelocity"), LastKeybedVelocity);
    Performance->SetNumberField(TEXT("outputEnergy"), PreviewEnergy);
    Root->SetObjectField(TEXT("performance"), Performance);

    TArray<TSharedPtr<FJsonValue>> ModeValues;
    ModeValues.Reserve(RenderedModes.Num());
    for (const FResonanceMode& Mode : RenderedModes)
    {
        const TSharedRef<FJsonObject> ModeObject = MakeShared<FJsonObject>();
        ModeObject->SetNumberField(TEXT("frequencyHz"), Mode.FrequencyHz);
        ModeObject->SetNumberField(TEXT("gain"), Mode.Gain);
        ModeObject->SetNumberField(TEXT("decaySeconds"), Mode.DecaySeconds);
        ModeValues.Add(MakeShared<FJsonValueObject>(ModeObject));
    }
    Root->SetArrayField(TEXT("modes"), ModeValues);

    const TSharedRef<FJsonObject> Waveguide = MakeShared<FJsonObject>();
    Waveguide->SetNumberField(TEXT("sustainNormalized"), WaveguideSustain);
    Waveguide->SetNumberField(TEXT("feedback"), StringDecay);
    Waveguide->SetNumberField(TEXT("damping"), WaveguideDamping);
    Waveguide->SetNumberField(TEXT("bodyCoupling"), WaveguideCoupling);
    Waveguide->SetNumberField(TEXT("pickupPosition"), WaveguidePickup);
    Waveguide->SetStringField(TEXT("excitation"),
        WaveguideExcitation == EResonanceExcitationType::Finger ? TEXT("Finger")
        : WaveguideExcitation == EResonanceExcitationType::Hammer ? TEXT("Hammer")
        : WaveguideExcitation == EResonanceExcitationType::Bow ? TEXT("Bow")
        : TEXT("Pick"));
    Root->SetObjectField(TEXT("waveguide"), Waveguide);

    const TSharedRef<FJsonObject> Wwise = MakeShared<FJsonObject>();
    Wwise->SetStringField(TEXT("event"), ResonanceForgeEditor::GetWwiseEventName(ActivePreset));
    Wwise->SetStringField(TEXT("integration"), TEXT("metadata only; import and Wwise processing are not rendered"));
    const TSharedRef<FJsonObject> Rtpc = MakeShared<FJsonObject>();
    Rtpc->SetNumberField(TEXT("RF_ImpactEnergy"), PreviewEnergy * 100.0f);
    Rtpc->SetNumberField(TEXT("RF_ImpactBrightness"), PreviewBrightness * 100.0f);
    Rtpc->SetNumberField(TEXT("RF_ObjectSize"), PreviewSize * 100.0f);
    Wwise->SetObjectField(TEXT("rtpc0To100"), Rtpc);
    Root->SetObjectField(TEXT("wwise"), Wwise);

    FString LabelJson;
    const TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&LabelJson);
    if (!FJsonSerializer::Serialize(Root, JsonWriter) || !FFileHelper::SaveStringToFile(LabelJson, *LabelPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "SampleLabelWriteFailed", "铸样失败 · 声源铭牌无法写入，未生成孤立 WAV");
        return FReply::Handled();
    }
    if (!FFileHelper::SaveArrayToFile(WaveData, *ExportPath))
    {
        IFileManager::Get().Delete(*LabelPath, false, true);
        LastStatus = NSLOCTEXT("ResonanceForge", "SampleWriteFailed", "铸样失败 · WAV 无法写入，已撤回声源铭牌");
        return FReply::Handled();
    }

    LastSampleExportPath = FPaths::ConvertRelativePathToFull(ExportPath);
    SetFlowStation(4);
    RefreshRecentSampleLabels();
    LastStatus = FText::Format(
        NSLOCTEXT("ResonanceForge", "SampleExported", "铸样完成 · WAV + 声源铭牌 / {0} 秒 / 48 kHz / 16-bit stereo"),
        FText::AsNumber(SampleExportDurationSeconds));
    return FReply::Handled();
}

void FResonanceForgeEditorModule::RefreshRecentSampleLabels()
{
    RecentSampleLabels.Reset();
    const FString ExportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ResonanceForge"), TEXT("Exports"));
    TArray<FString> LabelFiles;
    IFileManager::Get().FindFiles(LabelFiles, *FPaths::Combine(ExportDirectory, TEXT("*.rfrecipe.json")), true, false);
    for (const FString& LabelFile : LabelFiles)
    {
        FRecentSampleLabel& Label = RecentSampleLabels.AddDefaulted_GetRef();
        Label.Path = FPaths::Combine(ExportDirectory, LabelFile);
        Label.Timestamp = IFileManager::Get().GetTimeStamp(*Label.Path);
        const FString DisplayName = LabelFile.Replace(TEXT(".rfrecipe.json"), TEXT(""));
        Label.Summary = FText::FromString(DisplayName);

        FString LabelJson;
        TSharedPtr<FJsonObject> Root;
        if (FFileHelper::LoadFileToString(LabelJson, *Label.Path)
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(LabelJson), Root)
            && Root.IsValid()
            && Root->HasTypedField<EJson::Object>(TEXT("audio"))
            && Root->HasTypedField<EJson::Object>(TEXT("source")))
        {
            const TSharedPtr<FJsonObject> Audio = Root->GetObjectField(TEXT("audio"));
            const TSharedPtr<FJsonObject> Source = Root->GetObjectField(TEXT("source"));
            if (Root->HasTypedField<EJson::Object>(TEXT("generator")))
            {
                FString GeneratedAtUtc;
                FDateTime GeneratedTimestamp;
                if (Root->GetObjectField(TEXT("generator"))->TryGetStringField(TEXT("generatedAtUtc"), GeneratedAtUtc)
                    && FDateTime::ParseIso8601(*GeneratedAtUtc, GeneratedTimestamp))
                {
                    Label.Timestamp = GeneratedTimestamp;
                }
            }
            FString Preset;
            FString Model;
            double Note = 0.0;
            double StrikePosition = 0.5;
            double Duration = 0.0;
            double TailDb = 0.0;
            FString ExcitationName(TEXT("Pick"));
            if (Root->HasTypedField<EJson::Object>(TEXT("waveguide")))
            {
                Root->GetObjectField(TEXT("waveguide"))->TryGetStringField(TEXT("excitation"), ExcitationName);
            }
            Source->TryGetNumberField(TEXT("strikePosition"), StrikePosition);
            if (Source->TryGetStringField(TEXT("preset"), Preset)
                && Source->TryGetStringField(TEXT("model"), Model)
                && Source->TryGetNumberField(TEXT("midiNote"), Note)
                && Audio->TryGetNumberField(TEXT("durationSeconds"), Duration)
                && Audio->TryGetNumberField(TEXT("tailRelativeDb"), TailDb))
            {
                    const FText ModelSummary = Model == TEXT("WaveguideString")
                    ? FText::Format(
                        NSLOCTEXT("ResonanceForge", "RecentWaveguideGesture", "波导弦/{0} · 起振 {1}%"),
                        ExcitationName == TEXT("Finger")
                            ? NSLOCTEXT("ResonanceForge", "RecentFinger", "指腹")
                            : ExcitationName == TEXT("Hammer")
                                ? NSLOCTEXT("ResonanceForge", "RecentHammer", "锤击")
                                : ExcitationName == TEXT("Bow")
                                    ? NSLOCTEXT("ResonanceForge", "RecentBow", "弓擦")
                                    : NSLOCTEXT("ResonanceForge", "RecentPick", "拨片"),
                        FText::AsNumber(FMath::RoundToInt(FMath::Clamp(StrikePosition, 0.0, 1.0) * 100.0)))
                    : NSLOCTEXT("ResonanceForge", "RecentModal", "模态体");
                Label.Summary = FText::Format(
                    NSLOCTEXT("ResonanceForge", "RecentLabelSummary", "{0}\n{1} · {2} · Note {3} · {4} 秒 · 尾音 {5} dB"),
                    FText::FromString(DisplayName),
                    FText::FromString(Preset),
                    ModelSummary,
                    FText::AsNumber(FMath::RoundToInt(Note)),
                    FText::AsNumber(Duration),
                    FText::AsNumber(FMath::RoundToInt(TailDb)));
            }
        }
    }
    RecentSampleLabels.Sort([](const FRecentSampleLabel& A, const FRecentSampleLabel& B)
    {
        if (A.Timestamp == B.Timestamp)
        {
            return A.Path > B.Path;
        }
        return A.Timestamp > B.Timestamp;
    });
    if (RecentSampleLabels.Num() > 3)
    {
        RecentSampleLabels.SetNum(3);
    }
}

bool FResonanceForgeEditorModule::HasRecentSampleLabel(const int32 LabelIndex) const
{
    return RecentSampleLabels.IsValidIndex(LabelIndex);
}

FText FResonanceForgeEditorModule::GetRecentSampleLabelText(const int32 LabelIndex) const
{
    if (!RecentSampleLabels.IsValidIndex(LabelIndex))
    {
        return NSLOCTEXT("ResonanceForge", "RecentLabelEmpty", "空铭牌\n铸样后自动上架");
    }
    const FString SelectedLabelPath = LastSampleExportPath.IsEmpty()
        ? FString()
        : FPaths::ConvertRelativePathToFull(FPaths::ChangeExtension(LastSampleExportPath, TEXT("rfrecipe.json")));
    if (FPaths::ConvertRelativePathToFull(RecentSampleLabels[LabelIndex].Path).Equals(SelectedLabelPath, ESearchCase::IgnoreCase))
    {
        return FText::Format(
            NSLOCTEXT("ResonanceForge", "RecentLabelSelected", "◆ 当前回炉 · {0}"),
            RecentSampleLabels[LabelIndex].Summary);
    }
    return RecentSampleLabels[LabelIndex].Summary;
}

FReply FResonanceForgeEditorModule::ReforgeLatestSampleLabel()
{
    RefreshRecentSampleLabels();
    if (!RecentSampleLabels.IsValidIndex(0))
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "ReforgeNoLabel", "铭牌回炉失败 · 铸样目录中还没有 .rfrecipe.json");
        return FReply::Handled();
    }
    return ReforgeSampleLabelFromPath(RecentSampleLabels[0].Path);
}

FReply FResonanceForgeEditorModule::ReforgeRecentSampleLabel(const int32 LabelIndex)
{
    RefreshRecentSampleLabels();
    if (!RecentSampleLabels.IsValidIndex(LabelIndex))
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "ReforgeEmptySlot", "铭牌回炉失败 · 这个架位还没有声源版本");
        return FReply::Handled();
    }
    return ReforgeSampleLabelFromPath(RecentSampleLabels[LabelIndex].Path);
}

FReply FResonanceForgeEditorModule::ReforgeSampleLabelFromPath(const FString& LatestLabelPath)
{
    auto Fail = [this](const FText& Message)
    {
        LastStatus = Message;
        return FReply::Handled();
    };

    AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument();
    if (!Instrument)
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeNoInstrument", "铭牌回炉失败 · 请先打开试听场景并选择一个共振体"));
    }

    const FString ExportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ResonanceForge"), TEXT("Exports"));
    const FString CanonicalLabelPath = FPaths::ConvertRelativePathToFull(LatestLabelPath);
    const FString CanonicalExportDirectory = FPaths::ConvertRelativePathToFull(ExportDirectory);
    if (!CanonicalLabelPath.StartsWith(CanonicalExportDirectory + TEXT("/"), ESearchCase::IgnoreCase)
        && !CanonicalLabelPath.StartsWith(CanonicalExportDirectory + TEXT("\\"), ESearchCase::IgnoreCase))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeOutsideExport", "铭牌回炉失败 · 文件不在本工程铸样目录中"));
    }

    FString LabelJson;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(LabelJson, *CanonicalLabelPath)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(LabelJson), Root)
        || !Root.IsValid())
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeInvalidJson", "铭牌回炉失败 · 最近铭牌不是有效 JSON"));
    }

    FString Schema;
    if (!Root->TryGetStringField(TEXT("schema"), Schema) || Schema != TEXT("resonance-forge/sample-label/v1"))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeUnsupportedSchema", "铭牌回炉失败 · 仅支持 sample-label/v1"));
    }
    if (!Root->HasTypedField<EJson::Object>(TEXT("audio"))
        || !Root->HasTypedField<EJson::Object>(TEXT("source"))
        || !Root->HasTypedField<EJson::Object>(TEXT("waveguide")))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeMissingSection", "铭牌回炉失败 · 缺少 audio、source 或 waveguide 段"));
    }

    const TSharedPtr<FJsonObject> AudioObject = Root->GetObjectField(TEXT("audio"));
    const TSharedPtr<FJsonObject> SourceObject = Root->GetObjectField(TEXT("source"));
    const TSharedPtr<FJsonObject> WaveguideObject = Root->GetObjectField(TEXT("waveguide"));
    const TSharedPtr<FJsonObject> PerformanceObject = Root->HasTypedField<EJson::Object>(TEXT("performance"))
        ? Root->GetObjectField(TEXT("performance"))
        : nullptr;
    auto ReadFinite = [](const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, double& OutValue)
    {
        return Object.IsValid() && Object->TryGetNumberField(Field, OutValue) && FMath::IsFinite(OutValue);
    };
    auto InRange = [](const double Value, const double Min, const double Max)
    {
        return Value >= Min && Value <= Max;
    };

    FString AudioFile;
    double Duration = 0.0;
    double TailDb = 0.0;
    double SampleRate = 0.0;
    double Channels = 0.0;
    double BitDepth = 0.0;
    if (!AudioObject->TryGetStringField(TEXT("file"), AudioFile)
        || FPaths::GetCleanFilename(AudioFile) != AudioFile
        || FPaths::GetExtension(AudioFile).ToLower() != TEXT("wav")
        || !ReadFinite(AudioObject, TEXT("durationSeconds"), Duration)
        || !InRange(Duration, 0.1, 12.0)
        || !ReadFinite(AudioObject, TEXT("sampleRate"), SampleRate) || SampleRate != 48000.0
        || !ReadFinite(AudioObject, TEXT("channels"), Channels) || Channels != 2.0
        || !ReadFinite(AudioObject, TEXT("bitDepth"), BitDepth) || BitDepth != 16.0
        || !ReadFinite(AudioObject, TEXT("tailRelativeDb"), TailDb)
        || !InRange(TailDb, -120.0, 6.0))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeInvalidAudio", "铭牌回炉失败 · 音频规格或文件名越界"));
    }
    const FString AudioPath = FPaths::Combine(ExportDirectory, AudioFile);
    if (!FPaths::FileExists(AudioPath))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeMissingWave", "铭牌回炉失败 · 铭牌对应的 WAV 已不存在"));
    }

    TArray<uint8> WaveBytes;
    uint16 WaveChannels = 0;
    uint32 WaveSampleRate = 0;
    uint16 WaveBitDepth = 0;
    uint32 WaveDataBytes = 0;
    if (!FFileHelper::LoadFileToArray(WaveBytes, *AudioPath) || WaveBytes.Num() < 44
        || FMemory::Memcmp(WaveBytes.GetData(), "RIFF", 4) != 0
        || FMemory::Memcmp(WaveBytes.GetData() + 8, "WAVE", 4) != 0
        || FMemory::Memcmp(WaveBytes.GetData() + 12, "fmt ", 4) != 0
        || FMemory::Memcmp(WaveBytes.GetData() + 36, "data", 4) != 0)
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeInvalidWave", "铭牌回炉失败 · 配套文件不是铸样台生成的标准 PCM WAV"));
    }
    FMemory::Memcpy(&WaveChannels, WaveBytes.GetData() + 22, sizeof(WaveChannels));
    FMemory::Memcpy(&WaveSampleRate, WaveBytes.GetData() + 24, sizeof(WaveSampleRate));
    FMemory::Memcpy(&WaveBitDepth, WaveBytes.GetData() + 34, sizeof(WaveBitDepth));
    FMemory::Memcpy(&WaveDataBytes, WaveBytes.GetData() + 40, sizeof(WaveDataBytes));
    const double WaveDuration = static_cast<double>(WaveDataBytes) / (48000.0 * 2.0 * 2.0);
    if (WaveChannels != 2 || WaveSampleRate != 48000 || WaveBitDepth != 16
        || WaveDataBytes + 44u > static_cast<uint32>(WaveBytes.Num())
        || !FMath::IsNearlyEqual(WaveDuration, Duration, 0.01))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeWaveMismatch", "铭牌回炉失败 · WAV 真实规格与铭牌不一致"));
    }

    const TArray<TSharedPtr<FJsonValue>>* EnvelopeValues = nullptr;
    TArray<float> ImportedEnvelope;
    if (AudioObject->TryGetArrayField(TEXT("envelopePeaks"), EnvelopeValues))
    {
        if (!EnvelopeValues || EnvelopeValues->Num() != 180)
        {
            return Fail(NSLOCTEXT("ResonanceForge", "ReforgeInvalidEnvelope", "铭牌回炉失败 · 余响拓片必须包含 180 段"));
        }
        ImportedEnvelope.Reserve(EnvelopeValues->Num());
        for (const TSharedPtr<FJsonValue>& Value : *EnvelopeValues)
        {
            if (!Value.IsValid() || Value->Type != EJson::Number)
            {
                return Fail(NSLOCTEXT("ResonanceForge", "ReforgeEnvelopeType", "铭牌回炉失败 · 余响拓片含有非数字数据"));
            }
            const double Peak = Value->AsNumber();
            if (!FMath::IsFinite(Peak) || !InRange(Peak, 0.0, 1.0))
            {
                return Fail(NSLOCTEXT("ResonanceForge", "ReforgeEnvelopeRange", "铭牌回炉失败 · 余响拓片含有越界数据"));
            }
            ImportedEnvelope.Add(static_cast<float>(Peak));
        }
    }

    FString PresetString;
    FString ModelString;
    double Energy = 0.0;
    double Brightness = 0.0;
    double ObjectSize = 0.0;
    double StrikePosition = 0.0;
    double MidiNote = 0.0;
    double Velocity = 0.0;
    if (!SourceObject->TryGetStringField(TEXT("preset"), PresetString)
        || !SourceObject->TryGetStringField(TEXT("model"), ModelString)
        || !ReadFinite(SourceObject, TEXT("energy"), Energy)
        || !ReadFinite(SourceObject, TEXT("brightness"), Brightness)
        || !ReadFinite(SourceObject, TEXT("objectSize"), ObjectSize)
        || !ReadFinite(SourceObject, TEXT("strikePosition"), StrikePosition)
        || !ReadFinite(SourceObject, TEXT("midiNote"), MidiNote)
        || !ReadFinite(SourceObject, TEXT("velocity"), Velocity)
        || !InRange(Energy, 0.0, 1.0) || !InRange(Brightness, 0.0, 1.0)
        || !InRange(ObjectSize, 0.0, 1.0) || !InRange(StrikePosition, 0.0, 1.0)
        || !InRange(MidiNote, 0.0, 127.0) || !InRange(Velocity, 0.0, 1.0))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeInvalidSource", "铭牌回炉失败 · 声源参数缺失或越界"));
    }
    const FName ImportedPreset(*PresetString);
    if (!UResonanceForgeSynthComponent::GetBuiltInPresetNames().Contains(ImportedPreset))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeUnknownPreset", "铭牌回炉失败 · 材质预设不受当前版本支持"));
    }
    EResonanceModelType ImportedModel;
    if (ModelString == TEXT("ModalImpact"))
    {
        ImportedModel = EResonanceModelType::ModalImpact;
    }
    else if (ModelString == TEXT("WaveguideString"))
    {
        ImportedModel = EResonanceModelType::WaveguideString;
    }
    else
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeUnknownModel", "铭牌回炉失败 · 声学模型不受当前版本支持"));
    }

    FString VelocityCurveName(TEXT("Linear"));
    if (PerformanceObject.IsValid()
        && (!PerformanceObject->TryGetStringField(TEXT("velocityCurve"), VelocityCurveName)
            || (VelocityCurveName != TEXT("SoftTouch") && VelocityCurveName != TEXT("Linear") && VelocityCurveName != TEXT("HeavyHand"))))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeInvalidVelocityCurve", "铭牌回炉失败 · 力度响应曲线不受当前版本支持"));
    }

    double Sustain = 0.0;
    double Damping = 0.0;
    double Coupling = 0.0;
    double Pickup = 0.35;
    FString ExcitationName(TEXT("Pick"));
    if (!ReadFinite(WaveguideObject, TEXT("sustainNormalized"), Sustain)
        || !ReadFinite(WaveguideObject, TEXT("damping"), Damping)
        || !ReadFinite(WaveguideObject, TEXT("bodyCoupling"), Coupling)
        || !InRange(Sustain, 0.0, 1.0) || !InRange(Damping, 0.0, 1.0) || !InRange(Coupling, 0.0, 1.0))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeInvalidWaveguide", "铭牌回炉失败 · 弦床参数缺失或越界"));
    }
    if (WaveguideObject->HasField(TEXT("pickupPosition"))
        && (!ReadFinite(WaveguideObject, TEXT("pickupPosition"), Pickup) || !InRange(Pickup, 0.0, 1.0)))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeInvalidPickup", "铭牌回炉失败 · 拾音位置越界"));
    }
    if (WaveguideObject->HasField(TEXT("excitation"))
        && (!WaveguideObject->TryGetStringField(TEXT("excitation"), ExcitationName)
            || (ExcitationName != TEXT("Finger") && ExcitationName != TEXT("Pick")
                && ExcitationName != TEXT("Hammer") && ExcitationName != TEXT("Bow"))))
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeInvalidExcitation", "铭牌回炉失败 · 起振手势不受当前版本支持"));
    }

    const TArray<TSharedPtr<FJsonValue>>* ModeValues = nullptr;
    if (!Root->TryGetArrayField(TEXT("modes"), ModeValues) || !ModeValues || ModeValues->IsEmpty() || ModeValues->Num() > 64)
    {
        return Fail(NSLOCTEXT("ResonanceForge", "ReforgeInvalidModes", "铭牌回炉失败 · 共振齿列数量无效"));
    }
    TArray<FResonanceMode> ImportedModes;
    ImportedModes.Reserve(ModeValues->Num());
    for (const TSharedPtr<FJsonValue>& Value : *ModeValues)
    {
        if (!Value.IsValid() || Value->Type != EJson::Object)
        {
            return Fail(NSLOCTEXT("ResonanceForge", "ReforgeModeType", "铭牌回炉失败 · 共振齿必须是对象"));
        }
        const TSharedPtr<FJsonObject> ModeObject = Value->AsObject();
        double Frequency = 0.0;
        double Gain = 0.0;
        double Decay = 0.0;
        if (!ReadFinite(ModeObject, TEXT("frequencyHz"), Frequency)
            || !ReadFinite(ModeObject, TEXT("gain"), Gain)
            || !ReadFinite(ModeObject, TEXT("decaySeconds"), Decay)
            || !InRange(Frequency, 20.0, 20000.0) || !InRange(Gain, 0.0, 4.0) || !InRange(Decay, 0.01, 20.0))
        {
            return Fail(NSLOCTEXT("ResonanceForge", "ReforgeModeRange", "铭牌回炉失败 · 共振齿含有缺失或越界数据"));
        }
        FResonanceMode& ImportedMode = ImportedModes.AddDefaulted_GetRef();
        ImportedMode.FrequencyHz = static_cast<float>(Frequency);
        ImportedMode.Gain = static_cast<float>(Gain);
        ImportedMode.DecaySeconds = static_cast<float>(Decay);
    }

    ActivePreset = ImportedPreset;
    ActiveModel = ImportedModel;
    PreviewEnergy = static_cast<float>(Energy);
    PreviewBrightness = static_cast<float>(Brightness);
    PreviewSize = static_cast<float>(ObjectSize);
    PreviewStrikePosition = static_cast<float>(StrikePosition);
    LastKeybedNote = FMath::RoundToInt(MidiNote);
    LastKeybedVelocity = static_cast<float>(Velocity);
    VelocityCurve = VelocityCurveName == TEXT("SoftTouch")
        ? EResonanceVelocityCurve::SoftTouch
        : VelocityCurveName == TEXT("HeavyHand")
            ? EResonanceVelocityCurve::HeavyHand
            : EResonanceVelocityCurve::Linear;
    WaveguideSustain = static_cast<float>(Sustain);
    WaveguideDamping = static_cast<float>(Damping);
    WaveguideCoupling = static_cast<float>(Coupling);
    WaveguidePickup = static_cast<float>(Pickup);
    WaveguideExcitation = ExcitationName == TEXT("Finger")
        ? EResonanceExcitationType::Finger
        : ExcitationName == TEXT("Hammer")
            ? EResonanceExcitationType::Hammer
            : ExcitationName == TEXT("Bow")
                ? EResonanceExcitationType::Bow
                : EResonanceExcitationType::Pick;
    LastSampleEnvelope = MoveTemp(ImportedEnvelope);
    LastSampleTailDb = static_cast<float>(TailDb);
    LastSampleDurationSeconds = static_cast<float>(Duration);
    LastSampleExportPath = FPaths::ConvertRelativePathToFull(AudioPath);

    ApplyPreset(ActivePreset);
    ApplyModel(ActiveModel);
    ActiveModes = MoveTemp(ImportedModes);
    SelectedModeIndex = 0;
    ApplyModalModes(false, FText::GetEmpty());
    Instrument->Modify();
    Instrument->ObjectSize = PreviewSize;
    Instrument->VelocityCurve = VelocityCurve;
    Instrument->ManualStrikePosition = PreviewStrikePosition;
    Instrument->LastStrikePosition = PreviewStrikePosition;
    ApplyWaveguideParameters();
    Instrument->MarkPackageDirty();
    LastSampleReforgedSeconds = FPlatformTime::Seconds();
    SetFlowStation(4);
    Instrument->TriggerInstrument(PreviewEnergy, PreviewBrightness, LastKeybedNote, PreviewStrikePosition);
    LastStatus = FText::Format(
        NSLOCTEXT("ResonanceForge", "ReforgeComplete", "铭牌回炉完成 · {0} / {1} 根共振齿 / Note {2} · 已试听"),
        FText::FromName(ActivePreset),
        FText::AsNumber(ActiveModes.Num()),
        FText::AsNumber(LastKeybedNote));
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::RevealSampleExport()
{
    const FString Directory = LastSampleExportPath.IsEmpty()
        ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ResonanceForge"), TEXT("Exports"))
        : FPaths::GetPath(LastSampleExportPath);
    IFileManager::Get().MakeDirectory(*Directory, true);
    FPlatformProcess::ExploreFolder(*Directory);
    return FReply::Handled();
}

FText FResonanceForgeEditorModule::GetSampleExportStatusText() const
{
    if (LastSampleExportPath.IsEmpty())
    {
        return NSLOCTEXT("ResonanceForge", "SampleNotExported", "等待铸样 · 复用当前模态、落点、弦床与演奏参数");
    }
    return FText::Format(
        NSLOCTEXT("ResonanceForge", "SampleExportPath", "已生成 · {0} + 声源铭牌"),
        FText::FromString(FPaths::GetCleanFilename(LastSampleExportPath)));
}

FText FResonanceForgeEditorModule::GetSampleTailStatusText() const
{
    if (LastSampleEnvelope.IsEmpty())
    {
        if (LastSampleDurationSeconds > 0.0f)
        {
            return FText::Format(
                NSLOCTEXT("ResonanceForge", "SampleTailLegacyLabel", "旧版 v1 铭牌未携带余响拓片 · 末段记录约 {0} dB（相对峰值）"),
                FText::AsNumber(FMath::RoundToInt(LastSampleTailDb)));
        }
        return NSLOCTEXT("ResonanceForge", "SampleTailWaiting", "余响拓片 · 铸样后显示真实振幅包络与末段电平");
    }
    if (LastSampleTailDb <= -48.0f)
    {
        return FText::Format(
            NSLOCTEXT("ResonanceForge", "SampleTailSettled", "余响已收束 · 末 100 ms 约 {0} dB（相对峰值）"),
            FText::AsNumber(FMath::RoundToInt(LastSampleTailDb)));
    }
    const FText Advice = LastSampleDurationSeconds < 6.0f
        ? NSLOCTEXT("ResonanceForge", "SampleTailLongerAdvice", "建议延长一档，或降低回响长度")
        : NSLOCTEXT("ResonanceForge", "SampleTailShapeAdvice", "可保留长尾，或降低回响长度");
    return FText::Format(
        NSLOCTEXT("ResonanceForge", "SampleTailActive", "尾音仍活跃 · 末 100 ms 约 {0} dB（相对峰值） · {1}"),
        FText::AsNumber(FMath::RoundToInt(LastSampleTailDb)),
        Advice);
}

FReply FResonanceForgeEditorModule::ForgeSharedRecipeAsset()
{
    AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument();
    if (!Instrument)
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "SharedRecipeNoInstrument", "无法铸印共享配方 · 请先打开试听场景并选择一个共振体");
        return FReply::Handled();
    }

    FString DisplayName = SharedRecipeName.TrimStartAndEnd();
    if (DisplayName.IsEmpty())
    {
        DisplayName = TEXT("新声学配方");
    }
    FString BaseAssetName = ObjectTools::SanitizeObjectName(TEXT("DA_RF_") + DisplayName);
    if (BaseAssetName.IsEmpty())
    {
        BaseAssetName = TEXT("DA_RF_SharedRecipe");
    }

    const FString RootPath(TEXT("/Game/ResonanceForge/Profiles/"));
    FString AssetName = BaseAssetName;
    FString PackageName = RootPath + AssetName;
    for (int32 Suffix = 2; FPackageName::DoesPackageExist(PackageName); ++Suffix)
    {
        AssetName = FString::Printf(TEXT("%s_%02d"), *BaseAssetName, Suffix);
        PackageName = RootPath + AssetName;
    }

    UPackage* Package = CreatePackage(*PackageName);
    UResonanceMaterialProfile* Profile = NewObject<UResonanceMaterialProfile>(
        Package, *AssetName, RF_Public | RF_Standalone);
    if (!Profile)
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "SharedRecipeCreateFailed", "共享配方创建失败");
        return FReply::Handled();
    }

    Profile->DisplayName = FText::FromString(DisplayName);
    Profile->SourcePreset = ActivePreset;
    Profile->ModelType = ActiveModel;
    Profile->Modes = ActiveModes.IsEmpty()
        ? UResonanceForgeSynthComponent::GetBuiltInModes(ActivePreset)
        : ActiveModes;
    Profile->StringDecay = FMath::Lerp(
        ResonanceForgeEditor::WaveguideDecayMin,
        ResonanceForgeEditor::WaveguideDecayMax,
        FMath::Clamp(WaveguideSustain, 0.0f, 1.0f));
    Profile->StringDamping = FMath::Clamp(WaveguideDamping, 0.0f, 1.0f);
    Profile->BodyCoupling = FMath::Clamp(WaveguideCoupling, 0.0f, 1.0f);
    Profile->PickupPosition = FMath::Clamp(WaveguidePickup, 0.0f, 1.0f);
    Profile->ExcitationType = WaveguideExcitation;

    FAssetRegistryModule::AssetCreated(Profile);
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        PackageName, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Profile, *Filename, SaveArgs))
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "SharedRecipeSaveFailed", "共享配方已创建，但保存到 Content 失败");
        return FReply::Handled();
    }

    Instrument->Modify();
    Instrument->NativeSynth->ApplyMaterialProfile(Profile);
    Instrument->MarkPackageDirty();
    if (GEditor)
    {
        TArray<UObject*> ObjectsToSync = {Profile};
        GEditor->SyncBrowserToObjects(ObjectsToSync);
    }
    LastStatus = FText::Format(
        NSLOCTEXT("ResonanceForge", "SharedRecipeCreated", "共享配方「{0}」已铸印并挂到当前对象 · Content 浏览器已定位"),
        FText::FromString(DisplayName));
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::OpenDemoMap()
{
    if (GEditor)
    {
        if (ULevelEditorSubsystem* LevelSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
        {
            const FString FabMap(TEXT("/Game/CarpentersWorkshop/ResonanceForge/L_RF_WorkshopShowcase"));
            const FString BaseMap(TEXT("/Game/ResonanceForge/Demo/Maps/L_RF_PhysicsLab"));
            const bool bHasFabMap = FPackageName::DoesPackageExist(FabMap);
            const bool bLoaded = LevelSubsystem->LoadLevel(bHasFabMap ? FabMap : BaseMap);
            LastStatus = bLoaded
                ? (bHasFabMap
                    ? NSLOCTEXT("ResonanceForge", "FabMapLoaded", "Fab 增强声学工坊已打开 · 选择砧座或波导弦开始试听")
                    : NSLOCTEXT("ResonanceForge", "MapLoaded", "基础声学工坊已打开 · 选择砧座或波导弦开始试听"))
                : NSLOCTEXT("ResonanceForge", "MapFailed", "演示地图打开失败 · 请检查内容是否已生成");
        }
    }
    return FReply::Handled();
}

void FResonanceForgeEditorModule::SetFlowStation(const int32 Station)
{
    ActiveFlowStation = FMath::Clamp(Station, 0, 4);
}

void FResonanceForgeEditorModule::NavigateToFlowStation(const int32 Station)
{
    SetFlowStation(Station);
    if (!WorkbenchScrollBox.IsValid())
    {
        return;
    }

    TSharedPtr<SWidget> Target;
    switch (ActiveFlowStation)
    {
    case 0:
        Target = FlowObjectAnchor;
        break;
    case 1:
        Target = FlowExcitationAnchor;
        break;
    case 2:
        Target = ActiveModel == EResonanceModelType::WaveguideString ? FlowWaveguideAnchor : FlowModalAnchor;
        break;
    case 3:
        Target = FlowOutputAnchor;
        break;
    default:
        Target = FlowSampleAnchor;
        break;
    }
    if (Target.IsValid())
    {
        WorkbenchScrollBox->ScrollDescendantIntoView(Target, true, EDescendantScrollDestination::TopOrLeft, 18.0f);
    }
}

FText FResonanceForgeEditorModule::GetFlowGuideText() const
{
    switch (ActiveFlowStation)
    {
    case 0:
        return NSLOCTEXT("ResonanceForge", "FlowGuideObject", "先取一件共振体，再选择它要怎样发声。");
    case 1:
        return NSLOCTEXT("ResonanceForge", "FlowGuideExcitation", "声音已起振：接着去弦床或共振齿列塑形。");
    case 2:
        return NSLOCTEXT("ResonanceForge", "FlowGuideResonance", "共振已成形：切换监听闸门比较 UE 原声与 Wwise。");
    case 3:
        return NSLOCTEXT("ResonanceForge", "FlowGuideOutput", "出口已接通：满意后把这件声音铸成 WAV 与铭牌。");
    default:
        return NSLOCTEXT("ResonanceForge", "FlowGuideSample", "铸样已落盘：点击铭牌可回炉，或回到任一工位继续打磨。");
    }
}

FText FResonanceForgeEditorModule::GetSelectionText() const
{
    if (const AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        const UResonanceMaterialProfile* SharedProfile = Instrument->NativeSynth ? Instrument->NativeSynth->MaterialProfile : nullptr;
        const EResonanceModelType Model = SharedProfile ? SharedProfile->ModelType : Instrument->SynthesisModel;
        const FText ModelText = Model == EResonanceModelType::WaveguideString
            ? NSLOCTEXT("ResonanceForge", "WaveguideModelName", "数字波导弦")
            : NSLOCTEXT("ResonanceForge", "ModalModelName", "模态撞击体");
        const FText RecipeText = SharedProfile && !SharedProfile->DisplayName.IsEmpty()
            ? SharedProfile->DisplayName
            : FText::FromName(Instrument->ResonancePreset);
        return FText::Format(NSLOCTEXT("ResonanceForge", "Selected", "{0}  ·  {1}  ·  {2}"),
            FText::FromString(Instrument->GetActorLabel()), ModelText, RecipeText);
    }
    return NSLOCTEXT("ResonanceForge", "SelectionEmpty", "没有共振体 · 打开声学工坊，或在场景中选择一个共振对象");
}

FText FResonanceForgeEditorModule::GetPrimaryActionText() const
{
    if (ActiveModel == EResonanceModelType::WaveguideString)
    {
        return WaveguideExcitation == EResonanceExcitationType::Bow
            ? NSLOCTEXT("ResonanceForge", "BowNow", "拉一次弓")
            : NSLOCTEXT("ResonanceForge", "PluckNow", "激发当前弦");
    }
    return NSLOCTEXT("ResonanceForge", "StrikeNow", "敲击当前对象");
}

FText FResonanceForgeEditorModule::GetStatusText() const
{
    return LastStatus;
}

FText FResonanceForgeEditorModule::GetComparisonText() const
{
    if (!bHasReference)
    {
        return NSLOCTEXT("ResonanceForge", "NoReference", "钉住一次声纹，再调整材质或力度进行 A/B 比较");
    }

    const int32 EnergyDelta = FMath::RoundToInt((PreviewEnergy - ReferenceEnergy) * 100.0f);
    const int32 BrightnessDelta = FMath::RoundToInt((PreviewBrightness - ReferenceBrightness) * 100.0f);
    const int32 SizeDelta = FMath::RoundToInt((PreviewSize - ReferenceSize) * 100.0f);
    const int32 PositionDelta = FMath::RoundToInt((PreviewStrikePosition - ReferenceStrikePosition) * 100.0f);
    const int32 SustainDelta = FMath::RoundToInt((WaveguideSustain - ReferenceSustain) * 100.0f);
    const int32 DampingDelta = FMath::RoundToInt((WaveguideDamping - ReferenceDamping) * 100.0f);
    const int32 CouplingDelta = FMath::RoundToInt((WaveguideCoupling - ReferenceCoupling) * 100.0f);
    const int32 PickupDelta = FMath::RoundToInt((WaveguidePickup - ReferencePickup) * 100.0f);
    int32 ChangedModeCount = FMath::Abs(ActiveModes.Num() - ReferenceModes.Num());
    for (int32 Index = 0; Index < FMath::Min(ActiveModes.Num(), ReferenceModes.Num()); ++Index)
    {
        const FResonanceMode& Current = ActiveModes[Index];
        const FResonanceMode& Reference = ReferenceModes[Index];
        if (!FMath::IsNearlyEqual(Current.FrequencyHz, Reference.FrequencyHz, 0.5f)
            || !FMath::IsNearlyEqual(Current.Gain, Reference.Gain, 0.005f)
            || !FMath::IsNearlyEqual(Current.DecaySeconds, Reference.DecaySeconds, 0.005f))
        {
            ++ChangedModeCount;
        }
    }
    const auto SignedPercent = [](int32 Value)
    {
        return FText::FromString(FString::Printf(TEXT("%+d"), Value));
    };
    if (ActiveModel == EResonanceModelType::WaveguideString || ReferenceModel == EResonanceModelType::WaveguideString)
    {
        const auto GestureText = [](const EResonanceExcitationType Type)
        {
            return Type == EResonanceExcitationType::Finger
                ? NSLOCTEXT("ResonanceForge", "CompareFinger", "指腹")
                : Type == EResonanceExcitationType::Hammer
                    ? NSLOCTEXT("ResonanceForge", "CompareHammer", "锤击")
                    : Type == EResonanceExcitationType::Bow
                        ? NSLOCTEXT("ResonanceForge", "CompareBow", "弓擦")
                        : NSLOCTEXT("ResonanceForge", "ComparePick", "拨片");
        };
        return FText::Format(
            NSLOCTEXT("ResonanceForge", "WaveguideReferenceDifference", "参考「{0}」 · 起振 {1}%  拾音 {2}%  延音 {3}%  阻尼 {4}%  · {5} → {6}"),
            FText::FromName(ReferencePreset), SignedPercent(PositionDelta), SignedPercent(PickupDelta), SignedPercent(SustainDelta), SignedPercent(DampingDelta), GestureText(ReferenceExcitation), GestureText(WaveguideExcitation));
    }
    return FText::Format(
        NSLOCTEXT("ResonanceForge", "ReferenceDifference", "参考「{0}」 · 能量 {1}%  明亮 {2}%  尺度 {3}%  落点 {4}%  ·  变化 {5} 根共振齿"),
        FText::FromName(ReferencePreset),
        SignedPercent(EnergyDelta),
        SignedPercent(BrightnessDelta),
        SignedPercent(SizeDelta),
        SignedPercent(PositionDelta),
        FText::AsNumber(ChangedModeCount));
}

TSharedRef<SDockTab> FResonanceForgeEditorModule::SpawnWorkbench(const FSpawnTabArgs& Args)
{
    RefreshRecentSampleLabels();
    using namespace ResonanceForgeEditor;

    if (const AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        const UResonanceMaterialProfile* SharedProfile = Instrument->NativeSynth ? Instrument->NativeSynth->MaterialProfile : nullptr;
        ActiveModel = SharedProfile ? SharedProfile->ModelType : Instrument->SynthesisModel;
        ActivePreset = SharedProfile ? SharedProfile->SourcePreset : Instrument->ResonancePreset;
        PreviewSize = Instrument->ObjectSize;
        VelocityCurve = Instrument->VelocityCurve;
        PreviewStrikePosition = Instrument->LastStrikePosition;
        ListenMode = Instrument->ListenMode;
        if (Instrument->NativeSynth)
        {
            WaveguideSustain = FMath::GetRangePct(
                ResonanceForgeEditor::WaveguideDecayMin,
                ResonanceForgeEditor::WaveguideDecayMax,
                Instrument->NativeSynth->StringDecay);
            WaveguideDamping = Instrument->NativeSynth->StringDamping;
            WaveguideCoupling = Instrument->NativeSynth->BodyCoupling;
            WaveguidePickup = Instrument->NativeSynth->PickupPosition;
            WaveguideExcitation = Instrument->NativeSynth->ExcitationType;
            ActiveModes = Instrument->NativeSynth->GetEffectiveModes();
            SelectedModeIndex = FMath::Clamp(SelectedModeIndex, 0, FMath::Max(0, ActiveModes.Num() - 1));
        }
        if (Instrument->WwiseBridge)
        {
            Instrument->WwiseBridge->AutoBindDemoAssets();
        }
    }
    if (MidiDeviceOptions.IsEmpty())
    {
        RefreshMidiDevices();
    }

    auto ModelButton = [this](const EResonanceModelType ModelType, const FText& Label, const FText& Detail, const FLinearColor& Color)
    {
        return SNew(SButton)
            .ContentPadding(FMargin(14, 12))
            .ButtonColorAndOpacity_Lambda([this, ModelType, Color]
            {
                return ActiveModel == ModelType ? Color * 0.54f : FLinearColor(0.045f, 0.058f, 0.070f, 1.0f);
            })
            .OnClicked_Lambda([this, ModelType]
            {
                ApplyModel(ModelType);
                AuditionCurrentSound(ModelType == EResonanceModelType::WaveguideString
                    ? NSLOCTEXT("ResonanceForge", "WaveguideAudition", "数字波导弦")
                    : NSLOCTEXT("ResonanceForge", "ModalAudition", "模态撞击体"));
                return FReply::Handled();
            })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted).AutoWrapText(true)]
            ];
    };

    auto ListenGateButton = [this](const EResonanceForgeListenMode Mode, const FText& Label, const FText& Detail, const FLinearColor& Color)
    {
        return SNew(SButton)
            .ContentPadding(FMargin(12, 9))
            .ButtonColorAndOpacity_Lambda([this, Mode, Color]
            {
                return ListenMode == Mode ? Color * 0.55f : FLinearColor(0.035f, 0.042f, 0.041f, 1.0f);
            })
            .OnClicked_Lambda([this, Mode]{ return SetListenMode(Mode); })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
                [SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted).AutoWrapText(true)]
            ];
    };

    auto ExcitationGestureButton = [this](const EResonanceExcitationType Type, const FText& Label, const FText& Detail, const FLinearColor& Color)
    {
        return SNew(SButton)
            .ContentPadding(FMargin(11, 8))
            .ButtonColorAndOpacity_Lambda([this, Type, Color]
            {
                return WaveguideExcitation == Type ? Color * 0.55f : FLinearColor(0.035f, 0.030f, 0.025f, 1.0f);
            })
            .OnClicked_Lambda([this, Type]{ return SetWaveguideExcitation(Type); })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
                [SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted)]
            ];
    };

    auto VelocityCurveButton = [this](const EResonanceVelocityCurve Curve, const FText& Label, const FText& Detail, const FLinearColor& Color)
    {
        return SNew(SButton)
            .ContentPadding(FMargin(10, 7))
            .ButtonColorAndOpacity_Lambda([this, Curve, Color]
            {
                return VelocityCurve == Curve ? Color * 0.52f : FLinearColor(0.032f, 0.028f, 0.024f, 1.0f);
            })
            .OnClicked_Lambda([this, Curve]{ return SetVelocityCurve(Curve); })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
                [SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted)]
            ];
    };

    auto PresetButton = [this](const FName Preset, const FText& Label, const FLinearColor& Color)
    {
        return SNew(SButton)
            .Text(Label)
            .ContentPadding(FMargin(12, 8))
            .ButtonColorAndOpacity_Lambda([this, Preset, Color]
            {
                return ActivePreset == Preset ? Color * 0.48f : FLinearColor(0.045f, 0.058f, 0.070f, 1.0f);
            })
            .OnClicked_Lambda([this, Preset]
            {
                ApplyPreset(Preset);
                AuditionCurrentSound(FText::Format(
                    NSLOCTEXT("ResonanceForge", "PresetAudition", "材质「{0}」"),
                    FText::FromName(Preset)));
                return FReply::Handled();
            });
    };

    auto ParameterRow = [this](const FText& Name, const FText& Mapping, float* Value, const FLinearColor& Color)
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)[SNew(STextBlock).Text(Name).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)[SNew(STextBlock).Text(Mapping).ColorAndOpacity(Muted)]
                + SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([Value]
                {
                    return FText::Format(NSLOCTEXT("ResonanceForge", "ParameterPercent", "{0}%"), FText::AsNumber(FMath::RoundToInt(*Value * 100.0f)));
                }).ColorAndOpacity(Color).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 0)
            [
                SNew(SSlider)
                .Value_Lambda([Value]{ return *Value; })
                .OnValueChanged_Lambda([Value](float NewValue){ *Value = NewValue; })
                .OnMouseCaptureEnd_Lambda([this, Name]
                {
                    AuditionCurrentSound(FText::Format(
                        NSLOCTEXT("ResonanceForge", "ParameterAudition", "{0}调整完成"),
                        Name));
                })
                .SliderBarColor(Color)
                .SliderHandleColor(Color)
            ];
    };

    auto OutputReading = [](const FText& Label, const TAttribute<FText>& Reading, const FLinearColor& Color)
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0, 2, 10, 2)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush(TEXT("WhiteBrush")))
                .BorderBackgroundColor(Color)
                .Padding(FMargin(2, 0))
            ]
            + SHorizontalBox::Slot().FillWidth(1.0f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [SNew(STextBlock).Text(Label).ColorAndOpacity(Muted)]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
                [SNew(STextBlock).Text(Reading).ColorAndOpacity(Color).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
            ];
    };

    auto RecipeSlot = [this](const int32 SlotIndex, const FLinearColor& Color)
    {
        return SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
            .BorderBackgroundColor(FLinearColor(0.028f, 0.032f, 0.030f, 1.0f))
            .Padding(FMargin(12, 10))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [SNew(STextBlock).Text_Lambda([this, SlotIndex]{ return GetRecipeSlotText(SlotIndex); }).ColorAndOpacity(Color).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 4, 0)
                    [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "StoreRecipe", "存入当前")).OnClicked_Lambda([this, SlotIndex]{ return SaveRecipeSlot(SlotIndex); })]
                    + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
                    [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "RecallRecipe", "召回")).IsEnabled_Lambda([this, SlotIndex]{ return HasRecipeSlot(SlotIndex); }).OnClicked_Lambda([this, SlotIndex]{ return RecallRecipeSlot(SlotIndex); })]
                ]
            ];
    };

    auto WaveguideParameterRow = [this](const FText& Name, const FText& Detail, float* Value, const FLinearColor& Color)
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [SNew(STextBlock).Text(Name).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
                [SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted)]
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(STextBlock).Text_Lambda([Value]
                {
                    return FText::Format(NSLOCTEXT("ResonanceForge", "WaveguidePercent", "{0}%"), FText::AsNumber(FMath::RoundToInt(*Value * 100.0f)));
                }).ColorAndOpacity(Color).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 0)
            [
                SNew(SSlider)
                .Value_Lambda([Value]{ return *Value; })
                .OnValueChanged_Lambda([this, Value](float NewValue)
                {
                    *Value = NewValue;
                    ApplyWaveguideParameters();
                })
                .OnMouseCaptureEnd_Lambda([this, Name]
                {
                    AuditionCurrentSound(FText::Format(
                        NSLOCTEXT("ResonanceForge", "WaveguideParameterAudition", "{0}调整完成"),
                        Name));
                })
                .SliderBarColor(Color)
                .SliderHandleColor(Color)
            ];
    };

    auto ModeParameterRow = [this](const int32 Parameter, const FText& Name, const FText& Detail, const FLinearColor& Color)
    {
        auto GetNormalized = [this, Parameter]()
        {
            if (!ActiveModes.IsValidIndex(SelectedModeIndex))
            {
                return 0.0f;
            }
            const FResonanceMode& Mode = ActiveModes[SelectedModeIndex];
            if (Parameter == 0)
            {
                return FMath::GetRangePct(FMath::Loge(100.0f), FMath::Loge(8000.0f), FMath::Loge(FMath::Clamp(Mode.FrequencyHz, 100.0f, 8000.0f)));
            }
            return Parameter == 1
                ? FMath::Clamp(Mode.Gain / 1.5f, 0.0f, 1.0f)
                : FMath::GetRangePct(0.03f, 3.0f, FMath::Clamp(Mode.DecaySeconds, 0.03f, 3.0f));
        };
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [SNew(STextBlock).Text(Name).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)
                [SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted)]
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(STextBlock).Text_Lambda([this, Parameter]
                {
                    if (Parameter == 0) return GetSelectedModeFrequencyText();
                    return Parameter == 1 ? GetSelectedModeGainText() : GetSelectedModeDecayText();
                }).ColorAndOpacity(Color).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 0)
            [
                SNew(SSlider)
                .Value_Lambda(GetNormalized)
                .OnValueChanged_Lambda([this, Parameter](const float NewValue)
                {
                    if (!ActiveModes.IsValidIndex(SelectedModeIndex)) return;
                    FResonanceMode& Mode = ActiveModes[SelectedModeIndex];
                    if (Parameter == 0)
                    {
                        Mode.FrequencyHz = FMath::Exp(FMath::Lerp(FMath::Loge(100.0f), FMath::Loge(8000.0f), NewValue));
                    }
                    else if (Parameter == 1)
                    {
                        Mode.Gain = NewValue * 1.5f;
                    }
                    else
                    {
                        Mode.DecaySeconds = FMath::Lerp(0.03f, 3.0f, NewValue);
                    }
                    ApplyModalModes(false, FText::GetEmpty());
                })
                .OnMouseCaptureEnd_Lambda([this, Name]
                {
                    AuditionCurrentSound(FText::Format(
                        NSLOCTEXT("ResonanceForge", "ModeParameterAudition", "共振齿「{0}」调整完成"), Name));
                })
                .SliderBarColor(Color)
                .SliderHandleColor(Color)
            ];
    };

    TSharedRef<SDockTab> Workbench = SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(Panel).Padding(0)
            [
                SAssignNew(WorkbenchScrollBox, SScrollBox)
                + SScrollBox::Slot()
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 16, 22, 12)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1.0f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "Title", "共振铸造台")).Font(FAppStyle::GetFontStyle(TEXT("HeadingMedium")))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "Subtitle", "把碰撞与弦振，锻造成能进入游戏的声音。")).ColorAndOpacity(Muted)]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                            [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "CaptureWorkbench", "导出工作台截图")).ContentPadding(FMargin(12, 8)).OnClicked_Raw(this, &FResonanceForgeEditorModule::CaptureWorkbenchScreenshot)]
                            + SHorizontalBox::Slot().AutoWidth()
                            [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "OpenMap", "打开试听场景")).ContentPadding(FMargin(14, 8)).OnClicked_Raw(this, &FResonanceForgeEditorModule::OpenDemoMap)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 12)
                    [
                        SAssignNew(FlowObjectAnchor, SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(PanelRaised).Padding(FMargin(12, 9))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)[SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetSelectionText)]
                            + SHorizontalBox::Slot().AutoWidth().Padding(12, 0, 10, 0).VAlign(VAlign_Center)
                            [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "ReadSelection", "读取当前选择")).OnClicked_Raw(this, &FResonanceForgeEditorModule::SyncFromSelection)]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                            [SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetWwiseStatusText).ColorAndOpacity(Glass)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 18)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 7)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1.0f)
                            [SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "SignalChain", "铸造声路")).Font(FAppStyle::GetFontStyle(TEXT("BoldFont"))).ColorAndOpacity(Cyan)]
                            + SHorizontalBox::Slot().AutoWidth()
                            [SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "SignalChainHint", "点击工位，工作台会带你到对应工具")).ColorAndOpacity(Muted)]
                        ]
                        + SVerticalBox::Slot().AutoHeight()
                        [
                            SNew(SBorder)
                            .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                            .BorderBackgroundColor(FLinearColor(0.010f, 0.014f, 0.014f, 1.0f))
                            .Padding(FMargin(1))
                            [
                                SNew(SResonanceForgeFlowRail)
                                .ActiveStation_Lambda([this]{ return ActiveFlowStation; })
                                .GuideText_Raw(this, &FResonanceForgeEditorModule::GetFlowGuideText)
                                .OnStationSelected(FOnResonanceFlowStationSelected::CreateRaw(this, &FResonanceForgeEditorModule::NavigateToFlowStation))
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 14)
                    [
                        SAssignNew(FlowOutputAnchor, SBorder)
                        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                        .BorderBackgroundColor(FLinearColor(0.025f, 0.032f, 0.031f, 1.0f))
                        .Padding(FMargin(12, 10))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(0.65f).VAlign(VAlign_Center).Padding(0, 0, 16, 0)
                            [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "ListenGate", "监听闸门"), NSLOCTEXT("ResonanceForge", "ListenGateDetail", "先单听链路，再用双路叠听检查层叠关系。"))]
                            + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 6, 0)
                            [ListenGateButton(EResonanceForgeListenMode::NativeOnly, NSLOCTEXT("ResonanceForge", "NativeGate", "原声炉"), NSLOCTEXT("ResonanceForge", "NativeGateDetail", "只听 UE 物理声源"), Wood)]
                            + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 6, 0)
                            [ListenGateButton(EResonanceForgeListenMode::WwiseOnly, NSLOCTEXT("ResonanceForge", "WwiseGate", "Wwise 出口"), NSLOCTEXT("ResonanceForge", "WwiseGateDetail", "只听 Event 与 RTPC"), Glass)]
                            + SHorizontalBox::Slot().FillWidth(1.0f)
                            [ListenGateButton(EResonanceForgeListenMode::Layered, NSLOCTEXT("ResonanceForge", "LayeredGate", "双路叠听"), NSLOCTEXT("ResonanceForge", "LayeredGateDetail", "检查原声与中间件层叠"), Cyan)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 12)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1.55f).Padding(0, 0, 7, 0)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[WorkspaceTitle(NSLOCTEXT("ResonanceForge", "Response", "声纹炉膛"), NSLOCTEXT("ResonanceForge", "ResponseDetail", "轮廓不是装饰：尖锐起伏代表离散模态，柔顺波瓣代表数字波导的反馈传播。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(FLinearColor::White).Padding(1)
                                [
                                    SNew(SResonanceForgeVisualizer)
                                    .ModelType_Lambda([this]{ return ActiveModel; })
                                    .PresetName_Lambda([this]{ return ActivePreset; })
                                    .Energy_Lambda([this]{ return PreviewEnergy; })
                                    .Brightness_Lambda([this]{ return PreviewBrightness; })
                                    .Size_Lambda([this]{ return PreviewSize; })
                                    .Sustain_Lambda([this]{ return WaveguideSustain; })
                                    .Damping_Lambda([this]{ return WaveguideDamping; })
                                    .Coupling_Lambda([this]{ return WaveguideCoupling; })
                                    .Pickup_Lambda([this]{ return WaveguidePickup; })
                                    .ExcitationType_Lambda([this]{ return WaveguideExcitation; })
                                    .HasReference_Lambda([this]{ return bHasReference; })
                                    .ReferenceModelType_Lambda([this]{ return ReferenceModel; })
                                    .ReferencePresetName_Lambda([this]{ return ReferencePreset; })
                                    .ReferenceEnergy_Lambda([this]{ return ReferenceEnergy; })
                                    .ReferenceBrightness_Lambda([this]{ return ReferenceBrightness; })
                                    .ReferenceSize_Lambda([this]{ return ReferenceSize; })
                                    .ReferenceSustain_Lambda([this]{ return ReferenceSustain; })
                                    .ReferenceDamping_Lambda([this]{ return ReferenceDamping; })
                                    .ReferenceCoupling_Lambda([this]{ return ReferenceCoupling; })
                                    .ReferencePickup_Lambda([this]{ return ReferencePickup; })
                                    .ReferenceExcitationType_Lambda([this]{ return ReferenceExcitation; })
                                ]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                                [SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetComparisonText).ColorAndOpacity(Muted)]
                                + SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
                                [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "PinReference", "钉住当前声纹")).OnClicked_Raw(this, &FResonanceForgeEditorModule::PinReference)]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                                [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "SwapReference", "交换并试听 A/B")).IsEnabled_Lambda([this]{ return bHasReference; }).OnClicked_Raw(this, &FResonanceForgeEditorModule::SwapAndPreviewReference)]
                                + SHorizontalBox::Slot().AutoWidth()
                                [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "ClearReference", "清除参考")).IsEnabled_Lambda([this]{ return bHasReference; }).OnClicked_Raw(this, &FResonanceForgeEditorModule::ClearReference)]
                            ]
                        ]
                        + SHorizontalBox::Slot().FillWidth(0.95f).Padding(7, 0, 0, 0)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[WorkspaceTitle(NSLOCTEXT("ResonanceForge", "Model", "这次要做哪一种声源？"), NSLOCTEXT("ResonanceForge", "ModelDetail", "点击模型即可换炉并试听；撞击体适合道具，波导弦适合有音高的声源。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 7)
                            [ModelButton(EResonanceModelType::ModalImpact, NSLOCTEXT("ResonanceForge", "Modal", "模态撞击体"), NSLOCTEXT("ResonanceForge", "ModalHelp", "钢、木、玻璃的离散共振峰"), Cyan)]
                            + SVerticalBox::Slot().AutoHeight()
                            [ModelButton(EResonanceModelType::WaveguideString, NSLOCTEXT("ResonanceForge", "String", "数字波导弦"), NSLOCTEXT("ResonanceForge", "StringHelp", "手势起振、延迟线传播与阻尼反馈"), Wood)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 4, 22, 12)
                    [
                        SAssignNew(FlowExcitationAnchor, SBorder)
                        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                        .BorderBackgroundColor(FLinearColor(0.030f, 0.040f, 0.036f, 1.0f))
                        .Padding(FMargin(14, 12))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(0.82f).VAlign(VAlign_Center)
                            [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "MidiPerformance", "演奏入口"), NSLOCTEXT("ResonanceForge", "MidiMapping", "Note On 起音  ·  Note Off 收弓  ·  CC1 推拉弓压与明亮度"))]
                            + SHorizontalBox::Slot().FillWidth(1.15f).Padding(16, 0, 8, 0).VAlign(VAlign_Center)
                            [
                                SAssignNew(MidiDeviceCombo, SComboBox<TSharedPtr<FString>>)
                                .OptionsSource(&MidiDeviceOptions)
                                .InitiallySelectedItem(SelectedMidiDevice)
                                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                                {
                                    return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : TEXT("未知设备")));
                                })
                                .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
                                {
                                    SelectedMidiDevice = Item;
                                })
                                [
                                    SNew(STextBlock).Text_Lambda([this]
                                    {
                                        return FText::FromString(SelectedMidiDevice.IsValid() ? *SelectedMidiDevice : TEXT("未发现 MIDI 输入"));
                                    })
                                ]
                            ]
                            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0).VAlign(VAlign_Center)
                            [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "RefreshMidi", "刷新")).OnClicked_Raw(this, &FResonanceForgeEditorModule::RefreshMidiDevices)]
                            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0).VAlign(VAlign_Center)
                            [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "ConnectMidi", "连接")).IsEnabled_Lambda([this]{ return SelectedMidiDevice.IsValid(); }).OnClicked_Raw(this, &FResonanceForgeEditorModule::ConnectSelectedMidiDevice)]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                            [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "DisconnectMidi", "断开")).OnClicked_Raw(this, &FResonanceForgeEditorModule::DisconnectMidiDevice)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(36, 0, 36, 12)
                    [SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetMidiStatusText).ColorAndOpacity(Glass)]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 12)
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                        .BorderBackgroundColor(FLinearColor(0.050f, 0.035f, 0.022f, 1.0f))
                        .Padding(FMargin(14, 12))
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f)
                                [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "AuditionKeybed", "试音键床"), NSLOCTEXT("ResonanceForge", "AuditionKeybedDetail", "横向选择音高，越靠下敲击力度越重；拖过锤键可连续演奏。"))]
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                                [SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetKeybedStatusText).ColorAndOpacity(Wood)]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                                .BorderBackgroundColor(FLinearColor(0.030f, 0.024f, 0.019f, 1.0f))
                                .Padding(FMargin(9, 7))
                                [
                                    SNew(SHorizontalBox)
                                    + SHorizontalBox::Slot().FillWidth(0.72f).VAlign(VAlign_Center).Padding(0, 0, 10, 0)
                                    [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "VelocityCam", "力度凸轮"), NSLOCTEXT("ResonanceForge", "VelocityCamDetail", "把键速压进一枚可见的机械曲线；同一次敲击，可以换一种手感。"))]
                                    + SHorizontalBox::Slot().FillWidth(0.58f).Padding(0, 0, 5, 0)
                                    [VelocityCurveButton(EResonanceVelocityCurve::SoftTouch, NSLOCTEXT("ResonanceForge", "VelocitySoft", "软触"), NSLOCTEXT("ResonanceForge", "VelocitySoftDetail", "抬升弱奏"), Wood)]
                                    + SHorizontalBox::Slot().FillWidth(0.58f).Padding(0, 0, 5, 0)
                                    [VelocityCurveButton(EResonanceVelocityCurve::Linear, NSLOCTEXT("ResonanceForge", "VelocityLinear", "线性"), NSLOCTEXT("ResonanceForge", "VelocityLinearDetail", "原样传递"), Steel)]
                                    + SHorizontalBox::Slot().FillWidth(0.58f).Padding(0, 0, 10, 0)
                                    [VelocityCurveButton(EResonanceVelocityCurve::HeavyHand, NSLOCTEXT("ResonanceForge", "VelocityHeavy", "重手"), NSLOCTEXT("ResonanceForge", "VelocityHeavyDetail", "压低轻奏"), Glass)]
                                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                                    [
                                        SNew(SVerticalBox)
                                        + SVerticalBox::Slot().AutoHeight()
                                        [SNew(SResonanceVelocityCam).InputVelocity_Lambda([this]{ return LastKeybedVelocity; }).Curve_Lambda([this]{ return VelocityCurve; })]
                                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 2, 2, 0)
                                        [SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetVelocityMappingText).ColorAndOpacity(Wood)]
                                    ]
                                ]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                                .BorderBackgroundColor(FLinearColor(0.020f, 0.017f, 0.014f, 1.0f))
                                .Padding(FMargin(8, 6))
                                [
                                    SNew(SResonanceKeybed)
                                    .LastNote_Lambda([this]{ return LastKeybedNote; })
                                    .LastVelocity_Lambda([this]{ return LastKeybedVelocity; })
                                    .NoteGlow_Raw(this, &FResonanceForgeEditorModule::GetKeybedGlow)
                                    .OnNotePlayed(FOnResonanceKeyPlayed::CreateRaw(this, &FResonanceForgeEditorModule::TriggerKeybedNote))
                                ]
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 2, 22, 12)
                    [
                        SAssignNew(FlowWaveguideAnchor, SBorder)
                        .Visibility_Lambda([this]
                        {
                            return ActiveModel == EResonanceModelType::WaveguideString ? EVisibility::Visible : EVisibility::Collapsed;
                        })
                        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                        .BorderBackgroundColor(FLinearColor(0.058f, 0.031f, 0.018f, 1.0f))
                        .Padding(FMargin(14, 12))
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()
                                [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "StringBed", "弦床"), NSLOCTEXT("ResonanceForge", "StringBedDetail", "上方拖起振锤，下方拖拾音梭；松手试听一次，比较力从哪里进入、声音从哪里离开。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                                .BorderBackgroundColor(FLinearColor(0.030f, 0.024f, 0.019f, 1.0f))
                                .Padding(FMargin(9, 7))
                                [
                                    SNew(SHorizontalBox)
                                    + SHorizontalBox::Slot().FillWidth(0.58f).VAlign(VAlign_Center).Padding(0, 0, 12, 0)
                                    [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "ExcitationRack", "起振手势架"), NSLOCTEXT("ResonanceForge", "ExcitationRackDetail", "同一根弦，换一种进入弦路的力。"))]
                                    + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 5, 0)
                                    [ExcitationGestureButton(EResonanceExcitationType::Finger, NSLOCTEXT("ResonanceForge", "FingerGesture", "指腹"), NSLOCTEXT("ResonanceForge", "FingerGestureDetail", "柔和位移"), Wood)]
                                    + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 5, 0)
                                    [ExcitationGestureButton(EResonanceExcitationType::Pick, NSLOCTEXT("ResonanceForge", "PickGesture", "拨片"), NSLOCTEXT("ResonanceForge", "PickGestureDetail", "噪声脉冲"), Steel)]
                                    + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 5, 0)
                                    [ExcitationGestureButton(EResonanceExcitationType::Hammer, NSLOCTEXT("ResonanceForge", "HammerGesture", "锤击"), NSLOCTEXT("ResonanceForge", "HammerGestureDetail", "局部冲击"), Glass)]
                                    + SHorizontalBox::Slot().FillWidth(1.0f)
                                    [ExcitationGestureButton(EResonanceExcitationType::Bow, NSLOCTEXT("ResonanceForge", "BowGesture", "弓擦"), NSLOCTEXT("ResonanceForge", "BowGestureDetail", "持续摩擦"), FLinearColor(0.83f, 0.28f, 0.12f, 1.0f))]
                                ]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 8, 0)
                                [WaveguideParameterRow(NSLOCTEXT("ResonanceForge", "Sustain", "回响长度"), NSLOCTEXT("ResonanceForge", "SustainDetail", "反馈保留"), &WaveguideSustain, Wood)]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(8, 0)
                                [WaveguideParameterRow(NSLOCTEXT("ResonanceForge", "Damping", "弦路阻尼"), NSLOCTEXT("ResonanceForge", "DampingDetail", "高频耗散"), &WaveguideDamping, Steel)]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(8, 0, 0, 0)
                                [WaveguideParameterRow(NSLOCTEXT("ResonanceForge", "Coupling", "箱体耦合"), NSLOCTEXT("ResonanceForge", "CouplingDetail", "弦体传能"), &WaveguideCoupling, Glass)]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 15, 0, 0)
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                                .BorderBackgroundColor(FLinearColor(0.022f, 0.018f, 0.014f, 1.0f))
                                .Padding(FMargin(8, 5))
                                [
                                    SNew(SResonanceStringPath)
                                    .PickupPosition_Lambda([this]{ return WaveguidePickup; })
                                    .StrikePosition_Lambda([this]{ return PreviewStrikePosition; })
                                    .Sustain_Lambda([this]{ return WaveguideSustain; })
                                    .Damping_Lambda([this]{ return WaveguideDamping; })
                                    .Coupling_Lambda([this]{ return WaveguideCoupling; })
                                    .OnPickupChanged(FOnResonancePickupChanged::CreateRaw(this, &FResonanceForgeEditorModule::ApplyWaveguidePickup))
                                    .OnStrikeChanged(FOnResonanceStringStrikeChanged::CreateRaw(this, &FResonanceForgeEditorModule::ApplyStrikePosition))
                                ]
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 2, 22, 12)
                    [
                        SAssignNew(FlowModalAnchor, SBorder)
                        .Visibility_Lambda([this]
                        {
                            return ActiveModel == EResonanceModelType::ModalImpact ? EVisibility::Visible : EVisibility::Collapsed;
                        })
                        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                        .BorderBackgroundColor(FLinearColor(0.038f, 0.050f, 0.048f, 1.0f))
                        .Padding(FMargin(14, 12))
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f)
                                [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "StrikeRail", "落点划线规"), NSLOCTEXT("ResonanceForge", "StrikeRailDetail", "拖动铜色锤头选择敲击位置；下方短齿显示这一落点会激起哪些模态。"))]
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                                [SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetStrikePositionText).ColorAndOpacity(Wood).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 16)
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                                .BorderBackgroundColor(FLinearColor(0.026f, 0.021f, 0.017f, 1.0f))
                                .Padding(FMargin(8, 4))
                                [
                                    SNew(SResonanceStrikeRail)
                                    .StrikePosition_Lambda([this]{ return PreviewStrikePosition; })
                                    .LiveImpactPosition_Lambda([this]{ return LiveImpactPosition; })
                                    .LiveImpactGlow_Raw(this, &FResonanceForgeEditorModule::GetLiveImpactGlow)
                                    .ModeCount_Lambda([this]{ return ActiveModes.Num(); })
                                    .OnPositionChanged(FOnStrikeRailChanged::CreateRaw(this, &FResonanceForgeEditorModule::ApplyStrikePosition))
                                ]
                            ]
                            + SVerticalBox::Slot().AutoHeight()
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f)
                                [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "ModeRack", "共振齿列"), NSLOCTEXT("ResonanceForge", "ModeRackDetail", "点击一根齿把它夹住；高度是重量，短尾是余响，横向位置是频率。"))]
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                                [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "ResetModeRack", "按材质重新排齿")).OnClicked_Lambda([this]{ ResetModalModesToPreset(); return FReply::Handled(); })]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 2)
                            [
                                SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                                .BorderBackgroundColor(FLinearColor(0.018f, 0.025f, 0.025f, 1.0f))
                                .Padding(FMargin(8, 4))
                                [
                                    SNew(SResonanceModeRack)
                                    .Modes_Lambda([this]{ return ActiveModes; })
                                    .SelectedMode_Lambda([this]{ return SelectedModeIndex; })
                                    .OnModeSelected(FOnResonanceModeSelected::CreateRaw(this, &FResonanceForgeEditorModule::SelectModalMode))
                                ]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 10, 0)
                                [ModeParameterRow(0, NSLOCTEXT("ResonanceForge", "ModeFrequency", "齿位"), NSLOCTEXT("ResonanceForge", "ModeFrequencyDetail", "频率"), Wood)]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(10, 0)
                                [ModeParameterRow(1, NSLOCTEXT("ResonanceForge", "ModeWeight", "齿重"), NSLOCTEXT("ResonanceForge", "ModeWeightDetail", "响度权重"), Steel)]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(10, 0, 0, 0)
                                [ModeParameterRow(2, NSLOCTEXT("ResonanceForge", "ModeTail", "余响"), NSLOCTEXT("ResonanceForge", "ModeTailDetail", "衰减时间"), Glass)]
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 4, 22, 12)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 10, 0)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[WorkspaceTitle(NSLOCTEXT("ResonanceForge", "Material", "给对象一种听感"), NSLOCTEXT("ResonanceForge", "MaterialDetail", "点击预设会同步表面、共振峰与衰减，并立即试听。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 5, 0)[PresetButton(TEXT("拉丝钢"), NSLOCTEXT("ResonanceForge", "Steel", "拉丝钢"), Steel)]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(5, 0)[PresetButton(TEXT("硬木"), NSLOCTEXT("ResonanceForge", "Wood", "硬木"), Wood)]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(5, 0, 0, 0)[PresetButton(TEXT("薄玻璃"), NSLOCTEXT("ResonanceForge", "Glass", "薄玻璃"), Glass)]
                            ]
                        ]
                        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(10, 0, 0, 0)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[WorkspaceTitle(NSLOCTEXT("ResonanceForge", "Performance", "塑造这一次发声"), NSLOCTEXT("ResonanceForge", "PerformanceDetail", "拖动时看声纹与出口刻度，松手试听一次；同一数值发送给 Wwise。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)[ParameterRow(NSLOCTEXT("ResonanceForge", "Energy", "激励能量"), FText::FromString(TEXT("RF_ImpactEnergy")), &PreviewEnergy, Steel)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)[ParameterRow(NSLOCTEXT("ResonanceForge", "Brightness", "明亮度"), FText::FromString(TEXT("RF_ImpactBrightness")), &PreviewBrightness, Glass)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)[ParameterRow(NSLOCTEXT("ResonanceForge", "Size", "共振尺度"), FText::FromString(TEXT("RF_ObjectSize")), &PreviewSize, Wood)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 4, 22, 12)
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                        .BorderBackgroundColor(FLinearColor(0.050f, 0.032f, 0.018f, 1.0f))
                        .Padding(FMargin(14, 12))
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()
                            [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "StrikeScale", "锤击标尺"), NSLOCTEXT("ResonanceForge", "StrikeScaleDetail", "轻触、常规、重击会重设能量与明亮度并立即试听；共振尺度保持不变。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 5, 0)
                                [
                                    SNew(SButton)
                                    .Text(NSLOCTEXT("ResonanceForge", "StrikeLight", "轻触  ·  克制起音"))
                                    .ContentPadding(FMargin(12, 10))
                                    .ButtonColorAndOpacity(FLinearColor(0.12f, 0.20f, 0.22f, 1.0f))
                                    .OnClicked_Lambda([this]{ return TriggerStrikePreset(0.22f, 0.30f, NSLOCTEXT("ResonanceForge", "StrikeLightStatus", "轻触")); })
                                ]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(5, 0)
                                [
                                    SNew(SButton)
                                    .Text(NSLOCTEXT("ResonanceForge", "StrikeRegular", "常规  ·  清晰主体"))
                                    .ContentPadding(FMargin(12, 10))
                                    .ButtonColorAndOpacity(FLinearColor(0.22f, 0.19f, 0.12f, 1.0f))
                                    .OnClicked_Lambda([this]{ return TriggerStrikePreset(0.58f, 0.55f, NSLOCTEXT("ResonanceForge", "StrikeRegularStatus", "常规锤击")); })
                                ]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(5, 0, 0, 0)
                                [
                                    SNew(SButton)
                                    .Text(NSLOCTEXT("ResonanceForge", "StrikeHeavy", "重击  ·  明亮爆发"))
                                    .ContentPadding(FMargin(12, 10))
                                    .ButtonColorAndOpacity(FLinearColor(0.34f, 0.12f, 0.045f, 1.0f))
                                    .OnClicked_Lambda([this]{ return TriggerStrikePreset(0.92f, 0.80f, NSLOCTEXT("ResonanceForge", "StrikeHeavyStatus", "重击")); })
                                ]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 0)
                            [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "WwiseOutputScale", "Wwise 出口刻度"), NSLOCTEXT("ResonanceForge", "WwiseOutputScaleDetail", "基于已写入曲线控制点的近似读数；实际输出由 Wwise 运行时求值。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 10, 0)
                                [OutputReading(NSLOCTEXT("ResonanceForge", "WwiseVolumeLabel", "RF_ImpactEnergy → Volume"), TAttribute<FText>::CreateRaw(this, &FResonanceForgeEditorModule::GetWwiseVolumeText), Steel)]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(10, 0)
                                [OutputReading(NSLOCTEXT("ResonanceForge", "WwiseLowpassLabel", "RF_ImpactBrightness → Low-pass"), TAttribute<FText>::CreateRaw(this, &FResonanceForgeEditorModule::GetWwiseLowpassText), Glass)]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(10, 0, 0, 0)
                                [OutputReading(NSLOCTEXT("ResonanceForge", "WwisePitchLabel", "RF_ObjectSize → Pitch"), TAttribute<FText>::CreateRaw(this, &FResonanceForgeEditorModule::GetWwisePitchText), Wood)]
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 4, 22, 12)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()
                        [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "RecipeShelf", "配方架"), NSLOCTEXT("ResonanceForge", "RecipeShelfDetail", "把顺手的声纹存进三个本地槽位；关闭编辑器后仍可召回，不会污染团队资产。"))]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 6, 0)[RecipeSlot(0, Steel)]
                            + SHorizontalBox::Slot().FillWidth(1.0f).Padding(6, 0)[RecipeSlot(1, Wood)]
                            + SHorizontalBox::Slot().FillWidth(1.0f).Padding(6, 0, 0, 0)[RecipeSlot(2, Glass)]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0).VAlign(VAlign_Center)
                            [SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "SharedRecipeLabel", "团队共享配方")).ColorAndOpacity(Muted)]
                            + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 8, 0)
                            [
                                SNew(SEditableTextBox)
                                .Text_Lambda([this]{ return FText::FromString(SharedRecipeName); })
                                .HintText(NSLOCTEXT("ResonanceForge", "SharedRecipeHint", "例如：短促铜片"))
                                .OnTextChanged_Lambda([this](const FText& Text){ SharedRecipeName = Text.ToString(); })
                            ]
                            + SHorizontalBox::Slot().AutoWidth()
                            [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "ForgeSharedRecipe", "铸印为 Content 资产")).OnClicked_Raw(this, &FResonanceForgeEditorModule::ForgeSharedRecipeAsset)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 4, 22, 12)
                    [
                        SAssignNew(FlowSampleAnchor, SBorder)
                        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                        .BorderBackgroundColor(FLinearColor(0.045f, 0.030f, 0.018f, 1.0f))
                        .Padding(FMargin(14, 12))
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()
                            [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "SampleForge", "铸样台"), NSLOCTEXT("ResonanceForge", "SampleForgeDetail", "把当前物理声源离线锻成 WAV，并附一张记录配方与 Wwise 路由的声源铭牌。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SResonanceDecayPrint)
                                .Envelope(&LastSampleEnvelope)
                                .TailDb_Lambda([this]{ return LastSampleTailDb; })
                                .DurationSeconds_Lambda([this]{ return LastSampleDurationSeconds; })
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
                            [
                                SNew(STextBlock)
                                .Text_Raw(this, &FResonanceForgeEditorModule::GetSampleTailStatusText)
                                .ColorAndOpacity_Lambda([this]{ return LastSampleTailDb <= -48.0f ? ResonanceForgeEditor::Glass : ResonanceForgeEditor::Wood; })
                                .AutoWrapText(true)
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 8, 0)
                                [
                                    SNew(SEditableTextBox)
                                    .Text_Lambda([this]{ return FText::FromString(SampleExportName); })
                                    .HintText(NSLOCTEXT("ResonanceForge", "SampleNameHint", "例如：RF_WoodString_G3"))
                                    .OnTextChanged_Lambda([this](const FText& Text){ SampleExportName = Text.ToString(); })
                                ]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
                                [SNew(SButton).Text(FText::FromString(TEXT("1.5 秒"))).ButtonColorAndOpacity_Lambda([this]{ return FMath::IsNearlyEqual(SampleExportDurationSeconds, 1.5f) ? ResonanceForgeEditor::Wood * 0.55f : FLinearColor::White; }).OnClicked_Lambda([this]{ SampleExportDurationSeconds = 1.5f; return FReply::Handled(); })]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
                                [SNew(SButton).Text(FText::FromString(TEXT("3 秒"))).ButtonColorAndOpacity_Lambda([this]{ return FMath::IsNearlyEqual(SampleExportDurationSeconds, 3.0f) ? ResonanceForgeEditor::Wood * 0.55f : FLinearColor::White; }).OnClicked_Lambda([this]{ SampleExportDurationSeconds = 3.0f; return FReply::Handled(); })]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                                [SNew(SButton).Text(FText::FromString(TEXT("6 秒"))).ButtonColorAndOpacity_Lambda([this]{ return FMath::IsNearlyEqual(SampleExportDurationSeconds, 6.0f) ? ResonanceForgeEditor::Wood * 0.55f : FLinearColor::White; }).OnClicked_Lambda([this]{ SampleExportDurationSeconds = 6.0f; return FReply::Handled(); })]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
                                [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "ExportSample", "铸成 WAV")).OnClicked_Raw(this, &FResonanceForgeEditorModule::ExportCurrentSample)]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
                                [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "ReforgeSample", "回炉最近铭牌")).OnClicked_Raw(this, &FResonanceForgeEditorModule::ReforgeLatestSampleLabel)]
                                + SHorizontalBox::Slot().AutoWidth()
                                [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "RevealSample", "打开目录")).OnClicked_Raw(this, &FResonanceForgeEditorModule::RevealSampleExport)]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 9, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f)
                                [SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetSampleExportStatusText).ColorAndOpacity(Glass)]
                                + SHorizontalBox::Slot().AutoWidth()
                                [SNew(STextBlock).Text(FText::FromString(TEXT("48 kHz · 16-bit · stereo · Saved/ResonanceForge/Exports"))).ColorAndOpacity(Muted)]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
                            [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "LabelRack", "铭牌架"), NSLOCTEXT("ResonanceForge", "LabelRackDetail", "最近三份铸样自动上架；铭牌写着声音身份，点击即可把那一版送回炉膛。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 5, 0)
                                [
                                    SNew(SButton)
                                    .Text_Lambda([this]{ return GetRecentSampleLabelText(0); })
                                    .IsEnabled_Lambda([this]{ return HasRecentSampleLabel(0); })
                                    .ContentPadding(FMargin(10, 8))
                                    .ButtonColorAndOpacity(ResonanceForgeEditor::Steel * 0.44f)
                                    .OnClicked_Lambda([this]{ return ReforgeRecentSampleLabel(0); })
                                ]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(5, 0)
                                [
                                    SNew(SButton)
                                    .Text_Lambda([this]{ return GetRecentSampleLabelText(1); })
                                    .IsEnabled_Lambda([this]{ return HasRecentSampleLabel(1); })
                                    .ContentPadding(FMargin(10, 8))
                                    .ButtonColorAndOpacity(ResonanceForgeEditor::Wood * 0.44f)
                                    .OnClicked_Lambda([this]{ return ReforgeRecentSampleLabel(1); })
                                ]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(5, 0, 0, 0)
                                [
                                    SNew(SButton)
                                    .Text_Lambda([this]{ return GetRecentSampleLabelText(2); })
                                    .IsEnabled_Lambda([this]{ return HasRecentSampleLabel(2); })
                                    .ContentPadding(FMargin(10, 8))
                                    .ButtonColorAndOpacity(ResonanceForgeEditor::Glass * 0.44f)
                                    .OnClicked_Lambda([this]{ return ReforgeRecentSampleLabel(2); })
                                ]
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 4, 22, 12)
                    [
                        SNew(SButton)
                        .Text_Raw(this, &FResonanceForgeEditorModule::GetPrimaryActionText)
                        .ContentPadding(FMargin(14, 12))
                        .ButtonColorAndOpacity(Cyan * 0.58f)
                        .OnClicked_Raw(this, &FResonanceForgeEditorModule::TriggerPreview)
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 20)
                    [
                        SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(FLinearColor(0.015f, 0.09f, 0.11f, 1.0f)).Padding(FMargin(12, 9))
                        [SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetStatusText).ColorAndOpacity(Glass)]
                    ]
                ]
            ]
        ];
    WorkbenchWidget = Workbench->GetContent();
    return Workbench;
}

IMPLEMENT_MODULE(FResonanceForgeEditorModule, ResonanceForgeEditor)
