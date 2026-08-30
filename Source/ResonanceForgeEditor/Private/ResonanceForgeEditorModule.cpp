#include "ResonanceForgeEditorModule.h"

#include "Editor.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "Materials/MaterialInterface.h"
#include "../../ResonanceForgeWwise/Public/ResonanceForgeImpactInstrumentActor.h"
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

    TSharedRef<SWidget> SectionTitle(const FText& Eyebrow, const FText& Title)
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Eyebrow).ColorAndOpacity(Cyan)]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
            [SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))];
    }

    TSharedRef<SWidget> RouteNode(const FText& Step, const FText& Title, const FText& Detail, const FLinearColor& Color)
    {
        return SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
            .BorderBackgroundColor(PanelRaised)
            .Padding(FMargin(12, 10))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Step).ColorAndOpacity(Color)]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
                [SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
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
            const bool bLoaded = LevelSubsystem->LoadLevel(TEXT("/Game/ResonanceForge/Demo/Maps/L_RF_PhysicsLab"));
            LastStatus = bLoaded
                ? NSLOCTEXT("ResonanceForge", "MapLoaded", "演示地图已打开 · 点击 PIE 观察三种材质的落球撞击")
                : NSLOCTEXT("ResonanceForge", "MapFailed", "演示地图打开失败 · 请检查内容是否已生成");
        }
    }
    return FReply::Handled();
}

FText FResonanceForgeEditorModule::GetSelectionText() const
{
    if (const AResonanceForgeImpactInstrumentActor* Instrument = ResolveInstrument())
    {
        return FText::Format(NSLOCTEXT("ResonanceForge", "Selected", "当前对象 · {0}  /  预设 · {1}"),
            FText::FromString(Instrument->GetActorLabel()), FText::FromName(Instrument->ResonancePreset));
    }
    return NSLOCTEXT("ResonanceForge", "SelectionEmpty", "当前对象 · 自动使用场景中的第一个共振体");
}

FText FResonanceForgeEditorModule::GetStatusText() const
{
    return LastStatus;
}

TSharedRef<SDockTab> FResonanceForgeEditorModule::SpawnWorkbench(const FSpawnTabArgs& Args)
{
    using namespace ResonanceForgeEditor;

    auto PresetButton = [this](const FName Preset, const FText& Label, const FText& Detail, const FLinearColor& Color)
    {
        return SNew(SButton)
            .ContentPadding(FMargin(12, 10))
            .ButtonColorAndOpacity_Lambda([this, Preset, Color]
            {
                return ActivePreset == Preset ? Color * 0.58f : PanelRaised;
            })
            .OnClicked_Lambda([this, Preset]
            {
                ApplyPreset(Preset);
                return FReply::Handled();
            })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)[SNew(STextBlock).Text(Detail).ColorAndOpacity(Muted)]
            ];
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
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 18, 22, 16)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1.0f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "Kicker", "RESONANCE FORGE  /  UE × WWISE")).ColorAndOpacity(Cyan)]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)[SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "Title", "共振铸造台")).Font(FAppStyle::GetFontStyle(TEXT("HeadingLarge")))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)[SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "Subtitle", "把看得见的材质与碰撞，铸造成可演奏、可发布的游戏声音。")).ColorAndOpacity(Muted)]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "OpenMap", "打开物理演示场景")).ContentPadding(FMargin(16, 9)).OnClicked_Raw(this, &FResonanceForgeEditorModule::OpenDemoMap)]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 14)
                    [
                        SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(PanelRaised).Padding(FMargin(12, 9))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1.0f)[SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetSelectionText)]
                            + SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(NSLOCTEXT("ResonanceForge", "Online", "● Wwise 2025.1 · Event / 3 RTPC 已绑定")).ColorAndOpacity(Glass)]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 14)
                    [
                        SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(PanelRaised).Padding(16)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[SectionTitle(NSLOCTEXT("ResonanceForge", "RouteKicker", "声音路由"), NSLOCTEXT("ResonanceForge", "RouteTitle", "从物理材质到 Wwise 的实时信号链"))]
                            + SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
                            [
                                SNew(SGridPanel).FillColumn(0, 1).FillColumn(1, 1).FillColumn(2, 1).FillColumn(3, 1)
                                + SGridPanel::Slot(0, 0).Padding(0, 0, 6, 0)[RouteNode(FText::FromString(TEXT("01")), FText::FromString(TEXT("物理撞击")), FText::FromString(TEXT("冲量 / 相对速度 / 尺寸")), Steel)]
                                + SGridPanel::Slot(1, 0).Padding(6, 0)[RouteNode(FText::FromString(TEXT("02")), FText::FromString(TEXT("材质共振")), FText::FromString(TEXT("钢 / 木 / 玻璃模态库")), Wood)]
                                + SGridPanel::Slot(2, 0).Padding(6, 0)[RouteNode(FText::FromString(TEXT("03")), FText::FromString(TEXT("表演映射")), FText::FromString(TEXT("MIDI Velocity / CC1")), Glass)]
                                + SGridPanel::Slot(3, 0).Padding(6, 0, 0, 0)[RouteNode(FText::FromString(TEXT("04")), FText::FromString(TEXT("Wwise 发布")), FText::FromString(TEXT("Event + Energy / Brightness / Size")), Cyan)]
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 14)
                    [
                        SNew(SGridPanel).FillColumn(0, 1.12f).FillColumn(1, 0.88f)
                        + SGridPanel::Slot(0, 0).Padding(0, 0, 7, 0)
                        [
                            SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(PanelRaised).Padding(16)
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().AutoHeight()[SectionTitle(NSLOCTEXT("ResonanceForge", "Library", "材质声源库"), NSLOCTEXT("ResonanceForge", "Choose", "选择视觉材质与共振预设"))]
                                + SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 7)[PresetButton(TEXT("拉丝钢"), FText::FromString(TEXT("拉丝钢  /  冷冽长鸣")), FText::FromString(TEXT("高频密集 · 金属度 1.0 · 长衰减")), Steel)]
                                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 7)[PresetButton(TEXT("硬木"), FText::FromString(TEXT("硬木  /  温暖短促")), FText::FromString(TEXT("中低频突出 · 粗糙木纹 · 快速阻尼")), Wood)]
                                + SVerticalBox::Slot().AutoHeight()[PresetButton(TEXT("薄玻璃"), FText::FromString(TEXT("薄玻璃  /  通透明亮")), FText::FromString(TEXT("稀疏高频 · 半透明 · 脆性尾音")), Glass)]
                            ]
                        ]
                        + SGridPanel::Slot(1, 0).Padding(7, 0, 0, 0)
                        [
                            SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(PanelRaised).Padding(16)
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().AutoHeight()[SectionTitle(NSLOCTEXT("ResonanceForge", "Performance", "实时控制"), NSLOCTEXT("ResonanceForge", "Parameters", "撞击参数与 RTPC 映射"))]
                                + SVerticalBox::Slot().AutoHeight().Padding(0, 17, 0, 0)[ParameterRow(FText::FromString(TEXT("撞击能量")), FText::FromString(TEXT("RF_ImpactEnergy")), &PreviewEnergy, Steel)]
                                + SVerticalBox::Slot().AutoHeight().Padding(0, 15, 0, 0)[ParameterRow(FText::FromString(TEXT("音色明亮度")), FText::FromString(TEXT("RF_ImpactBrightness")), &PreviewBrightness, Glass)]
                                + SVerticalBox::Slot().AutoHeight().Padding(0, 15, 0, 0)[ParameterRow(FText::FromString(TEXT("共振体尺寸")), FText::FromString(TEXT("RF_ObjectSize")), &PreviewSize, Wood)]
                                + SVerticalBox::Slot().AutoHeight().Padding(0, 20, 0, 0)
                                [SNew(SButton).Text(NSLOCTEXT("ResonanceForge", "Strike", "触发撞击 · UE 合成 + Wwise Event")).ContentPadding(FMargin(14, 11)).ButtonColorAndOpacity(Cyan * 0.58f).OnClicked_Raw(this, &FResonanceForgeEditorModule::TriggerPreview)]
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(22, 0, 22, 22)
                    [
                        SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).BorderBackgroundColor(FLinearColor(0.015f, 0.09f, 0.11f, 1.0f)).Padding(FMargin(12, 9))
                        [SNew(STextBlock).Text_Raw(this, &FResonanceForgeEditorModule::GetStatusText).ColorAndOpacity(Glass)]
                    ]
                ]
            ]
        ];
}

IMPLEMENT_MODULE(FResonanceForgeEditorModule, ResonanceForgeEditor)
