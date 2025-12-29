/*
 * @Author: Punal Manalan
 * @Description: P_EAISTools Module Interface - Editor tools for EAIS
 * @Date: 29/12/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEAISTools, Log, All);

class FPEAISToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FPEAISToolsModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FPEAISToolsModule>("P_EAISTools");
	}

	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("P_EAISTools");
	}

private:
	void RegisterMenuExtensions();
	void UnregisterMenuExtensions();
};
