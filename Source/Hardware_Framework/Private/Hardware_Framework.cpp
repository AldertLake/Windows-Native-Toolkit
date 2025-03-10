// Copyright Epic Games, Inc. All Rights Reserved.

#include "Hardware_Framework.h"

#define LOCTEXT_NAMESPACE "FHardware_FrameworkModule"

void FHardware_FrameworkModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
}

void FHardware_FrameworkModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHardware_FrameworkModule, Hardware_Framework)