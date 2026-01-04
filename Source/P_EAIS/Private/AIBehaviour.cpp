/*
 * @Author: Punal Manalan
 * @Description: Implementation of UAIBehaviour
 * @Date: 29/12/2025
 */

#include "AIBehaviour.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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
                Entry.Value.RawValue = ValueObj->GetStringField(TEXT("rawValue"));
                
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
                            
                            // Parse params
                            TSharedPtr<FJsonObject> ParamsObj;
                            if (ActionObj->HasTypedField<EJson::Object>(TEXT("params")))
                            {
                                ParamsObj = ActionObj->GetObjectField(TEXT("params"));
                            }
                            else if (ActionObj->HasTypedField<EJson::Object>(TEXT("Params")))
                            {
                                ParamsObj = ActionObj->GetObjectField(TEXT("Params"));
                            }
                            else if (ActionObj->HasTypedField<EJson::String>(TEXT("paramsJson")))
                            {
                                // Parse inner JSON string
                                FString ParamsJsonStr = ActionObj->GetStringField(TEXT("paramsJson"));
                                TSharedRef<TJsonReader<>> ParamsReader = TJsonReaderFactory<>::Create(ParamsJsonStr);
                                FJsonSerializer::Deserialize(ParamsReader, ParamsObj);
                            }
                            
                            if (ParamsObj.IsValid())
                            {
                                if (ParamsObj->HasField(TEXT("target")))
                                {
                                    Entry.Params.Target = ParamsObj->GetStringField(TEXT("target"));
                                }
                                else if (ParamsObj->HasField(TEXT("Target")))
                                {
                                    Entry.Params.Target = ParamsObj->GetStringField(TEXT("Target"));
                                }
                                if (ParamsObj->HasField(TEXT("power")))
                                {
                                    Entry.Params.Power = ParamsObj->GetNumberField(TEXT("power"));
                                }
                                else if (ParamsObj->HasField(TEXT("Power")))
                                {
                                    Entry.Params.Power = ParamsObj->GetNumberField(TEXT("Power"));
                                }
                                if (ParamsObj->HasField(TEXT("message")))
                                {
                                    Entry.Params.ExtraParams.Add(TEXT("message"), ParamsObj->GetStringField(TEXT("message")));
                                }
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
                            
                            TSharedPtr<FJsonObject> ParamsObj;
                            if (ActionObj->HasTypedField<EJson::Object>(TEXT("params")))
                            {
                                ParamsObj = ActionObj->GetObjectField(TEXT("params"));
                            }
                            else if (ActionObj->HasTypedField<EJson::Object>(TEXT("Params")))
                            {
                                ParamsObj = ActionObj->GetObjectField(TEXT("Params"));
                            }
                            else if (ActionObj->HasTypedField<EJson::String>(TEXT("paramsJson")))
                            {
                                // Parse inner JSON string
                                FString ParamsJsonStr = ActionObj->GetStringField(TEXT("paramsJson"));
                                TSharedRef<TJsonReader<>> ParamsReader = TJsonReaderFactory<>::Create(ParamsJsonStr);
                                FJsonSerializer::Deserialize(ParamsReader, ParamsObj);
                            }
                            
                            if (ParamsObj.IsValid())
                            {
                                if (ParamsObj->HasField(TEXT("target")))
                                {
                                    Entry.Params.Target = ParamsObj->GetStringField(TEXT("target"));
                                }
                                else if (ParamsObj->HasField(TEXT("Target")))
                                {
                                    Entry.Params.Target = ParamsObj->GetStringField(TEXT("Target"));
                                }
                                if (ParamsObj->HasField(TEXT("speed")))
                                {
                                    Entry.Params.Power = ParamsObj->GetNumberField(TEXT("speed"));
                                }
                                else if (ParamsObj->HasField(TEXT("Speed")))
                                {
                                    Entry.Params.Power = ParamsObj->GetNumberField(TEXT("Speed"));
                                }
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
                                FString TypeStr = CondObj->GetStringField(TEXT("type"));
                                if (TypeStr.Equals(TEXT("Event"), ESearchCase::IgnoreCase))
                                {
                                    Trans.Condition.Type = EAIConditionType::Event;
                                }
                                else if (TypeStr.Equals(TEXT("Timer"), ESearchCase::IgnoreCase))
                                {
                                    Trans.Condition.Type = EAIConditionType::Timer;
                                    Trans.Condition.Seconds = CondObj->GetNumberField(TEXT("seconds"));
                                }
                                else if (TypeStr.Equals(TEXT("Distance"), ESearchCase::IgnoreCase))
                                {
                                    Trans.Condition.Type = EAIConditionType::Distance;
                                }
                                
                                if (CondObj->HasField(TEXT("name")))
                                {
                                    Trans.Condition.Name = CondObj->GetStringField(TEXT("name"));
                                }
                                else if (CondObj->HasField(TEXT("key")))
                                {
                                    Trans.Condition.Name = CondObj->GetStringField(TEXT("key"));
                                }
                                else if (CondObj->HasField(TEXT("keyOrName")))
                                {
                                    Trans.Condition.Name = CondObj->GetStringField(TEXT("keyOrName"));
                                }
                                
                                // Parse "target" for Distance conditions
                                if (CondObj->HasField(TEXT("target")))
                                {
                                    Trans.Condition.Target = CondObj->GetStringField(TEXT("target"));
                                }
                                
                                // Parse value - support both "value" and "compareValue"
                                TSharedPtr<FJsonValue> ValueField;
                                if (CondObj->HasField(TEXT("value")))
                                {
                                    ValueField = CondObj->TryGetField(TEXT("value"));
                                }
                                else if (CondObj->HasField(TEXT("compareValue")))
                                {
                                    ValueField = CondObj->TryGetField(TEXT("compareValue"));
                                }
                                
                                if (ValueField.IsValid())
                                {
                                    if (ValueField->Type == EJson::Boolean)
                                    {
                                        Trans.Condition.Value = ValueField->AsBool() ? TEXT("true") : TEXT("false");
                                    }
                                    else if (ValueField->Type == EJson::Object)
                                    {
                                        // { "type": "...", "rawValue": "..." } format
                                        TSharedPtr<FJsonObject> ValueObj = ValueField->AsObject();
                                        Trans.Condition.Value = ValueObj->GetStringField(TEXT("rawValue"));
                                    }
                                    else if (ValueField->Type == EJson::String)
                                    {
                                        Trans.Condition.Value = ValueField->AsString();
                                    }
                                    else if (ValueField->Type == EJson::Number)
                                    {
                                        Trans.Condition.Value = FString::SanitizeFloat(ValueField->AsNumber());
                                    }
                                }
                                
                                FString OpStr = CondObj->GetStringField(TEXT("op"));
                                if (OpStr == TEXT("==")) Trans.Condition.Operator = EAIConditionOperator::Equal;
                                else if (OpStr == TEXT("!=")) Trans.Condition.Operator = EAIConditionOperator::NotEqual;
                                else if (OpStr == TEXT(">")) Trans.Condition.Operator = EAIConditionOperator::GreaterThan;
                                else if (OpStr == TEXT("<")) Trans.Condition.Operator = EAIConditionOperator::LessThan;
                                else if (OpStr == TEXT(">=")) Trans.Condition.Operator = EAIConditionOperator::GreaterOrEqual;
                                else if (OpStr == TEXT("<=")) Trans.Condition.Operator = EAIConditionOperator::LessOrEqual;
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
                            
                            // Parse params
                            TSharedPtr<FJsonObject> ParamsObj;
                            if (ActionObj->HasTypedField<EJson::Object>(TEXT("params")))
                            {
                                ParamsObj = ActionObj->GetObjectField(TEXT("params"));
                            }
                            else if (ActionObj->HasTypedField<EJson::Object>(TEXT("Params")))
                            {
                                ParamsObj = ActionObj->GetObjectField(TEXT("Params"));
                            }
                            else if (ActionObj->HasTypedField<EJson::String>(TEXT("paramsJson")))
                            {
                                // Parse inner JSON string
                                FString ParamsJsonStr = ActionObj->GetStringField(TEXT("paramsJson"));
                                TSharedRef<TJsonReader<>> ParamsReader = TJsonReaderFactory<>::Create(ParamsJsonStr);
                                FJsonSerializer::Deserialize(ParamsReader, ParamsObj);
                            }
                            
                            if (ParamsObj.IsValid())
                            {
                                if (ParamsObj->HasField(TEXT("target")))
                                {
                                    Entry.Params.Target = ParamsObj->GetStringField(TEXT("target"));
                                }
                                else if (ParamsObj->HasField(TEXT("Target")))
                                {
                                    Entry.Params.Target = ParamsObj->GetStringField(TEXT("Target"));
                                }
                                if (ParamsObj->HasField(TEXT("power")))
                                {
                                    Entry.Params.Power = ParamsObj->GetNumberField(TEXT("power"));
                                }
                                else if (ParamsObj->HasField(TEXT("Power")))
                                {
                                    Entry.Params.Power = ParamsObj->GetNumberField(TEXT("Power"));
                                }
                                if (ParamsObj->HasField(TEXT("message")))
                                {
                                    Entry.Params.ExtraParams.Add(TEXT("message"), ParamsObj->GetStringField(TEXT("message")));
                                }
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
                            
                            TSharedPtr<FJsonObject> ParamsObj;
                            if (ActionObj->HasTypedField<EJson::Object>(TEXT("params")))
                            {
                                ParamsObj = ActionObj->GetObjectField(TEXT("params"));
                            }
                            else if (ActionObj->HasTypedField<EJson::Object>(TEXT("Params")))
                            {
                                ParamsObj = ActionObj->GetObjectField(TEXT("Params"));
                            }
                            else if (ActionObj->HasTypedField<EJson::String>(TEXT("paramsJson")))
                            {
                                // Parse inner JSON string
                                FString ParamsJsonStr = ActionObj->GetStringField(TEXT("paramsJson"));
                                TSharedRef<TJsonReader<>> ParamsReader = TJsonReaderFactory<>::Create(ParamsJsonStr);
                                FJsonSerializer::Deserialize(ParamsReader, ParamsObj);
                            }
                            
                            if (ParamsObj.IsValid())
                            {
                                if (ParamsObj->HasField(TEXT("target")))
                                {
                                    Entry.Params.Target = ParamsObj->GetStringField(TEXT("target"));
                                }
                                else if (ParamsObj->HasField(TEXT("Target")))
                                {
                                    Entry.Params.Target = ParamsObj->GetStringField(TEXT("Target"));
                                }
                                if (ParamsObj->HasField(TEXT("speed")))
                                {
                                    Entry.Params.Power = ParamsObj->GetNumberField(TEXT("speed"));
                                }
                                else if (ParamsObj->HasField(TEXT("Speed")))
                                {
                                    Entry.Params.Power = ParamsObj->GetNumberField(TEXT("Speed"));
                                }
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
                                FString TypeStr = CondObj->GetStringField(TEXT("type"));
                                if (TypeStr.Equals(TEXT("Event"), ESearchCase::IgnoreCase))
                                {
                                    Trans.Condition.Type = EAIConditionType::Event;
                                }
                                else if (TypeStr.Equals(TEXT("Timer"), ESearchCase::IgnoreCase))
                                {
                                    Trans.Condition.Type = EAIConditionType::Timer;
                                    Trans.Condition.Seconds = CondObj->GetNumberField(TEXT("seconds"));
                                }
                                else if (TypeStr.Equals(TEXT("Distance"), ESearchCase::IgnoreCase))
                                {
                                    Trans.Condition.Type = EAIConditionType::Distance;
                                }
                                
                                if (CondObj->HasField(TEXT("name")))
                                {
                                    Trans.Condition.Name = CondObj->GetStringField(TEXT("name"));
                                }
                                else if (CondObj->HasField(TEXT("key")))
                                {
                                    Trans.Condition.Name = CondObj->GetStringField(TEXT("key"));
                                }
                                else if (CondObj->HasField(TEXT("keyOrName")))
                                {
                                    Trans.Condition.Name = CondObj->GetStringField(TEXT("keyOrName"));
                                }
                                
                                // Parse "target" for Distance conditions
                                if (CondObj->HasField(TEXT("target")))
                                {
                                    Trans.Condition.Target = CondObj->GetStringField(TEXT("target"));
                                }
                                
                                // Parse value - support both "value" and "compareValue"
                                TSharedPtr<FJsonValue> ValueField;
                                if (CondObj->HasField(TEXT("value")))
                                {
                                    ValueField = CondObj->TryGetField(TEXT("value"));
                                }
                                else if (CondObj->HasField(TEXT("compareValue")))
                                {
                                    ValueField = CondObj->TryGetField(TEXT("compareValue"));
                                }
                                
                                if (ValueField.IsValid())
                                {
                                    if (ValueField->Type == EJson::Boolean)
                                    {
                                        Trans.Condition.Value = ValueField->AsBool() ? TEXT("true") : TEXT("false");
                                    }
                                    else if (ValueField->Type == EJson::Object)
                                    {
                                        // { "type": "...", "rawValue": "..." } format
                                        TSharedPtr<FJsonObject> ValueObj = ValueField->AsObject();
                                        Trans.Condition.Value = ValueObj->GetStringField(TEXT("rawValue"));
                                    }
                                    else if (ValueField->Type == EJson::String)
                                    {
                                        Trans.Condition.Value = ValueField->AsString();
                                    }
                                    else if (ValueField->Type == EJson::Number)
                                    {
                                        Trans.Condition.Value = FString::SanitizeFloat(ValueField->AsNumber());
                                    }
                                }
                                
                                FString OpStr = CondObj->GetStringField(TEXT("op"));
                                if (OpStr == TEXT("==")) Trans.Condition.Operator = EAIConditionOperator::Equal;
                                else if (OpStr == TEXT("!=")) Trans.Condition.Operator = EAIConditionOperator::NotEqual;
                                else if (OpStr == TEXT(">")) Trans.Condition.Operator = EAIConditionOperator::GreaterThan;
                                else if (OpStr == TEXT("<")) Trans.Condition.Operator = EAIConditionOperator::LessThan;
                                else if (OpStr == TEXT(">=")) Trans.Condition.Operator = EAIConditionOperator::GreaterOrEqual;
                                else if (OpStr == TEXT("<=")) Trans.Condition.Operator = EAIConditionOperator::LessOrEqual;
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
        return false;
    }

    OutDef.bIsValid = true;
    return true;
}
