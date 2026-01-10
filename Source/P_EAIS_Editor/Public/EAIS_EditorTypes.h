// Copyright Punal Manalan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EAIS_Types.h"
#include "EAIS_EditorTypes.generated.h"

/**
 * Base class for editable AI conditions in the Details Panel.
 * Uses UObject hierarchy to support nesting (recursion) which Structs cannot do in UHT.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class P_EAIS_EDITOR_API UEAIS_EditorCondition : public UObject
{
    GENERATED_BODY()
public:
    /** Convert this editor condition to a runtime struct */
    virtual FAICondition ToRuntimeCondition() const PURE_VIRTUAL(UEAIS_EditorCondition::ToRuntimeCondition, return FAICondition(););
    
    /** Create an editor condition from a runtime struct */
    static UEAIS_EditorCondition* FromRuntimeCondition(UObject* Outer, const FAICondition& InCondition);
};

// ============================================================================

/**
 * Represents a standard condition (Check Blackboard, Event, Distance, etc).
 * Cannot be Composite (And, Or, Not).
 */
UCLASS(DisplayName = "Condition: Standard")
class P_EAIS_EDITOR_API UEAIS_EditorCondition_Leaf : public UEAIS_EditorCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Condition")
    EAIConditionType Type = EAIConditionType::Blackboard;

    UPROPERTY(EditAnywhere, Category = "Condition")
    FString Name;

    UPROPERTY(EditAnywhere, Category = "Condition", meta = (EditCondition = "Type == EAIConditionType::Distance || Type == EAIConditionType::Blackboard"))
    FString Target;

    UPROPERTY(EditAnywhere, Category = "Condition")
    EAIConditionOperator Operator = EAIConditionOperator::Equal;

    UPROPERTY(EditAnywhere, Category = "Condition")
    FString Value;

    virtual FAICondition ToRuntimeCondition() const override
    {
        FAICondition Cond;
        Cond.Type = Type;
        Cond.Name = Name;
        Cond.Target = Target;
        Cond.Operator = Operator;
        Cond.Value = Value;
        return Cond;
    }
};

// ============================================================================

/**
 * Composite Condition: AND
 * All sub-conditions must be true.
 */
UCLASS(DisplayName = "Condition: AND (All True)")
class P_EAIS_EDITOR_API UEAIS_EditorCondition_And : public UEAIS_EditorCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Instanced, Category = "Composite")
    TArray<UEAIS_EditorCondition*> Conditions;

    virtual FAICondition ToRuntimeCondition() const override
    {
        FAICondition Cond;
        Cond.Type = EAIConditionType::And;
        for (const auto* Sub : Conditions)
        {
            if (Sub) Cond.SubConditions.Add(Sub->ToRuntimeCondition());
        }
        return Cond;
    }
};

/**
 * Composite Condition: OR
 * Any sub-condition must be true.
 */
UCLASS(DisplayName = "Condition: OR (Any True)")
class P_EAIS_EDITOR_API UEAIS_EditorCondition_Or : public UEAIS_EditorCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Instanced, Category = "Composite")
    TArray<UEAIS_EditorCondition*> Conditions;

    virtual FAICondition ToRuntimeCondition() const override
    {
        FAICondition Cond;
        Cond.Type = EAIConditionType::Or;
        for (const auto* Sub : Conditions)
        {
            if (Sub) Cond.SubConditions.Add(Sub->ToRuntimeCondition());
        }
        return Cond;
    }
};

/**
 * Composite Condition: NOT
 * Inverse of the sub-condition.
 */
UCLASS(DisplayName = "Condition: NOT (Inverse)")
class P_EAIS_EDITOR_API UEAIS_EditorCondition_Not : public UEAIS_EditorCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Instanced, Category = "Composite")
    UEAIS_EditorCondition* Condition;

    virtual FAICondition ToRuntimeCondition() const override
    {
        FAICondition Cond;
        Cond.Type = EAIConditionType::Not;
        if (Condition) Cond.SubConditions.Add(Condition->ToRuntimeCondition());
        return Cond;
    }
};

// ============================================================================

/**
 * Wrapper for Transition editing, holding the Root Condition UObject.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class P_EAIS_EDITOR_API UEAIS_EditorTransition : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Transition")
    FString To;

    UPROPERTY(EditAnywhere, Category = "Transition")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, Instanced, Category = "Transition")
    UEAIS_EditorCondition* Condition;

    FAITransition ToRuntimeTransition() const
    {
        FAITransition Trans;
        Trans.To = To;
        Trans.Priority = Priority;
        if (Condition)
        {
            Trans.Condition = Condition->ToRuntimeCondition();
        }
        return Trans;
    }
};
