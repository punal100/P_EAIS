/*
 * @Author: Punal Manalan
 * @Description: Implementation of UEAISSubsystem
 * @Date: 29/12/2025
 */

#include "EAISSubsystem.h"
#include "EAISSubsystem.h"
#include "AIAction.h"
#include "Misc/ConfigCacheIni.h"
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
    // Collect all search paths
    TArray<FString> SearchPaths;
    SearchPaths.Add(FPaths::ProjectContentDir() / TEXT("AIProfiles")); // Default
    
    // Add additional paths from settings
    // Add additional paths from settings (Read raw config to avoid Editor dependency)
    // Try both old and new section names in case module move changed it
    TArray<FString> ConfigSections = {
        TEXT("/Script/P_EAIS.EAISSettings"),
        TEXT("/Script/P_EAIS_Editor.EAISSettings")
    };

    for (const FString& Section : ConfigSections)
    {
        TArray<FString> PathEntries;
        if (GConfig->GetArray(*Section, TEXT("AdditionalProfilePaths"), PathEntries, GGameIni))
        {
            for (const FString& Entry : PathEntries)
            {
                // Entry format example: (Path="../Plugins/P_MiniFootball/Content/AIProfiles")
                // We need to extract the value inside quotes
                FString PathValue = Entry;
                int32 QuoteStart = -1;
                int32 QuoteEnd = -1;
                
                if (PathValue.FindChar('"', QuoteStart))
                {
                    if (PathValue.FindLastChar('"', QuoteEnd))
                    {
                        if (QuoteEnd > QuoteStart)
                        {
                            PathValue = PathValue.Mid(QuoteStart + 1, QuoteEnd - QuoteStart - 1);
                        }
                    }
                }
                
                if (!PathValue.IsEmpty() && !PathValue.Contains(TEXT("("))) // Basic validation to ensure we stripped struct syntax
                {
                     if (FPaths::IsRelative(PathValue))
                     {
                         PathValue = FPaths::ProjectContentDir() / PathValue;
                     }
                     SearchPaths.Add(PathValue);
                }
            }
        }
    }

    FString ValidPath;
    for (const FString& Dir : SearchPaths)
    {
        FString TestPath = Dir / FilePath;
        if (FPaths::FileExists(TestPath))
        {
            ValidPath = TestPath;
            break;
        }
    }
    
    if (ValidPath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("UEAISSubsystem: Behavior file not found: %s (Searched %d paths)"), *FilePath, SearchPaths.Num());
        return nullptr;
    }

    UAIBehaviour* Behavior = NewObject<UAIBehaviour>(this);
    Behavior->JsonFilePath = FilePath; // Keep relative identifier
    
    FString Error;
    // We need to support parsing from full path now, or ensure ParseBehavior uses the found path
    // Assuming ParseBehavior reads JsonFilePath. If it reads from disk, we might need to update it or duplicate logic.
    // Let's check logic. Behavior->JsonFilePath is typically used for identification. 
    // We should probably pass the full content or update how ParseBehavior works.
    // However, looking at AIBehaviour.cpp (not visible here but inferred), it likely loads the file.
    // If AIBehaviour uses FFileHelper::LoadFileToString(JsonFilePath), then we MUST set JsonFilePath to ValidPath.
    
    Behavior->JsonFilePath = ValidPath; // Update to absolute path for loading
    
    if (!Behavior->ParseBehavior(Error))
    {
        UE_LOG(LogTemp, Warning, TEXT("UEAISSubsystem: Failed to parse behavior: %s"), *Error);
        return nullptr;
    }
    
    // Restore relative path for ID purposes if needed? 
    // Usually fine to keep full path or just the name. 
    // But let's stick to ValidPath which ensures it works.

    return Behavior;
}

TArray<FString> UEAISSubsystem::GetAvailableBehaviors() const
{
    TArray<FString> Result;
    TArray<FString> SearchPaths;
    
    // Default path
    SearchPaths.Add(FPaths::ProjectContentDir() / TEXT("AIProfiles"));
    
    // Settings paths
    // Settings paths (Raw Config Read)
    TArray<FString> ConfigSections = {
        TEXT("/Script/P_EAIS.EAISSettings"),
        TEXT("/Script/P_EAIS_Editor.EAISSettings")
    };

    for (const FString& Section : ConfigSections)
    {
        TArray<FString> PathEntries;
        if (GConfig->GetArray(*Section, TEXT("AdditionalProfilePaths"), PathEntries, GGameIni))
        {
            for (const FString& Entry : PathEntries)
            {
                FString PathValue = Entry;
                int32 QuoteStart = -1;
                int32 QuoteEnd = -1;
                
                if (PathValue.FindChar('"', QuoteStart))
                {
                    if (PathValue.FindLastChar('"', QuoteEnd))
                    {
                        if (QuoteEnd > QuoteStart)
                        {
                            PathValue = PathValue.Mid(QuoteStart + 1, QuoteEnd - QuoteStart - 1);
                        }
                    }
                }
                
                if (!PathValue.IsEmpty() && !PathValue.Contains(TEXT("(")))
                {
                     if (FPaths::IsRelative(PathValue))
                     {
                         PathValue = FPaths::ProjectContentDir() / PathValue;
                     }
                     SearchPaths.Add(PathValue);
                }
            }
        }
    }
    
    for (const FString& ProfilesDir : SearchPaths)
    {
        if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*ProfilesDir))
        {
            continue;
        }

        TArray<FString> FoundFiles;
        IFileManager::Get().FindFilesRecursive(FoundFiles, *ProfilesDir, TEXT("*.json"), true, false);
        
        for (const FString& FilePath : FoundFiles)
        {
            FString RelativePath = FilePath;
            FPaths::MakePathRelativeTo(RelativePath, *ProfilesDir);
            Result.Add(RelativePath); // Note: This might cause name collisions if same file exists in multiple dirs.
        }
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

