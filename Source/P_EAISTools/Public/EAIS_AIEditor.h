/*
 * @Author: Punal Manalan
 * @Description: EAIS AI Editor - Launcher widget for AI Graph Editor
 * @Date: 01/01/2026
 * 
 * ARCHITECTURE:
 * - This EUW is a LAUNCHER only
 * - Uses only P_MWCS-supported widgets (Button, TextBlock, ComboBoxString)
 * - Opens the real SGraphEditor-based AI Graph Editor via dockable tab
 */

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "EAIS_AIEditor.generated.h"

class UButton;
class UTextBlock;
class UComboBoxString;

/**
 * EAIS AI Editor Launcher Widget
 * 
 * A simple launcher for the AI Graph Editor.
 * The actual AI behavior editing happens in the SGraphEditor-based tab.
 * 
 * Features:
 * - Profile selection dropdown
 * - Open Graph Editor button (launches SEAIS_GraphEditor tab)
 * - List/Load/Validate profiles
 * - Export runtime JSON
 */
UCLASS()
class P_EAISTOOLS_API UEAIS_AIEditor : public UEditorUtilityWidget
{
    GENERATED_BODY()

public:
    // ==================== UI Bindings ====================

    /** Profile selection dropdown */
    UPROPERTY(meta = (BindWidget))
    UComboBoxString* ProfileDropdown;

    /** Status text display */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* StatusText;

    /** Current profile name display */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ProfileNameText;

    // ==================== Buttons ====================

    /** Open the graph editor tab */
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_OpenGraphEditor;

    /** List available profiles */
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_ListProfiles;

    /** Load selected profile */
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Load;

    /** Validate selected profile */
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Validate;

    /** Export runtime JSON */
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_ExportRuntime;

    /** Spawn test AI */
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_TestSpawn;

    // ==================== Widget Spec ====================

    /** Get the widget specification for P_MWCS generation */
    UFUNCTION(BlueprintCallable, Category = "EAIS Editor")
    static FString GetWidgetSpec();

    /** Get the widget class name */
    UFUNCTION(BlueprintPure, Category = "EAIS Editor")
    static FString GetWidgetClassName() { return TEXT("EUW_EAIS_AIEditor"); }

protected:
    virtual void NativeConstruct() override;

    // ==================== Button Handlers ====================

    UFUNCTION()
    void OnOpenGraphEditorClicked();

    UFUNCTION()
    void OnListProfilesClicked();

    UFUNCTION()
    void OnLoadClicked();

    UFUNCTION()
    void OnValidateClicked();

    UFUNCTION()
    void OnExportRuntimeClicked();

    UFUNCTION()
    void OnTestSpawnClicked();

    UFUNCTION()
    void OnProfileSelected(FString SelectedItem, ESelectInfo::Type SelectionType);

    // ==================== Helper Functions ====================

    /** Refresh the profiles dropdown */
    void RefreshProfileList();

    /** Get the profiles directory path */
    FString GetProfilesDirectory() const;

    /** Get the editor profiles directory path */
    FString GetEditorProfilesDirectory() const;

    /** Update status text */
    void SetStatus(const FString& Message, bool bIsError = false);

    /** Validate a profile JSON file */
    bool ValidateProfileFile(const FString& FilePath, FString& OutErrorMessage);

private:
    /** Currently selected profile name */
    FString SelectedProfileName;
};
