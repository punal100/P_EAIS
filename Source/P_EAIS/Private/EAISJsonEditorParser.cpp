// Copyright Punal Manalan. All Rights Reserved.

#include "EAISJsonEditorParser.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FEAISJsonEditorParser::LoadFromJson(const FString& JsonString, FAIBehaviorDef& OutDef, TMap<FString, FEAIS_EditorNodeData>& OutEditorNodes, FEAIS_EditorViewportData& OutViewport)
{
    TSharedPtr<FJsonObject> RootObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
    {
        return false;
    }

    // Basic info
    OutDef.Name = RootObj->GetStringField(TEXT("name"));
    OutDef.InitialState = RootObj->GetStringField(TEXT("initialState"));

    // States
    const TArray<TSharedPtr<FJsonValue>>* StatesArray;
    if (RootObj->TryGetArrayField(TEXT("states"), StatesArray))
    {
        for (const auto& StateVal : *StatesArray)
        {
            TSharedPtr<FJsonObject> StateObj = StateVal->AsObject();
            if (StateObj.IsValid())
            {
                FAIState NewState;
                ParseState(StateObj, NewState);
                OutDef.States.Add(NewState);
            }
        }
    }

    // Editor data
    const TSharedPtr<FJsonObject>* EditorObj;
    if (RootObj->TryGetObjectField(TEXT("editor"), EditorObj))
    {
        // Viewport
        const TSharedPtr<FJsonObject>* ViewportObj;
        if ((*EditorObj)->TryGetObjectField(TEXT("viewport"), ViewportObj))
        {
            OutViewport.ZoomAmount = (*ViewportObj)->GetNumberField(TEXT("zoom"));
            OutViewport.ViewOffset.X = (*ViewportObj)->GetNumberField(TEXT("x"));
            OutViewport.ViewOffset.Y = (*ViewportObj)->GetNumberField(TEXT("y"));
        }

        // Nodes
        const TSharedPtr<FJsonObject>* NodesObj;
        if ((*EditorObj)->TryGetObjectField(TEXT("nodes"), NodesObj))
        {
            for (auto& Pair : (*NodesObj)->Values)
            {
                TSharedPtr<FJsonObject> NodeDataObj = Pair.Value->AsObject();
                if (NodeDataObj.IsValid())
                {
                    FEAIS_EditorNodeData NodeData;
                    const TSharedPtr<FJsonObject>* PosObj;
                    if (NodeDataObj->TryGetObjectField(TEXT("pos"), PosObj))
                    {
                        NodeData.Position.X = (*PosObj)->GetNumberField(TEXT("x"));
                        NodeData.Position.Y = (*PosObj)->GetNumberField(TEXT("y"));
                    }
                    OutEditorNodes.Add(Pair.Key, NodeData);
                }
            }
        }
    }

    OutDef.bIsValid = true;
    return true;
}

void FEAISJsonEditorParser::ParseState(const TSharedPtr<FJsonObject>& StateObj, FAIState& OutState)
{
    if (!StateObj->TryGetStringField(TEXT("id"), OutState.Id))
    {
        UE_LOG(LogTemp, Warning, TEXT("FEAISJsonEditorParser: State has no 'id'"));
    }
    
    OutState.bTerminal = StateObj->GetBoolField(TEXT("terminal"));

    auto ParseActions = [&](const FString& FieldName, TArray<FAIActionEntry>& OutActions)
    {
        const TArray<TSharedPtr<FJsonValue>>* ActionsArray;
        if (StateObj->TryGetArrayField(FieldName, ActionsArray))
        {
            for (const auto& ActVal : *ActionsArray)
            {
                TSharedPtr<FJsonObject> ActObj = ActVal->AsObject();
                if (ActObj.IsValid())
                {
                    FAIActionEntry Entry;
                    if (!ActObj->TryGetStringField(TEXT("actionName"), Entry.Action))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("FEAISJsonEditorParser: Action in state '%s' missing 'actionName'"), *OutState.Id);
                    }
                    
                    const TSharedPtr<FJsonObject>* ParamsObj;
                    if (ActObj->TryGetObjectField(TEXT("params"), ParamsObj))
                    {
                        Entry.Params.Target = (*ParamsObj)->GetStringField(TEXT("target"));
                        Entry.Params.Power = (*ParamsObj)->GetNumberField(TEXT("power"));
                        // Flatten extra params
                        for (auto& P : (*ParamsObj)->Values)
                        {
                            if (P.Key != "target" && P.Key != "power")
                            {
                                Entry.Params.ExtraParams.Add(P.Key, P.Value->AsString());
                            }
                        }
                    }
                    OutActions.Add(Entry);
                }
            }
        }
    };

    ParseActions(TEXT("onEnter"), OutState.OnEnter);
    ParseActions(TEXT("onTick"), OutState.OnTick);
    ParseActions(TEXT("onExit"), OutState.OnExit);

    // Transitions
    const TArray<TSharedPtr<FJsonValue>>* TransitionsArray;
    if (StateObj->TryGetArrayField(TEXT("transitions"), TransitionsArray))
    {
        for (const auto& TransVal : *TransitionsArray)
        {
            TSharedPtr<FJsonObject> TransObj = TransVal->AsObject();
            if (TransObj.IsValid())
            {
                FAITransition NewTrans;
                if (!TransObj->TryGetStringField(TEXT("to"), NewTrans.To))
                {
                    UE_LOG(LogTemp, Warning, TEXT("FEAISJsonEditorParser: Transition in state '%s' missing 'to' target"), *OutState.Id);
                }
                NewTrans.Priority = TransObj->GetIntegerField(TEXT("priority"));
                
                const TSharedPtr<FJsonObject>* CondObj;
                if (TransObj->TryGetObjectField(TEXT("condition"), CondObj))
                {
                    NewTrans.Condition.Name = (*CondObj)->GetStringField(TEXT("keyOrName"));
                    FString OpStr = (*CondObj)->GetStringField(TEXT("op"));
                    // Mapping OpStr to EAIConditionOperator
                    if (OpStr == "Equal" || OpStr == "==") NewTrans.Condition.Operator = EAIConditionOperator::Equal;
                    else if (OpStr == "NotEqual" || OpStr == "!=") NewTrans.Condition.Operator = EAIConditionOperator::NotEqual;
                    // ... etc
                    NewTrans.Condition.Value = (*CondObj)->GetStringField(TEXT("compareValue"));
                }
                OutState.Transitions.Add(NewTrans);
            }
        }
    }
}

bool FEAISJsonEditorParser::SaveToJson(const FAIBehaviorDef& Def, const TMap<FString, FEAIS_EditorNodeData>& EditorNodes, const FEAIS_EditorViewportData& Viewport, FString& OutJsonString)
{
    TSharedRef<FJsonObject> RootObj = MakeShared<FJsonObject>();
    RootObj->SetStringField(TEXT("name"), Def.Name);
    RootObj->SetStringField(TEXT("initialState"), Def.InitialState);

    // States
    TArray<TSharedPtr<FJsonValue>> StatesArray;
    for (const auto& State : Def.States)
    {
        TSharedRef<FJsonObject> StateObj = MakeShared<FJsonObject>();
        SerializeState(State, StateObj);
        StatesArray.Add(MakeShared<FJsonValueObject>(StateObj));
    }
    RootObj->SetArrayField(TEXT("states"), StatesArray);

    // Editor
    TSharedRef<FJsonObject> EditorObj = MakeShared<FJsonObject>();
    
    TSharedRef<FJsonObject> ViewportObj = MakeShared<FJsonObject>();
    ViewportObj->SetNumberField(TEXT("zoom"), Viewport.ZoomAmount);
    ViewportObj->SetNumberField(TEXT("x"), Viewport.ViewOffset.X);
    ViewportObj->SetNumberField(TEXT("y"), Viewport.ViewOffset.Y);
    EditorObj->SetObjectField(TEXT("viewport"), ViewportObj);

    TSharedRef<FJsonObject> NodesObj = MakeShared<FJsonObject>();
    for (auto& Pair : EditorNodes)
    {
        TSharedRef<FJsonObject> NodeDataObj = MakeShared<FJsonObject>();
        TSharedRef<FJsonObject> PosObj = MakeShared<FJsonObject>();
        PosObj->SetNumberField(TEXT("x"), Pair.Value.Position.X);
        PosObj->SetNumberField(TEXT("y"), Pair.Value.Position.Y);
        NodeDataObj->SetObjectField(TEXT("pos"), PosObj);
        NodesObj->SetObjectField(Pair.Key, NodeDataObj);
    }
    EditorObj->SetObjectField(TEXT("nodes"), NodesObj);

    RootObj->SetObjectField(TEXT("editor"), EditorObj);

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
    return FJsonSerializer::Serialize(RootObj, Writer);
}

void FEAISJsonEditorParser::SerializeState(const FAIState& State, TSharedRef<FJsonObject> StateObj)
{
    StateObj->SetStringField(TEXT("id"), State.Id);
    StateObj->SetBoolField(TEXT("terminal"), State.bTerminal);

    auto SerializeActions = [&](const TArray<FAIActionEntry>& Actions)
    {
        TArray<TSharedPtr<FJsonValue>> Array;
        for (const auto& Entry : Actions)
        {
            TSharedRef<FJsonObject> ActObj = MakeShared<FJsonObject>();
            ActObj->SetStringField(TEXT("actionName"), Entry.Action);
            TSharedRef<FJsonObject> ParamsObj = MakeShared<FJsonObject>();
            ParamsObj->SetStringField(TEXT("target"), Entry.Params.Target);
            ParamsObj->SetNumberField(TEXT("power"), Entry.Params.Power);
            for (auto& P : Entry.Params.ExtraParams)
            {
                ParamsObj->SetStringField(P.Key, P.Value);
            }
            ActObj->SetObjectField(TEXT("params"), ParamsObj);
            Array.Add(MakeShared<FJsonValueObject>(ActObj));
        }
        return Array;
    };

    StateObj->SetArrayField(TEXT("onEnter"), SerializeActions(State.OnEnter));
    StateObj->SetArrayField(TEXT("onTick"), SerializeActions(State.OnTick));
    StateObj->SetArrayField(TEXT("onExit"), SerializeActions(State.OnExit));

    TArray<TSharedPtr<FJsonValue>> TransArray;
    for (const auto& Trans : State.Transitions)
    {
        TSharedRef<FJsonObject> TransObj = MakeShared<FJsonObject>();
        TransObj->SetStringField(TEXT("to"), Trans.To);
        TransObj->SetNumberField(TEXT("priority"), Trans.Priority);
        
        TSharedRef<FJsonObject> CondObj = MakeShared<FJsonObject>();
        CondObj->SetStringField(TEXT("keyOrName"), Trans.Condition.Name);
        // Map operator back to string
        FString OpStr = "Equal";
        switch(Trans.Condition.Operator) {
            case EAIConditionOperator::Equal: OpStr = "Equal"; break;
            case EAIConditionOperator::NotEqual: OpStr = "NotEqual"; break;
            // ...
        }
        CondObj->SetStringField(TEXT("op"), OpStr);
        CondObj->SetStringField(TEXT("compareValue"), Trans.Condition.Value);

        TransObj->SetObjectField(TEXT("condition"), CondObj);
        TransArray.Add(MakeShared<FJsonValueObject>(TransObj));
    }
    StateObj->SetArrayField(TEXT("transitions"), TransArray);
}
