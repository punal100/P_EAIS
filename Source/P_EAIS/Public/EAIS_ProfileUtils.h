#pragma once

#include "CoreMinimal.h"

namespace EAIS_ProfileUtils
{
    inline TArray<FString> MakeSortedUnique(const TSet<FString> &Names)
    {
        TArray<FString> Out = Names.Array();
        Out.Sort();
        return Out;
    }

    inline FString ChooseDefaultProfile(const TArray<FString> &SortedNames, const FString &PreferredName)
    {
        if (!PreferredName.IsEmpty())
        {
            for (const FString &Name : SortedNames)
            {
                if (Name.Equals(PreferredName, ESearchCase::IgnoreCase))
                {
                    return Name;
                }
            }
        }

        return (SortedNames.Num() > 0) ? SortedNames[0] : FString();
    }

    inline FString ResolveProfilePath(const FString& ProfileName, const FString& InBaseDir)
    {
        // If InBaseDir is provided and valid, use it. 
        // Otherwise fallback to default EAIS directory.
        FString BaseDir = InBaseDir;
        if (BaseDir.IsEmpty())
        {
            BaseDir = FPaths::ProjectPluginsDir() / TEXT("P_EAIS/Content/AIProfiles");
        }

        // Support .runtime.json or .json extensions
        FString FullPath = FPaths::Combine(BaseDir, ProfileName + TEXT(".runtime.json"));

        if (!FPaths::FileExists(FullPath))
        {
            // Try simple .json
            FullPath = FPaths::Combine(BaseDir, ProfileName + TEXT(".json"));
        }

        return FullPath;
    }
}
