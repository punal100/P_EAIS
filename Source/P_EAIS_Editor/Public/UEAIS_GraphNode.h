// Copyright Punal Manalan. All Rights Reserved.
// EAIS Graph Node - Represents an AI state in the visual editor

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "EAIS_Types.h"
#include "UEAIS_GraphNode.generated.h"

class UEdGraphPin;

/**
 * Graph node representing an AI state.
 * Used by the SGraphEditor-based visual AI editor.
 */
UCLASS()
class P_EAIS_EDITOR_API UEAIS_GraphNode : public UEdGraphNode
{
    GENERATED_BODY()

public:
    UEAIS_GraphNode();

    //~ Begin UEdGraphNode Interface
    virtual void AllocateDefaultPins() override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetTooltipText() const override;
    virtual bool CanUserDeleteNode() const override { return true; }
    virtual bool CanDuplicateNode() const override { return true; }
    //~ End UEdGraphNode Interface

    /** State ID (unique identifier) */
    UPROPERTY(EditAnywhere, Category = "State")
    FString StateId;

    /** Is this a terminal state (no outgoing transitions expected) */
    UPROPERTY(EditAnywhere, Category = "State")
    bool bIsTerminal = false;

    /** Is this the initial state */
    UPROPERTY(EditAnywhere, Category = "State")
    bool bIsInitialState = false;

    /** Actions executed when entering this state */
    UPROPERTY(EditAnywhere, Category = "Actions")
    TArray<FAIActionEntry> OnEnterActions;

    /** Actions executed every tick while in this state */
    UPROPERTY(EditAnywhere, Category = "Actions")
    TArray<FAIActionEntry> OnTickActions;

    /** Actions executed when exiting this state */
    UPROPERTY(EditAnywhere, Category = "Actions")
    TArray<FAIActionEntry> OnExitActions;

    /** Outgoing transitions */
    UPROPERTY(EditAnywhere, Category = "Transitions")
    TArray<FAITransition> Transitions;

    /** Editor comment */
    UPROPERTY(EditAnywhere, Category = "Editor")
    FString Comment;

    /** Node color override */
    UPROPERTY(EditAnywhere, Category = "Editor")
    FLinearColor NodeColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);

    /** Get the input pin */
    UEdGraphPin* GetInputPin() const;

    /** Get the output pin */
    UEdGraphPin* GetOutputPin() const;

    /** Create from FAIState */
    void InitFromState(const FAIState& State);

    /** Export to FAIState */
    FAIState ExportToState() const;

    /** Validate this node */
    bool ValidateNode(TArray<FString>& OutErrors) const;
};
