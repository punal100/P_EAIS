/*
 * @Author: Punal Manalan
 * @Description: EAIS Console Commands
 * @Date: 29/12/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "EAISConsoleCommands.generated.h"

/**
 * Console command handler for EAIS
 */
UCLASS()
class P_EAIS_API UEAISConsoleCommands : public UObject
{
    GENERATED_BODY()

public:
    /** Register console commands */
    static void RegisterCommands();

    /** Unregister console commands */
    static void UnregisterCommands();

private:
    static FAutoConsoleCommand* SpawnBotCommand;
    static FAutoConsoleCommand* DebugCommand;
    static FAutoConsoleCommand* InjectEventCommand;
    static FAutoConsoleCommand* ListActionsCommand;

    // Command handlers with correct signature for FConsoleCommandWithArgsDelegate
    static void SpawnBotHandler(const TArray<FString>& Args);
    static void SetDebugHandler(const TArray<FString>& Args);
    static void InjectEventHandler(const TArray<FString>& Args);
    static void ListActionsHandler(const TArray<FString>& Args);
};
