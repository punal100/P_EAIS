// Copyright Punal Manalan. All Rights Reserved.

#include "SEAIS_GraphEditor.h"
#include "UEAIS_GraphNode.h"
#include "EAIS_GraphSchema.h"
#include "FEAISJsonEditorParser.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Commands/GenericCommands.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"

#define LOCTEXT_NAMESPACE "SEAIS_GraphEditor"

void SEAIS_GraphEditor::Construct(const FArguments& InArgs)
{
    BindCommands();
    
    // Create the graph editor and details view
    TSharedRef<SGraphEditor> GraphEditor = CreateGraphEditor();
    TSharedRef<SWidget> Details = CreateDetailsView();
    
    ChildSlot
    [
        SNew(SSplitter)
        .Orientation(Orient_Horizontal)
        
        + SSplitter::Slot()
        .Value(0.7f)
        [
            GraphEditor
        ]
        
        + SSplitter::Slot()
        .Value(0.3f)
        [
            SNew(SBox)
            .Padding(2)
            [
                Details
            ]
        ]
    ];
    
    // Auto-load a profile if available, otherwise create demo nodes
    FString ProfilesDir = FPaths::ProjectDir() / TEXT("Plugins/P_EAIS/Editor/AI");
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *(ProfilesDir / TEXT("*.editor.json")), true, false);
    
    if (Files.Num() > 0)
    {
        FString FilePath = ProfilesDir / Files[0];
        if (LoadFromFile(FilePath))
        {
            UE_LOG(LogTemp, Display, TEXT("SEAIS_GraphEditor: Loaded %s"), *Files[0]);
        }
    }
    else
    {
        // Create demo nodes for demonstration
        CreateNewGraph(TEXT("Demo"));
        
        if (EdGraph)
        {
            // Create sample state nodes
            UEAIS_GraphNode* IdleNode = NewObject<UEAIS_GraphNode>(EdGraph);
            IdleNode->StateId = TEXT("idle");
            IdleNode->Comment = TEXT("Idle State");
            IdleNode->bIsInitialState = true;
            IdleNode->NodePosX = 100;
            IdleNode->NodePosY = 200;
            EdGraph->AddNode(IdleNode, false, false);
            IdleNode->CreateNewGuid();
            IdleNode->PostPlacedNewNode();
            IdleNode->AllocateDefaultPins();
            
            UEAIS_GraphNode* MoveNode = NewObject<UEAIS_GraphNode>(EdGraph);
            MoveNode->StateId = TEXT("move");
            MoveNode->Comment = TEXT("Move to Ball");
            MoveNode->NodePosX = 400;
            MoveNode->NodePosY = 200;
            EdGraph->AddNode(MoveNode, false, false);
            MoveNode->CreateNewGuid();
            MoveNode->PostPlacedNewNode();
            MoveNode->AllocateDefaultPins();
            
            UE_LOG(LogTemp, Display, TEXT("SEAIS_GraphEditor: Created demo graph with 2 sample nodes"));
        }
    }
}

TSharedRef<SWidget> SEAIS_GraphEditor::CreateDetailsView()
{
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    
    FDetailsViewArgs Args;
    Args.bHideSelectionTip = true;
    Args.bShowPropertyMatrixButton = false;
    Args.NotifyHook = nullptr;
    
    DetailsView = PropertyModule.CreateDetailView(Args);
    DetailsView->SetObject(nullptr);
    
    return DetailsView.ToSharedRef();
}


TSharedRef<SGraphEditor> SEAIS_GraphEditor::CreateGraphEditor()
{
    // Create the graph if it doesn't exist
    if (!EdGraph)
    {
        EdGraph = NewObject<UEdGraph>();
        EdGraph->Schema = UEAIS_GraphSchema::StaticClass();
        EdGraph->AddToRoot(); // Prevent GC
    }
    
    SGraphEditor::FGraphEditorEvents Events;
    Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &SEAIS_GraphEditor::OnNodeSelectionChanged);
    Events.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &SEAIS_GraphEditor::OnNodeDoubleClicked);
    
    return SNew(SGraphEditor)
        .AdditionalCommands(GraphEditorCommands)
        .GraphToEdit(EdGraph)
        .GraphEvents(Events)
        .IsEditable(true)
        .ShowGraphStateOverlay(false);
}

void SEAIS_GraphEditor::BindCommands()
{
    GraphEditorCommands = MakeShareable(new FUICommandList);
    
    GraphEditorCommands->MapAction(
        FGenericCommands::Get().Delete,
        FExecuteAction::CreateSP(this, &SEAIS_GraphEditor::DeleteSelectedNodes),
        FCanExecuteAction::CreateSP(this, &SEAIS_GraphEditor::CanDeleteNodes)
    );
}

bool SEAIS_GraphEditor::LoadFromFile(const FString& FilePath)
{
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
    {
        return false;
    }
    
    FAIEditorGraph EditorGraph;
    FString Error;
    
    if (!FEAISJsonEditorParser::ParseEditorJson(JsonContent, EditorGraph, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("SEAIS_GraphEditor: Failed to parse: %s"), *Error);
        return false;
    }
    
    ImportFromEditorGraph(EditorGraph);
    CurrentFilePath = FilePath;
    
    return true;
}

bool SEAIS_GraphEditor::SaveToFile(const FString& FilePath)
{
    FAIEditorGraph EditorGraph = ExportToEditorGraph();
    FString JsonContent = FEAISJsonEditorParser::SerializeEditorGraph(EditorGraph);
    
    if (!FFileHelper::SaveStringToFile(JsonContent, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("SEAIS_GraphEditor: Failed to save to %s"), *FilePath);
        return false;
    }
    
    CurrentFilePath = FilePath;
    return true;
}

bool SEAIS_GraphEditor::ExportRuntimeJson(const FString& FilePath)
{
    FAIEditorGraph EditorGraph = ExportToEditorGraph();
    
    FAIBehaviorDef RuntimeDef;
    FString Error;
    
    if (!FEAISJsonEditorParser::ConvertEditorGraphToRuntime(EditorGraph, RuntimeDef, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("SEAIS_GraphEditor: Failed to convert: %s"), *Error);
        return false;
    }
    
    FString JsonContent = FEAISJsonSerializer::SerializeRuntime(RuntimeDef);
    
    return FFileHelper::SaveStringToFile(JsonContent, *FilePath);
}


bool SEAIS_GraphEditor::ValidateGraph(TArray<FString>& OutErrors)
{
    OutErrors.Empty();
    bool bValid = true;
    
    if (!EdGraph)
    {
        OutErrors.Add(TEXT("No graph loaded"));
        return false;
    }
    
    // Find initial state
    bool bHasInitialState = false;
    TSet<FString> StateIds;
    
    for (UEdGraphNode* Node : EdGraph->Nodes)
    {
        UEAIS_GraphNode* StateNode = Cast<UEAIS_GraphNode>(Node);
        if (!StateNode) continue;
        
        if (StateIds.Contains(StateNode->StateId))
        {
            OutErrors.Add(FString::Printf(TEXT("Duplicate state ID: %s"), *StateNode->StateId));
            bValid = false;
        }
        StateIds.Add(StateNode->StateId);
        
        if (StateNode->bIsInitialState)
        {
            bHasInitialState = true;
        }
        
        // Validate individual node
        TArray<FString> NodeErrors;
        if (!StateNode->ValidateNode(NodeErrors))
        {
            OutErrors.Append(NodeErrors);
            bValid = false;
        }
    }
    
    if (!bHasInitialState)
    {
        OutErrors.Add(TEXT("No initial state defined"));
        bValid = false;
    }
    
    // Check transition targets exist
    for (UEdGraphNode* Node : EdGraph->Nodes)
    {
        UEAIS_GraphNode* StateNode = Cast<UEAIS_GraphNode>(Node);
        if (!StateNode) continue;
        
        for (const FAITransition& Trans : StateNode->Transitions)
        {
            if (!StateIds.Contains(Trans.To))
            {
                OutErrors.Add(FString::Printf(TEXT("State '%s' has transition to non-existent state '%s'"), *StateNode->StateId, *Trans.To));
                bValid = false;
            }
        }
    }
    
    return bValid;
}

void SEAIS_GraphEditor::CreateNewGraph(const FString& Name)
{
    ClearGraph();
    
    // Create default initial state
    const UEAIS_GraphSchema* Schema = UEAIS_GraphSchema::Get();
    if (Schema && EdGraph)
    {
        UEAIS_GraphNode* InitialNode = Schema->CreateStateNode(EdGraph, FVector2D(200, 200), TEXT("Idle"));
        if (InitialNode)
        {
            InitialNode->bIsInitialState = true;
        }
    }
}

void SEAIS_GraphEditor::FocusOnNode(const FString& StateId)
{
    if (!EdGraph || !GraphEditorWidget.IsValid()) return;
    
    for (UEdGraphNode* Node : EdGraph->Nodes)
    {
        UEAIS_GraphNode* StateNode = Cast<UEAIS_GraphNode>(Node);
        if (StateNode && StateNode->StateId == StateId)
        {
            GraphEditorWidget->JumpToNode(Node, false, false);
            break;
        }
    }
}

void SEAIS_GraphEditor::ClearGraph()
{
    if (EdGraph)
    {
        EdGraph->Nodes.Empty();
        EdGraph->NotifyGraphChanged();
    }
    CurrentFilePath.Empty();
}

void SEAIS_GraphEditor::OnGraphChanged(const FEdGraphEditAction& Action)
{
    // Handle graph changes (for auto-validation, etc.)
}

void SEAIS_GraphEditor::OnNodeSelectionChanged(const TSet<UObject*>& NewSelection)
{
    if (DetailsView.IsValid())
    {
        TArray<UObject*> SelectedObjects;
        for (UObject* Obj : NewSelection)
        {
            SelectedObjects.Add(Obj);
        }
        DetailsView->SetObjects(SelectedObjects);
    }
}

void SEAIS_GraphEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
    // Handle double-click (open property editor)
}

void SEAIS_GraphEditor::DeleteSelectedNodes()
{
    UE_LOG(LogTemp, Warning, TEXT("[SEAIS_GraphEditor] Delete command triggered"));
    if (!GraphEditorWidget.IsValid()) return;
    
    const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
    UE_LOG(LogTemp, Warning, TEXT("[SEAIS_GraphEditor] Selected nodes count: %d"), SelectedNodes.Num());
    
    for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
    {
        UEdGraphNode* Node = Cast<UEdGraphNode>(*It);
        if (Node && Node->CanUserDeleteNode())
        {
            Node->DestroyNode();
        }
    }
}

bool SEAIS_GraphEditor::CanDeleteNodes() const
{
    if (!GraphEditorWidget.IsValid()) return false;
    return GraphEditorWidget->GetSelectedNodes().Num() > 0;
}

FAIEditorGraph SEAIS_GraphEditor::ExportToEditorGraph() const
{
    FAIEditorGraph Result;
    
    if (!EdGraph) return Result;
    
    for (UEdGraphNode* Node : EdGraph->Nodes)
    {
        UEAIS_GraphNode* StateNode = Cast<UEAIS_GraphNode>(Node);
        if (!StateNode) continue;
        
        FAIEditorGraph::FEditorState EdState;
        EdState.Id = StateNode->StateId;
        EdState.bTerminal = StateNode->bIsTerminal;
        EdState.OnEnter = StateNode->OnEnterActions;
        EdState.OnTick = StateNode->OnTickActions;
        EdState.OnExit = StateNode->OnExitActions;
        EdState.Transitions = StateNode->Transitions;
        
        Result.States.Add(EdState);
        
        if (StateNode->bIsInitialState)
        {
            Result.InitialState = StateNode->StateId;
        }
    }
    
    return Result;
}

void SEAIS_GraphEditor::ImportFromEditorGraph(const FAIEditorGraph& InGraph)
{
    ClearGraph();
    
    if (!EdGraph) return;
    
    const UEAIS_GraphSchema* Schema = UEAIS_GraphSchema::Get();
    if (!Schema) return;
    
    // Create nodes for each state
    TMap<FString, UEAIS_GraphNode*> NodeMap;
    float YOffset = 0;
    
    for (const FAIEditorGraph::FEditorState& State : InGraph.States)
    {
        // Get position from editor metadata if available
        FVector2D Position(200, 100 + YOffset);
        YOffset += 150;
        
        UEAIS_GraphNode* Node = Schema->CreateStateNode(EdGraph, Position, State.Id);
        if (Node)
        {
            Node->bIsTerminal = State.bTerminal;
            Node->OnEnterActions = State.OnEnter;
            Node->OnTickActions = State.OnTick;
            Node->OnExitActions = State.OnExit;
            Node->Transitions = State.Transitions;
            Node->bIsInitialState = (State.Id == InGraph.InitialState);
            
            NodeMap.Add(State.Id, Node);
        }
    }
    
    // Create connections based on transitions
    for (auto& Pair : NodeMap)
    {
        UEAIS_GraphNode* SourceNode = Pair.Value;
        
        for (const FAITransition& Trans : SourceNode->Transitions)
        {
            if (UEAIS_GraphNode** TargetNodePtr = NodeMap.Find(Trans.To))
            {
                UEAIS_GraphNode* TargetNode = *TargetNodePtr;
                UEdGraphPin* OutputPin = SourceNode->GetOutputPin();
                UEdGraphPin* InputPin = TargetNode->GetInputPin();
                
                if (OutputPin && InputPin)
                {
                    OutputPin->MakeLinkTo(InputPin);
                }
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE

