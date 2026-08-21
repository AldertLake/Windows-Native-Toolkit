// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "SystemUtilityModule.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogWNT);

void FSystemUtilityModule::StartupModule()
{
    UE_LOG(LogWNT, Log, TEXT("SystemUtilityModule: Module loaded."));
}

void FSystemUtilityModule::ShutdownModule()
{
    UE_LOG(LogWNT, Log, TEXT("SystemUtilityModule: Module unloaded."));
}

IMPLEMENT_MODULE(FSystemUtilityModule, SystemUtilityModule);


