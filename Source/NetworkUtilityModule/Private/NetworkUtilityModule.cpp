// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "NetworkUtilityModule.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

#define LOCTEXT_NAMESPACE "FNetworkUtilityModule"

void FNetworkUtilityModule::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("NetworkUtilityModule: StartupModule called"));
}

void FNetworkUtilityModule::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("NetworkUtilityModule: ShutdownModule called"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNetworkUtilityModule, NetworkUtilityModule);


