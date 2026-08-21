// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "DeviceFrameworkModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FDeviceFrameworkModule"

DEFINE_LOG_CATEGORY(LogWNT);

void FDeviceFrameworkModule::StartupModule()
{
    UE_LOG(LogWNT, Log, TEXT("DeviceFrameworkModule: StartupModule called"));

}

void FDeviceFrameworkModule::ShutdownModule()
{
    UE_LOG(LogWNT, Log, TEXT("DeviceFrameworkModule: ShutdownModule called"));

}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDeviceFrameworkModule, DeviceFrameworkModule);



