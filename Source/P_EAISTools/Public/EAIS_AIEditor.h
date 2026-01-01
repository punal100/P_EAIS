/*
 * @Author: Punal Manalan
 * @Description: EAIS AI Editor - Visual editor widget for creating and editing AI behaviors
 * @Date: 01/01/2026
 */

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "EAIS_AIEditor.generated.h"

class UButton;
class UTextBlock;
class UListView;
class UScrollBox;
class UEditableTextBox;
class UMultiLineEditableTextBox;
class UComboBoxString;
class UBorder;

/**
 * EAIS AI Editor Widget
 * A visual editor for creating, editing, validating, and testing AI behavior JSON profiles.
 * 
 * Features:
 * - Load/Save JSON files from Content/AIProfiles
 * - Validate JSON against schema
 * - Edit AI states, transitions, and actions
 * - Preview AI behavior on selected character
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

    /** Profile name input */
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* ProfileNameInput;

    /** JSON Editor text area */
    UPROPERTY(meta = (BindWidget))
    UMultiLineEditableTextBox* JsonEditor;

    /** Status text display */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* StatusText;

    /** Validation result text */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ValidationText;

    /** States list container */
    UPROPERTY(meta = (BindWidget))
    UScrollBox* StatesPanel;

    /** Inspector panel container */
    UPROPERTY(meta = (BindWidget))
    UScrollBox* InspectorPanel;

    // ==================== Buttons ====================

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_New;

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Load;

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Save;

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Validate;

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Format;

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
    void OnNewClicked();

    UFUNCTION()
    void OnLoadClicked();

    UFUNCTION()
    void OnSaveClicked();

    UFUNCTION()
    void OnValidateClicked();

    UFUNCTION()
    void OnFormatClicked();

    UFUNCTION()
    void OnTestSpawnClicked();

    UFUNCTION()
    void OnProfileSelected(FString SelectedItem, ESelectInfo::Type SelectionType);

    // ==================== Helper Functions ====================

    /** Refresh the profiles dropdown */
    void RefreshProfileList();

    /** Load a profile by name */
    bool LoadProfile(const FString& ProfileName);

    /** Save current profile */
    bool SaveCurrentProfile();

    /** Validate the current JSON */
    bool ValidateJson(FString& OutErrorMessage);

    /** Format/prettify the JSON */
    void FormatJson();

    /** Update status text */
    void SetStatus(const FString& Message, bool bIsError = false);

    /** Update validation display */
    void SetValidationResult(const FString& Message, bool bIsValid);

    /** Spawn AI for testing */
    void SpawnTestAI();

    /** Get the profiles directory path */
    FString GetProfilesDirectory() const;

    /** Parse states from JSON and populate panel */
    void ParseAndDisplayStates();

private:
    /** Currently loaded profile name */
    FString CurrentProfileName;

    /** Is the current profile modified */
    bool bIsModified;
};
