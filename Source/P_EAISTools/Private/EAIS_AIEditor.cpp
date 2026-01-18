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
#include "Misc/ConfigCacheIni.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Framework/Docking/TabManager.h"
#include "Containers/Set.h" // For TSet in RefreshProfileList

#include "EAIS_ProfileUtils.h"

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

    // Display resolved profile paths (helps users know where to place files)
    if (RuntimePathText)
    {
        RuntimePathText->SetText(FText::FromString(
            FString::Printf(TEXT("Runtime Profiles: %s"), *GetProfilesDirectory())));
    }
    if (EditorPathText)
    {
        EditorPathText->SetText(FText::FromString(
            FString::Printf(TEXT("Editor Profiles: %s"), *GetEditorProfilesDirectory())));
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
    for (const FString &File : FoundFiles)
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

    FString ProfilesDir = GetProfilesDirectory();
    FString EditorDir = GetEditorProfilesDirectory();
    FString ErrorMessage;
    bool bValidated = false;

    // Try .editor.json first
    FString EditorProfilePath = FPaths::Combine(EditorDir, SelectedProfileName + TEXT(".editor.json"));
    if (FPaths::FileExists(EditorProfilePath))
    {
        if (ValidateProfileFile(EditorProfilePath, ErrorMessage))
        {
            SetStatus(FString::Printf(TEXT("✓ %s.editor.json is valid!"), *SelectedProfileName));
            bValidated = true;
        }
    }

    if (!bValidated)
    {
        // Try .runtime.json
        FString RuntimeProfilePath = FPaths::Combine(ProfilesDir, SelectedProfileName + TEXT(".runtime.json"));
        if (FPaths::FileExists(RuntimeProfilePath))
        {
            if (ValidateProfileFile(RuntimeProfilePath, ErrorMessage))
            {
                SetStatus(FString::Printf(TEXT("✓ %s.runtime.json is valid!"), *SelectedProfileName));
                bValidated = true;
            }
        }
    }

    if (!bValidated)
    {
        // Try plain .json
        FString PlainProfilePath = FPaths::Combine(ProfilesDir, SelectedProfileName + TEXT(".json"));
        if (FPaths::FileExists(PlainProfilePath))
        {
            if (ValidateProfileFile(PlainProfilePath, ErrorMessage))
            {
                SetStatus(FString::Printf(TEXT("✓ %s.json is valid!"), *SelectedProfileName));
                bValidated = true;
            }
        }
    }

    if (!bValidated)
    {
        if (ErrorMessage.IsEmpty())
        {
            ErrorMessage = TEXT("Profile file not found");
        }
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
        UE_LOG(LogTemp, Warning, TEXT("[EAIS_AIEditor] ProfileDropdown is null, cannot refresh"));
        return;
    }

    ProfileDropdown->ClearOptions();

    TSet<FString> UniqueProfileNames;

    // Search runtime profiles (*.runtime.json and *.json)
    FString RuntimeProfilesDir = GetProfilesDirectory();
    UE_LOG(LogTemp, Log, TEXT("[EAIS_AIEditor] Searching runtime profiles in: %s"), *RuntimeProfilesDir);

    if (FPaths::DirectoryExists(RuntimeProfilesDir))
    {
        TArray<FString> RuntimeFiles;
        IFileManager::Get().FindFiles(RuntimeFiles, *RuntimeProfilesDir, TEXT("*.json"));

        UE_LOG(LogTemp, Log, TEXT("[EAIS_AIEditor] Found %d runtime files"), RuntimeFiles.Num());

        for (const FString &File : RuntimeFiles)
        {
            FString ProfileName = FPaths::GetBaseFilename(File);
            // Strip .runtime suffix if present
            ProfileName = ProfileName.Replace(TEXT(".runtime"), TEXT(""));
            UniqueProfileNames.Add(ProfileName);
            UE_LOG(LogTemp, Verbose, TEXT("[EAIS_AIEditor] Runtime profile: %s"), *ProfileName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[EAIS_AIEditor] Runtime profiles directory does not exist: %s"), *RuntimeProfilesDir);
    }

    // Search editor profiles (*.editor.json)
    FString EditorProfilesDir = GetEditorProfilesDirectory();
    UE_LOG(LogTemp, Log, TEXT("[EAIS_AIEditor] Searching editor profiles in: %s"), *EditorProfilesDir);

    if (FPaths::DirectoryExists(EditorProfilesDir))
    {
        TArray<FString> EditorFiles;
        IFileManager::Get().FindFiles(EditorFiles, *EditorProfilesDir, TEXT("*.editor.json"));

        UE_LOG(LogTemp, Log, TEXT("[EAIS_AIEditor] Found %d editor files"), EditorFiles.Num());

        for (const FString &File : EditorFiles)
        {
            FString ProfileName = FPaths::GetBaseFilename(File);
            // Strip .editor suffix
            ProfileName = ProfileName.Replace(TEXT(".editor"), TEXT(""));
            UniqueProfileNames.Add(ProfileName);
            UE_LOG(LogTemp, Verbose, TEXT("[EAIS_AIEditor] Editor profile: %s"), *ProfileName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[EAIS_AIEditor] Editor profiles directory does not exist: %s"), *EditorProfilesDir);
    }

    // Add all unique profiles to dropdown (deterministic order)
    const TArray<FString> SortedNames = EAIS_ProfileUtils::MakeSortedUnique(UniqueProfileNames);
    for (const FString &ProfileName : SortedNames)
    {
        ProfileDropdown->AddOption(ProfileName);
    }
    UE_LOG(LogTemp, Log, TEXT("[EAIS_AIEditor] Added %d unique profiles to dropdown"), SortedNames.Num());

    // Select default profile deterministically
    const FString DefaultName = EAIS_ProfileUtils::ChooseDefaultProfile(SortedNames, TEXT("Striker"));
    if (!DefaultName.IsEmpty())
    {
        ProfileDropdown->SetSelectedOption(DefaultName);
        SelectedProfileName = DefaultName;

        if (ProfileNameText)
        {
            ProfileNameText->SetText(FText::FromString(DefaultName));
        }
    }

    SetStatus(FString::Printf(TEXT("Found %d profiles. Select and click 'Load'."), UniqueProfileNames.Num()));
}

FString UEAIS_AIEditor::GetProfilesDirectory() const
{
    // Priority: Additional paths from settings > Plugin Content > Project Content
    
    // First, check AdditionalProfilePaths from settings (same logic as EAISSubsystem)
    TArray<FString> ConfigSections = {
        TEXT("/Script/P_EAIS.EAISSettings"),
        TEXT("/Script/P_EAIS_Editor.EAISSettings")
    };

    for (const FString& Section : ConfigSections)
    {
        TArray<FString> PathEntries;
        if (GConfig->GetArray(*Section, TEXT("AdditionalProfilePaths"), PathEntries, GGameIni))
        {
            for (const FString& Entry : PathEntries)
            {
                // Entry format example: (Path="../Plugins/P_MiniFootball/Content/AIProfiles")
                FString PathValue = Entry;
                int32 QuoteStart = -1;
                int32 QuoteEnd = -1;
                
                if (PathValue.FindChar('"', QuoteStart))
                {
                    if (PathValue.FindLastChar('"', QuoteEnd))
                    {
                        if (QuoteEnd > QuoteStart)
                        {
                            PathValue = PathValue.Mid(QuoteStart + 1, QuoteEnd - QuoteStart - 1);
                        }
                    }
                }
                
                if (!PathValue.IsEmpty() && !PathValue.Contains(TEXT("(")))
                {
                    FString FullPath;
                    if (FPaths::IsRelative(PathValue))
                    {
                        FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / PathValue);
                    }
                    else
                    {
                        FullPath = FPaths::ConvertRelativePathToFull(PathValue);
                    }
                    
                    if (FPaths::DirectoryExists(FullPath))
                    {
                        return FullPath;
                    }
                }
            }
        }
    }

    // Try plugin content directory (where profiles typically live)
    // NOTE: Must use ConvertRelativePathToFull() as FPaths functions may return relative paths
    FString PluginProfilesDir = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("P_EAIS/Content/AIProfiles")));
    if (FPaths::DirectoryExists(PluginProfilesDir))
    {
        return PluginProfilesDir;
    }

    // Try alternative plugin location (git submodule path)
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

    // Return plugin path anyway (for creation if needed)
    return PluginProfilesDir;
}

FString UEAIS_AIEditor::GetEditorProfilesDirectory() const
{
    // Try plugin directory first
    // NOTE: Must use ConvertRelativePathToFull() as FPaths functions may return relative paths
    FString PluginEditorDir = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("P_EAIS/Editor/AI")));
    if (FPaths::DirectoryExists(PluginEditorDir))
    {
        return PluginEditorDir;
    }

    // Try alternative plugin location (git submodule path)
    FString AltPluginDir = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/P_EAIS/Editor/AI")));
    if (FPaths::DirectoryExists(AltPluginDir))
    {
        return AltPluginDir;
    }

    return PluginEditorDir;
}

void UEAIS_AIEditor::SetStatus(const FString &Message, bool bIsError)
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

bool UEAIS_AIEditor::ValidateProfileFile(const FString &FilePath, FString &OutErrorMessage)
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
    const TArray<TSharedPtr<FJsonValue>> *StatesArray;
    if (!JsonObject->TryGetArrayField(TEXT("states"), StatesArray))
    {
        OutErrorMessage = TEXT("'states' must be an array");
        return false;
    }

    // Check each state
    for (int32 i = 0; i < StatesArray->Num(); i++)
    {
        const TSharedPtr<FJsonObject> *StateObject;
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
    // Version 3: Added RuntimePathText and EditorPathText for path visibility
    FString Spec;
    Spec += TEXT("{\n");
    Spec += TEXT("  \"WidgetClass\": \"EUW_EAIS_AIEditor\",\n");
    Spec += TEXT("  \"BlueprintName\": \"EUW_EAIS_AIEditor\",\n");
    Spec += TEXT("  \"ParentClass\": \"UEAIS_AIEditor\",\n");
    Spec += TEXT("  \"Version\": 3,\n");
    Spec += TEXT("  \"WidgetType\": \"EditorUtilityWidget\",\n");
    Spec += TEXT("  \"RootWidget\": {\n");
    Spec += TEXT("    \"Type\": \"VerticalBox\",\n");
    Spec += TEXT("    \"Children\": [\n");
    Spec += TEXT("      { \"Type\": \"TextBlock\", \"Text\": \"EAIS AI Editor\" },\n");
    Spec += TEXT("      { \"Type\": \"TextBlock\", \"Text\": \"Select profile and Open Graph Editor\" },\n");
    Spec += TEXT("      { \"Type\": \"TextBlock\", \"Name\": \"RuntimePathText\", \"Text\": \"Runtime: (resolving...)\" },\n");
    Spec += TEXT("      { \"Type\": \"TextBlock\", \"Name\": \"EditorPathText\", \"Text\": \"Editor: (resolving...)\" },\n");
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
