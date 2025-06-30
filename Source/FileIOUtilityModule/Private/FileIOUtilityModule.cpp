// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "FileIOUtilityModule.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

#define LOCTEXT_NAMESPACE "FFileIOUtilityModule"

void FFileIOUtilityModule::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("FileIOUtilityModule: StartupModule called"));
    // Place any initialization code here (e.g., registering file handlers)
}

void FFileIOUtilityModule::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("FileIOUtilityModule: ShutdownModule called"));
    // Insert cleanup logic (e.g., unregistering handlers)
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFileIOUtilityModule, FileIOUtilityModule);
