// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "HardwareInfoLibrary.h"
#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"
#include "RHI.h"
#include "HAL/PlatformMisc.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsSystemIncludes.h"
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <Xinput.h>
#include <shellapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include "Windows/HideWindowsPlatformTypes.h"

using Microsoft::WRL::ComPtr;





namespace WNT_Private
{

    struct FAdapterEntry
    {
        ComPtr<IDXGIAdapter3> Adapter;
        DXGI_ADAPTER_DESC1 Desc;
        int32 Index;
        bool bIsActiveRHI;
    };

    static FCriticalSection CacheMutex;
    static TArray<FAdapterEntry> CachedAdapters;
    static int32 ActiveRHIIndex = 0;
    static bool bEnumerated = false;

    static EGPUVendor VendorIdToEnum(uint32 Id)
    {
        switch (Id)
        {
        case 0x10DE: return EGPUVendor::Nvidia;
        case 0x1002: return EGPUVendor::AMD;
        case 0x8086: return EGPUVendor::Intel;
        case 0x5143: return EGPUVendor::Qualcomm;
        default:     return EGPUVendor::Unknown;
        }
    }

    static void EnumerateAdapters()
    {
        if (bEnumerated) return;
        bEnumerated = true;

        ComPtr<IDXGIFactory4> Factory;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&Factory)))) return;

        ComPtr<IDXGIAdapter1> Adapter1;
        for (UINT i = 0; Factory->EnumAdapters1(i, &Adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 Desc;
            if (FAILED(Adapter1->GetDesc1(&Desc))) { Adapter1.Reset(); continue; }
            if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { Adapter1.Reset(); continue; }

            FAdapterEntry Entry;
            Entry.Desc = Desc;
            Entry.Index = CachedAdapters.Num();
            Entry.bIsActiveRHI = (GRHIVendorId != 0 && Desc.VendorId == GRHIVendorId);

            if (Entry.bIsActiveRHI) ActiveRHIIndex = Entry.Index;
            Adapter1.As(&Entry.Adapter);
            CachedAdapters.Add(MoveTemp(Entry));
            Adapter1.Reset();
        }


        if (CachedAdapters.Num() > 0 && ActiveRHIIndex >= CachedAdapters.Num())
            ActiveRHIIndex = 0;
    }

    static FAdapterEntry* GetAdapter(int32 Index)
    {
        FScopeLock Lock(&CacheMutex);
        EnumerateAdapters();
        return CachedAdapters.IsValidIndex(Index) ? &CachedAdapters[Index] : nullptr;
    }

    static FAdapterEntry* GetRHIAdapter()
    {
        FScopeLock Lock(&CacheMutex);
        EnumerateAdapters();
        return CachedAdapters.IsValidIndex(ActiveRHIIndex) ? &CachedAdapters[ActiveRHIIndex] : nullptr;
    }


    static FCriticalSection PerfMutex;
    static PDH_HQUERY PerfQuery = nullptr;
    static PDH_HCOUNTER DedicatedCounter = nullptr;
    static PDH_HCOUNTER SharedCounter = nullptr;
    static PDH_HCOUNTER GPUEngineCounter = nullptr;
    static PDH_HCOUNTER CPUCounter = nullptr;
    static bool bPerfInitialized = false;

    static bool InitPerfCounters()
    {
        if (bPerfInitialized) return PerfQuery != nullptr;
        bPerfInitialized = true;

        if (::PdhOpenQuery(nullptr, 0, &PerfQuery) != ERROR_SUCCESS)
        {
            PerfQuery = nullptr;
            return false;
        }


        ::PdhAddEnglishCounter(PerfQuery, TEXT("\\GPU Adapter Memory(*)\\Dedicated Usage"), 0, &DedicatedCounter);
        ::PdhAddEnglishCounter(PerfQuery, TEXT("\\GPU Adapter Memory(*)\\Shared Usage"), 0, &SharedCounter);
        ::PdhAddEnglishCounter(PerfQuery, TEXT("\\GPU Engine(*)\\Utilization Percentage"), 0, &GPUEngineCounter);
        ::PdhAddEnglishCounter(PerfQuery, TEXT("\\Processor(_Total)\\% Processor Time"), 0, &CPUCounter);


        ::PdhCollectQueryData(PerfQuery);
        return true;
    }

    static void CollectPerfData()
    {
        if (PerfQuery) ::PdhCollectQueryData(PerfQuery);
    }

    static FString FormatLUID(const LUID& Luid)
    {
        return FString::Printf(TEXT("luid_0x%08x_0x%08x"),
            static_cast<uint32>(Luid.HighPart), Luid.LowPart);
    }


    static int64 QueryCounterForLUID(PDH_HCOUNTER Counter, const FString& LuidStr)
    {
        if (!Counter) return 0;

        DWORD BufSize = 0, Count = 0;
        ::PdhGetFormattedCounterArray(Counter, PDH_FMT_LARGE, &BufSize, &Count, nullptr);
        if (BufSize == 0) return 0;

        TArray<uint8> Buf;
        Buf.SetNumUninitialized(BufSize);
        auto* Items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(Buf.GetData());

        if (::PdhGetFormattedCounterArray(Counter, PDH_FMT_LARGE, &BufSize, &Count, Items) != ERROR_SUCCESS)
            return 0;

        int64 Total = 0;
        for (DWORD i = 0; i < Count; ++i)
        {
            FString Name(Items[i].szName);
            if (Name.Contains(LuidStr))
                Total += Items[i].FmtValue.largeValue;
        }
        return Total;
    }


    static float QueryGPUUtil(const FString& LuidStr)
    {
        if (!GPUEngineCounter) return 0.0f;

        DWORD BufSize = 0, Count = 0;
        ::PdhGetFormattedCounterArray(GPUEngineCounter, PDH_FMT_DOUBLE, &BufSize, &Count, nullptr);
        if (BufSize == 0) return 0.0f;

        TArray<uint8> Buf;
        Buf.SetNumUninitialized(BufSize);
        auto* Items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(Buf.GetData());

        if (::PdhGetFormattedCounterArray(GPUEngineCounter, PDH_FMT_DOUBLE, &BufSize, &Count, Items) != ERROR_SUCCESS)
            return 0.0f;


        TMap<FString, double> EngineUtils;
        for (DWORD i = 0; i < Count; ++i)
        {
            FString Name(Items[i].szName);
            if (!Name.Contains(LuidStr)) continue;


            int32 EngIdx = INDEX_NONE;
            Name.FindLastChar(TEXT('_'), EngIdx);
            FString EngType = (EngIdx != INDEX_NONE) ? Name.Mid(Name.Find(TEXT("engtype_"))) : TEXT("unknown");

            double& Sum = EngineUtils.FindOrAdd(EngType);
            Sum += Items[i].FmtValue.doubleValue;
        }

        double MaxUtil = 0.0;
        for (auto& Pair : EngineUtils)
        {
            MaxUtil = FMath::Max(MaxUtil, Pair.Value);
        }
        return static_cast<float>(FMath::Clamp(MaxUtil, 0.0, 100.0));
    }
}

#endif

namespace WNT_Command
{
    static FString GetShellExecutable(EWNTCommandShell Shell)
    {
#if PLATFORM_WINDOWS
        return Shell == EWNTCommandShell::PowerShell ? TEXT("powershell.exe") : TEXT("cmd.exe");
#else
        return FString();
#endif
    }

    static FString EscapeForDoubleQuotes(const FString& Value)
    {
        FString Escaped = Value;
        Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
        return Escaped;
    }

    static FString BuildShellParameters(EWNTCommandShell Shell, const FString& Command)
    {
        if (Shell == EWNTCommandShell::PowerShell)
        {
            return FString::Printf(TEXT("-NoProfile -ExecutionPolicy Bypass -Command \"%s\""), *EscapeForDoubleQuotes(Command));
        }
        return FString::Printf(TEXT("/S /C \"%s\""), *Command);
    }

    static bool StartElevated(const FString& Executable, const FString& Parameters, bool bHidden, const FString& WorkingDirectory, FWNTCommandResult* OutResult)
    {
#if PLATFORM_WINDOWS
        SHELLEXECUTEINFO ShellInfo = {};
        ShellInfo.cbSize = sizeof(SHELLEXECUTEINFO);
        ShellInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        ShellInfo.lpVerb = TEXT("runas");
        ShellInfo.lpFile = *Executable;
        ShellInfo.lpParameters = *Parameters;
        ShellInfo.lpDirectory = WorkingDirectory.IsEmpty() ? nullptr : *WorkingDirectory;
        ShellInfo.nShow = bHidden ? SW_HIDE : SW_SHOW;

        const BOOL bStarted = ShellExecuteEx(&ShellInfo);
        if (!bStarted)
        {
            if (OutResult)
            {
                OutResult->ErrorMessage = FString::Printf(TEXT("Failed to start elevated command. Windows error: %lu"), GetLastError());
            }
            return false;
        }

        if (OutResult)
        {
            OutResult->bStarted = true;
            if (ShellInfo.hProcess)
            {
                WaitForSingleObject(ShellInfo.hProcess, INFINITE);
                DWORD ExitCode = 0;
                if (GetExitCodeProcess(ShellInfo.hProcess, &ExitCode))
                {
                    OutResult->ExitCode = static_cast<int32>(ExitCode);
                }
                OutResult->bCompleted = true;
                CloseHandle(ShellInfo.hProcess);
            }
        }
        else if (ShellInfo.hProcess)
        {
            CloseHandle(ShellInfo.hProcess);
        }

        return true;
#else
        if (OutResult)
        {
            OutResult->ErrorMessage = TEXT("Elevated commands are only available on Windows.");
        }
        return false;
#endif
    }

    static FWNTCommandResult RunCaptured(const FString& Executable, const FString& Parameters, const FWNTCommandOptions& Options)
    {
        FWNTCommandResult Result;

        if (Executable.IsEmpty() || Parameters.IsEmpty())
        {
            Result.ErrorMessage = TEXT("Command is empty or unsupported on this platform.");
            return Result;
        }

        void* ReadPipe = nullptr;
        void* WritePipe = nullptr;
        void* StdErrReadPipe = nullptr;
        void* StdErrWritePipe = nullptr;

        const bool bStdOutPipe = FPlatformProcess::CreatePipe(ReadPipe, WritePipe);
        const bool bStdErrPipe = FPlatformProcess::CreatePipe(StdErrReadPipe, StdErrWritePipe);
        if (!bStdOutPipe || !bStdErrPipe)
        {
            if (ReadPipe || WritePipe)
            {
                FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
            }
            if (StdErrReadPipe || StdErrWritePipe)
            {
                FPlatformProcess::ClosePipe(StdErrReadPipe, StdErrWritePipe);
            }
            Result.ErrorMessage = TEXT("Failed to create output pipes.");
            return Result;
        }

        uint32 ProcessId = 0;
        FProcHandle ProcHandle = FPlatformProcess::CreateProc(
            *Executable,
            *Parameters,
            false,
            Options.bHidden,
            Options.bHidden,
            &ProcessId,
            0,
            Options.WorkingDirectory.IsEmpty() ? nullptr : *Options.WorkingDirectory,
            WritePipe,
            nullptr,
            StdErrWritePipe);

        if (!ProcHandle.IsValid())
        {
            FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
            FPlatformProcess::ClosePipe(StdErrReadPipe, StdErrWritePipe);
            Result.ErrorMessage = TEXT("Failed to start command process.");
            return Result;
        }

        Result.bStarted = true;
        const double StartTime = FPlatformTime::Seconds();
        while (FPlatformProcess::IsProcRunning(ProcHandle))
        {
            Result.StdOut += FPlatformProcess::ReadPipe(ReadPipe);
            Result.StdErr += FPlatformProcess::ReadPipe(StdErrReadPipe);

            if (Options.TimeoutSeconds > 0.0f && (FPlatformTime::Seconds() - StartTime) >= Options.TimeoutSeconds)
            {
                Result.bTimedOut = true;
                FPlatformProcess::TerminateProc(ProcHandle, true);
                break;
            }

            FPlatformProcess::Sleep(0.02f);
        }

        Result.StdOut += FPlatformProcess::ReadPipe(ReadPipe);
        Result.StdErr += FPlatformProcess::ReadPipe(StdErrReadPipe);

        if (!Result.bTimedOut)
        {
            int32 ReturnCode = -1;
            if (FPlatformProcess::GetProcReturnCode(ProcHandle, &ReturnCode))
            {
                Result.ExitCode = ReturnCode;
            }
            Result.bCompleted = true;
        }
        else
        {
            Result.ErrorMessage = TEXT("Command timed out and was terminated.");
        }

        FPlatformProcess::CloseProc(ProcHandle);
        FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
        FPlatformProcess::ClosePipe(StdErrReadPipe, StdErrWritePipe);
        return Result;
    }

    static FWNTCommandResult RunCommand(const FString& Command, FWNTCommandOptions Options)
    {
        FWNTCommandResult Result;
        const FString TrimmedCommand = Command.TrimStartAndEnd();
        if (TrimmedCommand.IsEmpty())
        {
            Result.ErrorMessage = TEXT("Command is empty.");
            return Result;
        }

        const FString Executable = GetShellExecutable(Options.Shell);
        const FString Parameters = BuildShellParameters(Options.Shell, TrimmedCommand);
        if (Executable.IsEmpty())
        {
            Result.ErrorMessage = TEXT("Command execution is only available on Windows.");
            return Result;
        }

        if (Options.bRunAsAdmin)
        {
            if (Options.bCaptureOutput)
            {
                Result.StdErr = TEXT("Output capture is not available for elevated commands launched through UAC.");
            }
            StartElevated(Executable, Parameters, Options.bHidden, Options.WorkingDirectory, &Result);
            return Result;
        }

        if (Options.bCaptureOutput)
        {
            return RunCaptured(Executable, Parameters, Options);
        }

        uint32 ProcessId = 0;
        FProcHandle Handle = FPlatformProcess::CreateProc(
            *Executable,
            *Parameters,
            true,
            Options.bHidden,
            Options.bHidden,
            &ProcessId,
            0,
            Options.WorkingDirectory.IsEmpty() ? nullptr : *Options.WorkingDirectory,
            nullptr);

        Result.bStarted = Handle.IsValid();
        Result.bCompleted = Result.bStarted;
        Result.ExitCode = Result.bStarted ? 0 : -1;
        if (Handle.IsValid())
        {
            FPlatformProcess::CloseProc(Handle);
        }
        else
        {
            Result.ErrorMessage = TEXT("Failed to start command process.");
        }

        return Result;
    }
}

void USystemInfoBPLibrary::GetMemoryInfo(
    int64& TotalPhysicalMB, int64& UsedPhysicalMB, int64& FreePhysicalMB,
    int64& TotalVirtualMB, int64& UsedVirtualMB, int64& FreeVirtualMB)
{
#if PLATFORM_WINDOWS
    MEMORYSTATUSEX MemInfo;
    MemInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (::GlobalMemoryStatusEx(&MemInfo))
    {
        TotalPhysicalMB = static_cast<int64>(MemInfo.ullTotalPhys >> 20);
        FreePhysicalMB  = static_cast<int64>(MemInfo.ullAvailPhys >> 20);
        UsedPhysicalMB  = TotalPhysicalMB - FreePhysicalMB;
        TotalVirtualMB  = static_cast<int64>(MemInfo.ullTotalPageFile >> 20);
        FreeVirtualMB   = static_cast<int64>(MemInfo.ullAvailPageFile >> 20);
        UsedVirtualMB   = TotalVirtualMB - FreeVirtualMB;
        return;
    }
#endif
    TotalPhysicalMB = UsedPhysicalMB = FreePhysicalMB = 0;
    TotalVirtualMB  = UsedVirtualMB  = FreeVirtualMB  = 0;
}





void USystemInfoBPLibrary::GetCPUInfo(
    FString& DeviceName, ECPUVendor& Vendor,
    int32& PhysicalCores, int32& LogicalThreads)
{
    DeviceName = FPlatformMisc::GetCPUBrand().TrimStartAndEnd();
    if (DeviceName.IsEmpty()) DeviceName = TEXT("Unknown Processor");

    PhysicalCores  = FPlatformMisc::NumberOfCores();
    LogicalThreads = FPlatformMisc::NumberOfCoresIncludingHyperthreads();

    const FString VendorId = FPlatformMisc::GetCPUVendor();
    if (VendorId.Equals(TEXT("GenuineIntel"), ESearchCase::IgnoreCase) || DeviceName.Contains(TEXT("Intel")))
        Vendor = ECPUVendor::Intel;
    else if (VendorId.Equals(TEXT("AuthenticAMD"), ESearchCase::IgnoreCase) || DeviceName.Contains(TEXT("AMD")))
        Vendor = ECPUVendor::AMD;
    else if (DeviceName.Contains(TEXT("Apple")))
        Vendor = ECPUVendor::Apple;
    else if (DeviceName.Contains(TEXT("Snapdragon")) || DeviceName.Contains(TEXT("Qualcomm")))
        Vendor = ECPUVendor::Qualcomm;
    else
        Vendor = ECPUVendor::Generic;
}

float USystemInfoBPLibrary::GetCPUUsagePercent()
{
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::PerfMutex);
    if (WNT_Private::InitPerfCounters() && WNT_Private::CPUCounter)
    {
        WNT_Private::CollectPerfData();

        PDH_FMT_COUNTERVALUE Val;
        if (::PdhGetFormattedCounterValue(WNT_Private::CPUCounter, PDH_FMT_DOUBLE, nullptr, &Val) == ERROR_SUCCESS)
        {
            return static_cast<float>(FMath::Clamp(Val.doubleValue, 0.0, 100.0));
        }
    }
#endif
    return 0.0f;
}





TArray<FGPUAdapterInfo> USystemInfoBPLibrary::GetAllGPUAdapters()
{
    TArray<FGPUAdapterInfo> Result;
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::CacheMutex);
    WNT_Private::EnumerateAdapters();

    Result.Reserve(WNT_Private::CachedAdapters.Num());
    for (const auto& Entry : WNT_Private::CachedAdapters)
    {
        FGPUAdapterInfo Info;
        Info.AdapterIndex = Entry.Index;
        Info.AdapterName = FString(Entry.Desc.Description);
        Info.Vendor = WNT_Private::VendorIdToEnum(Entry.Desc.VendorId);
        Info.DedicatedVideoMemoryMB = static_cast<int64>(Entry.Desc.DedicatedVideoMemory >> 20);
        Info.SharedSystemMemoryMB = static_cast<int64>(Entry.Desc.SharedSystemMemory >> 20);
        Info.bIsActiveRHI = Entry.bIsActiveRHI;
        Result.Add(MoveTemp(Info));
    }
#endif
    return Result;
}

FGPUAdapterRuntimeInfo USystemInfoBPLibrary::GetGPUAdapterRuntimeInfo(int32 Adapter)
{
    FGPUAdapterRuntimeInfo Result;
#if PLATFORM_WINDOWS
    if (auto* Entry = WNT_Private::GetAdapter(Adapter))
    {
        Result.bSuccess = true;
        Result.AdapterInfo.AdapterIndex = Entry->Index;
        Result.AdapterInfo.AdapterName = FString(Entry->Desc.Description);
        Result.AdapterInfo.Vendor = WNT_Private::VendorIdToEnum(Entry->Desc.VendorId);
        Result.AdapterInfo.DedicatedVideoMemoryMB = static_cast<int64>(Entry->Desc.DedicatedVideoMemory >> 20);
        Result.AdapterInfo.SharedSystemMemoryMB = static_cast<int64>(Entry->Desc.SharedSystemMemory >> 20);
        Result.AdapterInfo.bIsActiveRHI = Entry->bIsActiveRHI;
        Result.UsedDedicatedVRAMMB = GetUsedDedicatedVRAM(Adapter);
        Result.UsedSharedVRAMMB = GetUsedVirtualVRAM(Adapter);
        Result.GameVRAMUsageMB = Entry->bIsActiveRHI ? GetGameVRAMUsage() : 0;
        Result.UsagePercent = GetGPUUsagePercent(Adapter);
    }
    else
    {
        Result.ErrorMessage = TEXT("GPU adapter index is invalid.");
    }
#else
    Result.ErrorMessage = TEXT("GPU runtime information is only available on Windows.");
#endif
    return Result;
}

FString USystemInfoBPLibrary::GetGPUName(int32 Adapter)
{
#if PLATFORM_WINDOWS
    if (auto* Entry = WNT_Private::GetAdapter(Adapter))
    {
        return FString(Entry->Desc.Description);
    }
#endif
    return TEXT("Unknown Graphics Adapter");
}

EGPUVendor USystemInfoBPLibrary::GetGPUManufacturer(int32 Adapter)
{
#if PLATFORM_WINDOWS
    if (auto* Entry = WNT_Private::GetAdapter(Adapter))
    {
        return WNT_Private::VendorIdToEnum(Entry->Desc.VendorId);
    }
#endif
    return EGPUVendor::Unknown;
}





int64 USystemInfoBPLibrary::GetTotalDedicatedVRAM(int32 Context)
{
#if PLATFORM_WINDOWS
    if (auto* Entry = WNT_Private::GetAdapter(Context))
    {
        return static_cast<int64>(Entry->Desc.DedicatedVideoMemory >> 20);
    }
#endif
    return 0;
}

int64 USystemInfoBPLibrary::GetUsedDedicatedVRAM(int32 Context)
{
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::PerfMutex);
    if (!WNT_Private::InitPerfCounters()) return 0;

    auto* Entry = WNT_Private::GetAdapter(Context);
    if (!Entry) return 0;

    WNT_Private::CollectPerfData();
    const FString LuidStr = WNT_Private::FormatLUID(Entry->Desc.AdapterLuid);
    return WNT_Private::QueryCounterForLUID(WNT_Private::DedicatedCounter, LuidStr) >> 20;
#else
    return 0;
#endif
}





int64 USystemInfoBPLibrary::GetTotalVirtualVRAM(int32 Context)
{
#if PLATFORM_WINDOWS
    if (auto* Entry = WNT_Private::GetAdapter(Context))
    {
        return static_cast<int64>(Entry->Desc.SharedSystemMemory >> 20);
    }
#endif
    return 0;
}

int64 USystemInfoBPLibrary::GetUsedVirtualVRAM(int32 Context)
{
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::PerfMutex);
    if (!WNT_Private::InitPerfCounters()) return 0;

    auto* Entry = WNT_Private::GetAdapter(Context);
    if (!Entry) return 0;

    WNT_Private::CollectPerfData();
    const FString LuidStr = WNT_Private::FormatLUID(Entry->Desc.AdapterLuid);
    return WNT_Private::QueryCounterForLUID(WNT_Private::SharedCounter, LuidStr) >> 20;
#else
    return 0;
#endif
}





int64 USystemInfoBPLibrary::GetGameVRAMUsage()
{
#if PLATFORM_WINDOWS
    auto* Entry = WNT_Private::GetRHIAdapter();
    if (Entry && Entry->Adapter)
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO MemInfo;
        if (SUCCEEDED(Entry->Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &MemInfo)))
        {
            return static_cast<int64>(MemInfo.CurrentUsage >> 20);
        }
    }
#endif
    return 0;
}





float USystemInfoBPLibrary::GetGPUUsagePercent(int32 Adapter)
{
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::PerfMutex);
    if (!WNT_Private::InitPerfCounters()) return 0.0f;

    auto* Entry = WNT_Private::GetAdapter(Adapter);
    if (!Entry) return 0.0f;

    WNT_Private::CollectPerfData();
    const FString LuidStr = WNT_Private::FormatLUID(Entry->Desc.AdapterLuid);
    return WNT_Private::QueryGPUUtil(LuidStr);
#else
    return 0.0f;
#endif
}





void USystemInfoBPLibrary::GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard)
{
    HasGamepad = false; HasMouse = false; HasKeyboard = false;
#if PLATFORM_WINDOWS
    UINT DeviceCount = 0;
    if (::GetRawInputDeviceList(nullptr, &DeviceCount, sizeof(RAWINPUTDEVICELIST)) != static_cast<UINT>(-1) && DeviceCount > 0)
    {
        TArray<RAWINPUTDEVICELIST> Devices;
        Devices.SetNumUninitialized(DeviceCount);
        const UINT Result = ::GetRawInputDeviceList(Devices.GetData(), &DeviceCount, sizeof(RAWINPUTDEVICELIST));
        if (Result != static_cast<UINT>(-1))
        {
            for (UINT i = 0; i < Result; ++i)
            {
                if (Devices[i].dwType == RIM_TYPEMOUSE) HasMouse = true;
                else if (Devices[i].dwType == RIM_TYPEKEYBOARD) HasKeyboard = true;
            }
        }
    }
    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
    {
        XINPUT_STATE State = {};
        if (::XInputGetState(i, &State) == ERROR_SUCCESS) { HasGamepad = true; break; }
    }
#endif
}





EGraphicsRHI USystemInfoBPLibrary::GetRHIName()
{
    if (!GDynamicRHI) return EGraphicsRHI::Unknown;
    const FString RHIName = FString(GDynamicRHI->GetName());
    if (RHIName == TEXT("D3D12"))             return EGraphicsRHI::DirectX12;
    if (RHIName == TEXT("D3D11"))             return EGraphicsRHI::DirectX11;
    if (RHIName.StartsWith(TEXT("Vulkan")))   return EGraphicsRHI::Vulkan;
    if (RHIName == TEXT("Metal"))             return EGraphicsRHI::Metal;
    if (RHIName.Contains(TEXT("OpenGL")))     return EGraphicsRHI::OpenGL;
    return EGraphicsRHI::Unknown;
}

bool USystemInfoBPLibrary::CanRestartGame()
{
    if (IsRunningCommandlet())
    {
        return false;
    }
#if WITH_EDITOR
    if (GIsEditor)
    {
        return false;
    }
#endif
    return FPlatformProcess::ExecutablePath() && FCString::Strlen(FPlatformProcess::ExecutablePath()) > 0;
}

void USystemInfoBPLibrary::RestartGameWithCommandLine(const FString& ExtraCommandLine)
{
#if PLATFORM_WINDOWS
    if (!CanRestartGame())
    {
        return;
    }

    static bool bIsRestarting = false;
    if (bIsRestarting) return;

    const FString RestartSentinel = TEXT("--restarted");
    if (ExtraCommandLine.Contains(RestartSentinel, ESearchCase::IgnoreCase)) return;

    FString Sanitized;
    Sanitized.Reserve(ExtraCommandLine.Len());
    for (const TCHAR C : ExtraCommandLine)
    {
        if (FChar::IsAlpha(C) || FChar::IsDigit(C) ||
            C == TEXT(' ') || C == TEXT('-') || C == TEXT('_') ||
            C == TEXT('=') || C == TEXT(':') || C == TEXT('/') ||
            C == TEXT('\\') || C == TEXT('.') || C == TEXT(','))
            Sanitized.AppendChar(C);
        else
            Sanitized.AppendChar(TEXT(' '));
    }
    Sanitized.TrimEndInline();

    FString CmdLine;
    if (!Sanitized.IsEmpty()) CmdLine = Sanitized + TEXT(" ");
    CmdLine += RestartSentinel;
    bIsRestarting = true;

    FCoreDelegates::OnPreExit.Broadcast();
    if (GConfig) GConfig->Flush(false, GEngineIni);

    const FString ExePath = FPlatformProcess::ExecutablePath();
    if (ExePath.IsEmpty()) { bIsRestarting = false; return; }

    FProcHandle ProcHandle = FPlatformProcess::CreateProc(
        *ExePath, *CmdLine, true, false, false, nullptr, 0, nullptr, nullptr);
    if (!ProcHandle.IsValid()) { bIsRestarting = false; return; }

    constexpr double TimeoutSeconds = 5.0;
    const double StartTime = FPlatformTime::Seconds();
    bool bChildRunning = false;
    while ((FPlatformTime::Seconds() - StartTime) < TimeoutSeconds)
    {
        if (FPlatformProcess::IsProcRunning(ProcHandle)) { bChildRunning = true; break; }
        FPlatformProcess::Sleep(0.05f);
    }
    FPlatformProcess::CloseProc(ProcHandle);
    if (bChildRunning) FPlatformMisc::RequestExit(true);
    else bIsRestarting = false;
#endif
}

bool USystemInfoBPLibrary::ExecuteWindowsCMD(const FString& Command, bool bRunAsAdmin, bool bHidden)
{
    FWNTCommandOptions Options;
    Options.Shell = EWNTCommandShell::Cmd;
    Options.bRunAsAdmin = bRunAsAdmin;
    Options.bHidden = bHidden;
    Options.bCaptureOutput = false;
    return WNT_Command::RunCommand(Command, Options).bStarted;
}

bool USystemInfoBPLibrary::ExecutePowerShell(const FString& Command, bool bRunAsAdmin, bool bHidden)
{
    FWNTCommandOptions Options;
    Options.Shell = EWNTCommandShell::PowerShell;
    Options.bRunAsAdmin = bRunAsAdmin;
    Options.bHidden = bHidden;
    Options.bCaptureOutput = false;
    return WNT_Command::RunCommand(Command, Options).bStarted;
}

void USystemInfoBPLibrary::RunCommandAsync(const FString& Command, FWNTCommandOptions Options, FOnWNTCommandResult OnResult)
{
    Async(EAsyncExecution::Thread, [Command, Options, OnResult]()
    {
        const FWNTCommandResult Result = WNT_Command::RunCommand(Command, Options);
        AsyncTask(ENamedThreads::GameThread, [OnResult, Result]()
        {
            OnResult.ExecuteIfBound(Result);
        });
    });
}

bool USystemInfoBPLibrary::CanForceKillGame()
{
    if (IsRunningCommandlet())
    {
        return false;
    }
#if WITH_EDITOR
    if (GIsEditor)
    {
        return false;
    }
#endif
    return true;
}

void USystemInfoBPLibrary::ForceKillGame()
{
    if (!CanForceKillGame())
    {
        return;
    }

#if PLATFORM_WINDOWS
    ::TerminateProcess(::GetCurrentProcess(), 0);
#else
    FPlatformMisc::RequestExit(true);
#endif
}


