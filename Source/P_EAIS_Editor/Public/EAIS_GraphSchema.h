// Copyright Punal Manalan. All Rights Reserved.
// EAIS Graph Schema - Defines graph rules and connection logic

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "EAIS_GraphSchema.generated.h"

class UEAIS_GraphNode;
class UEdGraph;

/** Action to create a new state node */
USTRUCT()
struct FEAIS_GraphSchemaAction_NewState : public FEdGraphSchemaAction
{
    GENERATED_BODY()

    FEAIS_GraphSchemaAction_NewState()
        : FEdGraphSchemaAction()
    {}

    FEAIS_GraphSchemaAction_NewState(FText InNodeCategory, FText InMenuDesc, FText InToolTip, int32 InGrouping)
        : FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
    {}

    virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

/**
 * Schema for the EAIS AI Graph.
 * Defines what nodes can be created and how they connect.
 */
UCLASS()
class P_EAIS_EDITOR_API UEAIS_GraphSchema : public UEdGraphSchema
{
    GENERATED_BODY()

public:
    //~ Begin UEdGraphSchema Interface
    virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    virtual void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
    virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
    virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
    virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
    virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
    virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
    virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override { return true; }
    //~ End UEdGraphSchema Interface

    /** Create a new state node at the given location */
    UEAIS_GraphNode* CreateStateNode(UEdGraph* Graph, const FVector2D& Location, const FString& StateId = TEXT("NewState")) const;

    /** Get the schema instance (mutable) */
    static UEAIS_GraphSchema* GetMutable();
    
    /** Get the schema instance */
    static const UEAIS_GraphSchema* Get();
};
