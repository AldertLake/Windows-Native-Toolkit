// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "DeviceFrameworkModule.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

#define LOCTEXT_NAMESPACE "FDeviceFrameworkModule"

void FDeviceFrameworkModule::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("DeviceFrameworkModule: StartupModule called"));

}

void FDeviceFrameworkModule::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("DeviceFrameworkModule: ShutdownModule called"));

}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDeviceFrameworkModule, DeviceFrameworkModule);


