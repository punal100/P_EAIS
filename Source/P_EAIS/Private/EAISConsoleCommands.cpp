/*
 * @Author: Punal Manalan
 * @Description: Implementation of EAIS Console Commands
 * @Date: 29/12/2025
 */

#include "EAISConsoleCommands.h"
#include "EAISSubsystem.h"
#include "AIComponent.h"
#include "AIBehaviour.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

FAutoConsoleCommand* UEAISConsoleCommands::SpawnBotCommand = nullptr;
FAutoConsoleCommand* UEAISConsoleCommands::DebugCommand = nullptr;
FAutoConsoleCommand* UEAISConsoleCommands::InjectEventCommand = nullptr;
FAutoConsoleCommand* UEAISConsoleCommands::ListActionsCommand = nullptr;

void UEAISConsoleCommands::RegisterCommands()
{
    // EAIS.SpawnBot <TeamID> <ProfileName>
    SpawnBotCommand = new FAutoConsoleCommand(
        TEXT("EAIS.SpawnBot"),
        TEXT("Spawn an AI bot with the specified profile. Usage: EAIS.SpawnBot <TeamID> <ProfileName>"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEAISConsoleCommands::SpawnBotHandler)
    );

    // EAIS.Debug <0|1>
    DebugCommand = new FAutoConsoleCommand(
        TEXT("EAIS.Debug"),
        TEXT("Enable/disable EAIS debug mode. Usage: EAIS.Debug <0|1>"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEAISConsoleCommands::SetDebugHandler)
    );

    // EAIS.InjectEvent <AIName> <EventName>
    InjectEventCommand = new FAutoConsoleCommand(
        TEXT("EAIS.InjectEvent"),
        TEXT("Inject an event to an AI. Usage: EAIS.InjectEvent <AIName> <EventName>"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEAISConsoleCommands::InjectEventHandler)
    );

    // EAIS.ListActions
    ListActionsCommand = new FAutoConsoleCommand(
        TEXT("EAIS.ListActions"),
        TEXT("List all registered AI actions"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEAISConsoleCommands::ListActionsHandler)
    );
}

void UEAISConsoleCommands::UnregisterCommands()
{
    delete SpawnBotCommand;
    delete DebugCommand;
    delete InjectEventCommand;
    delete ListActionsCommand;

    SpawnBotCommand = nullptr;
    DebugCommand = nullptr;
    InjectEventCommand = nullptr;
    ListActionsCommand = nullptr;
}

static UWorld* GetGameWorld()
{
    if (GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
            {
                return Context.World();
            }
        }
    }
    return nullptr;
}

void UEAISConsoleCommands::SpawnBotHandler(const TArray<FString>& Args)
{
    UWorld* World = GetGameWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("EAIS.SpawnBot: No world context"));
        return;
    }

    if (Args.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("EAIS.SpawnBot: Usage: EAIS.SpawnBot <TeamID> <ProfileName>"));
        return;
    }

    int32 TeamID = FCString::Atoi(*Args[0]);
    FString ProfileName = Args[1];

    UE_LOG(LogTemp, Log, TEXT("EAIS.SpawnBot: Spawning bot with Team=%d, Profile=%s"), TeamID, *ProfileName);

    UEAISSubsystem* Subsystem = UEAISSubsystem::Get(World);
    if (!Subsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("EAIS.SpawnBot: Subsystem not found"));
        return;
    }

    // Load behavior
    FString BehaviorPath = ProfileName + TEXT(".json");
    UAIBehaviour* Behavior = Subsystem->LoadBehaviorFromFile(BehaviorPath);
    if (!Behavior)
    {
        UE_LOG(LogTemp, Warning, TEXT("EAIS.SpawnBot: Failed to load behavior '%s'"), *BehaviorPath);
        return;
    }

    // TODO: Spawn actual pawn using P_MiniFootball spawning system
    UE_LOG(LogTemp, Log, TEXT("EAIS.SpawnBot: Loaded behavior '%s' successfully. Pawn spawning requires P_MiniFootball integration."), *ProfileName);
}

void UEAISConsoleCommands::SetDebugHandler(const TArray<FString>& Args)
{
    UWorld* World = GetGameWorld();
    if (!World)
    {
        return;
    }

    bool bEnabled = true;
    if (Args.Num() > 0)
    {
        bEnabled = FCString::Atoi(*Args[0]) != 0;
    }

    UEAISSubsystem* Subsystem = UEAISSubsystem::Get(World);
    if (Subsystem)
    {
        Subsystem->SetGlobalDebugMode(bEnabled);
    }

    // Also set debug on all AI components
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (UAIComponent* AIComp = Actor->FindComponentByClass<UAIComponent>())
        {
            AIComp->bDebugMode = bEnabled;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("EAIS.Debug: %s"), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void UEAISConsoleCommands::InjectEventHandler(const TArray<FString>& Args)
{
    UWorld* World = GetGameWorld();
    if (!World)
    {
        return;
    }

    if (Args.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("EAIS.InjectEvent: Usage: EAIS.InjectEvent <AIName> <EventName>"));
        return;
    }

    FString AIName = Args[0];
    FString EventName = Args[1];

    // Find AI components and inject event
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    int32 Count = 0;
    for (AActor* Actor : AllActors)
    {
        if (UAIComponent* AIComp = Actor->FindComponentByClass<UAIComponent>())
        {
            FString BehaviorName = AIComp->GetBehaviorName();
            if (BehaviorName.Contains(AIName) || AIName.Equals(TEXT("*")))
            {
                AIComp->EnqueueSimpleEvent(EventName);
                Count++;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("EAIS.InjectEvent: Injected '%s' to %d AI(s)"), *EventName, Count);
}

void UEAISConsoleCommands::ListActionsHandler(const TArray<FString>& Args)
{
    UWorld* World = GetGameWorld();
    if (!World)
    {
        return;
    }

    UEAISSubsystem* Subsystem = UEAISSubsystem::Get(World);
    if (!Subsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("EAIS.ListActions: Subsystem not found"));
        return;
    }

    TArray<FString> Actions = Subsystem->GetRegisteredActionNames();
    
    UE_LOG(LogTemp, Log, TEXT("EAIS.ListActions: %d registered actions:"), Actions.Num());
    for (const FString& Action : Actions)
    {
        UE_LOG(LogTemp, Log, TEXT("  - %s"), *Action);
    }
}
