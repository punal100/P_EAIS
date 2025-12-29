/*
 * @Author: Punal Manalan
 * @Description: FAIInterpreter - JSON AI state machine interpreter
 * @Date: 29/12/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "EAIS_Types.h"
#include "AIInterpreter.generated.h"

class UAIComponent;
class UAIAction;
class UEAISSubsystem;

/**
 * Runtime interpreter for AI state machines.
 * Parses JSON behavior definitions and executes states/transitions.
 */
USTRUCT(BlueprintType)
struct P_EAIS_API FAIInterpreter
{
    GENERATED_BODY()

public:
    FAIInterpreter();

    // ==================== Initialization ====================

    /** Load behavior from JSON string */
    bool LoadFromJson(const FString& JsonString, FString& OutError);

    /** Load behavior from parsed definition */
    bool LoadFromDef(const FAIBehaviorDef& BehaviorDef);

    /** Initialize the interpreter with an owner component */
    void Initialize(UAIComponent* OwnerComp);

    /** Reset to initial state */
    void Reset();

    // ==================== Runtime ====================

    /** Tick the interpreter */
    void Tick(float DeltaSeconds);

    /** Enqueue an event for processing */
    void EnqueueEvent(const FString& EventName, const FAIEventPayload& Payload);

    /** Force transition to a specific state */
    bool ForceTransition(const FString& StateId);

    /** Pause/unpause the interpreter */
    void SetPaused(bool bPause);

    /** Check if interpreter is paused */
    bool IsPaused() const { return bIsPaused; }

    /** Step one tick (for debugging) */
    void StepTick();

    // ==================== Blackboard ====================

    /** Set a blackboard value */
    void SetBlackboardValue(const FString& Key, const FBlackboardValue& Value);

    /** Get a blackboard value */
    bool GetBlackboardValue(const FString& Key, FBlackboardValue& OutValue) const;

    /** Set blackboard bool */
    void SetBlackboardBool(const FString& Key, bool Value);

    /** Get blackboard bool */
    bool GetBlackboardBool(const FString& Key) const;

    /** Set blackboard float */
    void SetBlackboardFloat(const FString& Key, float Value);

    /** Get blackboard float */
    float GetBlackboardFloat(const FString& Key) const;

    /** Set blackboard vector */
    void SetBlackboardVector(const FString& Key, const FVector& Value);

    /** Get blackboard vector */
    FVector GetBlackboardVector(const FString& Key) const;

    /** Set blackboard object */
    void SetBlackboardObject(const FString& Key, UObject* Value);

    /** Get blackboard object */
    UObject* GetBlackboardObject(const FString& Key) const;

    // ==================== State Information ====================

    /** Get current state ID */
    FString GetCurrentStateId() const { return CurrentStateId; }

    /** Get behavior name */
    FString GetBehaviorName() const { return BehaviorDef.Name; }

    /** Is the interpreter valid and running? */
    bool IsValid() const { return BehaviorDef.bIsValid && !CurrentStateId.IsEmpty(); }

    /** Get all state IDs */
    TArray<FString> GetAllStateIds() const;

    /** Get elapsed time in current state */
    float GetStateElapsedTime() const { return StateElapsedTime; }

    /** Get total runtime */
    float GetTotalRuntime() const { return TotalRuntime; }

    // ==================== Delegates ====================

    /** Called when state changes */
    FOnAIStateChanged OnStateChanged;

    /** Called when an action is executed */
    FOnAIActionExecuted OnActionExecuted;

private:
    /** The loaded behavior definition */
    FAIBehaviorDef BehaviorDef;

    /** Current state ID */
    FString CurrentStateId;

    /** Previous state ID */
    FString PreviousStateId;

    /** Blackboard storage */
    TMap<FString, FBlackboardValue> Blackboard;

    /** Event queue */
    TArray<FAIQueuedEvent> EventQueue;

    /** Recently received events (for condition checking) */
    TSet<FString> RecentEvents;

    /** Owner component */
    TWeakObjectPtr<UAIComponent> OwnerComponent;

    /** Elapsed time in current state */
    float StateElapsedTime = 0.0f;

    /** Total runtime */
    float TotalRuntime = 0.0f;

    /** Is interpreter paused */
    bool bIsPaused = false;

    /** Should step one tick */
    bool bShouldStep = false;

    /** Timer tracking for timer conditions */
    TMap<FString, float> TimerValues;

    // ==================== Internal Methods ====================

    /** Get state by ID */
    const FAIState* GetState(const FString& StateId) const;

    /** Enter a state */
    void EnterState(const FString& StateId);

    /** Exit current state */
    void ExitState();

    /** Execute actions */
    void ExecuteActions(const TArray<FAIActionEntry>& Actions);

    /** Evaluate a condition */
    bool EvaluateCondition(const FAICondition& Condition) const;

    /** Process queued events */
    void ProcessEvents();

    /** Clear recent events */
    void ClearRecentEvents();
};
