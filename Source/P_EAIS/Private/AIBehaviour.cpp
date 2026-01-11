// Copyright Punal Manalan. All Rights Reserved.

#include "AIBehaviour.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
// Include P_MEIS for input injection
#include "Manager/CPP_BPL_InputBinding.h"

/** Helper to parse a condition recursively from JSON */
static void ParseConditionInternal(const TSharedPtr<FJsonObject>& CondObj, FAICondition& OutCond)
{
    if (!CondObj.IsValid()) return;

    FString TypeStr;
    if (CondObj->TryGetStringField(TEXT("type"), TypeStr))
    {
        if (TypeStr.Equals(TEXT("Event"), ESearchCase::IgnoreCase)) OutCond.Type = EAIConditionType::Event;
        else if (TypeStr.Equals(TEXT("Timer"), ESearchCase::IgnoreCase)) OutCond.Type = EAIConditionType::Timer;
        else if (TypeStr.Equals(TEXT("Distance"), ESearchCase::IgnoreCase)) OutCond.Type = EAIConditionType::Distance;
        else if (TypeStr.Equals(TEXT("And"), ESearchCase::IgnoreCase)) OutCond.Type = EAIConditionType::And;
        else if (TypeStr.Equals(TEXT("Or"), ESearchCase::IgnoreCase)) OutCond.Type = EAIConditionType::Or;
        else if (TypeStr.Equals(TEXT("Not"), ESearchCase::IgnoreCase)) OutCond.Type = EAIConditionType::Not;
        else OutCond.Type = EAIConditionType::Blackboard;
    }

    // Handle Composite Conditions
    if (OutCond.Type == EAIConditionType::And || 
        OutCond.Type == EAIConditionType::Or || 
        OutCond.Type == EAIConditionType::Not)
    {
        const TArray<TSharedPtr<FJsonValue>>* SubCondsArray = nullptr;
        if (CondObj->TryGetArrayField(TEXT("conditions"), SubCondsArray))
        {
            for (const auto& SubCondVal : *SubCondsArray)
            {
                if (SubCondVal->Type == EJson::Object)
                {
                    FAICondition SubCond;
                    ParseConditionInternal(SubCondVal->AsObject(), SubCond);
                    OutCond.SubConditions.Add(SubCond);
                }
            }
        }
    }
    
    CondObj->TryGetStringField(TEXT("name"), OutCond.Name);
    if (OutCond.Name.IsEmpty()) CondObj->TryGetStringField(TEXT("key"), OutCond.Name);
    if (OutCond.Name.IsEmpty()) CondObj->TryGetStringField(TEXT("keyOrName"), OutCond.Name);
    
    CondObj->TryGetStringField(TEXT("target"), OutCond.Target);
    
    // Parse value
    TSharedPtr<FJsonValue> ValueField = CondObj->TryGetField(TEXT("value"));
    if (!ValueField.IsValid()) ValueField = CondObj->TryGetField(TEXT("compareValue"));
    
    if (ValueField.IsValid())
    {
        if (ValueField->Type == EJson::Boolean) OutCond.Value = ValueField->AsBool() ? TEXT("true") : TEXT("false");
        else if (ValueField->Type == EJson::Object) 
        {
            ValueField->AsObject()->TryGetStringField(TEXT("rawValue"), OutCond.Value);
        }
        else if (ValueField->Type == EJson::String) OutCond.Value = ValueField->AsString();
        else if (ValueField->Type == EJson::Number) OutCond.Value = FString::SanitizeFloat(ValueField->AsNumber());
    }
    
    if (OutCond.Type == EAIConditionType::Timer)
    {
        CondObj->TryGetNumberField(TEXT("seconds"), OutCond.Seconds);
    }
    
    FString OpStr;
    if (CondObj->TryGetStringField(TEXT("op"), OpStr))
    {
        if (OpStr == TEXT("==") || OpStr.Equals(TEXT("Equal"), ESearchCase::IgnoreCase)) OutCond.Operator = EAIConditionOperator::Equal;
        else if (OpStr == TEXT("!=") || OpStr.Equals(TEXT("NotEqual"), ESearchCase::IgnoreCase)) OutCond.Operator = EAIConditionOperator::NotEqual;
        else if (OpStr == TEXT(">") || OpStr.Equals(TEXT("GreaterThan"), ESearchCase::IgnoreCase)) OutCond.Operator = EAIConditionOperator::GreaterThan;
        else if (OpStr == TEXT("<") || OpStr.Equals(TEXT("LessThan"), ESearchCase::IgnoreCase)) OutCond.Operator = EAIConditionOperator::LessThan;
        else if (OpStr == TEXT(">=") || OpStr.Equals(TEXT("GreaterOrEqual"), ESearchCase::IgnoreCase)) OutCond.Operator = EAIConditionOperator::GreaterOrEqual;
        else if (OpStr == TEXT("<=") || OpStr.Equals(TEXT("LessOrEqual"), ESearchCase::IgnoreCase)) OutCond.Operator = EAIConditionOperator::LessOrEqual;
    }
}

/** Helper to parse action params from JSON */
static void ParseActionParamsInternal(const TSharedPtr<FJsonObject>& ParamsObj, FAIActionParams& OutParams)
{
    if (!ParamsObj.IsValid()) return;

    // Standard fields
    if (!ParamsObj->TryGetStringField(TEXT("target"), OutParams.Target))
        ParamsObj->TryGetStringField(TEXT("Target"), OutParams.Target);
    
    if (ParamsObj->HasField(TEXT("power"))) OutParams.Power = ParamsObj->GetNumberField(TEXT("power"));
    else if (ParamsObj->HasField(TEXT("Power"))) OutParams.Power = ParamsObj->GetNumberField(TEXT("Power"));
    else if (ParamsObj->HasField(TEXT("speed"))) OutParams.Power = ParamsObj->GetNumberField(TEXT("speed"));
    else if (ParamsObj->HasField(TEXT("Speed"))) OutParams.Power = ParamsObj->GetNumberField(TEXT("Speed"));

    // Flatten all fields to ExtraParams
    for (const auto& Pair : ParamsObj->Values)
    {
        if (Pair.Value->Type == EJson::String)
            OutParams.ExtraParams.Add(Pair.Key, Pair.Value->AsString());
        else if (Pair.Value->Type == EJson::Number)
            OutParams.ExtraParams.Add(Pair.Key, FString::SanitizeFloat(Pair.Value->AsNumber()));
        else if (Pair.Value->Type == EJson::Boolean)
            OutParams.ExtraParams.Add(Pair.Key, Pair.Value->AsBool() ? TEXT("true") : TEXT("false"));
        else if (Pair.Value->Type == EJson::Object)
        {
             TSharedPtr<FJsonObject> SubObj = Pair.Value->AsObject();
             for (const auto& SubPair : SubObj->Values)
             {
                 if (SubPair.Value->Type == EJson::String)
                     OutParams.ExtraParams.Add(SubPair.Key, SubPair.Value->AsString());
                 else if (SubPair.Value->Type == EJson::Number)
                     OutParams.ExtraParams.Add(SubPair.Key, FString::SanitizeFloat(SubPair.Value->AsNumber()));
                 else if (SubPair.Value->Type == EJson::Boolean)
                     OutParams.ExtraParams.Add(SubPair.Key, SubPair.Value->AsBool() ? TEXT("true") : TEXT("false"));
             }
        }
    }
}

UAIBehaviour::UAIBehaviour()
{
    BehaviorName = TEXT("NewBehavior");
}

FString UAIBehaviour::GetJsonContent() const
{
    // Prefer external file if specified
    if (!JsonFilePath.IsEmpty())
    {
        FString FullPath = FPaths::ProjectContentDir() / TEXT("AIProfiles") / JsonFilePath;
        FString FileContent;
        if (FFileHelper::LoadFileToString(FileContent, *FullPath))
        {
            return FileContent;
        }
        UE_LOG(LogTemp, Warning, TEXT("UAIBehaviour: Failed to load JSON from %s"), *FullPath);
    }
    
    return EmbeddedJson;
}

bool UAIBehaviour::ParseBehavior(FString& OutError)
{
    FString JsonContent = GetJsonContent();
    if (JsonContent.IsEmpty())
    {
        OutError = TEXT("No JSON content available");
        ParsedBehavior.bIsValid = false;
        return false;
    }

    return ParseJsonInternal(JsonContent, ParsedBehavior, OutError);
}

bool UAIBehaviour::ReloadFromFile(FString& OutError)
{
    if (JsonFilePath.IsEmpty())
    {
        OutError = TEXT("No external file path specified");
        return false;
    }

    return ParseBehavior(OutError);
}

FPrimaryAssetId UAIBehaviour::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("AIBehaviour"), GetFName());
}

#if WITH_EDITOR
void UAIBehaviour::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Re-parse when JSON changes
    FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UAIBehaviour, EmbeddedJson) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UAIBehaviour, JsonFilePath))
    {
        FString Error;
        ParseBehavior(Error);
        if (!Error.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("UAIBehaviour::PostEditChangeProperty - Parse error: %s"), *Error);
        }
    }
}
#endif

bool UAIBehaviour::ParseJsonInternal(const FString& JsonString, FAIBehaviorDef& OutDef, FString& OutError)
{
    OutDef = FAIBehaviorDef();
    OutDef.bIsValid = false;

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON");
        return false;
    }

    // Parse name
    if (RootObject->HasField(TEXT("name")))
    {
        OutDef.Name = RootObject->GetStringField(TEXT("name"));
    }
    else if (RootObject->HasField(TEXT("Name")))
    {
        OutDef.Name = RootObject->GetStringField(TEXT("Name"));
    }
    else
    {
        OutDef.Name = BehaviorName;
        UE_LOG(LogTemp, Warning, TEXT("AIBehaviour: JSON missing 'name' field. Using fallback."));
    }

    // Parse blackboard (can be Object or Array format)
    TSharedPtr<FJsonObject> BlackboardObj;
    const TArray<TSharedPtr<FJsonValue>>* BlackboardArray = nullptr;
    
    if (RootObject->HasTypedField<EJson::Object>(TEXT("blackboard")))
    {
        BlackboardObj = RootObject->GetObjectField(TEXT("blackboard"));
    }
    else if (RootObject->HasTypedField<EJson::Object>(TEXT("Blackboard")))
    {
        BlackboardObj = RootObject->GetObjectField(TEXT("Blackboard"));
    }
    else if (RootObject->HasTypedField<EJson::Array>(TEXT("blackboard")))
    {
        RootObject->TryGetArrayField(TEXT("blackboard"), BlackboardArray);
    }
    else if (RootObject->HasTypedField<EJson::Array>(TEXT("Blackboard")))
    {
        RootObject->TryGetArrayField(TEXT("Blackboard"), BlackboardArray);
    }

    // Handle Object format
    if (BlackboardObj.IsValid())
    {
        for (const auto& Pair : BlackboardObj->Values)
        {
            FEAISBlackboardEntry Entry;
            Entry.Key = Pair.Key;
            
            if (Pair.Value->Type == EJson::Boolean)
            {
                Entry.Value.Type = EBlackboardValueType::Bool;
                Entry.Value.RawValue = Pair.Value->AsBool() ? TEXT("true") : TEXT("false");
                Entry.Value.BoolValue = Pair.Value->AsBool();
            }
            else if (Pair.Value->Type == EJson::Number)
            {
                Entry.Value.Type = EBlackboardValueType::Float;
                Entry.Value.RawValue = FString::SanitizeFloat(Pair.Value->AsNumber());
                Entry.Value.FloatValue = Pair.Value->AsNumber();
            }
            else if (Pair.Value->Type == EJson::String)
            {
                Entry.Value.Type = EBlackboardValueType::String;
                Entry.Value.RawValue = Pair.Value->AsString();
                Entry.Value.StringValue = Pair.Value->AsString();
            }
            else if (Pair.Value->Type == EJson::Null)
            {
                Entry.Value.Type = EBlackboardValueType::Object;
                Entry.Value.RawValue = TEXT("null");
            }

            OutDef.Blackboard.Add(Entry);
        }
    }
    // Handle Array format: [{ "key": "...", "value": { "type": "...", "rawValue": "..." } }]
    else if (BlackboardArray)
    {
        for (const TSharedPtr<FJsonValue>& EntryVal : *BlackboardArray)
        {
            if (EntryVal->Type != EJson::Object) continue;
            
            TSharedPtr<FJsonObject> EntryObj = EntryVal->AsObject();
            FEAISBlackboardEntry Entry;
            
            Entry.Key = EntryObj->GetStringField(TEXT("key"));
            
            TSharedPtr<FJsonObject> ValueObj;
            if (EntryObj->HasTypedField<EJson::Object>(TEXT("value")))
            {
                ValueObj = EntryObj->GetObjectField(TEXT("value"));
            }
            
            if (ValueObj.IsValid())
            {
                FString TypeStr = ValueObj->GetStringField(TEXT("type"));
                // Entry.Value.RawValue = ValueObj->GetStringField(TEXT("rawValue"));
                if (ValueObj->HasTypedField<EJson::String>(TEXT("rawValue")))
                {
                    Entry.Value.RawValue = ValueObj->GetStringField(TEXT("rawValue"));
                }
                else
                {
                     Entry.Value.RawValue = TEXT("");
                }
                
                if (TypeStr.Equals(TEXT("Bool"), ESearchCase::IgnoreCase))
                {
                    Entry.Value.Type = EBlackboardValueType::Bool;
                    Entry.Value.BoolValue = Entry.Value.RawValue.Equals(TEXT("true"), ESearchCase::IgnoreCase);
                }
                else if (TypeStr.Equals(TEXT("Int"), ESearchCase::IgnoreCase))
                {
                    Entry.Value.Type = EBlackboardValueType::Int;
                    Entry.Value.IntValue = FCString::Atoi(*Entry.Value.RawValue);
                }
                else if (TypeStr.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
                {
                    Entry.Value.Type = EBlackboardValueType::Float;
                    Entry.Value.FloatValue = FCString::Atof(*Entry.Value.RawValue);
                }
                else if (TypeStr.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
                {
                    Entry.Value.Type = EBlackboardValueType::Vector;
                    Entry.Value.VectorValue.InitFromString(Entry.Value.RawValue);
                }
                else
                {
                    Entry.Value.Type = EBlackboardValueType::String;
                    Entry.Value.StringValue = Entry.Value.RawValue;
                }
            }
            
            OutDef.Blackboard.Add(Entry);
        }
    }

    // Parse states
    const TArray<TSharedPtr<FJsonValue>>* StatesArray = nullptr;
    if (RootObject->HasTypedField<EJson::Array>(TEXT("states")))
    {
        RootObject->TryGetArrayField(TEXT("states"), StatesArray);
    }
    else if (RootObject->HasTypedField<EJson::Array>(TEXT("States")))
    {
        RootObject->TryGetArrayField(TEXT("States"), StatesArray);
    }
    else if (RootObject->HasTypedField<EJson::Object>(TEXT("States")))
    {
        // States as object with keys
        TSharedPtr<FJsonObject> StatesObj = RootObject->GetObjectField(TEXT("States"));
        for (const auto& StatePair : StatesObj->Values)
        {
            if (StatePair.Value->Type == EJson::Object)
            {
                FAIState State;
                State.Id = StatePair.Key;
                
                TSharedPtr<FJsonObject> StateObj = StatePair.Value->AsObject();
                
                // Parse OnEnter
                const TArray<TSharedPtr<FJsonValue>>* OnEnterArray = nullptr;
                if (StateObj->TryGetArrayField(TEXT("OnEnter"), OnEnterArray) ||
                    StateObj->TryGetArrayField(TEXT("onEnter"), OnEnterArray))
                {
                    for (const auto& ActionVal : *OnEnterArray)
                    {
                        if (ActionVal->Type == EJson::Object)
                        {
                            FAIActionEntry Entry;
                            TSharedPtr<FJsonObject> ActionObj = ActionVal->AsObject();
                            
                            // Parse action name (try multiple field names)
                            if (!ActionObj->TryGetStringField(TEXT("Action"), Entry.Action))
                            {
                                if (!ActionObj->TryGetStringField(TEXT("action"), Entry.Action))
                                {
                                    ActionObj->TryGetStringField(TEXT("actionName"), Entry.Action);
                                }
                            }
                            
                            if (Entry.Action.IsEmpty())
                            {
                                UE_LOG(LogTemp, Warning, TEXT("AIBehaviour: Action object missing 'action' name"));
                                continue;
                            }
                            
                            // Parse params using helper
                            TSharedPtr<FJsonObject> ParamsObj;
                            if (ActionObj->HasTypedField<EJson::Object>(TEXT("params")))
                                ParamsObj = ActionObj->GetObjectField(TEXT("params"));
                            else if (ActionObj->HasTypedField<EJson::Object>(TEXT("Params")))
                                ParamsObj = ActionObj->GetObjectField(TEXT("Params"));
                            else if (ActionObj->HasTypedField<EJson::String>(TEXT("paramsJson")))
                            {
                                FString ParamsJsonStr;
                                if (ActionObj->TryGetStringField(TEXT("paramsJson"), ParamsJsonStr))
                                {
                                    TSharedRef<TJsonReader<>> ParamsReader = TJsonReaderFactory<>::Create(ParamsJsonStr);
                                    FJsonSerializer::Deserialize(ParamsReader, ParamsObj);
                                }
                            }
                            
                            if (ParamsObj.IsValid())
                            {
                                ParseActionParamsInternal(ParamsObj, Entry.Params);
                            }

                            State.OnEnter.Add(Entry);
                        }
                    }
                }

                // Parse OnTick
                const TArray<TSharedPtr<FJsonValue>>* OnTickArray = nullptr;
                if (StateObj->TryGetArrayField(TEXT("OnTick"), OnTickArray) ||
                    StateObj->TryGetArrayField(TEXT("onTick"), OnTickArray))
                {
                    for (const auto& ActionVal : *OnTickArray)
                    {
                        if (ActionVal->Type == EJson::Object)
                        {
                            FAIActionEntry Entry;
                            TSharedPtr<FJsonObject> ActionObj = ActionVal->AsObject();
                            
                            // Parse action name (try multiple field names)
                            if (!ActionObj->TryGetStringField(TEXT("Action"), Entry.Action))
                            {
                                if (!ActionObj->TryGetStringField(TEXT("action"), Entry.Action))
                                {
                                    ActionObj->TryGetStringField(TEXT("actionName"), Entry.Action);
                                }
                            }

                            if (Entry.Action.IsEmpty())
                            {
                                UE_LOG(LogTemp, Warning, TEXT("AIBehaviour: Action object missing 'action' name"));
                                continue;
                            }
                            
                            // Parse params using helper
                            TSharedPtr<FJsonObject> ParamsObj;
                            if (ActionObj->HasTypedField<EJson::Object>(TEXT("params")))
                                ParamsObj = ActionObj->GetObjectField(TEXT("params"));
                            else if (ActionObj->HasTypedField<EJson::Object>(TEXT("Params")))
                                ParamsObj = ActionObj->GetObjectField(TEXT("Params"));
                            else if (ActionObj->HasTypedField<EJson::String>(TEXT("paramsJson")))
                            {
                                FString ParamsJsonStr;
                                if (ActionObj->TryGetStringField(TEXT("paramsJson"), ParamsJsonStr))
                                {
                                    TSharedRef<TJsonReader<>> ParamsReader = TJsonReaderFactory<>::Create(ParamsJsonStr);
                                    FJsonSerializer::Deserialize(ParamsReader, ParamsObj);
                                }
                            }
                            
                            if (ParamsObj.IsValid())
                            {
                                ParseActionParamsInternal(ParamsObj, Entry.Params);
                            }

                            State.OnTick.Add(Entry);
                        }
                    }
                }

                // Parse Transitions
                const TArray<TSharedPtr<FJsonValue>>* TransitionsArray = nullptr;
                if (StateObj->TryGetArrayField(TEXT("Transitions"), TransitionsArray) ||
                    StateObj->TryGetArrayField(TEXT("transitions"), TransitionsArray))
                {
                    for (const auto& TransVal : *TransitionsArray)
                    {
                        if (TransVal->Type == EJson::Object)
                        {
                            FAITransition Trans;
                            TSharedPtr<FJsonObject> TransObj = TransVal->AsObject();
                            if (!TransObj->TryGetStringField(TEXT("Target"), Trans.To))
                            {
                                TransObj->TryGetStringField(TEXT("to"), Trans.To);
                            }
                            
                            // Parse condition - check type first to avoid errors
                            TSharedPtr<FJsonValue> ConditionField = TransObj->TryGetField(TEXT("Condition"));
                            if (!ConditionField.IsValid())
                            {
                                ConditionField = TransObj->TryGetField(TEXT("condition"));
                            }
                            
                            // Default condition (for string shortcuts)
                            Trans.Condition.Type = EAIConditionType::Blackboard;
                            Trans.Condition.Operator = EAIConditionOperator::Equal;
                            Trans.Condition.Value = TEXT("true");
                            
                            TSharedPtr<FJsonObject> CondObj;
                            if (ConditionField.IsValid())
                            {
                                if (ConditionField->Type == EJson::String)
                                {
                                    // String shortcut: "Condition": "HasBall" → Blackboard check for HasBall == true
                                    Trans.Condition.Name = ConditionField->AsString();
                                }
                                else if (ConditionField->Type == EJson::Object)
                                {
                                    // Full object condition
                                    CondObj = ConditionField->AsObject();
                                }
                            }
                            
                            if (CondObj.IsValid())
                            {
                                ParseConditionInternal(CondObj, Trans.Condition);
                            }
                            
                            State.Transitions.Add(Trans);
                        }
                    }
                }

                OutDef.States.Add(State);
                
                // Set initial state if not set
                if (OutDef.InitialState.IsEmpty())
                {
                    OutDef.InitialState = State.Id;
                }
            }
        }
    }

    // Also handle array of states (original format)
    if (StatesArray)
    {
        for (const auto& StateVal : *StatesArray)
        {
            if (StateVal->Type == EJson::Object)
            {
                FAIState State;
                TSharedPtr<FJsonObject> StateObj = StateVal->AsObject();
                
                if (!StateObj->TryGetStringField(TEXT("id"), State.Id))
                {
                    StateObj->TryGetStringField(TEXT("Id"), State.Id);
                }

                if (State.Id.IsEmpty())
                {
                    UE_LOG(LogTemp, Warning, TEXT("AIBehaviour: State object missing 'id'"));
                }
                
                // Parse OnEnter
                const TArray<TSharedPtr<FJsonValue>>* OnEnterArray = nullptr;
                if (StateObj->TryGetArrayField(TEXT("OnEnter"), OnEnterArray) ||
                    StateObj->TryGetArrayField(TEXT("onEnter"), OnEnterArray))
                {
                    for (const auto& ActionVal : *OnEnterArray)
                    {
                        if (ActionVal->Type == EJson::Object)
                        {
                            FAIActionEntry Entry;
                            TSharedPtr<FJsonObject> ActionObj = ActionVal->AsObject();
                            
                            // Parse action name (try multiple field names)
                            if (!ActionObj->TryGetStringField(TEXT("Action"), Entry.Action))
                            {
                                if (!ActionObj->TryGetStringField(TEXT("action"), Entry.Action))
                                {
                                    ActionObj->TryGetStringField(TEXT("actionName"), Entry.Action);
                                }
                            }

                            if (Entry.Action.IsEmpty())
                            {
                                UE_LOG(LogTemp, Warning, TEXT("AIBehaviour: Action object missing 'action' name"));
                                continue;
                            }
                            
                            // Parse params using helper
                            TSharedPtr<FJsonObject> ParamsObj;
                            if (ActionObj->HasTypedField<EJson::Object>(TEXT("params")))
                                ParamsObj = ActionObj->GetObjectField(TEXT("params"));
                            else if (ActionObj->HasTypedField<EJson::Object>(TEXT("Params")))
                                ParamsObj = ActionObj->GetObjectField(TEXT("Params"));
                            else if (ActionObj->HasTypedField<EJson::String>(TEXT("paramsJson")))
                            {
                                FString ParamsJsonStr = ActionObj->GetStringField(TEXT("paramsJson"));
                                TSharedRef<TJsonReader<>> ParamsReader = TJsonReaderFactory<>::Create(ParamsJsonStr);
                                FJsonSerializer::Deserialize(ParamsReader, ParamsObj);
                            }
                            
                            if (ParamsObj.IsValid())
                            {
                                ParseActionParamsInternal(ParamsObj, Entry.Params);
                            }

                            State.OnEnter.Add(Entry);
                        }
                    }
                }

                // Parse OnTick
                const TArray<TSharedPtr<FJsonValue>>* OnTickArray = nullptr;
                if (StateObj->TryGetArrayField(TEXT("OnTick"), OnTickArray) ||
                    StateObj->TryGetArrayField(TEXT("onTick"), OnTickArray))
                {
                    for (const auto& ActionVal : *OnTickArray)
                    {
                        if (ActionVal->Type == EJson::Object)
                        {
                            FAIActionEntry Entry;
                            TSharedPtr<FJsonObject> ActionObj = ActionVal->AsObject();
                            
                            // Parse action name (try multiple field names)
                            if (!ActionObj->TryGetStringField(TEXT("Action"), Entry.Action))
                            {
                                if (!ActionObj->TryGetStringField(TEXT("action"), Entry.Action))
                                {
                                    ActionObj->TryGetStringField(TEXT("actionName"), Entry.Action);
                                }
                            }

                            if (Entry.Action.IsEmpty())
                            {
                                UE_LOG(LogTemp, Warning, TEXT("AIBehaviour: Action object missing 'action' name"));
                                continue;
                            }
                            
                            // Parse params using helper
                            TSharedPtr<FJsonObject> ParamsObj;
                            if (ActionObj->HasTypedField<EJson::Object>(TEXT("params")))
                                ParamsObj = ActionObj->GetObjectField(TEXT("params"));
                            else if (ActionObj->HasTypedField<EJson::Object>(TEXT("Params")))
                                ParamsObj = ActionObj->GetObjectField(TEXT("Params"));
                            else if (ActionObj->HasTypedField<EJson::String>(TEXT("paramsJson")))
                            {
                                FString ParamsJsonStr;
                                if (ActionObj->TryGetStringField(TEXT("paramsJson"), ParamsJsonStr))
                                {
                                    TSharedRef<TJsonReader<>> ParamsReader = TJsonReaderFactory<>::Create(ParamsJsonStr);
                                    FJsonSerializer::Deserialize(ParamsReader, ParamsObj);
                                }
                            }
                            
                            if (ParamsObj.IsValid())
                            {
                                ParseActionParamsInternal(ParamsObj, Entry.Params);
                            }

                            State.OnTick.Add(Entry);
                        }
                    }
                }

                // Parse Transitions
                const TArray<TSharedPtr<FJsonValue>>* TransitionsArray = nullptr;
                if (StateObj->TryGetArrayField(TEXT("Transitions"), TransitionsArray) ||
                    StateObj->TryGetArrayField(TEXT("transitions"), TransitionsArray))
                {
                    for (const auto& TransVal : *TransitionsArray)
                    {
                        if (TransVal->Type == EJson::Object)
                        {
                            FAITransition Trans;
                            TSharedPtr<FJsonObject> TransObj = TransVal->AsObject();
                            if (!TransObj->TryGetStringField(TEXT("Target"), Trans.To))
                            {
                                TransObj->TryGetStringField(TEXT("to"), Trans.To);
                            }
                            
                            // Parse condition - check type first to avoid errors
                            TSharedPtr<FJsonValue> ConditionField = TransObj->TryGetField(TEXT("Condition"));
                            if (!ConditionField.IsValid())
                            {
                                ConditionField = TransObj->TryGetField(TEXT("condition"));
                            }
                            
                            // Default condition (for string shortcuts)
                            Trans.Condition.Type = EAIConditionType::Blackboard;
                            Trans.Condition.Operator = EAIConditionOperator::Equal;
                            Trans.Condition.Value = TEXT("true");
                            
                            TSharedPtr<FJsonObject> CondObj;
                            if (ConditionField.IsValid())
                            {
                                if (ConditionField->Type == EJson::String)
                                {
                                    // String shortcut: "Condition": "HasBall" → Blackboard check for HasBall == true
                                    Trans.Condition.Name = ConditionField->AsString();
                                }
                                else if (ConditionField->Type == EJson::Object)
                                {
                                    // Full object condition
                                    CondObj = ConditionField->AsObject();
                                }
                            }
                            
                            if (CondObj.IsValid())
                            {
                                ParseConditionInternal(CondObj, Trans.Condition);
                            }
                            
                            State.Transitions.Add(Trans);
                        }
                    }
                }
                
                OutDef.States.Add(State);
                
                if (OutDef.InitialState.IsEmpty())
                {
                    OutDef.InitialState = State.Id;
                }
            }
        }
    }

    if (OutDef.States.Num() == 0)
    {
        OutError = TEXT("No states defined in behavior");
        UE_LOG(LogTemp, Error, TEXT("AIBehaviour: No states defined."));
        return false;
    }

    OutDef.bIsValid = true;
    return true;
}
