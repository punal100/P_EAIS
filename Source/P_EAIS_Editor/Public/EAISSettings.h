/*
 * @Author: Punal Manalan
 * @Description: Configuration settings for EAIS
 * @Date: 10/01/2026
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EAISSettings.generated.h"

/**
 * Global settings for Enhanced AI System
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="EAIS Settings"))
class UEAISSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** Additional directories to scan for AI behavior profiles (relative to Project Content) */
    UPROPERTY(Config, EditAnywhere, Category="General", meta=(ContentDir))
    TArray<FDirectoryPath> AdditionalProfilePaths;
};
