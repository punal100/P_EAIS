// Copyright Punal Manalan. All Rights Reserved.

#include "SEAIS_GraphEditor.h"
#include "UEAIS_GraphNode.h"
#include "EAIS_GraphSchema.h"
#include "FEAISJsonEditorParser.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Commands/GenericCommands.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"

#include "EAIS_ProfileUtils.h"

#define LOCTEXT_NAMESPACE "SEAIS_GraphEditor"

void SEAIS_GraphEditor::Construct(const FArguments &InArgs)
{
    BindCommands();

    // Create the graph editor, details view, and toolbar
    TSharedRef<SGraphEditor> GraphEditor = CreateGraphEditor();
    GraphEditorWidget = GraphEditor; // Store reference for delete/selection operations
    TSharedRef<SWidget> Details = CreateDetailsView();
    TSharedRef<SWidget> Toolbar = CreateToolbar();

    ChildSlot
        [SNew(SVerticalBox)

         // Toolbar at top
         + SVerticalBox::Slot()
               .AutoHeight()
               .Padding(2)
                   [Toolbar]

         // Main content (Graph + Details)
         + SVerticalBox::Slot()
               .FillHeight(1.0f)
                   [SNew(SSplitter)
                        .Orientation(Orient_Horizontal)

                    + SSplitter::Slot()
                          .Value(0.7f)
                              [GraphEditor]

                    + SSplitter::Slot()
                          .Value(0.3f)
                              [SNew(SBox)
                                   .Padding(2)
                                       [Details]]]];

    // Populate profile dropdown and auto-load first profile if available
    RefreshProfileList();

    if (ProfileOptions.Num() > 0)
    {
        OnLoadProfileClicked();
    }
    else
    {
        // Create demo nodes for demonstration
        CreateNewGraph(TEXT("Demo"));

        if (EdGraph)
        {
            // Create sample state nodes
            UEAIS_GraphNode *IdleNode = NewObject<UEAIS_GraphNode>(EdGraph);
            IdleNode->StateId = TEXT("idle");
            IdleNode->Comment = TEXT("Idle State");
            IdleNode->bIsInitialState = true;
            IdleNode->NodePosX = 100;
            IdleNode->NodePosY = 200;
            EdGraph->AddNode(IdleNode, false, false);
            IdleNode->CreateNewGuid();
            IdleNode->PostPlacedNewNode();
            IdleNode->AllocateDefaultPins();

            UEAIS_GraphNode *MoveNode = NewObject<UEAIS_GraphNode>(EdGraph);
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
    FPropertyEditorModule &PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

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
        FCanExecuteAction::CreateSP(this, &SEAIS_GraphEditor::CanDeleteNodes));
}

bool SEAIS_GraphEditor::LoadFromFile(const FString &FilePath)
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

bool SEAIS_GraphEditor::SaveToFile(const FString &FilePath)
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

bool SEAIS_GraphEditor::ExportRuntimeJson(const FString &FilePath)
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

bool SEAIS_GraphEditor::ValidateGraph(TArray<FString> &OutErrors)
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

    for (UEdGraphNode *Node : EdGraph->Nodes)
    {
        UEAIS_GraphNode *StateNode = Cast<UEAIS_GraphNode>(Node);
        if (!StateNode)
            continue;

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
    for (UEdGraphNode *Node : EdGraph->Nodes)
    {
        UEAIS_GraphNode *StateNode = Cast<UEAIS_GraphNode>(Node);
        if (!StateNode)
            continue;

        for (const FAITransition &Trans : StateNode->Transitions)
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

void SEAIS_GraphEditor::CreateNewGraph(const FString &Name)
{
    ClearGraph();

    // Create default initial state
    const UEAIS_GraphSchema *Schema = UEAIS_GraphSchema::Get();
    if (Schema && EdGraph)
    {
        UEAIS_GraphNode *InitialNode = Schema->CreateStateNode(EdGraph, FVector2D(200, 200), TEXT("Idle"));
        if (InitialNode)
        {
            InitialNode->bIsInitialState = true;
        }
    }
}

void SEAIS_GraphEditor::FocusOnNode(const FString &StateId)
{
    if (!EdGraph || !GraphEditorWidget.IsValid())
        return;

    for (UEdGraphNode *Node : EdGraph->Nodes)
    {
        UEAIS_GraphNode *StateNode = Cast<UEAIS_GraphNode>(Node);
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

void SEAIS_GraphEditor::OnGraphChanged(const FEdGraphEditAction &Action)
{
    // Handle graph changes (for auto-validation, etc.)
}

void SEAIS_GraphEditor::OnNodeSelectionChanged(const TSet<UObject *> &NewSelection)
{
    if (DetailsView.IsValid())
    {
        TArray<UObject *> SelectedObjects;
        for (UObject *Obj : NewSelection)
        {
            SelectedObjects.Add(Obj);
        }
        DetailsView->SetObjects(SelectedObjects);
    }
}

void SEAIS_GraphEditor::OnNodeDoubleClicked(UEdGraphNode *Node)
{
    // Handle double-click (open property editor)
}

void SEAIS_GraphEditor::DeleteSelectedNodes()
{
    UE_LOG(LogTemp, Warning, TEXT("[SEAIS_GraphEditor] Delete command triggered"));
    if (!GraphEditorWidget.IsValid())
        return;

    const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
    UE_LOG(LogTemp, Warning, TEXT("[SEAIS_GraphEditor] Selected nodes count: %d"), SelectedNodes.Num());

    for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
    {
        UEdGraphNode *Node = Cast<UEdGraphNode>(*It);
        if (Node && Node->CanUserDeleteNode())
        {
            Node->DestroyNode();
        }
    }
}

bool SEAIS_GraphEditor::CanDeleteNodes() const
{
    if (!GraphEditorWidget.IsValid())
        return false;
    return GraphEditorWidget->GetSelectedNodes().Num() > 0;
}

FAIEditorGraph SEAIS_GraphEditor::ExportToEditorGraph() const
{
    FAIEditorGraph Result;

    if (!EdGraph)
        return Result;

    for (UEdGraphNode *Node : EdGraph->Nodes)
    {
        UEAIS_GraphNode *StateNode = Cast<UEAIS_GraphNode>(Node);
        if (!StateNode)
            continue;

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

void SEAIS_GraphEditor::ImportFromEditorGraph(const FAIEditorGraph &InGraph)
{
    ClearGraph();

    if (!EdGraph)
        return;

    const UEAIS_GraphSchema *Schema = UEAIS_GraphSchema::Get();
    if (!Schema)
        return;

    // Create nodes for each state
    TMap<FString, UEAIS_GraphNode *> NodeMap;
    float YOffset = 0;

    for (const FAIEditorGraph::FEditorState &State : InGraph.States)
    {
        // Get position from editor metadata if available
        FVector2D Position(200, 100 + YOffset);
        YOffset += 150;

        UEAIS_GraphNode *Node = Schema->CreateStateNode(EdGraph, Position, State.Id);
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
    for (auto &Pair : NodeMap)
    {
        UEAIS_GraphNode *SourceNode = Pair.Value;

        for (const FAITransition &Trans : SourceNode->Transitions)
        {
            if (UEAIS_GraphNode **TargetNodePtr = NodeMap.Find(Trans.To))
            {
                UEAIS_GraphNode *TargetNode = *TargetNodePtr;
                UEdGraphPin *OutputPin = SourceNode->GetOutputPin();
                UEdGraphPin *InputPin = TargetNode->GetInputPin();

                if (OutputPin && InputPin)
                {
                    OutputPin->MakeLinkTo(InputPin);
                }
            }
        }
    }
}

// ============================================================================
// PROFILE DROPDOWN IMPLEMENTATION
// ============================================================================

TSharedRef<SWidget> SEAIS_GraphEditor::CreateToolbar()
{
    return SNew(SVerticalBox)
        // Path info row - shows users where profiles are located
        + SVerticalBox::Slot()
              .AutoHeight()
              .Padding(4, 2, 4, 0)
              [
                  SNew(STextBlock)
                      .Text_Lambda([this]()
                      {
                          return FText::FromString(FString::Printf(
                              TEXT("Profiles: %s"), *GetProfilesDirectory()));
                      })
                      .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                      .ColorAndOpacity(FSlateColor(FLinearColor::Gray))
              ]
        // Main toolbar row
        + SVerticalBox::Slot()
              .AutoHeight()
              [
                  SNew(SBorder)
                      .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                      .Padding(FMargin(4.0f))
                      [
                          SNew(SHorizontalBox)

                          // Profile Label
                          + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0, 0, 8, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(LOCTEXT("ProfileLabel", "Profile:"))
                                ]

                          // Profile Dropdown
                          + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0, 0, 8, 0)
                                [
                                    SAssignNew(ProfileDropdown, STextComboBox)
                                        .OptionsSource(&ProfileOptions)
                                        .OnSelectionChanged(this, &SEAIS_GraphEditor::OnProfileSelected)
                                        .ToolTipText(LOCTEXT("ProfileDropdownTip", "Select an AI behavior profile"))
                                ]

                          // Load Button
                          + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0, 0, 4, 0)
                                [
                                    SNew(SButton)
                                        .Text(LOCTEXT("LoadButton", "Load"))
                                        .OnClicked_Lambda([this]()
                                        {
                                            OnLoadProfileClicked();
                                            return FReply::Handled();
                                        })
                                        .ToolTipText(LOCTEXT("LoadButtonTip", "Load the selected profile"))
                                ]

                          // Refresh Button
                          + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                [
                                    SNew(SButton)
                                        .Text(LOCTEXT("RefreshButton", "Refresh"))
                                        .OnClicked_Lambda([this]()
                                        {
                                            OnRefreshClicked();
                                            return FReply::Handled();
                                        })
                                        .ToolTipText(LOCTEXT("RefreshButtonTip", "Refresh the profile list"))
                                ]
                      ]
              ];
}

FString SEAIS_GraphEditor::GetEditorProfilesDirectory() const
{
    // Try plugin directory first (standard install)
    // NOTE: Must use ConvertRelativePathToFull() as FPaths functions may return relative paths
    FString PluginEditorDir = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("P_EAIS/Editor/AI")));
    if (FPaths::DirectoryExists(PluginEditorDir))
    {
        return PluginEditorDir;
    }

    // Try alternative plugin location (git submodule / explicit Plugins folder)
    FString AltPluginDir = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/P_EAIS/Editor/AI")));
    if (FPaths::DirectoryExists(AltPluginDir))
    {
        return AltPluginDir;
    }

    // Return primary location for creation purposes
    UE_LOG(LogTemp, Warning, TEXT("SEAIS_GraphEditor: Editor profiles directory not found, tried: %s, %s"),
           *PluginEditorDir, *AltPluginDir);
    return PluginEditorDir;
}

FString SEAIS_GraphEditor::GetProfilesDirectory() const
{
    // Try plugin content directory first
    // NOTE: Must use ConvertRelativePathToFull() as FPaths functions may return relative paths
    FString PluginProfilesDir = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("P_EAIS/Content/AIProfiles")));
    if (FPaths::DirectoryExists(PluginProfilesDir))
    {
        return PluginProfilesDir;
    }

    // Try alternative plugin location
    FString AltPluginDir = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/P_EAIS/Content/AIProfiles")));
    if (FPaths::DirectoryExists(AltPluginDir))
    {
        return AltPluginDir;
    }

    // Fallback to project content directory
    FString ProjectProfilesDir = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectContentDir(), TEXT("AIProfiles")));
    if (FPaths::DirectoryExists(ProjectProfilesDir))
    {
        return ProjectProfilesDir;
    }

    UE_LOG(LogTemp, Warning, TEXT("SEAIS_GraphEditor: Profiles directory not found"));
    return PluginProfilesDir;
}

void SEAIS_GraphEditor::RefreshProfileList()
{
    ProfileOptions.Empty();
    TSet<FString> UniqueProfileNames;

    // Search editor profiles (*.editor.json)
    FString EditorProfilesDir = GetEditorProfilesDirectory();
    UE_LOG(LogTemp, Log, TEXT("SEAIS_GraphEditor: Searching editor profiles in: %s"), *EditorProfilesDir);

    if (FPaths::DirectoryExists(EditorProfilesDir))
    {
        TArray<FString> EditorFiles;
        // Use 3-parameter overload: (OutFiles, Directory, Extension)
        IFileManager::Get().FindFiles(EditorFiles, *EditorProfilesDir, TEXT("*.editor.json"));

        UE_LOG(LogTemp, Log, TEXT("SEAIS_GraphEditor: Found %d editor files"), EditorFiles.Num());

        for (const FString &File : EditorFiles)
        {
            FString ProfileName = FPaths::GetBaseFilename(File);
            // Strip .editor suffix
            ProfileName = ProfileName.Replace(TEXT(".editor"), TEXT(""));
            UniqueProfileNames.Add(ProfileName);
            UE_LOG(LogTemp, Verbose, TEXT("SEAIS_GraphEditor: Editor profile: %s"), *ProfileName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SEAIS_GraphEditor: Editor profiles directory does not exist: %s"), *EditorProfilesDir);
    }

    // Search runtime profiles (*.runtime.json and *.json)
    FString RuntimeProfilesDir = GetProfilesDirectory();
    UE_LOG(LogTemp, Log, TEXT("SEAIS_GraphEditor: Searching runtime profiles in: %s"), *RuntimeProfilesDir);

    if (FPaths::DirectoryExists(RuntimeProfilesDir))
    {
        TArray<FString> RuntimeFiles;
        // Use 3-parameter overload: (OutFiles, Directory, Extension)
        IFileManager::Get().FindFiles(RuntimeFiles, *RuntimeProfilesDir, TEXT("*.json"));

        UE_LOG(LogTemp, Log, TEXT("SEAIS_GraphEditor: Found %d runtime files"), RuntimeFiles.Num());

        for (const FString &File : RuntimeFiles)
        {
            FString ProfileName = FPaths::GetBaseFilename(File);
            // Strip .runtime suffix if present
            ProfileName = ProfileName.Replace(TEXT(".runtime"), TEXT(""));
            UniqueProfileNames.Add(ProfileName);
            UE_LOG(LogTemp, Verbose, TEXT("SEAIS_GraphEditor: Runtime profile: %s"), *ProfileName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SEAIS_GraphEditor: Runtime profiles directory does not exist: %s"), *RuntimeProfilesDir);
    }

    // Add all unique profiles to options (deterministic order)
    const TArray<FString> SortedNames = EAIS_ProfileUtils::MakeSortedUnique(UniqueProfileNames);
    for (const FString &ProfileName : SortedNames)
    {
        ProfileOptions.Add(MakeShared<FString>(ProfileName));
        UE_LOG(LogTemp, Log, TEXT("SEAIS_GraphEditor: Added profile to dropdown: %s"), *ProfileName);
    }

    UE_LOG(LogTemp, Log, TEXT("SEAIS_GraphEditor: Found %d unique profiles total"), ProfileOptions.Num());

    // Update dropdown and select first item
    if (ProfileDropdown.IsValid() && ProfileOptions.Num() > 0)
    {
        ProfileDropdown->RefreshOptions();

        const FString DefaultName = EAIS_ProfileUtils::ChooseDefaultProfile(SortedNames, TEXT("Striker"));
        TSharedPtr<FString> DefaultItem;

        for (const TSharedPtr<FString> &Item : ProfileOptions)
        {
            if (Item.IsValid() && Item->Equals(DefaultName, ESearchCase::IgnoreCase))
            {
                DefaultItem = Item;
                break;
            }
        }

        if (!DefaultItem.IsValid())
        {
            DefaultItem = ProfileOptions[0];
        }

        ProfileDropdown->SetSelectedItem(DefaultItem);
        SelectedProfileName = DefaultItem.IsValid() ? *DefaultItem : FString();
        UE_LOG(LogTemp, Log, TEXT("SEAIS_GraphEditor: Selected default profile: %s"), *SelectedProfileName);
    }
    else if (ProfileOptions.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SEAIS_GraphEditor: No profiles found! Check that *.editor.json or *.json files exist in the profile directories."));
    }
}

void SEAIS_GraphEditor::OnProfileSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
    if (NewSelection.IsValid())
    {
        SelectedProfileName = *NewSelection;
        UE_LOG(LogTemp, Log, TEXT("SEAIS_GraphEditor: Selected profile: %s"), *SelectedProfileName);
    }
}

void SEAIS_GraphEditor::OnLoadProfileClicked()
{
    if (SelectedProfileName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("SEAIS_GraphEditor: No profile selected"));
        return;
    }

    // Try to find the editor file first (*.editor.json)
    FString EditorProfilesDir = GetEditorProfilesDirectory();
    FString EditorFilePath = EditorProfilesDir / (SelectedProfileName + TEXT(".editor.json"));

    if (FPaths::FileExists(EditorFilePath))
    {
        if (LoadFromFile(EditorFilePath))
        {
            UE_LOG(LogTemp, Display, TEXT("SEAIS_GraphEditor: Loaded editor profile: %s"), *SelectedProfileName);
            return;
        }
    }

    // Try runtime format
    FString RuntimeProfilesDir = GetProfilesDirectory();
    FString RuntimeFilePath = RuntimeProfilesDir / (SelectedProfileName + TEXT(".runtime.json"));

    if (FPaths::FileExists(RuntimeFilePath))
    {
        if (LoadFromFile(RuntimeFilePath))
        {
            UE_LOG(LogTemp, Display, TEXT("SEAIS_GraphEditor: Loaded runtime profile: %s"), *SelectedProfileName);
            return;
        }
    }

    // Try plain .json
    FString PlainFilePath = RuntimeProfilesDir / (SelectedProfileName + TEXT(".json"));
    if (FPaths::FileExists(PlainFilePath))
    {
        if (LoadFromFile(PlainFilePath))
        {
            UE_LOG(LogTemp, Display, TEXT("SEAIS_GraphEditor: Loaded profile: %s"), *SelectedProfileName);
            return;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("SEAIS_GraphEditor: Failed to find profile file for: %s"), *SelectedProfileName);
}

void SEAIS_GraphEditor::OnRefreshClicked()
{
    RefreshProfileList();
    UE_LOG(LogTemp, Log, TEXT("SEAIS_GraphEditor: Refreshed profile list, found %d profiles"), ProfileOptions.Num());
}

#undef LOCTEXT_NAMESPACE
