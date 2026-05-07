// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
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

    /** Total dedicated video memory in megabytes. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    int64 DedicatedVideoMemoryMB = 0;

    /** Total shared system memory available to this adapter in megabytes. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    int64 SharedSystemMemoryMB = 0;

    /** True if Unreal Engine is actively rendering with this adapter. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    bool bIsActiveRHI = false;
};

/** Runtime usage metrics and status for one GPU adapter. */
USTRUCT(BlueprintType)
struct FGPUAdapterRuntimeInfo
{
    GENERATED_BODY()

    /** True when the runtime query completed successfully. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    bool bSuccess = false;

    /** Readable error when bSuccess is false. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    FString ErrorMessage;

    /** Static adapter information such as name, vendor, and totals. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    FGPUAdapterInfo AdapterInfo;

    /** Dedicated VRAM currently used by the system for this adapter, in megabytes. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    int64 UsedDedicatedVRAMMB = 0;

    /** Shared GPU memory currently used by the system for this adapter, in megabytes. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    int64 UsedSharedVRAMMB = 0;

    /** VRAM committed by the current Unreal process when this is the active RHI adapter, in megabytes. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    int64 GameVRAMUsageMB = 0;

    /** Current adapter utilization from Windows performance counters, from 0 to 100. */
    UPROPERTY(BlueprintReadOnly, Category = "GPU")
    float UsagePercent = 0.0f;
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

    /** True when the command process started successfully. */
    UPROPERTY(BlueprintReadOnly, Category = "Command")
    bool bStarted = false;

    /** True when the command finished before timeout or cancellation. */
    UPROPERTY(BlueprintReadOnly, Category = "Command")
    bool bCompleted = false;

    /** True when the command exceeded TimeoutSeconds and was terminated. */
    UPROPERTY(BlueprintReadOnly, Category = "Command")
    bool bTimedOut = false;

    /** Process exit code when it is available. */
    UPROPERTY(BlueprintReadOnly, Category = "Command")
    int32 ExitCode = -1;

    /** Captured standard output for non-admin commands. */
    UPROPERTY(BlueprintReadOnly, Category = "Command")
    FString StdOut;

    /** Captured standard error or diagnostic details. */
    UPROPERTY(BlueprintReadOnly, Category = "Command")
    FString StdErr;

    /** Readable failure reason when the command cannot start or complete. */
    UPROPERTY(BlueprintReadOnly, Category = "Command")
    FString ErrorMessage;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnWNTCommandResult, FWNTCommandResult, Result);

/** Native Windows hardware, command, and process helper nodes for Blueprints. */
UCLASS()
class DEVICEFRAMEWORKMODULE_API USystemInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Retrieves memory information in megabytes for physical memory and committed virtual memory. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get Memory Info (MB)"))
    static void GetMemoryInfo(int64& TotalPhysicalMB, int64& UsedPhysicalMB, int64& FreePhysicalMB, int64& TotalVirtualMB, int64& UsedVirtualMB, int64& FreeVirtualMB);

    /** Retrieves CPU brand, vendor, physical core count, and logical thread count. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get CPU Info"))
    static void GetCPUInfo(FString& DeviceName, ECPUVendor& Vendor, int32& PhysicalCores, int32& LogicalThreads);

    /** Returns current system-wide CPU utilization from 0 to 100. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get CPU Usage Percent"))
    static float GetCPUUsagePercent();

    /** Returns all hardware GPU adapters installed in the system. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|GPU", meta = (DisplayName = "Get All GPU Adapters"))
    static TArray<FGPUAdapterInfo> GetAllGPUAdapters();

    /** Returns one complete runtime GPU snapshot for a specific adapter. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|GPU", meta = (DisplayName = "Get GPU Runtime Info"))
    static FGPUAdapterRuntimeInfo GetGPUAdapterRuntimeInfo(int32 Adapter);

    /** Retrieves the GPU adapter name for a specific adapter. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|GPU", meta = (DisplayName = "Get GPU Name"))
    static FString GetGPUName(int32 Adapter);

    /** Retrieves the GPU vendor for a specific adapter. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|GPU", meta = (DisplayName = "Get GPU Manufacturer"))
    static EGPUVendor GetGPUManufacturer(int32 Adapter);

    /** Returns total dedicated GPU memory for an adapter in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|GPU", meta = (DisplayName = "Get Total Dedicated VRAM"))
    static int64 GetTotalDedicatedVRAM(int32 Context);

    /** Returns dedicated GPU memory usage for an adapter in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|GPU", meta = (DisplayName = "Get Used Dedicated VRAM"))
    static int64 GetUsedDedicatedVRAM(int32 Context);

    /** Returns total shared GPU memory for an adapter in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|GPU", meta = (DisplayName = "Get Total Shared VRAM"))
    static int64 GetTotalVirtualVRAM(int32 Context);

    /** Returns shared GPU memory usage for an adapter in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|GPU", meta = (DisplayName = "Get Used Shared VRAM"))
    static int64 GetUsedVirtualVRAM(int32 Context);

    /** Returns VRAM committed by the current Unreal process in megabytes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|GPU", meta = (DisplayName = "Get Game VRAM Usage"))
    static int64 GetGameVRAMUsage();

    /** Returns current GPU utilization for a specific adapter from 0 to 100. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|GPU", meta = (DisplayName = "Get GPU Usage Percent"))
    static float GetGPUUsagePercent(int32 Adapter);

    /** Checks for physically connected gamepad, mouse, and keyboard devices. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Check Connected Input Devices"))
    static void GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard);

    /** Returns the active Unreal rendering hardware interface. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations", meta = (DisplayName = "Get Active RHI"))
    static EGraphicsRHI GetRHIName();

    /** Returns true when relaunching the current executable is safe in this runtime context. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Can Relaunch Game"))
    static bool CanRestartGame();

    /** Relaunches the packaged game executable with optional command-line arguments. Refuses to run in editor. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Relaunch Game"))
    static void RestartGameWithCommandLine(const FString& ExtraCommandLine);

    /** Executes a cmd.exe command. Hidden mode avoids opening a command window for normal non-admin commands. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Run Command Prompt Command"))
    static bool ExecuteWindowsCMD(const FString& Command, bool bRunAsAdmin, bool bHidden);

    /** Executes a PowerShell command. Admin mode uses UAC and cannot be fully silent. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Run PowerShell Command"))
    static bool ExecutePowerShell(const FString& Command, bool bRunAsAdmin, bool bHidden);

    /** Runs cmd.exe or PowerShell asynchronously and returns exit code plus captured output when available. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (AutoCreateRefTerm = "OnResult", DisplayName = "Run Command Async"))
    static void RunCommandAsync(const FString& Command, FWNTCommandOptions Options, FOnWNTCommandResult OnResult);

    /** Returns true when hard process termination is allowed in this runtime context. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Can Force Kill Game"))
    static bool CanForceKillGame();

    /** Immediately terminates the packaged game process. Refuses to run in editor to protect unsaved work. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management", meta = (DisplayName = "Force Kill Game Process"))
    static void ForceKillGame();
};

