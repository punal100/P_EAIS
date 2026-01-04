/*
 * @Author: Punal Manalan
 * @Description: Commandlet to generate/repair EAIS Editor EUW
 * @Date: 04/01/2026
 */

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "EAIS_GenerateEUWCommandlet.generated.h"

/**
 * Commandlet to generate or repair the EAIS Editor EUW via P_MWCS.
 * 
 * Usage:
 *   UnrealEditor-Cmd.exe "Project.uproject" -run=EAIS_GenerateEUW
 */
UCLASS()
class P_EAISTOOLS_API UEAIS_GenerateEUWCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UEAIS_GenerateEUWCommandlet();
    
    virtual int32 Main(const FString& Params) override;
};
