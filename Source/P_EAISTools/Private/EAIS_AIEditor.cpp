/*
 * @Author: Punal Manalan
 * @Description: EAIS AI Editor - Implementation
 * @Date: 01/01/2026
 */

#include "EAIS_AIEditor.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UEAIS_AIEditor::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind buttons
    if (Btn_New)
    {
        Btn_New->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnNewClicked);
    }
    if (Btn_Load)
    {
        Btn_Load->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnLoadClicked);
    }
    if (Btn_Save)
    {
        Btn_Save->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnSaveClicked);
    }
    if (Btn_Validate)
    {
        Btn_Validate->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnValidateClicked);
    }
    if (Btn_Format)
    {
        Btn_Format->OnClicked.AddDynamic(this, &UEAIS_AIEditor::OnFormatClicked);
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
    SetStatus(TEXT("Ready. Select a profile or create a new one."));
    bIsModified = false;
}

void UEAIS_AIEditor::OnNewClicked()
{
    // Create new profile template
    const FString NewProfileTemplate = TEXT(R"({
  "name": "NewProfile",
  "blackboard": {
    "HasBall": false
  },
  "states": {
    "Idle": {
      "OnEnter": [
        { "Action": "LookAround" }
      ],
      "OnTick": [],
      "Transitions": [
        { "Target": "ChaseBall", "Condition": { "type": "Event", "name": "BallSeen" } }
      ]
    },
    "ChaseBall": {
      "OnTick": [
        { "Action": "MoveTo", "Params": { "Target": "ball" } }
      ],
      "Transitions": [
        { "Target": "Idle", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": true } }
      ]
    }
  }
})");

    if (JsonEditor)
    {
        JsonEditor->SetText(FText::FromString(NewProfileTemplate));
    }
    if (ProfileNameInput)
    {
        ProfileNameInput->SetText(FText::FromString(TEXT("NewProfile")));
    }
    
    CurrentProfileName.Empty();
    bIsModified = true;
    SetStatus(TEXT("New profile created. Edit and save when ready."));
    ParseAndDisplayStates();
}

void UEAIS_AIEditor::OnLoadClicked()
{
    if (!ProfileDropdown)
    {
        SetStatus(TEXT("Error: Profile dropdown not found"), true);
        return;
    }

    FString SelectedProfile = ProfileDropdown->GetSelectedOption();
    if (SelectedProfile.IsEmpty())
    {
        SetStatus(TEXT("Please select a profile from the dropdown"), true);
        return;
    }

    if (LoadProfile(SelectedProfile))
    {
        SetStatus(FString::Printf(TEXT("Loaded: %s"), *SelectedProfile));
    }
}

void UEAIS_AIEditor::OnSaveClicked()
{
    if (!SaveCurrentProfile())
    {
        return;
    }
    
    RefreshProfileList();
    SetStatus(FString::Printf(TEXT("Saved: %s"), *CurrentProfileName));
}

void UEAIS_AIEditor::OnValidateClicked()
{
    FString ErrorMessage;
    bool bIsValid = ValidateJson(ErrorMessage);
    
    if (bIsValid)
    {
        SetValidationResult(TEXT("✓ JSON is valid"), true);
        SetStatus(TEXT("Validation passed"));
    }
    else
    {
        SetValidationResult(FString::Printf(TEXT("✕ %s"), *ErrorMessage), false);
        SetStatus(TEXT("Validation failed"), true);
    }
}

void UEAIS_AIEditor::OnFormatClicked()
{
    FormatJson();
    SetStatus(TEXT("JSON formatted"));
}

void UEAIS_AIEditor::OnTestSpawnClicked()
{
    SpawnTestAI();
}

void UEAIS_AIEditor::OnProfileSelected(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectionType == ESelectInfo::OnMouseClick || SelectionType == ESelectInfo::OnKeyPress)
    {
        // User manually selected, prompt to load
        SetStatus(FString::Printf(TEXT("Selected: %s. Click Load to edit."), *SelectedItem));
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
        ProfileDropdown->AddOption(ProfileName);
    }

    if (FoundFiles.Num() > 0)
    {
        ProfileDropdown->SetSelectedOption(FPaths::GetBaseFilename(FoundFiles[0]));
    }
}

bool UEAIS_AIEditor::LoadProfile(const FString& ProfileName)
{
    FString ProfilePath = FPaths::Combine(GetProfilesDirectory(), ProfileName + TEXT(".json"));
    
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *ProfilePath))
    {
        SetStatus(FString::Printf(TEXT("Failed to load: %s"), *ProfilePath), true);
        return false;
    }

    if (JsonEditor)
    {
        JsonEditor->SetText(FText::FromString(JsonContent));
    }
    if (ProfileNameInput)
    {
        ProfileNameInput->SetText(FText::FromString(ProfileName));
    }

    CurrentProfileName = ProfileName;
    bIsModified = false;
    
    ParseAndDisplayStates();
    
    // Auto-validate on load
    FString ErrorMessage;
    bool bIsValid = ValidateJson(ErrorMessage);
    SetValidationResult(bIsValid ? TEXT("✓ Valid") : FString::Printf(TEXT("✕ %s"), *ErrorMessage), bIsValid);
    
    return true;
}

bool UEAIS_AIEditor::SaveCurrentProfile()
{
    if (!JsonEditor || !ProfileNameInput)
    {
        SetStatus(TEXT("Error: UI not initialized"), true);
        return false;
    }

    FString ProfileName = ProfileNameInput->GetText().ToString();
    if (ProfileName.IsEmpty())
    {
        SetStatus(TEXT("Please enter a profile name"), true);
        return false;
    }

    // Validate before saving
    FString ErrorMessage;
    if (!ValidateJson(ErrorMessage))
    {
        SetStatus(FString::Printf(TEXT("Cannot save invalid JSON: %s"), *ErrorMessage), true);
        return false;
    }

    FString JsonContent = JsonEditor->GetText().ToString();
    FString ProfilePath = FPaths::Combine(GetProfilesDirectory(), ProfileName + TEXT(".json"));

    if (!FFileHelper::SaveStringToFile(JsonContent, *ProfilePath))
    {
        SetStatus(FString::Printf(TEXT("Failed to save: %s"), *ProfilePath), true);
        return false;
    }

    CurrentProfileName = ProfileName;
    bIsModified = false;
    return true;
}

bool UEAIS_AIEditor::ValidateJson(FString& OutErrorMessage)
{
    if (!JsonEditor)
    {
        OutErrorMessage = TEXT("JSON editor not found");
        return false;
    }

    FString JsonContent = JsonEditor->GetText().ToString();
    
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
        OutErrorMessage = TEXT("Missing required field: 'name'");
        return false;
    }
    if (!JsonObject->HasField(TEXT("states")))
    {
        OutErrorMessage = TEXT("Missing required field: 'states'");
        return false;
    }

    // Validate states object
    const TSharedPtr<FJsonObject>* StatesObject;
    if (!JsonObject->TryGetObjectField(TEXT("states"), StatesObject))
    {
        OutErrorMessage = TEXT("'states' must be an object");
        return false;
    }

    // Check each state has valid structure
    for (const auto& StatePair : (*StatesObject)->Values)
    {
        const TSharedPtr<FJsonObject>* StateObject;
        if (!StatePair.Value->TryGetObject(StateObject))
        {
            OutErrorMessage = FString::Printf(TEXT("State '%s' must be an object"), *StatePair.Key);
            return false;
        }
    }

    return true;
}

void UEAIS_AIEditor::FormatJson()
{
    if (!JsonEditor)
    {
        return;
    }

    FString JsonContent = JsonEditor->GetText().ToString();
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        FString FormattedJson;
        TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = 
            TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&FormattedJson);
        
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        JsonEditor->SetText(FText::FromString(FormattedJson));
    }
    else
    {
        SetStatus(TEXT("Cannot format: Invalid JSON"), true);
    }
}

void UEAIS_AIEditor::SetStatus(const FString& Message, bool bIsError)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(Message));
        StatusText->SetColorAndOpacity(bIsError ? FLinearColor::Red : FLinearColor::White);
    }
}

void UEAIS_AIEditor::SetValidationResult(const FString& Message, bool bIsValid)
{
    if (ValidationText)
    {
        ValidationText->SetText(FText::FromString(Message));
        ValidationText->SetColorAndOpacity(bIsValid ? FLinearColor::Green : FLinearColor::Red);
    }
}

void UEAIS_AIEditor::SpawnTestAI()
{
    if (!ProfileNameInput)
    {
        SetStatus(TEXT("Cannot spawn: No profile name"), true);
        return;
    }

    FString ProfileName = ProfileNameInput->GetText().ToString();
    if (ProfileName.IsEmpty())
    {
        SetStatus(TEXT("Enter a profile name first"), true);
        return;
    }

    // Save first if modified
    if (bIsModified)
    {
        if (!SaveCurrentProfile())
        {
            return;
        }
    }

    // Execute console command to spawn
    if (UWorld* World = GetWorld())
    {
        FString Command = FString::Printf(TEXT("EAIS.SpawnBot 1 %s"), *ProfileName);
        GEngine->Exec(World, *Command);
        SetStatus(FString::Printf(TEXT("Spawned AI with profile: %s"), *ProfileName));
    }
}

FString UEAIS_AIEditor::GetProfilesDirectory() const
{
    return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("AIProfiles"));
}

void UEAIS_AIEditor::ParseAndDisplayStates()
{
    if (!StatesPanel || !JsonEditor)
    {
        return;
    }

    StatesPanel->ClearChildren();

    FString JsonContent = JsonEditor->GetText().ToString();
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return;
    }

    const TSharedPtr<FJsonObject>* StatesObject;
    if (!JsonObject->TryGetObjectField(TEXT("states"), StatesObject))
    {
        return;
    }

    // States are displayed via the Inspector panel - this is a simplified view
    // Full implementation would create interactive state widgets
}

FString UEAIS_AIEditor::GetWidgetSpec()
{
    return TEXT(R"({
  "WidgetClass": "EUW_EAIS_AIEditor",
  "WidgetType": "EditorUtilityWidget",
  "RootWidget": {
    "Type": "VerticalBox",
    "Padding": { "Left": 10, "Top": 10, "Right": 10, "Bottom": 10 },
    "Children": [
      {
        "Type": "HorizontalBox",
        "Slot": { "AutoSize": true },
        "Children": [
          {
            "Type": "TextBlock",
            "Text": "EAIS AI Editor",
            "Font": { "Size": 20, "TypeFace": "Bold" }
          },
          { "Type": "Spacer", "Slot": { "FillWidth": 1.0 } },
          {
            "Type": "ComboBoxString",
            "Name": "ProfileDropdown",
            "MinWidth": 200
          },
          {
            "Type": "Button",
            "Name": "Btn_Load",
            "Text": "Load",
            "Style": { "BackgroundColor": "#2196F3" }
          },
          {
            "Type": "Button",
            "Name": "Btn_New",
            "Text": "New",
            "Style": { "BackgroundColor": "#4CAF50" }
          }
        ]
      },
      {
        "Type": "HorizontalBox",
        "Slot": { "AutoSize": true },
        "Children": [
          {
            "Type": "TextBlock",
            "Text": "Profile Name:"
          },
          {
            "Type": "EditableTextBox",
            "Name": "ProfileNameInput",
            "Slot": { "FillWidth": 1.0 },
            "HintText": "Enter profile name..."
          },
          {
            "Type": "Button",
            "Name": "Btn_Save",
            "Text": "Save",
            "Style": { "BackgroundColor": "#4CAF50" }
          }
        ]
      },
      {
        "Type": "Separator"
      },
      {
        "Type": "HorizontalBox",
        "Slot": { "FillHeight": 1.0 },
        "Children": [
          {
            "Type": "VerticalBox",
            "Slot": { "FillWidth": 0.6 },
            "Children": [
              {
                "Type": "HorizontalBox",
                "Slot": { "AutoSize": true },
                "Children": [
                  {
                    "Type": "TextBlock",
                    "Text": "JSON Editor",
                    "Font": { "Size": 14, "TypeFace": "Bold" }
                  },
                  { "Type": "Spacer", "Slot": { "FillWidth": 1.0 } },
                  {
                    "Type": "Button",
                    "Name": "Btn_Format",
                    "Text": "Format"
                  },
                  {
                    "Type": "Button",
                    "Name": "Btn_Validate",
                    "Text": "Validate"
                  }
                ]
              },
              {
                "Type": "MultiLineEditableTextBox",
                "Name": "JsonEditor",
                "Slot": { "FillHeight": 1.0 },
                "Style": { "BackgroundColor": "#1e1e1e", "FontFamily": "Courier New" }
              }
            ]
          },
          {
            "Type": "Separator",
            "Orientation": "Vertical"
          },
          {
            "Type": "VerticalBox",
            "Slot": { "FillWidth": 0.4 },
            "Children": [
              {
                "Type": "TextBlock",
                "Text": "States",
                "Font": { "Size": 14, "TypeFace": "Bold" }
              },
              {
                "Type": "ScrollBox",
                "Name": "StatesPanel",
                "Slot": { "FillHeight": 0.5 }
              },
              {
                "Type": "TextBlock",
                "Text": "Inspector",
                "Font": { "Size": 14, "TypeFace": "Bold" }
              },
              {
                "Type": "ScrollBox",
                "Name": "InspectorPanel",
                "Slot": { "FillHeight": 0.5 }
              }
            ]
          }
        ]
      },
      {
        "Type": "Separator"
      },
      {
        "Type": "HorizontalBox",
        "Slot": { "AutoSize": true },
        "Children": [
          {
            "Type": "TextBlock",
            "Name": "StatusText",
            "Text": "Ready"
          },
          { "Type": "Spacer", "Slot": { "FillWidth": 0.5 } },
          {
            "Type": "TextBlock",
            "Name": "ValidationText",
            "Text": ""
          },
          { "Type": "Spacer", "Slot": { "FillWidth": 0.5 } },
          {
            "Type": "Button",
            "Name": "Btn_TestSpawn",
            "Text": "Test Spawn AI",
            "Style": { "BackgroundColor": "#FF9800" }
          }
        ]
      }
    ]
  }
})");
}
