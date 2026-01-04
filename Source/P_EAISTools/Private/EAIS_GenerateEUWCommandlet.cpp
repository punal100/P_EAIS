/*
 * @Author: Punal Manalan
 * @Description: Commandlet implementation for EAIS EUW generation
 * @Date: 04/01/2026
 */

#include "EAIS_GenerateEUWCommandlet.h"
#include "MWCS_Service.h"

UEAIS_GenerateEUWCommandlet::UEAIS_GenerateEUWCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UEAIS_GenerateEUWCommandlet::Main(const FString& Params)
{
    UE_LOG(LogTemp, Display, TEXT("========================================"));
    UE_LOG(LogTemp, Display, TEXT("EAIS_GenerateEUW Commandlet"));
    UE_LOG(LogTemp, Display, TEXT("========================================"));
    
    UE_LOG(LogTemp, Display, TEXT("Requesting P_MWCS to generate/repair EAIS Editor EUW..."));
    
    // Call P_MWCS to generate/repair the External Tool EUW
    FMWCS_Report Report = FMWCS_Service::Get().GenerateOrRepairExternalToolEuw(TEXT("EAIS"));
    
    // Log results
    UE_LOG(LogTemp, Display, TEXT("Specs=%d Created=%d Repaired=%d Errors=%d Warnings=%d"),
        Report.SpecsProcessed,
        Report.AssetsCreated,
        Report.AssetsRepaired,
        Report.NumErrors(),
        Report.NumWarnings());
    
    // Log any issues
    for (const FMWCS_Issue& Issue : Report.Issues)
    {
        if (Issue.Severity == EMWCS_IssueSeverity::Error)
        {
            UE_LOG(LogTemp, Error, TEXT("[%s] %s (%s)"), *Issue.Code, *Issue.Message, *Issue.Context);
        }
        else if (Issue.Severity == EMWCS_IssueSeverity::Warning)
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] %s (%s)"), *Issue.Code, *Issue.Message, *Issue.Context);
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("[%s] %s (%s)"), *Issue.Code, *Issue.Message, *Issue.Context);
        }
    }
    
    if (Report.HasErrors())
    {
        UE_LOG(LogTemp, Error, TEXT("EAIS EUW Generation FAILED with %d errors!"), Report.NumErrors());
        return 1;
    }
    
    if (Report.AssetsCreated > 0 || Report.AssetsRepaired > 0)
    {
        UE_LOG(LogTemp, Display, TEXT("SUCCESS: EAIS EUW generated/repaired successfully!"));
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("No changes needed - EUW is up to date."));
    }
    
    UE_LOG(LogTemp, Display, TEXT("========================================"));
    
    return 0;
}
