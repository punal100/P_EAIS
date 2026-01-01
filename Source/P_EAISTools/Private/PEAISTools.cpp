/*
 * @Author: Punal Manalan
 * @Description: P_EAISTools Module Implementation
 * @Date: 29/12/2025
 * @Updated: 01/01/2026 - Fixed Editor typo, implemented OpenEditor functionality
 */

#include "PEAISTools.h"
#include "EAIS_AIEditor.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilitySubsystem.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "FPEAISToolsModule"

DEFINE_LOG_CATEGORY(LogEAISTools);

void FPEAISToolsModule::StartupModule()
{
	UE_LOG(LogEAISTools, Log, TEXT("P_EAISTools Module Starting..."));

	RegisterMenuExtensions();

	UE_LOG(LogEAISTools, Log, TEXT("P_EAISTools Module Started."));
}

void FPEAISToolsModule::ShutdownModule()
{
	UE_LOG(LogEAISTools, Log, TEXT("P_EAISTools Module Shutting Down..."));

	UnregisterMenuExtensions();

	UE_LOG(LogEAISTools, Log, TEXT("P_EAISTools Module Shut Down."));
}

void FPEAISToolsModule::RegisterMenuExtensions()
{
	// Register menu extensions for AI Editor access
	// Window -> Developer Tools -> EAIS AI Editor
	
	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension(
			"LevelEditor",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateLambda([](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("EAISEditorLabel", "EAIS AI Editor"),
					LOCTEXT("EAISEditorTooltip", "Open the EAIS Visual AI Editor Tool"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						OpenAIEditor();
					}))
				);
			})
		);

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
}

void FPEAISToolsModule::UnregisterMenuExtensions()
{
	// Menu extensions are automatically cleaned up
}

void FPEAISToolsModule::OpenAIEditor()
{
	UE_LOG(LogEAISTools, Log, TEXT("Opening EAIS AI Editor..."));

	// Try to find and open the Editor Utility Widget
	const FString EditorWidgetPath = TEXT("/P_EAIS/Editor/EUW_EAIS_AIEditor.EUW_EAIS_AIEditor");
	
	if (UEditorUtilityWidgetBlueprint* WidgetBP = LoadObject<UEditorUtilityWidgetBlueprint>(nullptr, *EditorWidgetPath))
	{
		if (UEditorUtilitySubsystem* Subsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>())
		{
			Subsystem->SpawnAndRegisterTab(WidgetBP);
			UE_LOG(LogEAISTools, Log, TEXT("EAIS AI Editor opened successfully."));
		}
	}
	else
	{
		UE_LOG(LogEAISTools, Warning, TEXT("Could not find AI Editor widget at: %s"), *EditorWidgetPath);
		UE_LOG(LogEAISTools, Warning, TEXT("Run 'DevTools/scripts/generate_Editor.bat' to generate the widget."));
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPEAISToolsModule, P_EAISTools)
