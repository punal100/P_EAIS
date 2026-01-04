/*
 * @Author: Punal Manalan
 * @Description: EAIS Tool Tab Implementation - Slate-based editor window
 * @Date: 01/01/2026
 */

#include "EAIS_ToolTab.h"
#include "EAIS_EditorWidgetSpec.h"

#include "MWCS_Service.h"
#include "MWCS_Report.h"

#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

const FName FEAIS_ToolTab::TabName(TEXT("EAIS.ToolTab"));

/**
 * SEAIS_ToolPanel
 * Slate compound widget for the EAIS tool panel UI
 */
class SEAIS_ToolPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SEAIS_ToolPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        RefreshProfileList();

        ChildSlot
        [
            SNew(SVerticalBox)
            
            // Title
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("EAIS — Enhanced AI System")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
            ]
            
            // Profile Selection Row
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(4)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Profile:")))
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(4)
                [
                    SAssignNew(ProfileComboBox, SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&ProfileOptions)
                    .OnGenerateWidget(this, &SEAIS_ToolPanel::OnGenerateProfileComboItem)
                    .OnSelectionChanged(this, &SEAIS_ToolPanel::OnProfileSelectionChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(this, &SEAIS_ToolPanel::GetSelectedProfileText)
                    ]
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(4)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Refresh")))
                    .OnClicked(this, &SEAIS_ToolPanel::OnRefreshProfiles)
                ]
            ]

            // Action Buttons Row 1
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Load Selected")))
                    .HAlign(HAlign_Center)
                    .OnClicked(this, &SEAIS_ToolPanel::OnLoadProfile)
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Create New")))
                    .HAlign(HAlign_Center)
                    .OnClicked(this, &SEAIS_ToolPanel::OnCreateProfile)
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Save")))
                    .HAlign(HAlign_Center)
                    .OnClicked(this, &SEAIS_ToolPanel::OnSaveProfile)
                ]
            ]
            
            // Action Buttons Row 2
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Validate")))
                    .OnClicked(this, &SEAIS_ToolPanel::OnValidate)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Format JSON")))
                    .OnClicked(this, &SEAIS_ToolPanel::OnFormat)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Generate/Repair Editor EUW")))
                    .OnClicked(this, &SEAIS_ToolPanel::OnGenerateEditorEuw)
                ]
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8)
            [
                SNew(SSeparator)
            ]
            
            // Log Output
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(8)
            [
                SAssignNew(LogBox, SMultiLineEditableTextBox)
                .IsReadOnly(true)
                .Text(FText::FromString(TEXT("EAIS Tool ready. Select a profile or create new.")))
            ]
        ];
    }

private:
    TSharedPtr<SMultiLineEditableTextBox> LogBox;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> ProfileComboBox;
    TArray<TSharedPtr<FString>> ProfileOptions;
    TSharedPtr<FString> SelectedProfile;
    
    FString CurrentProfileName;
    FString CurrentProfileJson;

    void AppendLine(const FString& Line)
    {
        if (!LogBox.IsValid())
        {
            return;
        }
        FString Existing = LogBox->GetText().ToString();
        if (!Existing.IsEmpty())
        {
            Existing += TEXT("\n");
        }
        Existing += Line;
        
        // Auto scroll to bottom (simple implementation)
        LogBox->SetText(FText::FromString(Existing));
    }

    FString GetProfilesDirectory() const
    {
        return FPaths::ProjectContentDir() / TEXT("AIProfiles");
    }

    void RefreshProfileList()
    {
        ProfileOptions.Empty();
        FString ProfilesDir = GetProfilesDirectory();
        
        TArray<FString> Files;
        IFileManager::Get().FindFiles(Files, *(ProfilesDir / TEXT("*.json")), true, false);
        
        for (const FString& File : Files)
        {
            ProfileOptions.Add(MakeShared<FString>(FPaths::GetBaseFilename(File)));
        }
        
        if (ProfileComboBox.IsValid())
        {
            ProfileComboBox->RefreshOptions();
        }
    }

    TSharedRef<SWidget> OnGenerateProfileComboItem(TSharedPtr<FString> Item)
    {
        return SNew(STextBlock).Text(FText::FromString(*Item));
    }

    void OnProfileSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type)
    {
        SelectedProfile = NewValue;
    }

    FText GetSelectedProfileText() const
    {
        return SelectedProfile.IsValid() ? FText::FromString(*SelectedProfile) : FText::FromString(TEXT("Select a Profile..."));
    }

    FReply OnRefreshProfiles()
    {
        RefreshProfileList();
        AppendLine(TEXT("Profile list refreshed."));
        return FReply::Handled();
    }

    FReply OnCreateProfile()
    {
        FString ProfilesDir = GetProfilesDirectory();
        FString NewName = FString::Printf(TEXT("NewProfile_%s"), *FDateTime::Now().ToString(TEXT("%H%M%S")));
        FString NewPath = ProfilesDir / NewName + TEXT(".json");
        
        // Basic template
        FString Template = TEXT(R"({
  "name": "New AI Profile",
  "states": [
    {
      "id": "idle",
      "actions": []
    }
  ]
})");
        
        if (FFileHelper::SaveStringToFile(Template, *NewPath))
        {
            AppendLine(FString::Printf(TEXT("Created new profile: %s"), *NewName));
            RefreshProfileList();
            
            // Auto-select the new one
            for (auto Option : ProfileOptions)
            {
                if (*Option == NewName)
                {
                    SelectedProfile = Option;
                    if (ProfileComboBox.IsValid())
                    {
                        ProfileComboBox->SetSelectedItem(Option);
                    }
                    OnLoadProfile();
                    break;
                }
            }
        }
        else
        {
            AppendLine(TEXT("ERROR: Failed to create new profile file."));
        }
        
        return FReply::Handled();
    }

    FReply OnLoadProfile()
    {
        if (!SelectedProfile.IsValid())
        {
            AppendLine(TEXT("Please select a profile first."));
            return FReply::Handled();
        }

        CurrentProfileName = *SelectedProfile;
        FString FilePath = GetProfilesDirectory() / CurrentProfileName + TEXT(".json");
        
        if (FFileHelper::LoadFileToString(CurrentProfileJson, *FilePath))
        {
            AppendLine(FString::Printf(TEXT("[Load] Loaded: %s"), *CurrentProfileName));
            AppendLine(TEXT("Tip: Use the Graph Editor (Tools > EAIS Graph Editor) for visual editing"));
        }
        else
        {
            AppendLine(FString::Printf(TEXT("[Load] ERROR: Failed to read %s"), *CurrentProfileName));
        }
        
        return FReply::Handled();
    }

    FReply OnSaveProfile()
    {
        if (CurrentProfileName.IsEmpty())
        {
            AppendLine(TEXT("[Save] No profile loaded. Load or Create a profile first."));
            return FReply::Handled();
        }
        
        FString FilePath = GetProfilesDirectory() / CurrentProfileName + TEXT(".json");
        if (FFileHelper::SaveStringToFile(CurrentProfileJson, *FilePath))
        {
            AppendLine(FString::Printf(TEXT("[Save] Saved: %s"), *CurrentProfileName));
        }
        else
        {
            AppendLine(FString::Printf(TEXT("[Save] ERROR: Failed to save %s"), *CurrentProfileName));
        }
        
        return FReply::Handled();
    }

    FReply OnValidate()
    {
        if (CurrentProfileJson.IsEmpty())
        {
            AppendLine(TEXT("[Validate] No JSON loaded. Load a profile first."));
            return FReply::Handled();
        }
        
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CurrentProfileJson);
        
        if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
        {
            AppendLine(TEXT("[Validate] ERROR: Invalid JSON syntax"));
            return FReply::Handled();
        }
        
        // Check required fields
        TArray<FString> Errors;
        if (!JsonObject->HasField(TEXT("name")))
        {
            Errors.Add(TEXT("Missing required field: 'name'"));
        }
        if (!JsonObject->HasField(TEXT("states")))
        {
            Errors.Add(TEXT("Missing required field: 'states'"));
        }
        
        if (Errors.Num() > 0)
        {
            AppendLine(TEXT("[Validate] FAILED:"));
            for (const FString& Error : Errors)
            {
                AppendLine(FString::Printf(TEXT("  • %s"), *Error));
            }
        }
        else
        {
            AppendLine(TEXT("[Validate] ✓ Valid AI profile"));
        }
        
        return FReply::Handled();
    }

    FReply OnFormat()
    {
        if (CurrentProfileJson.IsEmpty())
        {
            AppendLine(TEXT("[Format] No JSON loaded. Load a profile first."));
            return FReply::Handled();
        }
        
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CurrentProfileJson);
        
        if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
        {
            AppendLine(TEXT("[Format] ERROR: Invalid JSON - cannot format"));
            return FReply::Handled();
        }
        
        FString FormattedJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&FormattedJson);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        
        CurrentProfileJson = FormattedJson;
        AppendLine(TEXT("[Format] ✓ JSON formatted"));
        
        return FReply::Handled();
    }

    FReply OnGenerateEditorEuw()
    {
        AppendLine(TEXT("[Generate EUW] Requesting P_MWCS to generate/repair EAIS Editor widget..."));
        
        // Call MWCS service to generate the EAIS Tool EUW via modular External Tool EUW API
        FMWCS_Report Report = FMWCS_Service::Get().GenerateOrRepairExternalToolEuw(TEXT("EAIS"));
        
        // Log the results
        AppendLine(FString::Printf(TEXT("[Generate EUW] Specs=%d Created=%d Repaired=%d Errors=%d Warnings=%d"),
            Report.SpecsProcessed,
            Report.AssetsCreated,
            Report.AssetsRepaired,
            Report.NumErrors(),
            Report.NumWarnings()));
        
        // Log any issues
        for (const FMWCS_Issue& Issue : Report.Issues)
        {
            FString Severity = (Issue.Severity == EMWCS_IssueSeverity::Error) ? TEXT("ERROR") : TEXT("WARNING");
            AppendLine(FString::Printf(TEXT("  [%s] %s: %s (%s)"), *Severity, *Issue.Code, *Issue.Message, *Issue.Context));
        }
        
        if (Report.NumErrors() == 0)
        {
            AppendLine(TEXT("[Generate EUW] ✓ SUCCESS: Editor Utility Widget created at /Game/Editor/EAIS/EUW_EAIS_AIEditor"));
        }
        else
        {
            AppendLine(TEXT("[Generate EUW] ✗ FAILED: See errors above"));
        }
        
        return FReply::Handled();
    }
};

static TSharedRef<SDockTab> SpawnEAISTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SEAIS_ToolPanel)
        ];
}

void FEAIS_ToolTab::Register()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateStatic(&SpawnEAISTab)
    )
    .SetDisplayName(FText::FromString(TEXT("EAIS AI Editor")))
    .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FEAIS_ToolTab::Unregister()
{
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

void FEAIS_ToolTab::Open()
{
    FGlobalTabmanager::Get()->TryInvokeTab(TabName);
}
