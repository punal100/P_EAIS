// Copyright Punal Manalan. All Rights Reserved.

#include "EAIS_GraphSchema.h"
#include "UEAIS_GraphNode.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Framework/Commands/GenericCommands.h"
#include "ToolMenu.h"
#include "GraphEditorActions.h"

#define LOCTEXT_NAMESPACE "EAIS_GraphSchema"

//////////////////////////////////////////////////////////////////////////
// FEAIS_GraphSchemaAction_NewState

UEdGraphNode* FEAIS_GraphSchemaAction_NewState::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
    const UEAIS_GraphSchema* Schema = Cast<UEAIS_GraphSchema>(ParentGraph->GetSchema());
    if (Schema)
    {
        UEAIS_GraphNode* NewNode = Schema->CreateStateNode(ParentGraph, Location);
        
        // Connect from source pin if provided
        if (FromPin && NewNode)
        {
            Schema->TryCreateConnection(FromPin, NewNode->GetInputPin());
        }
        
        return NewNode;
    }
    
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
// UEAIS_GraphSchema

void UEAIS_GraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    // Add action to create new state
    TSharedPtr<FEAIS_GraphSchemaAction_NewState> NewStateAction = MakeShareable(
        new FEAIS_GraphSchemaAction_NewState(
            LOCTEXT("StateCategory", "State"),
            LOCTEXT("NewState", "Add State"),
            LOCTEXT("NewStateTooltip", "Create a new AI state node"),
            0
        )
    );
    
    ContextMenuBuilder.AddAction(NewStateAction);
}

void UEAIS_GraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
    if (Context && Context->Node)
    {
        // Add context menu actions for nodes
        FToolMenuSection& Section = Menu->AddSection("EAIS_NodeActions", LOCTEXT("NodeActionsMenu", "Node Actions"));
        
        Section.AddMenuEntry(FGenericCommands::Get().Delete);
        Section.AddMenuEntry(FGenericCommands::Get().Cut);
        Section.AddMenuEntry(FGenericCommands::Get().Copy);
        Section.AddMenuEntry(FGenericCommands::Get().Duplicate);
    }
}

const FPinConnectionResponse UEAIS_GraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
    // Check for null
    if (!A || !B)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("InvalidPin", "Invalid pin"));
    }
    
    // Don't allow self-connection
    if (A->GetOwningNode() == B->GetOwningNode())
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SelfConnection", "Cannot connect a node to itself"));
    }
    
    // Must be different directions
    if (A->Direction == B->Direction)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameDirection", "Must connect output to input"));
    }
    
    // Allow the connection
    return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("ConnectNodes", "Connect states"));
}

bool UEAIS_GraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
    // Validate connection first
    FPinConnectionResponse Response = CanCreateConnection(A, B);
    if (Response.Response == CONNECT_RESPONSE_DISALLOW)
    {
        return false;
    }
    
    // Ensure A is output and B is input
    UEdGraphPin* OutputPin = (A->Direction == EGPD_Output) ? A : B;
    UEdGraphPin* InputPin = (A->Direction == EGPD_Input) ? A : B;
    
    // Create the connection
    OutputPin->MakeLinkTo(InputPin);
    
    // Add transition to the source node
    UEAIS_GraphNode* SourceNode = Cast<UEAIS_GraphNode>(OutputPin->GetOwningNode());
    UEAIS_GraphNode* TargetNode = Cast<UEAIS_GraphNode>(InputPin->GetOwningNode());
    
    if (SourceNode && TargetNode)
    {
        // Check if transition already exists
        bool bExists = false;
        for (const FAITransition& Trans : SourceNode->Transitions)
        {
            if (Trans.To == TargetNode->StateId)
            {
                bExists = true;
                break;
            }
        }
        
        if (!bExists)
        {
            FAITransition NewTransition;
            NewTransition.To = TargetNode->StateId;
            NewTransition.Priority = 100;
            NewTransition.Condition.Type = EAIConditionType::Blackboard;
            NewTransition.Condition.Operator = EAIConditionOperator::Equal;
            SourceNode->Transitions.Add(NewTransition);
        }
    }
    
    return true;
}

void UEAIS_GraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
    // Remove transitions when breaking links
    UEdGraphNode* OwnerNode = TargetPin.GetOwningNode();
    UEAIS_GraphNode* StateNode = Cast<UEAIS_GraphNode>(OwnerNode);
    
    if (StateNode && TargetPin.Direction == EGPD_Output)
    {
        // Get linked nodes before breaking
        TArray<UEdGraphPin*> LinkedPins = TargetPin.LinkedTo;
        
        for (UEdGraphPin* LinkedPin : LinkedPins)
        {
            if (UEAIS_GraphNode* TargetState = Cast<UEAIS_GraphNode>(LinkedPin->GetOwningNode()))
            {
                // Remove matching transition
                StateNode->Transitions.RemoveAll([&](const FAITransition& Trans)
                {
                    return Trans.To == TargetState->StateId;
                });
            }
        }
    }
    
    Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
}

void UEAIS_GraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
    for (UEdGraphPin* Pin : TargetNode.Pins)
    {
        if (Pin)
        {
            BreakPinLinks(*Pin, true);
        }
    }
    
    Super::BreakNodeLinks(TargetNode);
}

FLinearColor UEAIS_GraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
    return FLinearColor::White;
}

UEAIS_GraphNode* UEAIS_GraphSchema::CreateStateNode(UEdGraph* Graph, const FVector2D& Location, const FString& StateId) const
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UEAIS_GraphNode* NewNode = NewObject<UEAIS_GraphNode>(Graph);
    NewNode->StateId = StateId;
    
    Graph->AddNode(NewNode, true, false);
    
    NewNode->CreateNewGuid();
    NewNode->PostPlacedNewNode();
    NewNode->AllocateDefaultPins();
    
    NewNode->NodePosX = Location.X;
    NewNode->NodePosY = Location.Y;
    
    Graph->NotifyGraphChanged();
    
    return NewNode;
}

UEAIS_GraphSchema* UEAIS_GraphSchema::GetMutable()
{
    return const_cast<UEAIS_GraphSchema*>(GetDefault<UEAIS_GraphSchema>());
}

const UEAIS_GraphSchema* UEAIS_GraphSchema::Get()
{
    return GetDefault<UEAIS_GraphSchema>();
}

#undef LOCTEXT_NAMESPACE

