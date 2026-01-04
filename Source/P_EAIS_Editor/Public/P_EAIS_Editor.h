// Copyright Punal Manalan. All Rights Reserved.
// P_EAIS_Editor module - Editor-only functionality for EAIS

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class SDockTab;

class FP_EAIS_EditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    /** Spawns the EAIS Graph Editor tab */
    TSharedRef<SDockTab> SpawnGraphEditorTab(const class FSpawnTabArgs& Args);
};
