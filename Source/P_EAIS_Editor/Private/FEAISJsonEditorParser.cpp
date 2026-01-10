// Copyright Punal Manalan. All Rights Reserved.

#include "FEAISJsonEditorParser.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// Helper to parse action entry from JSON (supports both canonical JSON and runtime C++ types)
static bool ParseActionEntry(const TSharedPtr<FJsonObject>& ActObj, FAIActionEntry& OutEntry)
{
    if (!ActObj.IsValid()) return false;
    
    // Support canonical JSON field names
    if (ActObj->HasField(TEXT("actionName")))
    {
        OutEntry.Action = ActObj->GetStringField(TEXT("actionName"));
    }
    else if (ActObj->HasField(TEXT("Action")))
    {
        OutEntry.Action = ActObj->GetStringField(TEXT("Action"));
    }
    
    // Parse params - could be JSON string or object
    if (ActObj->HasField(TEXT("paramsJson")))
    {
        // For canonical format, we need to parse the JSON string into FAIActionParams
        // For now, just store in Target as a simple approach
        FString ParamsJsonStr = ActObj->GetStringField(TEXT("paramsJson"));
        // Parse the JSON string to extract params
        TSharedPtr<FJsonObject> ParamsObj;
        if (!ParamsJsonStr.IsEmpty() && ParamsJsonStr != TEXT("{}"))
        {
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ParamsJsonStr);
            FJsonSerializer::Deserialize(Reader, ParamsObj);
            if (ParamsObj.IsValid())
            {
                ParamsObj->TryGetStringField(TEXT("target"), OutEntry.Params.Target);
                ParamsObj->TryGetNumberField(TEXT("power"), OutEntry.Params.Power);
            }
        }
    }
    else if (ActObj->HasField(TEXT("Params")))
    {
        const TSharedPtr<FJsonObject>* ParamsObj = nullptr;
        if (ActObj->TryGetObjectField(TEXT("Params"), ParamsObj) && ParamsObj)
        {
            (*ParamsObj)->TryGetStringField(TEXT("Target"), OutEntry.Params.Target);
            (*ParamsObj)->TryGetNumberField(TEXT("Power"), OutEntry.Params.Power);
        }
    }
    
    return true;
}

// Helper to parse condition from JSON (supports both canonical JSON and runtime C++ types)
// Helper to parse condition from JSON (supports both canonical JSON and runtime C++ types)
static bool ParseCondition(const TSharedPtr<FJsonObject>& CondObj, FAICondition& OutCond)
{
    if (!CondObj.IsValid()) return false;
    
    FString TypeStr = CondObj->GetStringField(TEXT("type"));
    if (TypeStr == TEXT("Blackboard")) OutCond.Type = EAIConditionType::Blackboard;
    else if (TypeStr == TEXT("Event")) OutCond.Type = EAIConditionType::Event;
    else if (TypeStr == TEXT("Timer")) OutCond.Type = EAIConditionType::Timer;
    else if (TypeStr == TEXT("Distance")) OutCond.Type = EAIConditionType::Distance;
    else if (TypeStr == TEXT("And")) OutCond.Type = EAIConditionType::And;
    else if (TypeStr == TEXT("Or")) OutCond.Type = EAIConditionType::Or;
    else if (TypeStr == TEXT("Not")) OutCond.Type = EAIConditionType::Not;
    
    // Support canonical JSON field names
    if (CondObj->HasField(TEXT("keyOrName")))
    {
        OutCond.Name = CondObj->GetStringField(TEXT("keyOrName"));
    }
    else if (CondObj->HasField(TEXT("name")))
    {
        OutCond.Name = CondObj->GetStringField(TEXT("name"));
    }
    else if (CondObj->HasField(TEXT("key")))
    {
        OutCond.Name = CondObj->GetStringField(TEXT("key"));
    }
    
    FString OpStr;
    if (CondObj->TryGetStringField(TEXT("op"), OpStr))
    {
        if (OpStr == TEXT("Equal")) OutCond.Operator = EAIConditionOperator::Equal;
        else if (OpStr == TEXT("NotEqual")) OutCond.Operator = EAIConditionOperator::NotEqual;
        else if (OpStr == TEXT("GreaterThan")) OutCond.Operator = EAIConditionOperator::GreaterThan;
        else if (OpStr == TEXT("LessThan")) OutCond.Operator = EAIConditionOperator::LessThan;
        else if (OpStr == TEXT("GreaterOrEqual")) OutCond.Operator = EAIConditionOperator::GreaterOrEqual;
        else if (OpStr == TEXT("LessOrEqual")) OutCond.Operator = EAIConditionOperator::LessOrEqual;
    }
    
    // Parse compareValue (canonical format) or value (legacy)
    const TSharedPtr<FJsonObject>* CompareValueObj = nullptr;
    if (CondObj->TryGetObjectField(TEXT("compareValue"), CompareValueObj) && CompareValueObj)
    {
        OutCond.Value = (*CompareValueObj)->GetStringField(TEXT("rawValue"));
    }
    else if (CondObj->HasField(TEXT("value")))
    {
        // Legacy format - value is directly a string/bool/number
        const TSharedPtr<FJsonValue>& ValField = CondObj->Values.FindRef(TEXT("value"));
        if (ValField.IsValid())
        {
            if (ValField->Type == EJson::Boolean)
            {
                OutCond.Value = ValField->AsBool() ? TEXT("true") : TEXT("false");
            }
            else if (ValField->Type == EJson::Number)
            {
                OutCond.Value = FString::SanitizeFloat(ValField->AsNumber());
            }
            else
            {
                OutCond.Value = ValField->AsString();
            }
        }
    }
    
    CondObj->TryGetNumberField(TEXT("seconds"), OutCond.Seconds);
    CondObj->TryGetStringField(TEXT("target"), OutCond.Target);

    // Recursively parse sub-conditions for composite types
    const TArray<TSharedPtr<FJsonValue>>* SubCondsArr = nullptr;
    if (CondObj->TryGetArrayField(TEXT("conditions"), SubCondsArr))
    {
        for (const TSharedPtr<FJsonValue>& SubVal : *SubCondsArr)
        {
            const TSharedPtr<FJsonObject>* SubObj;
            if (SubVal->TryGetObject(SubObj))
            {
                FAICondition SubCond;
                ParseCondition(*SubObj, SubCond);
                OutCond.SubConditions.Add(SubCond);
            }
        }
    }
    
    return true;
}

// Helper to parse transition from JSON
static bool ParseTransition(const TSharedPtr<FJsonObject>& TransObj, FAITransition& OutTrans)
{
    if (!TransObj.IsValid()) return false;
    
    OutTrans.To = TransObj->GetStringField(TEXT("to"));
    TransObj->TryGetNumberField(TEXT("priority"), OutTrans.Priority);
    
    const TSharedPtr<FJsonObject>* CondObj = nullptr;
    if (TransObj->TryGetObjectField(TEXT("condition"), CondObj) && CondObj)
    {
        ParseCondition(*CondObj, OutTrans.Condition);
    }
    
    return true;
}

bool FEAISJsonEditorParser::ParseEditorJson(const FString& Json, FAIEditorGraph& OutGraph, FString& OutError)
{
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    TSharedPtr<FJsonObject> Root;

    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Invalid JSON");
        return false;
    }

    OutGraph.Name = Root->GetStringField(TEXT("name"));
    OutGraph.InitialState = Root->GetStringField(TEXT("initialState"));

    const TArray<TSharedPtr<FJsonValue>>* StatesArrayPtr = nullptr;
    if (Root->TryGetArrayField(TEXT("states"), StatesArrayPtr))
    {
        for (const TSharedPtr<FJsonValue>& Val : *StatesArrayPtr)
        {
            const TSharedPtr<FJsonObject>* StateObjPtr;
            if (!Val->TryGetObject(StateObjPtr)) continue;
            
            FAIEditorGraph::FEditorState S;
            S.Id = (*StateObjPtr)->GetStringField(TEXT("id"));
            (*StateObjPtr)->TryGetBoolField(TEXT("terminal"), S.bTerminal);
            
            // Parse action arrays
            const TArray<TSharedPtr<FJsonValue>>* Arr;
            if ((*StateObjPtr)->TryGetArrayField(TEXT("onEnter"), Arr))
            {
                for (const TSharedPtr<FJsonValue>& A : *Arr)
                {
                    const TSharedPtr<FJsonObject>* ActObj;
                    if (!A->TryGetObject(ActObj)) continue;
                    FAIActionEntry E;
                    ParseActionEntry(*ActObj, E);
                    S.OnEnter.Add(MoveTemp(E));
                }
            }
            if ((*StateObjPtr)->TryGetArrayField(TEXT("onTick"), Arr))
            {
                for (const TSharedPtr<FJsonValue>& A : *Arr)
                {
                    const TSharedPtr<FJsonObject>* ActObj;
                    if (!A->TryGetObject(ActObj)) continue;
                    FAIActionEntry E;
                    ParseActionEntry(*ActObj, E);
                    S.OnTick.Add(MoveTemp(E));
                }
            }
            if ((*StateObjPtr)->TryGetArrayField(TEXT("onExit"), Arr))
            {
                for (const TSharedPtr<FJsonValue>& A : *Arr)
                {
                    const TSharedPtr<FJsonObject>* ActObj;
                    if (!A->TryGetObject(ActObj)) continue;
                    FAIActionEntry E;
                    ParseActionEntry(*ActObj, E);
                    S.OnExit.Add(MoveTemp(E));
                }
            }
            
            // Parse transitions
            if ((*StateObjPtr)->TryGetArrayField(TEXT("transitions"), Arr))
            {
                for (const TSharedPtr<FJsonValue>& T : *Arr)
                {
                    const TSharedPtr<FJsonObject>* TransObj;
                    if (!T->TryGetObject(TransObj)) continue;
                    FAITransition Trans;
                    ParseTransition(*TransObj, Trans);
                    S.Transitions.Add(MoveTemp(Trans));
                }
            }
            
            OutGraph.States.Add(MoveTemp(S));
        }
    }
    else
    {
        OutError = TEXT("Editor JSON must include 'states' array");
        return false;
    }

    // Store editor metadata
    const TSharedPtr<FJsonObject>* EditorObj = nullptr;
    if (Root->TryGetObjectField(TEXT("editor"), EditorObj) && EditorObj)
    {
        OutGraph.EditorMetadata = *EditorObj;
    }

    return true;
}

bool FEAISJsonEditorParser::ConvertEditorGraphToRuntime(const FAIEditorGraph& InGraph, FAIBehaviorDef& OutDef, FString& OutError)
{
    OutDef.Name = InGraph.Name;
    OutDef.InitialState = InGraph.InitialState;

    for (const auto& EState : InGraph.States)
    {
        FAIState S;
        S.Id = EState.Id;
        S.bTerminal = EState.bTerminal;
        S.OnEnter = EState.OnEnter;
        S.OnTick = EState.OnTick;
        S.OnExit = EState.OnExit;
        S.Transitions = EState.Transitions;
        OutDef.States.Add(S);
    }

    if (OutDef.InitialState.IsEmpty())
    {
        OutError = TEXT("InitialState empty after conversion");
        return false;
    }

    OutDef.bIsValid = false; // Caller must run full validator
    return true;
}

FString FEAISJsonEditorParser::SerializeEditorGraph(const FAIEditorGraph& InGraph)
{
    // TODO: Implement full serialization
    return TEXT("{}");
}

// Helper to get condition type as string
static FString ConditionTypeToString(EAIConditionType Type)
{
    switch (Type)
    {
        case EAIConditionType::Blackboard: return TEXT("Blackboard");
        case EAIConditionType::Event: return TEXT("Event");
        case EAIConditionType::Timer: return TEXT("Timer");
        case EAIConditionType::Distance: return TEXT("Distance");
        default: return TEXT("Blackboard");
    }
}

// Helper to get operator as string
static FString OperatorToString(EAIConditionOperator Op)
{
    switch (Op)
    {
        case EAIConditionOperator::Equal: return TEXT("Equal");
        case EAIConditionOperator::NotEqual: return TEXT("NotEqual");
        case EAIConditionOperator::GreaterThan: return TEXT("GreaterThan");
        case EAIConditionOperator::LessThan: return TEXT("LessThan");
        case EAIConditionOperator::GreaterOrEqual: return TEXT("GreaterOrEqual");
        case EAIConditionOperator::LessOrEqual: return TEXT("LessOrEqual");
        default: return TEXT("Equal");
    }
}

// Helper to get value type as string
static FString ValueTypeToString(EBlackboardValueType Type)
{
    switch (Type)
    {
        case EBlackboardValueType::Bool: return TEXT("Bool");
        case EBlackboardValueType::Int: return TEXT("Int");
        case EBlackboardValueType::Float: return TEXT("Float");
        case EBlackboardValueType::String: return TEXT("String");
        case EBlackboardValueType::Vector: return TEXT("Vector");
        case EBlackboardValueType::Object: return TEXT("Object");
        default: return TEXT("String");
    }
}

FString FEAISJsonSerializer::SerializeRuntime(const FAIBehaviorDef& InDef)
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("name"), InDef.Name);
    Root->SetStringField(TEXT("initialState"), InDef.InitialState);
    
    // Serialize blackboard
    TArray<TSharedPtr<FJsonValue>> BlackboardArr;
    for (const FEAISBlackboardEntry& Entry : InDef.Blackboard)
    {
        TSharedPtr<FJsonObject> EntryObj = MakeShared<FJsonObject>();
        EntryObj->SetStringField(TEXT("key"), Entry.Key);
        
        TSharedPtr<FJsonObject> ValueObj = MakeShared<FJsonObject>();
        ValueObj->SetStringField(TEXT("type"), ValueTypeToString(Entry.Value.Type));
        ValueObj->SetStringField(TEXT("rawValue"), Entry.Value.RawValue);
        EntryObj->SetObjectField(TEXT("value"), ValueObj);
        
        BlackboardArr.Add(MakeShared<FJsonValueObject>(EntryObj));
    }
    Root->SetArrayField(TEXT("blackboard"), BlackboardArr);
    
    // Serialize states
    TArray<TSharedPtr<FJsonValue>> StatesArr;
    for (const FAIState& State : InDef.States)
    {
        TSharedPtr<FJsonObject> StateObj = MakeShared<FJsonObject>();
        StateObj->SetStringField(TEXT("id"), State.Id);
        StateObj->SetBoolField(TEXT("terminal"), State.bTerminal);
        
        // Serialize actions
        auto SerializeActions = [](const TArray<FAIActionEntry>& Actions) -> TArray<TSharedPtr<FJsonValue>>
        {
            TArray<TSharedPtr<FJsonValue>> Arr;
            for (const FAIActionEntry& Entry : Actions)
            {
                TSharedPtr<FJsonObject> ActObj = MakeShared<FJsonObject>();
                ActObj->SetStringField(TEXT("actionName"), Entry.Action);
                // Serialize params to JSON string
                TSharedPtr<FJsonObject> ParamsObj = MakeShared<FJsonObject>();
                if (!Entry.Params.Target.IsEmpty())
                {
                    ParamsObj->SetStringField(TEXT("target"), Entry.Params.Target);
                }
                if (Entry.Params.Power != 1.0f)
                {
                    ParamsObj->SetNumberField(TEXT("power"), Entry.Params.Power);
                }
                FString ParamsJson;
                TSharedRef<TJsonWriter<>> ParamsWriter = TJsonWriterFactory<>::Create(&ParamsJson);
                FJsonSerializer::Serialize(ParamsObj.ToSharedRef(), ParamsWriter);
                ActObj->SetStringField(TEXT("paramsJson"), ParamsJson);
                Arr.Add(MakeShared<FJsonValueObject>(ActObj));
            }
            return Arr;
        };
        
        StateObj->SetArrayField(TEXT("onEnter"), SerializeActions(State.OnEnter));
        StateObj->SetArrayField(TEXT("onTick"), SerializeActions(State.OnTick));
        StateObj->SetArrayField(TEXT("onExit"), SerializeActions(State.OnExit));
        
        // Helper to serialize condition recursively
        auto SerializeCondition = [&](auto& Self, const FAICondition& Cond) -> TSharedPtr<FJsonObject>
        {
            TSharedPtr<FJsonObject> CondObj = MakeShared<FJsonObject>();
            CondObj->SetStringField(TEXT("type"), ConditionTypeToString(Cond.Type));
            
            // Only leaf conditions use these fields
            if (Cond.Type != EAIConditionType::And && 
                Cond.Type != EAIConditionType::Or && 
                Cond.Type != EAIConditionType::Not)
            {
                CondObj->SetStringField(TEXT("keyOrName"), Cond.Name);
                CondObj->SetStringField(TEXT("op"), OperatorToString(Cond.Operator));
                
                TSharedPtr<FJsonObject> CompareObj = MakeShared<FJsonObject>();
                CompareObj->SetStringField(TEXT("type"), TEXT("String")); // Default to string
                CompareObj->SetStringField(TEXT("rawValue"), Cond.Value);
                CondObj->SetObjectField(TEXT("compareValue"), CompareObj);
                
                if (Cond.Seconds > 0.f)
                {
                    CondObj->SetNumberField(TEXT("seconds"), Cond.Seconds);
                }
                if (!Cond.Target.IsEmpty())
                {
                    CondObj->SetStringField(TEXT("target"), Cond.Target);
                }
            }
            else
            {
                // Composite conditions have 'conditions' array
                TArray<TSharedPtr<FJsonValue>> SubArr;
                for (const FAICondition& Sub : Cond.SubConditions)
                {
                    SubArr.Add(MakeShared<FJsonValueObject>(Self(Self, Sub)));
                }
                CondObj->SetArrayField(TEXT("conditions"), SubArr);
            }
            
            return CondObj;
        };

        // Serialize transitions
        TArray<TSharedPtr<FJsonValue>> TransArr;
        for (const FAITransition& Trans : State.Transitions)
        {
            TSharedPtr<FJsonObject> TransObj = MakeShared<FJsonObject>();
            TransObj->SetStringField(TEXT("to"), Trans.To);
            TransObj->SetNumberField(TEXT("priority"), Trans.Priority);
            
            // Allow lambda recursion
            auto RecursiveSerializer = [&](const FAICondition& C) { return SerializeCondition(SerializeCondition, C); };
            TransObj->SetObjectField(TEXT("condition"), RecursiveSerializer(Trans.Condition));
            
            TransArr.Add(MakeShared<FJsonValueObject>(TransObj));
        }
        StateObj->SetArrayField(TEXT("transitions"), TransArr);
        
        StatesArr.Add(MakeShared<FJsonValueObject>(StateObj));
    }
    Root->SetArrayField(TEXT("states"), StatesArr);
    
    FString OutJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    
    return OutJson;
}
