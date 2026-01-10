// Copyright Punal Manalan. All Rights Reserved.

#include "EAIS_EditorTypes.h"
#include "EAIS_Types.h"

UEAIS_EditorCondition* UEAIS_EditorCondition::FromRuntimeCondition(UObject* Outer, const FAICondition& InCondition)
{
    UEAIS_EditorCondition* Result = nullptr;

    switch (InCondition.Type)
    {
    case EAIConditionType::And:
    {
        UEAIS_EditorCondition_And* AndNode = NewObject<UEAIS_EditorCondition_And>(Outer);
        for (const FAICondition& Sub : InCondition.SubConditions)
        {
            AndNode->Conditions.Add(FromRuntimeCondition(Outer, Sub));
        }
        Result = AndNode;
        break;
    }
    case EAIConditionType::Or:
    {
        UEAIS_EditorCondition_Or* OrNode = NewObject<UEAIS_EditorCondition_Or>(Outer);
        for (const FAICondition& Sub : InCondition.SubConditions)
        {
            OrNode->Conditions.Add(FromRuntimeCondition(Outer, Sub));
        }
        Result = OrNode;
        break;
    }
    case EAIConditionType::Not:
    {
        UEAIS_EditorCondition_Not* NotNode = NewObject<UEAIS_EditorCondition_Not>(Outer);
        if (InCondition.SubConditions.Num() > 0)
        {
            NotNode->Condition = FromRuntimeCondition(Outer, InCondition.SubConditions[0]);
        }
        Result = NotNode;
        break;
    }
    default: // Leaf
    {
        UEAIS_EditorCondition_Leaf* Leaf = NewObject<UEAIS_EditorCondition_Leaf>(Outer);
        Leaf->Type = InCondition.Type;
        Leaf->Name = InCondition.Name;
        Leaf->Target = InCondition.Target;
        Leaf->Operator = InCondition.Operator;
        Leaf->Value = InCondition.Value;
        Result = Leaf;
        break;
    }
    }

    return Result;
}
