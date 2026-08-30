#include "ResonanceForgeEditorModule.h"
#include "SResonanceForgeVisualizer.h"

#include "Editor.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "../../ResonanceForgeWwise/Public/ResonanceForgeImpactInstrumentActor.h"
#include "ResonanceForgeSynthComponent.h"
#include "Styling/AppStyle.h"
#include "LevelEditorSubsystem.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace ResonanceForgeEditor
{
    static const FName TabName(TEXT("ResonanceForgeWorkbench"));
    static const FLinearColor Cyan(0.04f, 0.78f, 1.0f, 1.0f);
    static const FLinearColor Steel(0.22f, 0.63f, 0.92f, 1.0f);
    static const FLinearColor Wood(0.94f, 0.49f, 0.16f, 1.0f);
    static const FLinearColor Glass(0.30f, 0.92f, 0.84f, 1.0f);
    static const FLinearColor Panel(0.018f, 0.027f, 0.045f, 0.96f);
    static const FLinearColor PanelRaised(0.032f, 0.048f, 0.075f, 1.0f);
    static const FLinearColor Muted(0.58f, 0.67f, 0.75f, 1.0f);

    TSharedRef<SWidget> WorkspaceTitle(const FText& Title, const FText& Detail)
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
            [SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted).AutoWrapText(true)];
    }

    TSharedRef<SWidget> SignalNode(const FText& Index, const FText& Title, const TAttribute<FText>& Detail, const FLinearColor& Accent)
    {
        return SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
            .BorderBackgroundColor(FLinearColor(0.025f, 0.035f, 0.048f, 1.0f))
            .Padding(FMargin(13, 11))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                    [SNew(STextBlock).Text(Index).ColorAndOpacity(Accent).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                    + SHorizontalBox::Slot().FillWidth(1.0f)
                    [SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 0)
                [SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted).AutoWrapText(true)]
            ];
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
}

void FResonanceForgeEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ResonanceForgeEditor::TabName);
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
    Instrument->NativeSynth->SetSynthesisModel(ModelType);
    Instrument->MarkPackageDirty();
    LastStatus = ModelType == EResonanceModelType::WaveguideString
        ? NSLOCTEXT("ResonanceForge", "WaveguideSelected", "已切换到数字波导弦，使用 MIDI 或试听按钮演奏音高")
        : NSLOCTEXT("ResonanceForge", "ModalSelected", "已切换到模态撞击体，场景碰撞将激励材质共振");
}

FReply FResonanceForgeEditorModule::SyncFromSelection()
{
    if (const AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        ActiveModel = Instrument->SynthesisModel;
        ActivePreset = Instrument->ResonancePreset;
        PreviewSize = Instrument->ObjectSize;
        LastStatus = FText::Format(
            NSLOCTEXT("ResonanceForge", "SelectionSynced", "已读取「{0}」· 现在可以调整模型并试听"),
            FText::FromString(Instrument->GetActorLabel()));
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
        const FText ModelText = Instrument->SynthesisModel == EResonanceModelType::WaveguideString
            ? NSLOCTEXT("ResonanceForge", "WaveguideModelName", "数字波导弦")
            : NSLOCTEXT("ResonanceForge", "ModalModelName", "模态撞击体");
        return FText::Format(NSLOCTEXT("ResonanceForge", "Selected", "{0}  ·  {1}  ·  {2}"),
            FText::FromString(Instrument->GetActorLabel()), ModelText, FText::FromName(Instrument->ResonancePreset));
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

TSharedRef<SDockTab> FResonanceForgeEditorModule::SpawnWorkbench(const FSpawnTabArgs& Args)
{
    using namespace ResonanceForgeEditor;

    if (const AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        ActiveModel = Instrument->SynthesisModel;
        ActivePreset = Instrument->ResonancePreset;
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
                + SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(Mapping).ColorAndOpacity(Muted)]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 0)
            [
                SNew(SSlider).Value_Lambda([Value]{ return *Value; }).OnValueChanged_Lambda([Value](float NewValue){ *Value = NewValue; })
                .SliderBarColor(Color).SliderHandleColor(Color)
            ];
    };

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(Panel).Padding(0)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 16, 22, 12)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1.0f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "Title", "声学工作台")).Font(FAppStyle::GetFontStyle(TEXT("HeadingMedium")))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "Subtitle", "从场景对象出发，完成一次可听见、可发布的物理声源设计。")).ColorAndOpacity(Muted)]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "OpenMap", "打开试听场景")).ContentPadding(FMargin(14, 8)).OnClicked_Raw(this, &FResonanceForgeEditorModule::OpenDemoMap)]
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
                            [SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "Online", "Wwise 已连接 · 1 Event / 3 RTPC")).ColorAndOpacity(Glass)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 18)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                        [SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "SignalChain", "当前声音链")).Font(FAppStyle::GetFontStyle(TEXT("BoldFont"))).ColorAndOpacity(Cyan)]
                        + SVerticalBox::Slot().AutoHeight()
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1.0f)
                            [SignalNode(FText::FromString(TEXT("01")), NSLOCTEXT("ResonanceForge", "ObjectNode", "场景对象"), TAttribute<FText>::CreateLambda([this]{ return GetSelectionText(); }), Steel)]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
                            [SNew(STextBlock).Text(FText::FromString(TEXT("›"))).Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall"))).ColorAndOpacity(Muted)]
                            + SHorizontalBox::Slot().FillWidth(0.82f)
                            [SignalNode(FText::FromString(TEXT("02")), NSLOCTEXT("ResonanceForge", "ExciterNode", "施加激励"), TAttribute<FText>::CreateLambda([this]{ return GetExcitationText(); }), Wood)]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
                            [SNew(STextBlock).Text(FText::FromString(TEXT("›"))).Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall"))).ColorAndOpacity(Muted)]
                            + SHorizontalBox::Slot().FillWidth(0.82f)
                            [SignalNode(FText::FromString(TEXT("03")), NSLOCTEXT("ResonanceForge", "ResonatorNode", "形成共振"), TAttribute<FText>::CreateLambda([this]{ return GetResonanceText(); }), Glass)]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
                            [SNew(STextBlock).Text(FText::FromString(TEXT("›"))).Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall"))).ColorAndOpacity(Muted)]
                            + SHorizontalBox::Slot().FillWidth(0.95f)
                            [SignalNode(FText::FromString(TEXT("04")), NSLOCTEXT("ResonanceForge", "OutputNode", "发布声音"), FText::FromString(TEXT("UE Synth + Wwise Event / RTPC")), Cyan)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 12)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1.55f).Padding(0, 0, 7, 0)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[WorkspaceTitle(NSLOCTEXT("ResonanceForge", "Response", "听见模型正在做什么"), NSLOCTEXT("ResonanceForge", "ResponseDetail", "模态峰描述材质的固有频率；弦线描述波在数字弦中的往返传播。"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                            [
                                SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(FLinearColor::White).Padding(1)
                                [
                                    SNew(SResonanceForgeVisualizer)
                                    .ModelType_Lambda([this]{ return ActiveModel; })
                                    .PresetName_Lambda([this]{ return ActivePreset; })
                                    .Energy_Lambda([this]{ return PreviewEnergy; })
                                    .Brightness_Lambda([this]{ return PreviewBrightness; })
                                ]
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
}

IMPLEMENT_MODULE(FResonanceForgeEditorModule, ResonanceForgeEditor)
