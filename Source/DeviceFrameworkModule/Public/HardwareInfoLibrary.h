// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "HardwareInfoLibrary.generated.h"

UENUM(BlueprintType)
enum class EGraphicsRHI : uint8
{
    Unknown     UMETA(DisplayName = "Unknown"),
    DirectX11   UMETA(DisplayName = "DirectX 11"),
    DirectX12   UMETA(DisplayName = "DirectX 12"),
    Vulkan      UMETA(DisplayName = "Vulkan"),
    Metal       UMETA(DisplayName = "Metal"),
    OpenGL      UMETA(DisplayName = "OpenGL")
};

UENUM(BlueprintType)
enum class EGPUVendor : uint8
{
    Unknown     UMETA(DisplayName = "Unknown"),
    Nvidia      UMETA(DisplayName = "NVIDIA"),
    AMD         UMETA(DisplayName = "AMD"),
    Intel       UMETA(DisplayName = "Intel"),
    Qualcomm    UMETA(DisplayName = "Qualcomm")
};

UENUM(BlueprintType)
enum class ECPUVendor : uint8
{
    Unknown     UMETA(DisplayName = "Unknown"),
    Intel       UMETA(DisplayName = "Intel"),
    AMD         UMETA(DisplayName = "AMD"),
    Apple       UMETA(DisplayName = "Apple (Silicon)"),
    Qualcomm    UMETA(DisplayName = "Qualcomm (ARM)"),
    Generic     UMETA(DisplayName = "Generic")
};

UCLASS()
class DEVICEFRAMEWORKMODULE_API USystemInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    // --- Functions Related To Hardware Inforamtions

    // Retrieves memory information in megabytes for both physical memory & virtual memory (committed).
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get Memory Values In Meagbyte"))
    static void GetMemoryInfo(int64& TotalPhysicalMB, int64& UsedPhysicalMB, int64& FreePhysicalMB, int64& TotalVirtualMB, int64& UsedVirtualMB, int64& FreeVirtualMB);

    // Retrieves CPU details: name, manufacturer, core count, and thread count
    //This can work in platforms other than windows but recommanded to use only in windows since the plugin support
    //Only windwos paltform.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get CPU Informations"))
    static void GetCPUInfo(FString& DeviceName, ECPUVendor& Vendor, int32& PhysicalCores, int32& LogicalThreads);

    // Get VGA Name & Manufacturer Name If supported by the function.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get GPU Name & Manufacturer"))
    static void GetGPUNameAndManufacturer(FString& DeviceName, EGPUVendor& Manufacturer);

    // Return the total dedicated video memory of the user video graphics adapter.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get Total VRAM In MB"))
    static int32 GetTotalVRAMMB();

    // Return The Overall Video memory amount used by the whole system including the game in MB.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get Overall VRAM Usage In MB"))
    static int32 GetUsedVRAMMB();

    // Return the video memory amount used by the game process in MB.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get Game VRAM Usage In MB"))
    static int32 GetGameVRAMUsageMB();

    //Performs an accurate check for physically connected input devices on Windows.
    //Note that it has some limitation such as keyboard detection will always return true.
    //since windows has virtual keybaords.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Check Connected Input Devices"))
    static void GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard);
    
    // Returns The Active Hardware Rendering Interface's Name (e.g., "D3D11", "D3D12", "Vulkan")
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get Active RHI"))
    static EGraphicsRHI GetRHIName();



    // --- Function Related To Process Management

    //Will Restart Game - Don't ever let the ExtraCommandLine empty. just put -Restart if you have to extrancommandline.
    //Note : This will crash the editor so please run it only in packaged relase & not in any kind of editor run.
    //I did not add an #if WITH_EDITOR just so your editor crash & you learn to read descreption.
    //Do not use it in editor isnt that clear ?!!!!
    //You know what i want to say...right?..Editor=No
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Relaunch The Game"))
    static void RestartGameWithCommandLine(const FString& ExtraCommandLine);

    //This is used to execute a CMD command, you can even execute commands that require admin but
    //Player must allow you to do so. trying to bypass the UAC will result in game flagged as virus.
    //If UAC is set to never notify then bingo, you can play around & execute whatever you like without notifying user.
    //In most cases the Hidden bool do nothing since in most windwos builds the window of CMD will be hidden by default.
    //Do not harm players devices with bad commands, be civile don't be an asshole.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Execute Windows Command"))
    static bool ExecuteWindowsCMD(const FString& Command, bool bRunAsAdmin, bool bHidden);

    //Like the size said this function will force kill your beatiful game (i always spell the last word wrong smh).
    //NOTE : Do not use this in editor because it will crash the editor or at least force kill it. (FAFO).
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Force Kill Game Process"))
    static void ForceKillGame();


    // --- Deprecated Function List

    //Retrieves GPU details: name, manufacturer, VRAM stats, and current game VRAM usage in megabytes (DeprecatedFunction)
    //Warning : Never use this function it is an DeprecatedFunction & you should insted use the modern ones.
    //There is 0 reason to use this, it has bad accuracy & may lag your game, why using it when better options exist ? huh?
    //Remove it then..
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DeprecatedFunction, DeprecationMessage = "This function was splited into 3 different Functions, never use it"))
    static void GetGPUInfo(FString& Name, FString& Manufacturer, int32& TotalVRAMMB, int32& UsedVRAMMB, int32& FreeVRAMMB);

};