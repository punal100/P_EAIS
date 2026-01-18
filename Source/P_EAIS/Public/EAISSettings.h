#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"
#include "EAISSettings.generated.h"

/**
 * Project-wide settings for the Enhanced AI System (EAIS).
 *
 * This file exists primarily to provide a stable place for EAIS configuration
 * and to satisfy DevTools structure validation.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="EAIS Settings"))
class P_EAIS_API UEAISSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UEAISSettings() = default;

    /** Additional directories to scan for AI behavior profiles (relative to Project Content). */
    UPROPERTY(Config, EditAnywhere, Category="General", meta=(ContentDir))
    TArray<FDirectoryPath> AdditionalProfilePaths;

    /** Enables additional EAIS logging (when code checks this setting). */
    UPROPERTY(Config, EditAnywhere, Category="EAIS|Debug")
    bool bEnableDebugLogs = false;
};
