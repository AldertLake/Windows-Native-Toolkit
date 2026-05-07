// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "SystemUtilityModule.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

#define LOCTEXT_NAMESPACE "FSystemUtilityModule"

void FSystemUtilityModule::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("SystemUtilityModule: StartupModule invoked"));

}

void FSystemUtilityModule::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("SystemUtilityModule: ShutdownModule invoked"));

}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSystemUtilityModule, SystemUtilityModule);


