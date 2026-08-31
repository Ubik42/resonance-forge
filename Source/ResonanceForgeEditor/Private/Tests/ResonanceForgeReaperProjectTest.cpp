#include "Misc/AutomationTest.h"
#include "ResonanceForgeReaperProject.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResonanceForgeReaperProjectTest,
    "ResonanceForge.Editor.ReaperAuditionProject",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResonanceForgeReaperProjectTest::RunTest(const FString& Parameters)
{
    using namespace ResonanceForgeEditor;

    TArray<FReaperAuditionItem> Items;
    Items.Add({TEXT("C:/Temp/RF_Earlier.wav"), TEXT("早期 A"), 2.0f});
    Items.Add({TEXT("C:/Temp/RF_Current.wav"), TEXT("当前 \"B\""), 3.0f});

    FString Project;
    FString Error;
    TestTrue(TEXT("两份铸样可以生成 REAPER 工程"), FReaperProjectWriter::BuildAuditionProject(Items, Project, Error));
    TestTrue(TEXT("工程固定 48 kHz"), Project.Contains(TEXT("SAMPLERATE 48000")));
    TestTrue(TEXT("第一段从零秒开始"), Project.Contains(TEXT("POSITION 0.000000")));
    TestTrue(TEXT("第二段留出半秒间隙"), Project.Contains(TEXT("POSITION 2.500000")));
    TestTrue(TEXT("同目录相对路径写入 WAVE Source"), Project.Contains(TEXT("FILE \"RF_Current.wav\"")));
    TestFalse(TEXT("工程不固化开发机绝对路径"), Project.Contains(TEXT("C:/Temp")));
    TestTrue(TEXT("显示名不会破坏 RPP 引号"), Project.Contains(TEXT("当前 'B'")));

    TArray<FReaperAuditionItem> InvalidItems;
    InvalidItems.Add({TEXT("relative.wav"), TEXT("无效"), 1.0f});
    TestFalse(TEXT("拒绝相对音频路径"), FReaperProjectWriter::BuildAuditionProject(InvalidItems, Project, Error));
    return true;
}

#endif
