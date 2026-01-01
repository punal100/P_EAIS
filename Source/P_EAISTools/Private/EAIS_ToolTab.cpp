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
            
            // Buttons Row 1
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Load Profile")))
                    .OnClicked(this, &SEAIS_ToolPanel::OnLoadProfile)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Save Profile")))
                    .OnClicked(this, &SEAIS_ToolPanel::OnSaveProfile)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Validate")))
                    .OnClicked(this, &SEAIS_ToolPanel::OnValidate)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Format")))
                    .OnClicked(this, &SEAIS_ToolPanel::OnFormat)
                ]
            ]
            
            // Buttons Row 2
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("List Profiles")))
                    .OnClicked(this, &SEAIS_ToolPanel::OnListProfiles)
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
                .Text(FText::FromString(TEXT("EAIS Tool ready.")))
            ]
        ];
    }

private:
    TSharedPtr<SMultiLineEditableTextBox> LogBox;
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
        LogBox->SetText(FText::FromString(Existing));
    }

    FString GetProfilesDirectory() const
    {
        return FPaths::ProjectContentDir() / TEXT("AIProfiles");
    }

    FReply OnListProfiles()
    {
        AppendLine(TEXT("--- Available Profiles ---"));
        
        FString ProfilesDir = GetProfilesDirectory();
        TArray<FString> Files;
        IFileManager::Get().FindFiles(Files, *(ProfilesDir / TEXT("*.json")), true, false);
        
        if (Files.Num() == 0)
        {
            AppendLine(TEXT("No profiles found in Content/AIProfiles/"));
        }
        else
        {
            for (const FString& File : Files)
            {
                AppendLine(FString::Printf(TEXT("  • %s"), *FPaths::GetBaseFilename(File)));
            }
            AppendLine(FString::Printf(TEXT("Total: %d profiles"), Files.Num()));
        }
        
        return FReply::Handled();
    }

    FReply OnLoadProfile()
    {
        AppendLine(TEXT("[Load] Enter profile name in log (feature requires EUW for file dialog)"));
        AppendLine(TEXT("Tip: Use 'List Profiles' to see available profiles"));
        return FReply::Handled();
    }

    FReply OnSaveProfile()
    {
        if (CurrentProfileName.IsEmpty())
        {
            AppendLine(TEXT("[Save] No profile loaded. Load a profile first."));
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
