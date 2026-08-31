#include "ResonanceForgeEditorModule.h"
#include "SResonanceForgeVisualizer.h"

#include "Editor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "ImageUtils.h"
#include "Engine/Selection.h"
#include "Framework/Docking/TabManager.h"
#include "Materials/MaterialInterface.h"
#include "MIDIDeviceManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
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

    TSharedRef<SWidget> WorkspaceTitle(const FText& Title, const FText& Detail)
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
            [SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted).AutoWrapText(true)];
    }

    TSharedRef<SWidget> RecipeStage(const FText& Title, const TAttribute<FText>& Detail, const FLinearColor& Accent)
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor(Accent).Padding(FMargin(0, 2))]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
            [SNew(STextBlock).Text(Title).ColorAndOpacity(Accent).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
            [SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted).AutoWrapText(true)];
    }
}

void FResonanceForgeEditorModule::StartupModule()
{
    LastStatus = NSLOCTEXT("ResonanceForge", "Ready", "已就绪 · 选择场景中的共振体开始工作");
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
        ApplyWaveguideParameters();
    }
    FGlobalTabmanager::Get()->TryInvokeTab(ResonanceForgeEditor::TabName);

    FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this](float)
        {
            CaptureWorkbenchScreenshot();
            if (WorkbenchScrollBox.IsValid())
            {
                WorkbenchScrollBox->ScrollToEnd();
            }
            FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateLambda([this](float)
                {
                    CaptureWorkbenchImage(TEXT("resonance-forge-workbench-details.png"));
                    if (FParse::Param(FCommandLine::Get(), TEXT("ResonanceForgeCaptureAndExit")))
                    {
                        FPlatformMisc::RequestExit(false);
                    }
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

    UWorld* World = GEditor->PlayWorld ? GEditor->PlayWorld.Get() : GEditor->GetEditorWorldContext().World();
    if (World)
    {
        for (TActorIterator<AResonanceForgeImpactInstrumentActor> It(World); It; ++It)
        {
            return *It;
        }
    }
    return nullptr;
}

void FResonanceForgeEditorModule::ApplyPreset(const FName PresetName)
{
    ActivePreset = PresetName;
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
    Instrument->MarkPackageDirty();
    LastStatus = FText::Format(NSLOCTEXT("ResonanceForge", "Applied", "已应用「{0}」· 视觉材质与共振预设同步"), FText::FromName(PresetName));
}

void FResonanceForgeEditorModule::ApplyModel(const EResonanceModelType ModelType)
{
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
        Instrument->NativeSynth->StringDecay = FMath::Lerp(
            ResonanceForgeEditor::WaveguideDecayMin,
            ResonanceForgeEditor::WaveguideDecayMax,
            FMath::Clamp(WaveguideSustain, 0.0f, 1.0f));
        Instrument->NativeSynth->StringDamping = FMath::Clamp(WaveguideDamping, 0.0f, 1.0f);
        Instrument->NativeSynth->BodyCoupling = FMath::Clamp(WaveguideCoupling, 0.0f, 1.0f);
    }
}

FReply FResonanceForgeEditorModule::SyncFromSelection()
{
    if (const AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        const UResonanceMaterialProfile* SharedProfile = Instrument->NativeSynth ? Instrument->NativeSynth->MaterialProfile : nullptr;
        ActiveModel = SharedProfile ? SharedProfile->ModelType : Instrument->SynthesisModel;
        ActivePreset = SharedProfile ? SharedProfile->SourcePreset : Instrument->ResonancePreset;
        PreviewSize = Instrument->ObjectSize;
        if (Instrument->NativeSynth)
        {
            WaveguideSustain = FMath::GetRangePct(
                ResonanceForgeEditor::WaveguideDecayMin,
                ResonanceForgeEditor::WaveguideDecayMax,
                Instrument->NativeSynth->StringDecay);
            WaveguideDamping = Instrument->NativeSynth->StringDamping;
            WaveguideCoupling = Instrument->NativeSynth->BodyCoupling;
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
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->ObjectSize = PreviewSize;
        ApplyWaveguideParameters();
        Instrument->TriggerInstrument(PreviewEnergy, PreviewBrightness, 60);
        LastStatus = FText::Format(
            NSLOCTEXT("ResonanceForge", "Triggered", "已触发 {0} · Energy {1}% / Brightness {2}% / Size {3}%"),
            FText::FromName(Instrument->ResonancePreset),
            FText::AsNumber(FMath::RoundToInt(PreviewEnergy * 100.0f)),
            FText::AsNumber(FMath::RoundToInt(PreviewBrightness * 100.0f)),
            FText::AsNumber(FMath::RoundToInt(PreviewSize * 100.0f)));
    }
    else
    {
        LastStatus = NSLOCTEXT("ResonanceForge", "PreviewMissing", "无法预听 · 场景中没有可用的共振体");
    }
    return FReply::Handled();
}

FReply FResonanceForgeEditorModule::TriggerStrikePreset(
    const float Energy,
    const float Brightness,
    const FText& GestureName)
{
    PreviewEnergy = FMath::Clamp(Energy, 0.0f, 1.0f);
    PreviewBrightness = FMath::Clamp(Brightness, 0.0f, 1.0f);

    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->ObjectSize = PreviewSize;
        ApplyWaveguideParameters();
        Instrument->TriggerInstrument(PreviewEnergy, PreviewBrightness, 60);
        LastStatus = FText::Format(
            NSLOCTEXT("ResonanceForge", "StrikePresetTriggered", "{0}已触发 · {1} / {2} / {3}"),
            GestureName,
            GetWwiseVolumeText(),
            GetWwiseLowpassText(),
            GetWwisePitchText());
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
    ReferenceSustain = WaveguideSustain;
    ReferenceDamping = WaveguideDamping;
    ReferenceCoupling = WaveguideCoupling;
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
    const float PreviousSustain = WaveguideSustain;
    const float PreviousDamping = WaveguideDamping;
    const float PreviousCoupling = WaveguideCoupling;

    ActivePreset = ReferencePreset;
    ActiveModel = ReferenceModel;
    PreviewEnergy = ReferenceEnergy;
    PreviewBrightness = ReferenceBrightness;
    PreviewSize = ReferenceSize;
    WaveguideSustain = ReferenceSustain;
    WaveguideDamping = ReferenceDamping;
    WaveguideCoupling = ReferenceCoupling;

    ReferencePreset = PreviousPreset;
    ReferenceModel = PreviousModel;
    ReferenceEnergy = PreviousEnergy;
    ReferenceBrightness = PreviousBrightness;
    ReferenceSize = PreviousSize;
    ReferenceSustain = PreviousSustain;
    ReferenceDamping = PreviousDamping;
    ReferenceCoupling = PreviousCoupling;

    ApplyModel(ActiveModel);
    ApplyPreset(ActivePreset);
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
    float& OutCoupling) const
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
    GConfig->GetString(*Section, *(Prefix + TEXT(".Preset")), PresetString, GEditorPerProjectIni);
    GConfig->GetInt(*Section, *(Prefix + TEXT(".Model")), ModelValue, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Energy")), OutEnergy, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Brightness")), OutBrightness, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Size")), OutSize, GEditorPerProjectIni);
    OutSustain = 0.90f;
    OutDamping = 0.36f;
    OutCoupling = 0.22f;
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Sustain")), OutSustain, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Damping")), OutDamping, GEditorPerProjectIni);
    GConfig->GetFloat(*Section, *(Prefix + TEXT(".Coupling")), OutCoupling, GEditorPerProjectIni);
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
    return ReadRecipeSlot(SlotIndex, Preset, Model, Energy, Brightness, Size, Sustain, Damping, Coupling);
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
    if (!ReadRecipeSlot(SlotIndex, Preset, Model, Energy, Brightness, Size, Sustain, Damping, Coupling))
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
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".Sustain")), WaveguideSustain, GEditorPerProjectIni);
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".Damping")), WaveguideDamping, GEditorPerProjectIni);
    GConfig->SetFloat(*Section, *(Prefix + TEXT(".Coupling")), WaveguideCoupling, GEditorPerProjectIni);
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
    if (!ReadRecipeSlot(SlotIndex, Preset, Model, Energy, Brightness, Size, Sustain, Damping, Coupling))
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
    ApplyModel(ActiveModel);
    ApplyPreset(ActivePreset);
    ApplyWaveguideParameters();
    if (AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        Instrument->ObjectSize = PreviewSize;
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
        ? NSLOCTEXT("ResonanceForge", "MidiConnected", "MIDI 已连接 · Note On 控制音高与力度，CC1 塑造明亮度")
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
        return FText::Format(
            NSLOCTEXT("ResonanceForge", "MidiLiveActivity", "{0} · Note {1} / Velocity {2} · CC1 {3}"),
            FText::FromString(Instrument->GetConnectedMidiDeviceName()),
            FText::AsNumber(Instrument->LastMidiNote),
            FText::AsNumber(Instrument->LastMidiVelocity),
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
        ? NSLOCTEXT("ResonanceForge", "WwiseReady", "Wwise 已就绪 · 1 Event / 3 RTPC")
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
    Profile->Modes = UResonanceForgeSynthComponent::GetBuiltInModes(ActivePreset);
    Profile->StringDecay = FMath::Lerp(
        ResonanceForgeEditor::WaveguideDecayMin,
        ResonanceForgeEditor::WaveguideDecayMax,
        FMath::Clamp(WaveguideSustain, 0.0f, 1.0f));
    Profile->StringDamping = FMath::Clamp(WaveguideDamping, 0.0f, 1.0f);
    Profile->BodyCoupling = FMath::Clamp(WaveguideCoupling, 0.0f, 1.0f);

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

FText FResonanceForgeEditorModule::GetExcitationText() const
{
    return ActiveModel == EResonanceModelType::WaveguideString
        ? NSLOCTEXT("ResonanceForge", "StringExcitation", "拨弦噪声 / MIDI 力度")
        : NSLOCTEXT("ResonanceForge", "ImpactExcitation", "物理碰撞 / 冲量与速度");
}

FText FResonanceForgeEditorModule::GetResonanceText() const
{
    return ActiveModel == EResonanceModelType::WaveguideString
        ? NSLOCTEXT("ResonanceForge", "StringResonance", "延迟线传播 / 阻尼反馈")
        : FText::Format(NSLOCTEXT("ResonanceForge", "ImpactResonance", "{0} / 离散模态组"), FText::FromName(ActivePreset));
}

FText FResonanceForgeEditorModule::GetPrimaryActionText() const
{
    return ActiveModel == EResonanceModelType::WaveguideString
        ? NSLOCTEXT("ResonanceForge", "PluckNow", "拨动当前弦")
        : NSLOCTEXT("ResonanceForge", "StrikeNow", "敲击当前对象");
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
    const int32 SustainDelta = FMath::RoundToInt((WaveguideSustain - ReferenceSustain) * 100.0f);
    const int32 DampingDelta = FMath::RoundToInt((WaveguideDamping - ReferenceDamping) * 100.0f);
    const int32 CouplingDelta = FMath::RoundToInt((WaveguideCoupling - ReferenceCoupling) * 100.0f);
    const auto SignedPercent = [](int32 Value)
    {
        return FText::FromString(FString::Printf(TEXT("%+d"), Value));
    };
    if (ActiveModel == EResonanceModelType::WaveguideString || ReferenceModel == EResonanceModelType::WaveguideString)
    {
        return FText::Format(
            NSLOCTEXT("ResonanceForge", "WaveguideReferenceDifference", "参考「{0}」 · 延音 {1}%  阻尼 {2}%  耦合 {3}%"),
            FText::FromName(ReferencePreset), SignedPercent(SustainDelta), SignedPercent(DampingDelta), SignedPercent(CouplingDelta));
    }
    return FText::Format(
        NSLOCTEXT("ResonanceForge", "ReferenceDifference", "参考「{0}」 · 能量 {1}%  明亮 {2}%  尺度 {3}%"),
        FText::FromName(ReferencePreset),
        SignedPercent(EnergyDelta),
        SignedPercent(BrightnessDelta),
        SignedPercent(SizeDelta));
}

TSharedRef<SDockTab> FResonanceForgeEditorModule::SpawnWorkbench(const FSpawnTabArgs& Args)
{
    using namespace ResonanceForgeEditor;

    if (const AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        const UResonanceMaterialProfile* SharedProfile = Instrument->NativeSynth ? Instrument->NativeSynth->MaterialProfile : nullptr;
        ActiveModel = SharedProfile ? SharedProfile->ModelType : Instrument->SynthesisModel;
        ActivePreset = SharedProfile ? SharedProfile->SourcePreset : Instrument->ResonancePreset;
        PreviewSize = Instrument->ObjectSize;
        if (Instrument->NativeSynth)
        {
            WaveguideSustain = FMath::GetRangePct(
                ResonanceForgeEditor::WaveguideDecayMin,
                ResonanceForgeEditor::WaveguideDecayMax,
                Instrument->NativeSynth->StringDecay);
            WaveguideDamping = Instrument->NativeSynth->StringDamping;
            WaveguideCoupling = Instrument->NativeSynth->BodyCoupling;
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
                return FReply::Handled();
            })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted).AutoWrapText(true)]
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
                return FReply::Handled();
            });
    };

    auto ParameterRow = [](const FText& Name, const FText& Mapping, float* Value, const FLinearColor& Color)
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
                SNew(SSlider).Value_Lambda([Value]{ return *Value; }).OnValueChanged_Lambda([Value](float NewValue){ *Value = NewValue; })
                .SliderBarColor(Color).SliderHandleColor(Color)
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
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "Subtitle", "把碰撞与拨弦，锻造成能进入游戏的声音。")).ColorAndOpacity(Muted)]
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
                        SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(PanelRaised).Padding(FMargin(12, 9))
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
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                        [SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "SignalChain", "这件声音的配方")).Font(FAppStyle::GetFontStyle(TEXT("BoldFont"))).ColorAndOpacity(Cyan)]
                        + SVerticalBox::Slot().AutoHeight()
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1.0f)
                            [RecipeStage(NSLOCTEXT("ResonanceForge", "ObjectNode", "对象"), TAttribute<FText>::CreateLambda([this]{ return GetSelectionText(); }), Steel)]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
                            [SNew(STextBlock).Text(FText::FromString(TEXT("›"))).Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall"))).ColorAndOpacity(Muted)]
                            + SHorizontalBox::Slot().FillWidth(0.82f)
                            [RecipeStage(NSLOCTEXT("ResonanceForge", "ExciterNode", "激励"), TAttribute<FText>::CreateLambda([this]{ return GetExcitationText(); }), Wood)]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
                            [SNew(STextBlock).Text(FText::FromString(TEXT("›"))).Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall"))).ColorAndOpacity(Muted)]
                            + SHorizontalBox::Slot().FillWidth(0.82f)
                            [RecipeStage(NSLOCTEXT("ResonanceForge", "ResonatorNode", "共振"), TAttribute<FText>::CreateLambda([this]{ return GetResonanceText(); }), Glass)]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
                            [SNew(STextBlock).Text(FText::FromString(TEXT("›"))).Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall"))).ColorAndOpacity(Muted)]
                            + SHorizontalBox::Slot().FillWidth(0.95f)
                            [RecipeStage(NSLOCTEXT("ResonanceForge", "OutputNode", "出口"), FText::FromString(TEXT("UE 合成器 + Wwise Event / RTPC")), Cyan)]
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
                                    .HasReference_Lambda([this]{ return bHasReference; })
                                    .ReferenceModelType_Lambda([this]{ return ReferenceModel; })
                                    .ReferencePresetName_Lambda([this]{ return ReferencePreset; })
                                    .ReferenceEnergy_Lambda([this]{ return ReferenceEnergy; })
                                    .ReferenceBrightness_Lambda([this]{ return ReferenceBrightness; })
                                    .ReferenceSize_Lambda([this]{ return ReferenceSize; })
                                    .ReferenceSustain_Lambda([this]{ return ReferenceSustain; })
                                    .ReferenceDamping_Lambda([this]{ return ReferenceDamping; })
                                    .ReferenceCoupling_Lambda([this]{ return ReferenceCoupling; })
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
                            + SVerticalBox::Slot().AutoHeight()[WorkspaceTitle(NSLOCTEXT("ResonanceForge", "Model", "这次要做哪一种声源？"), NSLOCTEXT("ResonanceForge", "ModelDetail", "撞击体适合道具与环境音；波导弦适合有音高、可演奏的声源。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 7)
                            [ModelButton(EResonanceModelType::ModalImpact, NSLOCTEXT("ResonanceForge", "Modal", "模态撞击体"), NSLOCTEXT("ResonanceForge", "ModalHelp", "钢、木、玻璃的离散共振峰"), Cyan)]
                            + SVerticalBox::Slot().AutoHeight()
                            [ModelButton(EResonanceModelType::WaveguideString, NSLOCTEXT("ResonanceForge", "String", "数字波导弦"), NSLOCTEXT("ResonanceForge", "StringHelp", "噪声激励、延迟线传播与阻尼反馈"), Wood)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 4, 22, 12)
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                        .BorderBackgroundColor(FLinearColor(0.030f, 0.040f, 0.036f, 1.0f))
                        .Padding(FMargin(14, 12))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(0.82f).VAlign(VAlign_Center)
                            [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "MidiPerformance", "演奏入口"), NSLOCTEXT("ResonanceForge", "MidiMapping", "琴键 Note On → 音高与力度  ·  调制轮 CC1 → 明亮度"))]
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
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 2, 22, 12)
                    [
                        SNew(SBorder)
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
                            [WorkspaceTitle(NSLOCTEXT("ResonanceForge", "StringBed", "弦床"), NSLOCTEXT("ResonanceForge", "StringBedDetail", "控制能量在数字弦中的往返方式；声纹轮廓会同步回应，而不是只改变一组隐藏数值。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 10, 0)
                                [WaveguideParameterRow(NSLOCTEXT("ResonanceForge", "Sustain", "回响长度"), NSLOCTEXT("ResonanceForge", "SustainDetail", "反馈保留"), &WaveguideSustain, Wood)]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(10, 0)
                                [WaveguideParameterRow(NSLOCTEXT("ResonanceForge", "Damping", "弦路阻尼"), NSLOCTEXT("ResonanceForge", "DampingDetail", "高频耗散"), &WaveguideDamping, Steel)]
                                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(10, 0, 0, 0)
                                [WaveguideParameterRow(NSLOCTEXT("ResonanceForge", "Coupling", "箱体耦合"), NSLOCTEXT("ResonanceForge", "CouplingDetail", "弦体传能"), &WaveguideCoupling, Glass)]
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 4, 22, 12)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 10, 0)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[WorkspaceTitle(NSLOCTEXT("ResonanceForge", "Material", "给对象一种听感"), NSLOCTEXT("ResonanceForge", "MaterialDetail", "预设同时改变表面外观、共振峰分布与衰减。"))]
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
                            + SVerticalBox::Slot().AutoHeight()[WorkspaceTitle(NSLOCTEXT("ResonanceForge", "Performance", "塑造这一次发声"), NSLOCTEXT("ResonanceForge", "PerformanceDetail", "拖动参数后立即试听；同一数值也会发送给 Wwise。"))]
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
