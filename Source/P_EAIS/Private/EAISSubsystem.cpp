/*
 * @Author: Punal Manalan
 * @Description: Implementation of UEAISSubsystem
 * @Date: 29/12/2025
 */

#include "EAISSubsystem.h"
#include "AIAction.h"
#include "AIBehaviour.h"
#include "Engine/GameInstance.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

void UEAISSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    RegisterDefaultActions();

    UE_LOG(LogTemp, Log, TEXT("UEAISSubsystem: Initialized with %d actions"), ActionClasses.Num());
}

void UEAISSubsystem::Deinitialize()
{
    ActionClasses.Empty();
    ActionInstances.Empty();

    Super::Deinitialize();
}

UEAISSubsystem* UEAISSubsystem::Get(UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UEAISSubsystem>();
}

void UEAISSubsystem::RegisterAction(const FString& ActionName, TSubclassOf<UAIAction> ActionClass)
{
    if (ActionName.IsEmpty() || !ActionClass)
    {
        return;
    }

    ActionClasses.Add(ActionName, ActionClass);
    
    // Invalidate cached instance
    ActionInstances.Remove(ActionName);

    if (bGlobalDebugMode)
    {
        UE_LOG(LogTemp, Log, TEXT("UEAISSubsystem: Registered action '%s'"), *ActionName);
    }
}

void UEAISSubsystem::UnregisterAction(const FString& ActionName)
{
    ActionClasses.Remove(ActionName);
    ActionInstances.Remove(ActionName);
}

UAIAction* UEAISSubsystem::GetAction(const FString& ActionName)
{
    // Check cache first
    if (UAIAction** Cached = ActionInstances.Find(ActionName))
    {
        return *Cached;
    }

    // Create new instance
    TSubclassOf<UAIAction>* ActionClass = ActionClasses.Find(ActionName);
    if (!ActionClass || !*ActionClass)
    {
        return nullptr;
    }

    UAIAction* NewAction = NewObject<UAIAction>(this, *ActionClass);
    ActionInstances.Add(ActionName, NewAction);
    return NewAction;
}

TArray<FString> UEAISSubsystem::GetRegisteredActionNames() const
{
    TArray<FString> Result;
    ActionClasses.GetKeys(Result);
    return Result;
}

bool UEAISSubsystem::IsActionRegistered(const FString& ActionName) const
{
    return ActionClasses.Contains(ActionName);
}

UAIBehaviour* UEAISSubsystem::LoadBehaviorFromFile(const FString& FilePath)
{
    FString FullPath = FPaths::ProjectContentDir() / TEXT("AIProfiles") / FilePath;
    
    if (!FPaths::FileExists(FullPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("UEAISSubsystem: Behavior file not found: %s"), *FullPath);
        return nullptr;
    }

    UAIBehaviour* Behavior = NewObject<UAIBehaviour>(this);
    Behavior->JsonFilePath = FilePath;
    
    FString Error;
    if (!Behavior->ParseBehavior(Error))
    {
        UE_LOG(LogTemp, Warning, TEXT("UEAISSubsystem: Failed to parse behavior: %s"), *Error);
        return nullptr;
    }

    return Behavior;
}

TArray<FString> UEAISSubsystem::GetAvailableBehaviors() const
{
    TArray<FString> Result;
    
    FString ProfilesDir = FPaths::ProjectContentDir() / TEXT("AIProfiles");
    
    TArray<FString> FoundFiles;
    IFileManager::Get().FindFilesRecursive(FoundFiles, *ProfilesDir, TEXT("*.json"), true, false);
    
    for (const FString& FilePath : FoundFiles)
    {
        FString RelativePath = FilePath;
        FPaths::MakePathRelativeTo(RelativePath, *ProfilesDir);
        Result.Add(RelativePath);
    }

    return Result;
}

void UEAISSubsystem::SetGlobalDebugMode(bool bEnabled)
{
    bGlobalDebugMode = bEnabled;
    UE_LOG(LogTemp, Log, TEXT("UEAISSubsystem: Global debug mode %s"), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

FString UEAISSubsystem::GetDebugSummary() const
{
    FString Summary = FString::Printf(TEXT("EAIS Subsystem Summary:\n"));
    Summary += FString::Printf(TEXT("  Registered Actions: %d\n"), ActionClasses.Num());
    
    for (const auto& Pair : ActionClasses)
    {
        Summary += FString::Printf(TEXT("    - %s (%s)\n"), *Pair.Key, *Pair.Value->GetName());
    }

    Summary += FString::Printf(TEXT("  Available Behaviors: %d\n"), GetAvailableBehaviors().Num());
    
    return Summary;
}

void UEAISSubsystem::RegisterDefaultActions()
{
    // Register built-in actions
    RegisterAction(TEXT("MoveTo"), UAIAction_MoveTo::StaticClass());
    RegisterAction(TEXT("Kick"), UAIAction_Kick::StaticClass());
    RegisterAction(TEXT("AimAt"), UAIAction_AimAt::StaticClass());
    RegisterAction(TEXT("SetLookTarget"), UAIAction_SetLookTarget::StaticClass());
    RegisterAction(TEXT("Wait"), UAIAction_Wait::StaticClass());
    RegisterAction(TEXT("SetBlackboardKey"), UAIAction_SetBlackboardKey::StaticClass());
    RegisterAction(TEXT("InjectInput"), UAIAction_InjectInput::StaticClass());
    RegisterAction(TEXT("PassToTeammate"), UAIAction_PassToTeammate::StaticClass());
    RegisterAction(TEXT("LookAround"), UAIAction_LookAround::StaticClass());
    RegisterAction(TEXT("Log"), UAIAction_Log::StaticClass());
    RegisterAction(TEXT("Execute"), UAIAction_Execute::StaticClass());
}

