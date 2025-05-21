/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Hardware_FrameworkBPLibrary.generated.h"

UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API USystemInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "Hardware Info", meta = (DisplayName = "Get Memory Info"))
    static void GetMemoryInfo(int32& TotalPhysicalMB, int32& UsedPhysicalMB, int32& FreePhysicalMB,
        int32& TotalVirtualMB, int32& UsedVirtualMB, int32& FreeVirtualMB);


    UFUNCTION(BlueprintPure, Category = "Hardware Info", meta = (DisplayName = "Get CPU Info"))
    static void GetCPUInfo(FString& Name, FString& Manufacturer, int32& Cores, int32& Threads);


    UFUNCTION(BlueprintPure, Category = "Hardware Info", meta = (DisplayName = "Get GPU Info"))
    static void GetGPUInfo(FString& Name, FString& Manufacturer, int32& TotalVRAMMB,
        int32& UsedVRAMMB, int32& FreeVRAMMB);

    UFUNCTION(BlueprintPure, Category = "Hardware Info", meta = (DisplayName = "Get Input Devices"))
    static void GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard);
};