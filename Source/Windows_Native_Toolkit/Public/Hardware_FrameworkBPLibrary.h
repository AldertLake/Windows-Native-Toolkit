/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Hardware_FrameworkBPLibrary.generated.h"

UCLASS()
class USystemInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Memory Info
    UFUNCTION(BlueprintPure, Category = "Hardware Info")
    static void GetMemoryInfo(int32& TotalPhysicalMB, int32& UsedPhysicalMB, int32& FreePhysicalMB,
        int32& TotalVirtualMB, int32& UsedVirtualMB, int32& FreeVirtualMB);

    // CPU Info
    UFUNCTION(BlueprintPure, Category = "Hardware Info")
    static void GetCPUInfo(FString& Name, FString& Manufacturer, int32& Cores, int32& Threads);

    // GPU Info
    UFUNCTION(BlueprintPure, Category = "Hardware Info")
    static void GetGPUInfo(FString& Name, FString& Manufacturer, int32& TotalVRAMMB,
        int32& UsedVRAMMB, int32& FreeVRAMMB);

    // Input Devices
    UFUNCTION(BlueprintPure, Category = "System Info")
    static void GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard);
};