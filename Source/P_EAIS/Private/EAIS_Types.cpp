/*
 * @Author: Punal Manalan
 * @Description: Implementation of EAIS type helpers
 * @Date: 29/12/2025
 */

#include "EAIS_Types.h"

FString FBlackboardValue::ToString() const
{
    switch (Type)
    {
    case EBlackboardValueType::Bool:
        return BoolValue ? TEXT("true") : TEXT("false");
    case EBlackboardValueType::Int:
        return FString::FromInt(IntValue);
    case EBlackboardValueType::Float:
        return FString::SanitizeFloat(FloatValue);
    case EBlackboardValueType::String:
        return StringValue;
    case EBlackboardValueType::Vector:
        return VectorValue.ToString();
    case EBlackboardValueType::Object:
        return ObjectValue.IsValid() ? ObjectValue->GetName() : TEXT("null");
    default:
        return TEXT("");
    }
}

void FBlackboardValue::FromString(const FString& Value)
{
    switch (Type)
    {
    case EBlackboardValueType::Bool:
        BoolValue = Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("1"));
        break;
    case EBlackboardValueType::Int:
        IntValue = FCString::Atoi(*Value);
        break;
    case EBlackboardValueType::Float:
        FloatValue = FCString::Atof(*Value);
        break;
    case EBlackboardValueType::String:
        StringValue = Value;
        break;
    case EBlackboardValueType::Vector:
        VectorValue.InitFromString(Value);
        break;
    case EBlackboardValueType::Object:
        // Cannot parse objects from string
        break;
    }
}

bool FBlackboardValue::Compare(const FBlackboardValue& Other, EAIConditionOperator Op) const
{
    // For bool and object, only == and != make sense
    if (Type == EBlackboardValueType::Bool)
    {
        switch (Op)
        {
        case EAIConditionOperator::Equal:
            return BoolValue == Other.BoolValue;
        case EAIConditionOperator::NotEqual:
            return BoolValue != Other.BoolValue;
        default:
            return false;
        }
    }

    if (Type == EBlackboardValueType::Object)
    {
        switch (Op)
        {
        case EAIConditionOperator::Equal:
            return ObjectValue == Other.ObjectValue;
        case EAIConditionOperator::NotEqual:
            return ObjectValue != Other.ObjectValue;
        default:
            return false;
        }
    }

    // For numeric types
    float A = 0.0f, B = 0.0f;
    if (Type == EBlackboardValueType::Int)
    {
        A = static_cast<float>(IntValue);
        B = static_cast<float>(Other.IntValue);
    }
    else if (Type == EBlackboardValueType::Float)
    {
        A = FloatValue;
        B = Other.FloatValue;
    }
    else if (Type == EBlackboardValueType::String)
    {
        // String comparison
        int32 Cmp = StringValue.Compare(Other.StringValue);
        switch (Op)
        {
        case EAIConditionOperator::Equal:
            return Cmp == 0;
        case EAIConditionOperator::NotEqual:
            return Cmp != 0;
        case EAIConditionOperator::GreaterThan:
            return Cmp > 0;
        case EAIConditionOperator::LessThan:
            return Cmp < 0;
        case EAIConditionOperator::GreaterOrEqual:
            return Cmp >= 0;
        case EAIConditionOperator::LessOrEqual:
            return Cmp <= 0;
        default:
            return false;
        }
    }
    else if (Type == EBlackboardValueType::Vector)
    {
        // Use vector length for comparison
        A = VectorValue.Size();
        B = Other.VectorValue.Size();
    }

    switch (Op)
    {
    case EAIConditionOperator::Equal:
        return FMath::IsNearlyEqual(A, B);
    case EAIConditionOperator::NotEqual:
        return !FMath::IsNearlyEqual(A, B);
    case EAIConditionOperator::GreaterThan:
        return A > B;
    case EAIConditionOperator::LessThan:
        return A < B;
    case EAIConditionOperator::GreaterOrEqual:
        return A >= B;
    case EAIConditionOperator::LessOrEqual:
        return A <= B;
    default:
        return false;
    }
}
