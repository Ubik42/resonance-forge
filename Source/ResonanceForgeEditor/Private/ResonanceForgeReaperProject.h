#pragma once

#include "CoreMinimal.h"

namespace ResonanceForgeEditor
{
    struct FReaperAuditionItem
    {
        FString AudioPath;
        FString DisplayName;
        float DurationSeconds = 0.0f;
    };

    class FReaperProjectWriter final
    {
    public:
        static bool BuildAuditionProject(
            const TArray<FReaperAuditionItem>& Items,
            FString& OutProject,
            FString& OutError);
    };
}
