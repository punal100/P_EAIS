/*
 * @Author: Punal Manalan
 * @Description: P_EAISTools Module Implementation
 * @Date: 29/12/2025
 */

#include "PEAISTools.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

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
	// Register menu extensions for Edutor access
	// Window -> Developer Tools -> EAIS Edutor
	
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
					LOCTEXT("EAISEdutorLabel", "EAIS Edutor"),
					LOCTEXT("EAISEdutorTooltip", "Open the EAIS AI Editor Tool"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						// TODO: Open Edutor widget
						UE_LOG(LogEAISTools, Log, TEXT("Opening EAIS Edutor..."));
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

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPEAISToolsModule, P_EAISTools)
