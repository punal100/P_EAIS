/*
 * @Author: Punal Manalan
 * @Description: UAIComponent - Component for attaching AI to pawns/controllers
 * @Date: 29/12/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIInterpreter.h"
#include "AIComponent.generated.h"

class UAIBehaviour;

/**
 * Component that attaches to a Pawn or Controller to provide AI functionality.
 * Holds the interpreter instance and blackboard.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent), BlueprintType)
class P_EAIS_API UAIComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAIComponent();

    // ==================== Properties ====================

    /** The AI Behaviour asset to use */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    UAIBehaviour* AIBehaviour;

    /** Path to JSON file (alternative to asset) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FString JsonFilePath;

    /** Run mode (Server/Client/Both) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    EAIRunMode RunMode = EAIRunMode::Server;

    /** Tick interval (0 = every frame) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (ClampMin = "0.0"))
    float TickInterval = 0.0f;

    /** Auto-start on BeginPlay */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    bool bAutoStart = true;

    /** Debug mode - log state changes */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Debug")
    bool bDebugMode = false;

    // ==================== Component Lifecycle ====================

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ==================== Control ====================

    /** Initialize the AI with a behavior */
    UFUNCTION(BlueprintCallable, Category = "AI")
    bool InitializeAI(UAIBehaviour* Behavior);

    /** Initialize the AI from JSON string */
    UFUNCTION(BlueprintCallable, Category = "AI")
    bool InitializeAIFromJson(const FString& JsonString, FString& OutError);

    /** Start/Resume the AI with optional profile and path */
    UFUNCTION(BlueprintCallable, Category = "AI")
    void StartAI(const FString& ProfileName = TEXT(""), const FString& OptionalPath = TEXT(""));

    /** Stop/Pause the AI */
    UFUNCTION(BlueprintCallable, Category = "AI")
    void StopAI();

    /** Reset the AI to initial state */
    UFUNCTION(BlueprintCallable, Category = "AI")
    void ResetAI();

    /** Force transition to a state */
    UFUNCTION(BlueprintCallable, Category = "AI")
    bool ForceTransition(const FString& StateId);

    /** Enqueue an event */
    UFUNCTION(BlueprintCallable, Category = "AI")
    void EnqueueEvent(const FString& EventName, const FAIEventPayload& Payload);

    /** Enqueue a simple event (no payload) */
    UFUNCTION(BlueprintCallable, Category = "AI")
    void EnqueueSimpleEvent(const FString& EventName);

    // ==================== Blackboard Access ====================

    /** Set blackboard value */
    UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
    void SetBlackboardValue(const FString& Key, const FBlackboardValue& Value);

    /** Get blackboard value */
    UFUNCTION(BlueprintPure, Category = "AI|Blackboard")
    FBlackboardValue GetBlackboardValue(const FString& Key) const;

    /** Set blackboard bool */
    UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
    void SetBlackboardBool(const FString& Key, bool Value);

    /** Get blackboard bool */
    UFUNCTION(BlueprintPure, Category = "AI|Blackboard")
    bool GetBlackboardBool(const FString& Key) const;

    /** Set blackboard float */
    UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
    void SetBlackboardFloat(const FString& Key, float Value);

    /** Get blackboard float */
    UFUNCTION(BlueprintPure, Category = "AI|Blackboard")
    float GetBlackboardFloat(const FString& Key) const;

    /** Set blackboard vector */
    UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
    void SetBlackboardVector(const FString& Key, const FVector& Value);

    /** Get blackboard vector */
    UFUNCTION(BlueprintPure, Category = "AI|Blackboard")
    FVector GetBlackboardVector(const FString& Key) const;

    /** Set blackboard object */
    UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
    void SetBlackboardObject(const FString& Key, UObject* Value);

    /** Get blackboard object */
    UFUNCTION(BlueprintPure, Category = "AI|Blackboard")
    UObject* GetBlackboardObject(const FString& Key) const;

    // ==================== State Information ====================

    /** Get current state ID */
    UFUNCTION(BlueprintPure, Category = "AI")
    FString GetCurrentState() const;

    /** Get behavior name */
    UFUNCTION(BlueprintPure, Category = "AI")
    FString GetBehaviorName() const;

    /** Is AI running? */
    UFUNCTION(BlueprintPure, Category = "AI")
    bool IsRunning() const;

    /** Is AI valid and initialized? */
    UFUNCTION(BlueprintPure, Category = "AI")
    bool IsValid() const;

    /** Get state elapsed time */
    UFUNCTION(BlueprintPure, Category = "AI")
    float GetStateElapsedTime() const;

    /** Get all state IDs */
    UFUNCTION(BlueprintCallable, Category = "AI")
    TArray<FString> GetAllStates() const;

    // ==================== Owner Access ====================

    /** Get the owning pawn */
    UFUNCTION(BlueprintPure, Category = "AI")
    APawn* GetOwnerPawn() const;

    /** Get the owning controller */
    UFUNCTION(BlueprintPure, Category = "AI")
    AController* GetOwnerController() const;

    // ==================== Delegates ====================

    /** Called when AI state changes */
    UPROPERTY(BlueprintAssignable, Category = "AI|Events")
    FOnAIStateChanged OnStateChanged;

    /** Called when AI executes an action */
    UPROPERTY(BlueprintAssignable, Category = "AI|Events")
    FOnAIActionExecuted OnActionExecuted;

protected:
    /** The interpreter instance */
    FAIInterpreter Interpreter;

    /** Is the AI currently running */
    bool bIsRunning = false;

    /** Time since last tick */
    float TimeSinceLastTick = 0.0f;

    /** Internal state change handler */
    UFUNCTION()
    void HandleStateChanged(const FString& OldState, const FString& NewState);

    /** Check if we should run based on RunMode and net role */
    bool ShouldRun() const;
};
