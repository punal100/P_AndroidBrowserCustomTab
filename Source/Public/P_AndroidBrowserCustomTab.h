/*
 * @Author: Punal Manalan
 * @Description: Android Browser Custom Tab Plugin.
 * @Date: 09/10/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Main module class for ViewShed Analysis Plugin
 * Handles module lifecycle (startup/shutdown) and initialization
 */
class FP_AndroidBrowserCustomTabModule : public IModuleInterface
{
public:
	/** Called when module is loaded into memory */
	virtual void StartupModule() override;

	/** Called when module is unloaded from memory */
	virtual void ShutdownModule() override;
};
