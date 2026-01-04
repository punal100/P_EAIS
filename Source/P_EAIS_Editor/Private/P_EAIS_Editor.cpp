// Copyright Punal Manalan. All Rights Reserved.

#include "P_EAIS_Editor.h"
#include "SEAIS_GraphEditor.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FP_EAIS_EditorModule"

static const FName EAISGraphEditorTabName("EAISGraphEditorTab");

void FP_EAIS_EditorModule::StartupModule()
{
    // Register the EAIS Graph Editor tab spawner
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        EAISGraphEditorTabName,
        FOnSpawnTab::CreateRaw(this, &FP_EAIS_EditorModule::SpawnGraphEditorTab))
        .SetDisplayName(LOCTEXT("EAISGraphEditorTabTitle", "EAIS Graph Editor"))
        .SetTooltipText(LOCTEXT("EAISGraphEditorTabTooltip", "Open the EAIS AI Graph Editor"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

    // Register Menu Entry
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
    FToolMenuSection& Section = Menu->FindOrAddSection("EAIS", LOCTEXT("EAISSection", "EAIS"));
    
    Section.AddMenuEntry(
        "OpenEAISGraphEditor",
        LOCTEXT("OpenEAISGraphEditorLabel", "EAIS Graph Editor"),
        LOCTEXT("OpenEAISGraphEditorTooltip", "Open the EAIS AI Graph Editor"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"),
        FUIAction(FExecuteAction::CreateLambda([this]()
        {
            FGlobalTabmanager::Get()->TryInvokeTab(EAISGraphEditorTabName);
        }))
    );
}

void FP_EAIS_EditorModule::ShutdownModule()
{
    // Unregister the tab spawner
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(EAISGraphEditorTabName);
    }
}

TSharedRef<SDockTab> FP_EAIS_EditorModule::SpawnGraphEditorTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SEAIS_GraphEditor)
        ];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FP_EAIS_EditorModule, P_EAIS_Editor)
