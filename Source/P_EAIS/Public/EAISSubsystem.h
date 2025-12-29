/*
 * @Author: Punal Manalan
 * @Description: UEAISSubsystem - Global AI subsystem for managing actions and resources
 * @Date: 29/12/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EAISSubsystem.generated.h"

class UAIAction;
class UAIBehaviour;

/**
 * Game Instance Subsystem for EAIS.
 * Manages global AI resources, action registry, and blackboard factories.
 */
UCLASS()
class P_EAIS_API UEAISSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ==================== Subsystem Lifecycle ====================

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ==================== Static Access ====================

    /** Get the subsystem from a world context */
    UFUNCTION(BlueprintPure, Category = "EAIS", meta = (WorldContext = "WorldContextObject"))
    static UEAISSubsystem* Get(UObject* WorldContextObject);

    // ==================== Action Registry ====================

    /** Register an action class */
    UFUNCTION(BlueprintCallable, Category = "EAIS|Actions")
    void RegisterAction(const FString& ActionName, TSubclassOf<UAIAction> ActionClass);

    /** Unregister an action */
    UFUNCTION(BlueprintCallable, Category = "EAIS|Actions")
    void UnregisterAction(const FString& ActionName);

    /** Get an action instance by name */
    UFUNCTION(BlueprintCallable, Category = "EAIS|Actions")
    UAIAction* GetAction(const FString& ActionName);

    /** Get all registered action names */
    UFUNCTION(BlueprintCallable, Category = "EAIS|Actions")
    TArray<FString> GetRegisteredActionNames() const;

    /** Check if an action is registered */
    UFUNCTION(BlueprintPure, Category = "EAIS|Actions")
    bool IsActionRegistered(const FString& ActionName) const;

    // ==================== Behavior Management ====================

    /** Load a behavior from file */
    UFUNCTION(BlueprintCallable, Category = "EAIS|Behaviors")
    UAIBehaviour* LoadBehaviorFromFile(const FString& FilePath);

    /** Get all available behavior profiles in AIProfiles directory */
    UFUNCTION(BlueprintCallable, Category = "EAIS|Behaviors")
    TArray<FString> GetAvailableBehaviors() const;

    // ==================== Debug ====================

    /** Enable/disable global debug mode */
    UFUNCTION(BlueprintCallable, Category = "EAIS|Debug")
    void SetGlobalDebugMode(bool bEnabled);

    /** Is global debug mode enabled */
    UFUNCTION(BlueprintPure, Category = "EAIS|Debug")
    bool IsGlobalDebugMode() const { return bGlobalDebugMode; }

    /** Get debug summary */
    UFUNCTION(BlueprintCallable, Category = "EAIS|Debug")
    FString GetDebugSummary() const;

protected:
    /** Registered action classes */
    UPROPERTY()
    TMap<FString, TSubclassOf<UAIAction>> ActionClasses;

    /** Action instances (cached) */
    UPROPERTY()
    TMap<FString, UAIAction*> ActionInstances;

    /** Global debug mode */
    bool bGlobalDebugMode = false;

    /** Register default actions */
    void RegisterDefaultActions();
};
