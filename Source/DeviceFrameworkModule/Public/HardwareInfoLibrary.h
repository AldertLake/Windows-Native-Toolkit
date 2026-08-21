// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HardwareInfoLibrary.generated.h"

/** Rendering hardware interfaces that Unreal may currently use. */
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

/** Hardware GPU vendors recognized by the toolkit. */
UENUM(BlueprintType)
enum class EGPUVendor : uint8
{
    Unknown     UMETA(DisplayName = "Unknown"),
    Nvidia      UMETA(DisplayName = "NVIDIA"),
    AMD         UMETA(DisplayName = "AMD"),
    Intel       UMETA(DisplayName = "Intel"),
    Qualcomm    UMETA(DisplayName = "Qualcomm")
};

/** CPU vendors recognized by the toolkit. */
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

/** GPU usage groups exposed by Windows performance counters. */
UENUM(BlueprintType)
enum class EGPUUsageMode : uint8
{
    Overall             UMETA(DisplayName = "Overall"),
    ThreeDimensional    UMETA(DisplayName = "3D"),
    Copy                UMETA(DisplayName = "Copy"),
    Compute             UMETA(DisplayName = "Compute"),
    VideoDecode         UMETA(DisplayName = "Video Decode"),
    VideoEncode         UMETA(DisplayName = "Video Encode"),
    VideoProcessing     UMETA(DisplayName = "Video Processing"),
    Graphics            UMETA(DisplayName = "Graphics"),
    Overlay             UMETA(DisplayName = "Overlay"),
    Cryptographic       UMETA(DisplayName = "Cryptographic"),
    Other               UMETA(DisplayName = "Other / Unknown")
};

/** Advanced adapter fields that are less frequently needed in Blueprints. */
USTRUCT(BlueprintType)
struct FGPUAdapterAdvancedInfo
{
    GENERATED_BODY()

    /** Driver version reported by Windows for this adapter. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    FString DriverVersion;

    /** Driver date reported by Windows, formatted as YYYY-MM-DD when available. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    FString DriverDate;

    /** Physical device location, for example PCI bus and slot details. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    FString PhysicalLocation;

    /** Dedicated system memory reserved for the adapter in megabytes when Windows reports it. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    int64 HardwareReservedMemoryMB = 0;
};

/** Static information about one GPU adapter installed in the system. */
USTRUCT(BlueprintType)
struct FGPUAdapterInfo
{
    GENERATED_BODY()

    /** Zero-based adapter index used by GPU query functions. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    int32 AdapterIndex = -1;

    /** Friendly GPU adapter name reported by the display driver. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    FString AdapterName;

    /** GPU hardware vendor. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    EGPUVendor Vendor = EGPUVendor::Unknown;

    /** True if Unreal Engine is actively rendering with this adapter. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    bool bIsActiveRHI = false;

    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    FGPUAdapterAdvancedInfo Advanced;
};

/** Aggregated CPU cache sizes in kilobytes. */
USTRUCT(BlueprintType)
struct FCPUCacheInfo
{
    GENERATED_BODY()

    /** Total L1 cache reported by Windows in kilobytes. */
    UPROPERTY(BlueprintReadOnly, Category = "CPU")
    int32 L1CacheKB = 0;

    /** Total L2 cache reported by Windows in kilobytes. */
    UPROPERTY(BlueprintReadOnly, Category = "CPU")
    int32 L2CacheKB = 0;

    /** Total L3 cache reported by Windows in kilobytes. */
    UPROPERTY(BlueprintReadOnly, Category = "CPU")
    int32 L3CacheKB = 0;
};

/** Summary of motherboard memory-slot usage reported by Windows. */
USTRUCT(BlueprintType)
struct FMemorySlotInfo
{
    GENERATED_BODY()

    /** Total memory slots reported by the system firmware. */
    UPROPERTY(BlueprintReadOnly, Category = "Memory")
    int32 TotalSlots = 0;

    /** Number of slots currently populated with a memory module. */
    UPROPERTY(BlueprintReadOnly, Category = "Memory")
    int32 UsedSlots = 0;

    /** Readable form factor such as DIMM or SODIMM when it is available. */
    UPROPERTY(BlueprintReadOnly, Category = "Memory")
    FString FormFactor;
};

/** Chooses which Windows shell should run a command. */
UENUM(BlueprintType)
enum class EWNTCommandShell : uint8
{
    Cmd UMETA(DisplayName = "Command Prompt"),
    PowerShell UMETA(DisplayName = "PowerShell")
};

/** Options that control how a command prompt or PowerShell process starts. */
USTRUCT(BlueprintType)
struct FWNTCommandOptions
{
    GENERATED_BODY()

    /** Selects cmd.exe or powershell.exe. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    EWNTCommandShell Shell = EWNTCommandShell::Cmd;

    /** Requests elevation through UAC. Elevated commands cannot capture output silently. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    bool bRunAsAdmin = false;

    /** Hides the command window when Windows allows it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    bool bHidden = true;

    /** Captures standard output and standard error for non-admin commands. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    bool bCaptureOutput = true;

    /** Maximum run time in seconds for captured async commands. Zero disables timeout. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    float TimeoutSeconds = 0.0f;

    /** Optional working directory for the command process. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    FString WorkingDirectory;
};


/** Result payload returned by asynchronous command execution. */
USTRUCT(BlueprintType)
struct FWNTCommandResult
{
    GENERATED_BODY()

    /** Process exit code when it is available. 0 usually indicates success. */
    UPROPERTY(BlueprintReadOnly, Category = "Command")
    int32 ExitCode = -1;

    /** Captured standard output for non-admin commands. */
    UPROPERTY(BlueprintReadOnly, Category = "Command")
    FString TextOutput;

    /** Captured standard error or diagnostic details. */
    UPROPERTY(BlueprintReadOnly, Category = "Command")
    FString Error;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWNTCommandAsyncCompleted, FWNTCommandResult, Result);

/** Async command-execution node with direct success and fail execution pins. */
UCLASS()
class DEVICEFRAMEWORKMODULE_API UAsyncRunCommandAction : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    /** Called when the command starts and finishes successfully. */
    UPROPERTY(BlueprintAssignable)
    FWNTCommandAsyncCompleted OnSuccess;

    /** Called when the command fails to start, times out, or finishes with a failure result. */
    UPROPERTY(BlueprintAssignable)
    FWNTCommandAsyncCompleted OnFail;

    /** 
     * Runs cmd.exe or PowerShell asynchronously without blocking the game thread. 
     * Note: Output capture is physically impossible when 'Run As Admin' is true, because Windows isolates UAC-elevated processes for security reasons.
     */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Execute CLI Commands"))
    static UAsyncRunCommandAction* RunCommandAsync(const UObject* WorldContextObject, const FString& Command, FWNTCommandOptions Options);

    virtual void Activate() override;

private:
    void Finalize(const FWNTCommandResult& InResult, bool bSuccess);

    FString CommandText;
    FWNTCommandOptions CommandOptions;
    bool bAddedToRootForCompatibility = false;
};

/** Native Windows hardware, command, and process helper nodes for Blueprints. */
UCLASS()
class DEVICEFRAMEWORKMODULE_API USystemInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Retrieves physical memory information in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information", meta = (DisplayName = "Get Physical Memory Info"))
    static void GetPhysicalMemoryInfo(int64& TotalPhysicalMB, int64& UsedPhysicalMB, int64& FreePhysicalMB);

    /** Retrieves committed virtual-memory information in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information", meta = (DisplayName = "Get Virtual Memory Info"))
    static void GetVirtualMemoryInfo(int64& TotalVirtualMB, int64& UsedVirtualMB, int64& FreeVirtualMB);

    /** Returns memory speed, slot usage, and hardware-reserved memory when Windows reports them. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information", meta = (DisplayName = "Get Memory Info"))
    static void GetMemoryInfo(int32& MemorySpeedMHz, FMemorySlotInfo& SlotInfo, int64& HardwareReservedMemoryMB);

    /** Retrieves CPU brand, vendor, physical core count, logical thread count, and cache sizes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information", meta = (DisplayName = "Get CPU Info"))
    static void GetCPUInfo(FString& DeviceName, ECPUVendor& Vendor, int32& PhysicalCores, int32& LogicalThreads, FCPUCacheInfo& CacheInfo);

    /** Returns current system-wide CPU utilization from 0 to 100. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information", meta = (DisplayName = "Get Overall CPU Usage"))
    static float GetOverallCPUUsage();

    /** Returns current utilization for one logical processor from 0 to 100. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information", meta = (DisplayName = "Get CPU Thread Usage"))
    static float GetCPUThreadUsage(int32 Thread);

    /** Returns the current CPU speed in megahertz when Windows reports it. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information", meta = (DisplayName = "Get CPU Current Speed"))
    static int32 GetCPUCurrentSpeedMHz();

    /** Returns true when firmware-assisted CPU virtualization is enabled. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information", meta = (DisplayName = "Is CPU Virtualization Enabled"))
    static bool IsCPUVirtualizationEnabled();

    /** Returns all hardware GPU adapters installed in the system. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|GPU", meta = (DisplayName = "Get All GPU Adapters"))
    static TArray<FGPUAdapterInfo> GetAllGPUAdapters();

    /** Returns adapter information for one specific GPU, including advanced driver details. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|GPU", meta = (DisplayName = "Get GPU Information"))
    static FGPUAdapterInfo GetGPUInformation(int32 Adapter);

    /** Retrieves the GPU adapter name for a specific adapter. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|GPU", meta = (DisplayName = "Get GPU Name"))
    static FString GetGPUName(int32 Adapter);

    /** Retrieves the GPU vendor for a specific adapter. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|GPU", meta = (DisplayName = "Get GPU Manufacturer"))
    static EGPUVendor GetGPUManufacturer(int32 Adapter);

    /** Returns total dedicated GPU memory for an adapter in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|GPU", meta = (DisplayName = "Get Total Dedicated VRAM"))
    static int64 GetTotalDedicatedVRAM(int32 Adapter);

    /** Returns dedicated GPU memory usage for an adapter in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|GPU", meta = (DisplayName = "Get Used Dedicated VRAM"))
    static int64 GetUsedDedicatedVRAM(int32 Adapter);

    /** Returns total shared GPU memory for an adapter in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|GPU", meta = (DisplayName = "Get Total Shared VRAM"))
    static int64 GetTotalVirtualVRAM(int32 Adapter);

    /** Returns shared GPU memory usage for an adapter in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|GPU", meta = (DisplayName = "Get Used Shared VRAM"))
    static int64 GetUsedVirtualVRAM(int32 Adapter);

    /** Returns VRAM committed by the current Unreal process in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|GPU", meta = (DisplayName = "Get Game VRAM Usage"))
    static int64 GetGameVRAMUsage();

    /** Returns current GPU utilization for a specific adapter from 0 to 100. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|GPU", meta = (DisplayName = "Get GPU Usage Percent"))
    static float GetGPUUsagePercent(int32 Adapter, EGPUUsageMode UsageMode = EGPUUsageMode::Overall);

    /** Checks for physically connected gamepad and mouse devices. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information", meta = (DisplayName = "Check Connected Input Devices"))
    static void GetInputDevices(bool& HasGamepad, bool& HasMouse);

    /** Returns the active Unreal rendering hardware interface. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information", meta = (DisplayName = "Get Active RHI"))
    static EGraphicsRHI GetRHIName();

    /** Returns true when relaunching the current executable is safe in this runtime context. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Can Relaunch Game"))
    static bool CanRestartGame();

    /** Relaunches the packaged game executable with optional command-line arguments. Refuses to run in editor. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Relaunch Game"))
    static void RestartGameWithCommandLine(const FString& ExtraCommandLine);

    /** Executes a cmd.exe command. Hidden mode avoids opening a command window for normal non-admin commands. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DeprecatedFunction, DeprecationMessage = "Use Execute CLI Commands instead.", DisplayName = "Run Command Prompt Command"))
    static bool ExecuteWindowsCMD(const FString& Command, bool bRunAsAdmin, bool bHidden);

    /** Executes a PowerShell command. Admin mode uses UAC and cannot be fully silent. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DeprecatedFunction, DeprecationMessage = "Use Execute CLI Commands instead.", DisplayName = "Run PowerShell Command"))
    static bool ExecutePowerShell(const FString& Command, bool bRunAsAdmin, bool bHidden);

    /** Returns true when hard process termination is allowed in this runtime context. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Can Force Kill Game"))
    static bool CanForceKillGame();
    /** Immediately terminates the packaged game process. Refuses to run in editor to protect unsaved work. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Force Kill Game Process"))
    static void ForceKillGame();
};

