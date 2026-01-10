// Copyright Punal Manalan. All Rights Reserved.

#include "UEAIS_GraphNode.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "UEAIS_GraphNode"

UEAIS_GraphNode::UEAIS_GraphNode()
{
    bCanRenameNode = true;
}

void UEAIS_GraphNode::AllocateDefaultPins()
{
    // Input pin - for incoming transitions
    CreatePin(EGPD_Input, TEXT("Transition"), TEXT("In"));
    
    // Output pin - for outgoing transitions
    CreatePin(EGPD_Output, TEXT("Transition"), TEXT("Out"));
}

FText UEAIS_GraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (StateId.IsEmpty())
    {
        return LOCTEXT("NewState", "New State");
    }
    
    FString Title = StateId;
    if (bIsInitialState)
    {
        Title = FString::Printf(TEXT("[Initial] %s"), *StateId);
    }
    if (bIsTerminal)
    {
        Title = FString::Printf(TEXT("%s [Terminal]"), *Title);
    }
    
    return FText::FromString(Title);
}

FLinearColor UEAIS_GraphNode::GetNodeTitleColor() const
{
    if (bIsInitialState)
    {
        return FLinearColor::Green;
    }
    if (bIsTerminal)
    {
        return FLinearColor::Red;
    }
    return NodeColor;
}

FText UEAIS_GraphNode::GetTooltipText() const
{
    FString Tooltip = FString::Printf(TEXT("State: %s\n"), *StateId);
    
    if (!Comment.IsEmpty())
    {
        Tooltip += FString::Printf(TEXT("Comment: %s\n"), *Comment);
    }
    
    Tooltip += FString::Printf(TEXT("OnEnter: %d actions\n"), OnEnterActions.Num());
    Tooltip += FString::Printf(TEXT("OnTick: %d actions\n"), OnTickActions.Num());
    Tooltip += FString::Printf(TEXT("OnExit: %d actions\n"), OnExitActions.Num());
    Tooltip += FString::Printf(TEXT("Transitions: %d\n"), Transitions.Num());
    
    return FText::FromString(Tooltip);
}

UEdGraphPin* UEAIS_GraphNode::GetInputPin() const
{
    for (UEdGraphPin* Pin : Pins)
    {
        if (Pin && Pin->Direction == EGPD_Input)
        {
            return Pin;
        }
    }
    return nullptr;
}

UEdGraphPin* UEAIS_GraphNode::GetOutputPin() const
{
    for (UEdGraphPin* Pin : Pins)
    {
        if (Pin && Pin->Direction == EGPD_Output)
        {
            return Pin;
        }
    }
    return nullptr;
}

void UEAIS_GraphNode::InitFromState(const FAIState& State)
{
    StateId = State.Id;
    bIsTerminal = State.bTerminal;
    OnEnterActions = State.OnEnter;
    OnTickActions = State.OnTick;
    OnExitActions = State.OnExit;
    Transitions = State.Transitions;

    // Populate VisualTransitions from Runtime Transitions
    VisualTransitions.Empty();
    for (const FAITransition& Trans : Transitions)
    {
        UEAIS_EditorTransition* EditorTrans = NewObject<UEAIS_EditorTransition>(this);
        EditorTrans->To = Trans.To;
        EditorTrans->Priority = Trans.Priority;
        EditorTrans->Condition = UEAIS_EditorCondition::FromRuntimeCondition(EditorTrans, Trans.Condition);
        VisualTransitions.Add(EditorTrans);
    }
}

FAIState UEAIS_GraphNode::ExportToState() const
{
    FAIState State;
    State.Id = StateId;
    State.bTerminal = bIsTerminal;
    State.OnEnter = OnEnterActions;
    State.OnTick = OnTickActions;
    State.OnExit = OnExitActions;
    
    // Export VisualTransitions to Runtime Transitions
    State.Transitions.Empty();
    for (UEAIS_EditorTransition* Visual : VisualTransitions)
    {
        if (Visual)
        {
            State.Transitions.Add(Visual->ToRuntimeTransition());
        }
    }

    return State;
}

bool UEAIS_GraphNode::ValidateNode(TArray<FString>& OutErrors) const
{
    bool bValid = true;
    
    if (StateId.IsEmpty())
    {
        OutErrors.Add(TEXT("State ID is empty"));
        bValid = false;
    }
    
    if (!bIsTerminal && Transitions.Num() == 0 && OnTickActions.Num() == 0)
    {
        OutErrors.Add(FString::Printf(TEXT("State '%s' has no transitions and no OnTick actions (use terminal=true if intentional)"), *StateId));
        bValid = false;
    }
    
    for (const FAITransition& Trans : Transitions)
    {
        if (Trans.To.IsEmpty())
        {
            OutErrors.Add(FString::Printf(TEXT("State '%s' has transition with empty target"), *StateId));
            bValid = false;
        }
    }
    
    return bValid;
}

#undef LOCTEXT_NAMESPACE
