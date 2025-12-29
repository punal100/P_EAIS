/*
 * @Author: Punal Manalan
 * @Description: P_EAIS Module Implementation
 * @Date: 29/12/2025
 */

#include "PEAIS.h"
#include "EAISConsoleCommands.h"

#define LOCTEXT_NAMESPACE "FPEAISModule"

DEFINE_LOG_CATEGORY(LogEAIS);

void FPEAISModule::StartupModule()
{
	UE_LOG(LogEAIS, Log, TEXT("P_EAIS Module Starting..."));

	// Register console commands
	UEAISConsoleCommands::RegisterCommands();

	UE_LOG(LogEAIS, Log, TEXT("P_EAIS Module Started."));
}

void FPEAISModule::ShutdownModule()
{
	UE_LOG(LogEAIS, Log, TEXT("P_EAIS Module Shutting Down..."));

	// Unregister console commands
	UEAISConsoleCommands::UnregisterCommands();

	UE_LOG(LogEAIS, Log, TEXT("P_EAIS Module Shut Down."));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPEAISModule, P_EAIS)
