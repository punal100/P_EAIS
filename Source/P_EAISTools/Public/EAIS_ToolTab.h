/*
 * @Author: Punal Manalan
 * @Description: EAIS Tool Tab - Slate-based editor window for EAIS tools
 * @Date: 01/01/2026
 */

#pragma once

#include "CoreMinimal.h"

/**
 * FEAIS_ToolTab
 * Manages the EAIS editor tool window registration and spawning.
 * Opens a dockable Slate panel with AI Editor tools.
 */
class FEAIS_ToolTab
{
public:
    /** Unique tab name identifier */
    static const FName TabName;

    /** Register the tab spawner with the global tab manager */
    static void Register();

    /** Unregister the tab spawner */
    static void Unregister();

    /** Open/focus the EAIS tool tab */
    static void Open();
};
