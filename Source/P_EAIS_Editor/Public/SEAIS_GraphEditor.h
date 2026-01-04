// Copyright Punal Manalan. All Rights Reserved.
// EAIS Graph Editor - SGraphEditor-based AI behavior editor

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "GraphEditor.h"

class UEdGraph;
class UEAIS_GraphNode;
struct FAIEditorGraph;

/**
 * SGraphEditor-based visual AI editor.
 * This is the main graph editing widget, hosted in an editor tab.
 */
class P_EAIS_EDITOR_API SEAIS_GraphEditor : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SEAIS_GraphEditor) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    
    /** Load editor graph from JSON file */
    bool LoadFromFile(const FString& FilePath);
    
    /** Save editor graph to JSON file */
    bool SaveToFile(const FString& FilePath);
    
    /** Export runtime JSON (strips editor metadata) */
    bool ExportRuntimeJson(const FString& FilePath);
    
    /** Validate the current graph */
    bool ValidateGraph(TArray<FString>& OutErrors);
    
    /** Create a new empty graph */
    void CreateNewGraph(const FString& Name);
    
    /** Get the current graph */
    UEdGraph* GetGraph() const { return EdGraph; }
    
    /** Focus on a specific node */
    void FocusOnNode(const FString& StateId);
    
    /** Clear the current graph */
    void ClearGraph();

protected:
    /** Graph widget */
    TSharedPtr<SGraphEditor> GraphEditorWidget;
    
    /** The graph being edited */
    UEdGraph* EdGraph = nullptr;
    
    /** Current file path */
    FString CurrentFilePath;
    
    /** Graph editor commands */
    TSharedPtr<FUICommandList> GraphEditorCommands;
    
    /** Create the graph editor widget */
    TSharedRef<SGraphEditor> CreateGraphEditor();
    
    /** Details View */
    TSharedPtr<class IDetailsView> DetailsView;
    
    /** Create the details view widget */
    TSharedRef<SWidget> CreateDetailsView();
    
    /** Graph events */
    void OnGraphChanged(const FEdGraphEditAction& Action);
    void OnNodeSelectionChanged(const TSet<UObject*>& NewSelection);
    void OnNodeDoubleClicked(UEdGraphNode* Node);
    
    /** Bind commands */
    void BindCommands();
    
    /** Command handlers */
    void DeleteSelectedNodes();
    bool CanDeleteNodes() const;
    
    /** Convert graph to FAIEditorGraph */
    FAIEditorGraph ExportToEditorGraph() const;
    
    /** Import from FAIEditorGraph */
    void ImportFromEditorGraph(const FAIEditorGraph& InGraph);
};
