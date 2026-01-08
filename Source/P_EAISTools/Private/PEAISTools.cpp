/*
 * @Author: Punal Manalan
 * @Description: P_EAISTools Module Implementation
 * @Date: 29/12/2025
 * @Updated: 01/01/2026 - Refactored to use EAIS_ToolTab for editor window
 */

#include "PEAISTools.h"
#include "EAIS_ToolTab.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "FPEAISToolsModule"

DEFINE_LOG_CATEGORY(LogEAISTools);

void FPEAISToolsModule::StartupModule()
{
	UE_LOG(LogEAISTools, Log, TEXT("P_EAISTools Module Starting..."));

	// Register the EAIS Tool Tab
	FEAIS_ToolTab::Register();

	RegisterMenuExtensions();

	UE_LOG(LogEAISTools, Log, TEXT("P_EAISTools Module Started."));
}

void FPEAISToolsModule::ShutdownModule()
{
	UE_LOG(LogEAISTools, Log, TEXT("P_EAISTools Module Shutting Down..."));

	UnregisterMenuExtensions();

	// Unregister the EAIS Tool Tab
	FEAIS_ToolTab::Unregister();

	UE_LOG(LogEAISTools, Log, TEXT("P_EAISTools Module Shut Down."));
}

void FPEAISToolsModule::RegisterMenuExtensions()
{
	// Register menu extensions for AI Editor access
	// Tools -> EAIS -> AI Editor
	
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
	{
		UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		if (ToolsMenu)
		{
			FToolMenuSection& Section = ToolsMenu->FindOrAddSection("EAIS");
			Section.Label = LOCTEXT("EAISSection", "EAIS");
			
			Section.AddMenuEntry(
				"OpenEAISAIEditor",
				LOCTEXT("EAISEditorLabel", "EAIS AI Editor"),
				LOCTEXT("EAISEditorTooltip", "Open the EAIS Visual AI Editor Tool"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
				FUIAction(FExecuteAction::CreateStatic(&FPEAISToolsModule::OpenAIEditor))
			);
		}
	}));
}

void FPEAISToolsModule::UnregisterMenuExtensions()
{
	// Menu extensions are automatically cleaned up
}

void FPEAISToolsModule::OpenAIEditor()
{
	UE_LOG(LogEAISTools, Log, TEXT("Opening EAIS AI Editor..."));

	// Open the EAIS Tool Tab (Slate-based editor window)
	FEAIS_ToolTab::Open();

	UE_LOG(LogEAISTools, Log, TEXT("EAIS AI Editor opened."));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPEAISToolsModule, P_EAISTools)

