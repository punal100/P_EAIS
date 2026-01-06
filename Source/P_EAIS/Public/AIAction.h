/*
 * @Author: Punal Manalan
 * @Description: UAIAction - Base class for AI actions
 * @Date: 29/12/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EAIS_Types.h"
#include "AIAction.generated.h"

class UAIComponent;

/**
 * Base class for AI actions.
 * Actions are registered with the subsystem and executed by the interpreter.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class P_EAIS_API UAIAction : public UObject
{
    GENERATED_BODY()

public:
    /** Execute the action */
    UFUNCTION(BlueprintNativeEvent, Category = "AI Action")
    void Execute(UAIComponent* OwnerComponent, const FAIActionParams& Params);
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) {}

    /** Abort the action if it's running */
    UFUNCTION(BlueprintNativeEvent, Category = "AI Action")
    void Abort();
    virtual void Abort_Implementation() {}

    /** Get the action name for registry */
    UFUNCTION(BlueprintPure, Category = "AI Action")
    virtual FString GetActionName() const { return GetClass()->GetName(); }

    /** Is this action currently running? */
    UFUNCTION(BlueprintPure, Category = "AI Action")
    bool IsRunning() const { return bIsRunning; }

protected:
    /** Mark action as running */
    void SetRunning(bool bRunning) { bIsRunning = bRunning; }

    /** Complete the action */
    UFUNCTION(BlueprintCallable, Category = "AI Action")
    void Complete();

private:
    bool bIsRunning = false;
};

// ==================== Built-in Actions ====================

/**
 * MoveTo action - moves the AI toward a target
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_MoveTo : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual void Abort_Implementation() override;
    virtual FString GetActionName() const override { return TEXT("MoveTo"); }

private:
    TWeakObjectPtr<class UPathFollowingComponent> PathFollowingComp;
};

/**
 * Log action - prints a message to output log
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_Log : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual void Abort_Implementation() override;
    virtual FString GetActionName() const override { return TEXT("Log"); }
};

/**
 * Kick action - kicks the ball (via P_MEIS injection)
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_Kick : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual FString GetActionName() const override { return TEXT("Kick"); }
};

/**
 * AimAt action - sets look target for AI
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_AimAt : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual FString GetActionName() const override { return TEXT("AimAt"); }
};

/**
 * SetLookTarget action - sets the AI's look/focus target
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_SetLookTarget : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual FString GetActionName() const override { return TEXT("SetLookTarget"); }
};

/**
 * Wait action - waits for a specified duration
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_Wait : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual FString GetActionName() const override { return TEXT("Wait"); }
};

/**
 * SetBlackboardKey action - sets a blackboard value
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_SetBlackboardKey : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual FString GetActionName() const override { return TEXT("SetBlackboardKey"); }
};

/**
 * InjectInput action - injects input via P_MEIS
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_InjectInput : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual FString GetActionName() const override { return TEXT("InjectInput"); }
};

/**
 * PassToTeammate action - passes the ball to nearest teammate
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_PassToTeammate : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual FString GetActionName() const override { return TEXT("PassToTeammate"); }
};

/**
 * LookAround action - makes AI look around for awareness
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_LookAround : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual FString GetActionName() const override { return TEXT("LookAround"); }
};

/**
 * Execute action - bridges logical action IDs to IEAIS_ActionExecutor
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIAction_Execute : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override;
    virtual FString GetActionName() const override { return TEXT("Execute"); }
};

