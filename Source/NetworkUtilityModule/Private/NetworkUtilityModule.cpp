// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "NetworkUtilityModule.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

#define LOCTEXT_NAMESPACE "FNetworkUtilityModule"

void FNetworkUtilityModule::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("NetworkUtilityModule: StartupModule called"));
    // Insert initialization code here (network sockets, checks, etc.)
}

void FNetworkUtilityModule::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("NetworkUtilityModule: ShutdownModule called"));
    // Insert cleanup logic here
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNetworkUtilityModule, NetworkUtilityModule);
