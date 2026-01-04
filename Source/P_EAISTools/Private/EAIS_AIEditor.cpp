/*
 * @Author: Punal Manalan
 * @Description: EAIS AI Editor - Launcher Implementation
 * @Date: 01/01/2026
 */

#include "EAIS_AIEditor.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Framework/Docking/TabManager.h"

void UEAIS_AIEditor::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind buttons
    if (Btn_OpenGraphEditor)
    {
        Btn_OpenGraphEditor->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnOpenGraphEditorClicked);
    }
    if (Btn_ListProfiles)
    {
        Btn_ListProfiles->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnListProfilesClicked);
    }
    if (Btn_Load)
    {
        Btn_Load->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnLoadClicked);
    }
    if (Btn_Validate)
    {
        Btn_Validate->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnValidateClicked);
    }
    if (Btn_ExportRuntime)
    {
        Btn_ExportRuntime->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnExportRuntimeClicked);
    }
    if (Btn_TestSpawn)
    {
        Btn_TestSpawn->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnTestSpawnClicked);
    }
    if (ProfileDropdown)
    {
        ProfileDropdown->OnSelectionChanged.AddDynamic(this, &UEAIS_AIEditor::OnProfileSelected);
    }

    // Initialize
    RefreshProfileList();
    SetStatus(TEXT("EAIS Editor Ready. Select a profile and click 'Open Graph Editor'."));
}

void UEAIS_AIEditor::OnOpenGraphEditorClicked()
{
    // Open the SGraphEditor-based AI Graph Editor tab
    // This invokes the registered editor tab spawner
    FGlobalTabmanager::Get()->TryInvokeTab(FTabId("EAISGraphEditorTab"));
    SetStatus(TEXT("Graph Editor opened."));
}

void UEAIS_AIEditor::OnListProfilesClicked()
{
    RefreshProfileList();
    
    FString ProfilesDir = GetProfilesDirectory();
    TArray<FString> FoundFiles;
    IFileManager::Get().FindFiles(FoundFiles, *ProfilesDir, TEXT("*.json"));
    
    UE_LOG(LogTemp, Log, TEXT("--- Available Profiles in %s ---"), *ProfilesDir);
    for (const FString& File : FoundFiles)
    {
        UE_LOG(LogTemp, Log, TEXT("  • %s"), *FPaths::GetBaseFilename(File));
    }
    UE_LOG(LogTemp, Log, TEXT("Total: %d profiles"), FoundFiles.Num());
    
    SetStatus(FString::Printf(TEXT("Found %d profiles. Check Output Log for list."), FoundFiles.Num()));
}

void UEAIS_AIEditor::OnLoadClicked()
{
    if (!ProfileDropdown)
    {
        SetStatus(TEXT("Error: Profile dropdown not found"), true);
        return;
    }

    SelectedProfileName = ProfileDropdown->GetSelectedOption();
    if (SelectedProfileName.IsEmpty())
    {
        SetStatus(TEXT("Please select a profile from the dropdown"), true);
        return;
    }

    if (ProfileNameText)
    {
        ProfileNameText->SetText(FText::FromString(SelectedProfileName));
    }
    
    SetStatus(FString::Printf(TEXT("Selected: %s. Click 'Open Graph Editor' to edit."), *SelectedProfileName));
}

void UEAIS_AIEditor::OnValidateClicked()
{
    if (SelectedProfileName.IsEmpty())
    {
        SetStatus(TEXT("Please select and load a profile first"), true);
        return;
    }

    FString ProfilePath = FPaths::Combine(GetProfilesDirectory(), SelectedProfileName + TEXT(".json"));
    FString ErrorMessage;
    
    if (ValidateProfileFile(ProfilePath, ErrorMessage))
    {
        SetStatus(FString::Printf(TEXT("✓ %s is valid!"), *SelectedProfileName));
    }
    else
    {
        SetStatus(FString::Printf(TEXT("✕ Validation failed: %s"), *ErrorMessage), true);
    }
}

void UEAIS_AIEditor::OnExportRuntimeClicked()
{
    if (SelectedProfileName.IsEmpty())
    {
        SetStatus(TEXT("Please select a profile first"), true);
        return;
    }

    // Export from editor JSON to runtime JSON
    FString EditorPath = FPaths::Combine(GetEditorProfilesDirectory(), SelectedProfileName + TEXT(".editor.json"));
    FString RuntimePath = FPaths::Combine(GetProfilesDirectory(), SelectedProfileName + TEXT(".runtime.json"));
    
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *EditorPath))
    {
        // Try loading from runtime path if editor doesn't exist
        FString AltPath = FPaths::Combine(GetProfilesDirectory(), SelectedProfileName + TEXT(".json"));
        if (!FFileHelper::LoadFileToString(JsonContent, *AltPath))
        {
            SetStatus(FString::Printf(TEXT("Failed to load: %s"), *SelectedProfileName), true);
            return;
        }
    }

    // Parse and strip editor-only fields
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        SetStatus(TEXT("Failed to parse JSON"), true);
        return;
    }

    // Remove editor-only keys
    JsonObject->RemoveField(TEXT("editor"));
    JsonObject->RemoveField(TEXT("schemaVersion"));

    // Write runtime JSON
    FString OutputJson;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = 
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputJson);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    if (FFileHelper::SaveStringToFile(OutputJson, *RuntimePath))
    {
        SetStatus(FString::Printf(TEXT("Exported: %s"), *RuntimePath));
    }
    else
    {
        SetStatus(TEXT("Failed to save runtime JSON"), true);
    }
}

void UEAIS_AIEditor::OnTestSpawnClicked()
{
    if (SelectedProfileName.IsEmpty())
    {
        SetStatus(TEXT("Please select a profile first"), true);
        return;
    }

    // Execute console command to spawn
    if (GEngine)
    {
        FString Command = FString::Printf(TEXT("EAIS.SpawnBot 1 %s"), *SelectedProfileName);
        GEngine->Exec(nullptr, *Command);
        SetStatus(FString::Printf(TEXT("Spawning AI with profile: %s"), *SelectedProfileName));
    }
}

void UEAIS_AIEditor::OnProfileSelected(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectionType == ESelectInfo::OnMouseClick || SelectionType == ESelectInfo::OnKeyPress)
    {
        SelectedProfileName = SelectedItem;
        if (ProfileNameText)
        {
            ProfileNameText->SetText(FText::FromString(SelectedItem));
        }
        SetStatus(FString::Printf(TEXT("Selected: %s"), *SelectedItem));
    }
}

void UEAIS_AIEditor::RefreshProfileList()
{
    if (!ProfileDropdown)
    {
        return;
    }

    ProfileDropdown->ClearOptions();

    FString ProfilesDir = GetProfilesDirectory();
    TArray<FString> FoundFiles;
    IFileManager::Get().FindFiles(FoundFiles, *ProfilesDir, TEXT("*.json"));

    for (const FString& File : FoundFiles)
    {
        FString ProfileName = FPaths::GetBaseFilename(File);
        // Strip .runtime suffix if present
        ProfileName = ProfileName.Replace(TEXT(".runtime"), TEXT(""));
        ProfileDropdown->AddOption(ProfileName);
    }

    if (FoundFiles.Num() > 0)
    {
        ProfileDropdown->SetSelectedOption(FPaths::GetBaseFilename(FoundFiles[0]).Replace(TEXT(".runtime"), TEXT("")));
    }
}

FString UEAIS_AIEditor::GetProfilesDirectory() const
{
    return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("AIProfiles"));
}

FString UEAIS_AIEditor::GetEditorProfilesDirectory() const
{
    return FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("P_EAIS/Editor/AI"));
}

void UEAIS_AIEditor::SetStatus(const FString& Message, bool bIsError)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(Message));
        StatusText->SetColorAndOpacity(bIsError ? FLinearColor::Red : FLinearColor::White);
    }
    
    // Also log to output
    if (bIsError)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EAIS Editor] %s"), *Message);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[EAIS Editor] %s"), *Message);
    }
}

bool UEAIS_AIEditor::ValidateProfileFile(const FString& FilePath, FString& OutErrorMessage)
{
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
    {
        OutErrorMessage = TEXT("Failed to load file");
        return false;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutErrorMessage = TEXT("Invalid JSON syntax");
        return false;
    }

    // Check required fields
    if (!JsonObject->HasField(TEXT("name")))
    {
        OutErrorMessage = TEXT("Missing 'name' field");
        return false;
    }
    if (!JsonObject->HasField(TEXT("states")))
    {
        OutErrorMessage = TEXT("Missing 'states' field");
        return false;
    }

    // Validate states is an array
    const TArray<TSharedPtr<FJsonValue>>* StatesArray;
    if (!JsonObject->TryGetArrayField(TEXT("states"), StatesArray))
    {
        OutErrorMessage = TEXT("'states' must be an array");
        return false;
    }

    // Check each state
    for (int32 i = 0; i < StatesArray->Num(); i++)
    {
        const TSharedPtr<FJsonObject>* StateObject;
        if (!(*StatesArray)[i]->TryGetObject(StateObject))
        {
            OutErrorMessage = FString::Printf(TEXT("State %d must be an object"), i);
            return false;
        }
        if (!(*StateObject)->HasField(TEXT("id")))
        {
            OutErrorMessage = FString::Printf(TEXT("State %d missing 'id' field"), i);
            return false;
        }
    }

    return true;
}

FString UEAIS_AIEditor::GetWidgetSpec()
{
    // Simplified spec using ONLY P_MWCS-supported widgets
    // Building JSON string without raw literals to avoid macro issues
    FString Spec;
    Spec += TEXT("{\n");
    Spec += TEXT("  \"WidgetClass\": \"EUW_EAIS_AIEditor\",\n");
    Spec += TEXT("  \"BlueprintName\": \"EUW_EAIS_AIEditor\",\n");
    Spec += TEXT("  \"ParentClass\": \"UEAIS_AIEditor\",\n");
    Spec += TEXT("  \"Version\": 2,\n");
    Spec += TEXT("  \"WidgetType\": \"EditorUtilityWidget\",\n");
    Spec += TEXT("  \"RootWidget\": {\n");
    Spec += TEXT("    \"Type\": \"VerticalBox\",\n");
    Spec += TEXT("    \"Children\": [\n");
    Spec += TEXT("      { \"Type\": \"TextBlock\", \"Text\": \"EAIS AI Editor\" },\n");
    Spec += TEXT("      { \"Type\": \"TextBlock\", \"Text\": \"Select profile and Open Graph Editor\" },\n");
    Spec += TEXT("      { \"Type\": \"ComboBoxString\", \"Name\": \"ProfileDropdown\" },\n");
    Spec += TEXT("      { \"Type\": \"TextBlock\", \"Name\": \"ProfileNameText\", \"Text\": \"(none)\" },\n");
    Spec += TEXT("      { \"Type\": \"TextBlock\", \"Name\": \"StatusText\", \"Text\": \"Ready\" },\n");
    Spec += TEXT("      { \"Type\": \"Button\", \"Name\": \"Btn_OpenGraphEditor\", \"Text\": \"Open Graph Editor\" },\n");
    Spec += TEXT("      { \"Type\": \"Button\", \"Name\": \"Btn_ListProfiles\", \"Text\": \"Refresh\" },\n");
    Spec += TEXT("      { \"Type\": \"Button\", \"Name\": \"Btn_Load\", \"Text\": \"Load\" },\n");
    Spec += TEXT("      { \"Type\": \"Button\", \"Name\": \"Btn_Validate\", \"Text\": \"Validate\" },\n");
    Spec += TEXT("      { \"Type\": \"Button\", \"Name\": \"Btn_ExportRuntime\", \"Text\": \"Export\" },\n");
    Spec += TEXT("      { \"Type\": \"Button\", \"Name\": \"Btn_TestSpawn\", \"Text\": \"Test Spawn\" }\n");
    Spec += TEXT("    ]\n");
    Spec += TEXT("  }\n");
    Spec += TEXT("}\n");
    return Spec;
}

