/*
 * @Author: Punal Manalan
 * @Description: P_EAIS Module Interface - Enhanced AI System
 * @Date: 29/12/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Core includes for convenience
#include "EAIS_Types.h"
#include "AIBehaviour.h"
#include "AIInterpreter.h"
#include "AIAction.h"
#include "AIComponent.h"
#include "EAISSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEAIS, Log, All);

class FPEAISModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Get the module instance
	 */
	static FPEAISModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FPEAISModule>("P_EAIS");
	}

	/**
	 * Check if the module is loaded
	 */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("P_EAIS");
	}
};
