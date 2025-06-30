// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "SystemUtilityModule.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

#define LOCTEXT_NAMESPACE "FSystemUtilityModule"

void FSystemUtilityModule::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("SystemUtilityModule: StartupModule invoked"));
    // Initialization code here (e.g., initialize Windows APIs).
}

void FSystemUtilityModule::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("SystemUtilityModule: ShutdownModule invoked"));
    // Cleanup code here (e.g., release resources).
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSystemUtilityModule, SystemUtilityModule);
