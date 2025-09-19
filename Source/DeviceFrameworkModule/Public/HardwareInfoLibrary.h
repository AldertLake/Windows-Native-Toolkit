// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "HardwareInfoLibrary.generated.h"

UCLASS()
class USystemInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Retrieves memory information in megabytes
    UFUNCTION(BlueprintPure, Category = "Hardware Info")
    static void GetMemoryInfo(int32& TotalPhysicalMB, int32& UsedPhysicalMB, int32& FreePhysicalMB, int32& TotalVirtualMB, int32& UsedVirtualMB, int32& FreeVirtualMB);

    // Retrieves CPU details: name, manufacturer, core count, and thread count
    UFUNCTION(BlueprintPure, Category = "Hardware Info")
    static void GetCPUInfo(FString& Name, FString& Manufacturer, int32& Cores, int32& Threads);

    // Get VGA Name & Manufacturer Name.
    UFUNCTION(BlueprintPure, Category = "Hardware Info", meta = (DisplayName = "Get GPU Name & Manufacturer"))
    static void GetGPUNameAndManufacturer(FString& Name, FString& Manufacturer);

    //Return the total dedicated video memory of the user video graphics adapter.
    UFUNCTION(BlueprintPure, Category = "Hardware Info", meta = (DisplayName = "Get Total VRAM Amount"))
    static int32 GetTotalVRAMMB();

    //Return The Overall Video memory amound used by the whole systel including the game in MB.
    UFUNCTION(BlueprintPure, Category = "Hardware Info", meta = (DisplayName = "Get Overall VRAM Usage"))
    static int32 GetUsedVRAMMB();

    //Return the video memory amount used by the game process in MB.
    UFUNCTION(BlueprintPure, Category = "Hardware Info", meta = (DisplayName = "Get Game VRAM Usage"))
    static int32 GetGameVRAMUsageMB();

    // Retrieves GPU details: name, manufacturer, VRAM stats, and current game VRAM usage in megabytes (DeprecatedFunction)
    UFUNCTION(BlueprintPure, Category = "Hardware Info", meta = (DeprecatedFunction, DeprecationMessage = "This function was splited into 3 different Functions"))
    static void GetGPUInfo(FString& Name, FString& Manufacturer, int32& TotalVRAMMB, int32& UsedVRAMMB, int32& FreeVRAMMB);

    // Detects presence of input devices
    UFUNCTION(BlueprintPure, Category = "System Info")
    static void GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard);

    // Returns RHI name (e.g., "D3D11", "D3D12", "Vulkan") 
    UFUNCTION(BlueprintPure, Category = "System Info")
    static FString GetRHIName();

    //Restart Game - Don't ever let the ExtraCommandLine empty. just put -Restart if you have to extrancommandline.
    UFUNCTION(BlueprintCallable, Category = "Process Management")
    static void RestartGameWithCommandLine(const FString& ExtraCommandLine);

};