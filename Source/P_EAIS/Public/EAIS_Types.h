/*
 * @Author: Punal Manalan
 * @Description: Core types and structures for P_EAIS
 * @Date: 29/12/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "EAIS_Types.generated.h"

/**
 * Payload for AI events (input events, game events, etc.)
 */
USTRUCT(BlueprintType)
struct P_EAIS_API FAIEventPayload
{
    GENERATED_BODY()

    /** String parameters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    TMap<FString, FString> StringParams;

    /** Float parameters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    TMap<FString, float> FloatParams;

    /** Vector parameters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    TMap<FString, FVector> VectorParams;

    /** Object parameters - use GetObject/SetObject functions for safety */
    TMap<FString, TWeakObjectPtr<UObject>> ObjectParams;

    /** Helper to get object safely */
    UObject* GetObject(const FString& Key) const
    {
        const TWeakObjectPtr<UObject>* Found = ObjectParams.Find(Key);
        return Found && Found->IsValid() ? Found->Get() : nullptr;
    }

    /** Helper to set object */
    void SetObject(const FString& Key, UObject* Obj)
    {
        ObjectParams.Add(Key, Obj);
    }

    /** Timestamp of the event */
    UPROPERTY(BlueprintReadOnly, Category = "EAIS")
    float Timestamp = 0.0f;
};

/**
 * Parameters for AI actions
 */
USTRUCT(BlueprintType)
struct P_EAIS_API FAIActionParams
{
    GENERATED_BODY()

    /** Target actor or location name */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString Target;

    /** Speed/power multiplier (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    float Power = 1.0f;

    /** Additional string parameters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    TMap<FString, FString> ExtraParams;
};

/**
 * Condition types for state transitions
 */
UENUM(BlueprintType)
enum class EAIConditionType : uint8
{
    /** Check a blackboard key value */
    Blackboard,
    /** Check for an event occurrence */
    Event,
    /** Timer-based condition */
    Timer,
    /** Distance to a target */
    Distance,
    /** Custom condition (C++ registered) */
    Custom,
    /** Composite condition (All sub-conditions must be true) */
    And,
    /** Composite condition (Any sub-condition must be true) */
    Or,
    /** Composite condition (The sub-condition must be false) */
    Not
};

/**
 * Operators for condition evaluation
 */
UENUM(BlueprintType)
enum class EAIConditionOperator : uint8
{
    Equal,
    NotEqual,
    GreaterThan,
    LessThan,
    GreaterOrEqual,
    LessOrEqual
};

/**
 * Blackboard entry value types
 */
UENUM(BlueprintType)
enum class EBlackboardValueType : uint8
{
    Bool,
    Int,
    Float,
    String,
    Vector,
    Object
};

/**
 * A blackboard value that can hold different types
 * COMMENT: Serialized value representation.
 * RULE: Parse/convert on load; NEVER parse strings in Tick().
 */
USTRUCT(BlueprintType)
struct P_EAIS_API FBlackboardValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    EBlackboardValueType Type = EBlackboardValueType::String;

    /** Raw value as string for JSON serialization (canonical format) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString RawValue;

    // Runtime-parsed values (populated on load)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    bool BoolValue = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    int32 IntValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    float FloatValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString StringValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FVector VectorValue = FVector::ZeroVector;

    /** Object reference - not exposed to Blueprint due to TWeakObjectPtr limitations */
    TWeakObjectPtr<UObject> ObjectValue;

    /** Get the object value safely */
    UObject* GetObjectValue() const { return ObjectValue.IsValid() ? ObjectValue.Get() : nullptr; }

    /** Set the object value */
    void SetObjectValue(UObject* Obj) { ObjectValue = Obj; }

    // Constructors and helpers
    FBlackboardValue() = default;

    explicit FBlackboardValue(bool Value) : Type(EBlackboardValueType::Bool), RawValue(Value ? TEXT("true") : TEXT("false")), BoolValue(Value) {}
    explicit FBlackboardValue(int32 Value) : Type(EBlackboardValueType::Int), RawValue(FString::FromInt(Value)), IntValue(Value) {}
    explicit FBlackboardValue(float Value) : Type(EBlackboardValueType::Float), RawValue(FString::SanitizeFloat(Value)), FloatValue(Value) {}
    explicit FBlackboardValue(const FString& Value) : Type(EBlackboardValueType::String), RawValue(Value), StringValue(Value) {}
    explicit FBlackboardValue(const FVector& Value) : Type(EBlackboardValueType::Vector), RawValue(Value.ToString()), VectorValue(Value) {}
    explicit FBlackboardValue(UObject* Value) : Type(EBlackboardValueType::Object), ObjectValue(Value) {}

    /** Convert to string for display/comparison */
    FString ToString() const;

    /** Parse from string based on current type */
    void FromString(const FString& Value);

    /** Compare with another value */
    bool Compare(const FBlackboardValue& Other, EAIConditionOperator Op) const;
};

/**
 * EAIS Blackboard entry is a key + typed value (canonical representation)
 */
USTRUCT(BlueprintType)
struct P_EAIS_API FEAISBlackboardEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FBlackboardValue Value;
};

/**
 * A condition for state transitions
 * NOTE: Canonical JSON uses "keyOrName" and "op", but C++ uses Name and Operator
 * for backward compatibility with existing implementation.
 */
USTRUCT(BlueprintType)
struct P_EAIS_API FAICondition
{
    GENERATED_BODY()

    /** Type of condition */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    EAIConditionType Type = EAIConditionType::Blackboard;

    /** Key or name (blackboard key, event name, etc.) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString Name;

    /** Comparison operator */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    EAIConditionOperator Operator = EAIConditionOperator::Equal;

    /** Value to compare against */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString Value;

    /** For timer conditions: duration in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    float Seconds = 0.0f;

    /** For distance conditions: target actor/location */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString Target;

    /** Sub-conditions for composite condition types (And, Or, Not) */
    // Note: UPROPERTY removed because UHT does not support recursive structs.
    // This is populated manually via JSON parsing and used for modular evaluation.
    TArray<FAICondition> SubConditions;
};

/**
 * An action to execute in a state
 * NOTE: Canonical JSON uses "actionName" and "paramsJson", but C++ uses Action and Params
 * for backward compatibility with existing implementation.
 */
USTRUCT(BlueprintType)
struct P_EAIS_API FAIActionEntry
{
    GENERATED_BODY()

    /** Name of the action (registered in subsystem) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString Action;

    /** Parameters for the action */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FAIActionParams Params;
};

/**
 * A transition between states
 */
USTRUCT(BlueprintType)
struct P_EAIS_API FAITransition
{
    GENERATED_BODY()

    /** Target state ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString To;

    /** Condition for the transition */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FAICondition Condition;

    /** Priority (higher = evaluated first) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    int32 Priority = 0;
};

/**
 * A state in the AI state machine
 */
USTRUCT(BlueprintType)
struct P_EAIS_API FAIState
{
    GENERATED_BODY()

    /** Unique state identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString Id;

    /** Is this a terminal state (no outgoing transitions expected) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    bool bTerminal = false;

    /** Actions to execute when entering this state */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    TArray<FAIActionEntry> OnEnter;

    /** Actions to execute every tick while in this state */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    TArray<FAIActionEntry> OnTick;

    /** Actions to execute when exiting this state */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    TArray<FAIActionEntry> OnExit;

    /** Transitions to other states */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    TArray<FAITransition> Transitions;
};

/**
 * AI behavior definition (parsed from JSON)
 */
USTRUCT(BlueprintType)
struct P_EAIS_API FAIBehaviorDef
{
    GENERATED_BODY()

    /** Name of the behavior */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString Name;

    /** Initial state ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString InitialState;

    /** Blackboard default values (canonical format) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    TArray<FEAISBlackboardEntry> Blackboard;

    /** All states in this behavior */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    TArray<FAIState> States;

    /** Is this behavior valid and parsed correctly? */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EAIS")
    bool bIsValid = false;
};



/**
 * Queued AI event
 */
USTRUCT()
struct FAIQueuedEvent
{
    GENERATED_BODY()

    FString EventName;
    FAIEventPayload Payload;
    float QueuedTime = 0.0f;
};

/**
 * Run mode for AI interpreter
 */
UENUM(BlueprintType)
enum class EAIRunMode : uint8
{
    /** Run on server only (authoritative) */
    Server,
    /** Run on owning client only (cosmetic) */
    Client,
    /** Run on both server and client */
    Both
};

/** Delegate for when AI state changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIStateChanged, const FString&, OldState, const FString&, NewState);

/** Delegate for when AI executes an action */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIActionExecuted, const FString&, ActionName, const FAIActionParams&, Params);
