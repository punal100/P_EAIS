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
}
