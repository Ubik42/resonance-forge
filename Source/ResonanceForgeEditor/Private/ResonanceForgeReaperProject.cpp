#include "ResonanceForgeReaperProject.h"

#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace ResonanceForgeEditor
{
    namespace
    {
        FString SanitizeQuotedValue(const FString& Value)
        {
            FString Safe = Value;
            Safe.ReplaceInline(TEXT("\r"), TEXT(" "));
            Safe.ReplaceInline(TEXT("\n"), TEXT(" "));
            Safe.ReplaceInline(TEXT("\""), TEXT("'"));
            return Safe.TrimStartAndEnd();
        }

        FString NewReaperGuid()
        {
            return FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensInBraces);
        }
    }

    bool FReaperProjectWriter::BuildAuditionProject(
        const TArray<FReaperAuditionItem>& Items,
        FString& OutProject,
        FString& OutError)
    {
        OutProject.Reset();
        OutError.Reset();
        if (Items.IsEmpty() || Items.Num() > 3)
        {
            OutError = TEXT("REAPER 对照带需要 1–3 份铸样");
            return false;
        }

        for (const FReaperAuditionItem& Item : Items)
        {
            if (Item.AudioPath.IsEmpty()
                || FPaths::IsRelative(Item.AudioPath)
                || FPaths::GetExtension(Item.AudioPath).ToLower() != TEXT("wav")
                || Item.AudioPath.Contains(TEXT("\""))
                || Item.AudioPath.Contains(TEXT("\r"))
                || Item.AudioPath.Contains(TEXT("\n"))
                || !FMath::IsFinite(Item.DurationSeconds)
                || Item.DurationSeconds < 0.05f
                || Item.DurationSeconds > 600.0f)
            {
                OutError = TEXT("铸样路径或时长不符合 REAPER 交接要求");
                return false;
            }
        }

        OutProject = TEXT("<REAPER_PROJECT 0.1 \"7.0/win64\" 0\n");
        OutProject += TEXT("  SAMPLERATE 48000 0 0\n");
        OutProject += TEXT("  TEMPO 120 4 4\n");
        OutProject += TEXT("  RIPPLE 0\n");
        OutProject += TEXT("  AUTOXFADE 1\n");
        OutProject += TEXT("  PANMODE 3\n");
        OutProject += TEXT("  CURSOR 0\n");
        OutProject += TEXT("  <TRACK ") + NewReaperGuid() + TEXT("\n");
        OutProject += TEXT("    NAME \"共振铸样对照带\"\n");
        OutProject += TEXT("    PEAKCOL 16576\n");
        OutProject += TEXT("    BEAT -1\n");
        OutProject += TEXT("    AUTOMODE 0\n");
        OutProject += TEXT("    VOLPAN 1 0 -1 -1 1\n");
        OutProject += TEXT("    MUTESOLO 0 0 0\n");
        OutProject += TEXT("    IPHASE 0\n");
        OutProject += TEXT("    ISBUS 0 0\n");
        OutProject += TEXT("    SHOWINMIX 1 0.6667 0.5 1 0.5 -1 -1 -1\n");
        OutProject += TEXT("    REC 0 0 1 0 0 0 0 0\n");
        OutProject += TEXT("    VU 2\n");
        OutProject += TEXT("    NCHAN 2\n");
        OutProject += TEXT("    TRACKID ") + NewReaperGuid() + TEXT("\n");
        OutProject += TEXT("    MAINSEND 1 0\n");

        double PositionSeconds = 0.0;
        for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex)
        {
            const FReaperAuditionItem& Item = Items[ItemIndex];
            const FString AudioFileName = FPaths::GetCleanFilename(Item.AudioPath);
            const FString DisplayName = SanitizeQuotedValue(Item.DisplayName.IsEmpty()
                ? FPaths::GetBaseFilename(Item.AudioPath)
                : Item.DisplayName);
            const FString ItemGuid = NewReaperGuid();
            OutProject += TEXT("    <ITEM\n");
            OutProject += FString::Printf(TEXT("      POSITION %.6f\n"), PositionSeconds);
            OutProject += FString::Printf(TEXT("      LENGTH %.6f\n"), Item.DurationSeconds);
            OutProject += TEXT("      LOOP 0\n");
            OutProject += TEXT("      ALLTAKES 0\n");
            OutProject += TEXT("      FADEIN 1 0 0 1 0 0 0\n");
            OutProject += TEXT("      FADEOUT 1 0 0 1 0 0 0\n");
            OutProject += TEXT("      MUTE 0 0\n");
            OutProject += FString::Printf(TEXT("      IID %d\n"), ItemIndex + 1);
            OutProject += FString::Printf(TEXT("      NAME \"%02d · %s\"\n"), ItemIndex + 1, *DisplayName);
            OutProject += TEXT("      VOLPAN 1 0 1 -1\n");
            OutProject += TEXT("      SOFFS 0\n");
            OutProject += TEXT("      PLAYRATE 1 1 0 -1 0 0.0025\n");
            OutProject += TEXT("      CHANMODE 0\n");
            OutProject += TEXT("      GUID ") + ItemGuid + TEXT("\n");
            OutProject += TEXT("      <SOURCE WAVE\n");
            OutProject += TEXT("        FILE \"") + AudioFileName + TEXT("\"\n");
            OutProject += TEXT("      >\n");
            OutProject += TEXT("    >\n");
            PositionSeconds += static_cast<double>(Item.DurationSeconds) + 0.5;
        }

        OutProject += TEXT("  >\n");
        OutProject += TEXT(">\n");
        return true;
    }
}
