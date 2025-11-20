// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "HardwareInfoLibrary.h"

#include "HardwareInfo.h"            
#include "RHI.h"                   
#include "HAL/PlatformMisc.h"      
#include "Misc/ConfigCacheIni.h"    
#include "Misc/CoreDelegates.h" 

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsSystemIncludes.h"
#include <dxgi1_4.h>
#include <Xinput.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

// --- PRIVATE HELPER FUNCTION ---

static bool QueryGPUStats(int32& OutTotalMB, int32& OutBudgetMB, int32& OutUsageMB)
{
    OutTotalMB = 0;
    OutBudgetMB = 0;
    OutUsageMB = 0;

#if PLATFORM_WINDOWS
    IDXGIFactory4* pFactory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&pFactory)))
    {
        return false;
    }

    IDXGIAdapter3* pAdapter = nullptr;
    uint32 AdapterIndex = 0;
    bool bFound = false;

    const uint32 TargetVendorId = GRHIVendorId;

    while (pFactory->EnumAdapters(AdapterIndex, reinterpret_cast<IDXGIAdapter**>(&pAdapter)) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        pAdapter->GetDesc(&desc);

        if (desc.VendorId == TargetVendorId || TargetVendorId == 0)
        {
            OutTotalMB = static_cast<int32>(desc.DedicatedVideoMemory / 1024 / 1024);

            DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
            if (SUCCEEDED(pAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo)))
            {
                OutUsageMB = static_cast<int32>(videoMemoryInfo.CurrentUsage / 1024 / 1024);
                OutBudgetMB = static_cast<int32>(videoMemoryInfo.Budget / 1024 / 1024);
            }
            bFound = true;
        }

        pAdapter->Release();
        if (bFound) break;
        AdapterIndex++;
    }

    pFactory->Release();
    return bFound;
#else
    return false;
#endif
}



// --- PUBLIC BP FUNCTIONS ---

void USystemInfoBPLibrary::GetMemoryInfo(int64& TotalPhysicalMB, int64& UsedPhysicalMB, int64& FreePhysicalMB,
    int64& TotalVirtualMB, int64& UsedVirtualMB, int64& FreeVirtualMB)
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memInfo))
    {
        // 1. PHYSICAL MEMORY (RAM)
        // We use >> 20 to convert Bytes to MiB (matches Task Manager "MB")
        TotalPhysicalMB = static_cast<int64>(memInfo.ullTotalPhys >> 20);
        FreePhysicalMB = static_cast<int64>(memInfo.ullAvailPhys >> 20);
        UsedPhysicalMB = TotalPhysicalMB - FreePhysicalMB;

        // 2. VIRTUAL MEMORY (Commit Limit / Page File)
        // CRITICAL FIX: Use ullTotalPageFile, NOT ullTotalVirtual
        // ullTotalPageFile = Physical RAM + Size of Page File on Disk
        TotalVirtualMB = static_cast<int64>(memInfo.ullTotalPageFile >> 20);
        FreeVirtualMB = static_cast<int64>(memInfo.ullAvailPageFile >> 20);
        UsedVirtualMB = TotalVirtualMB - FreeVirtualMB;
    }
    else
    {
        TotalPhysicalMB = UsedPhysicalMB = FreePhysicalMB = 0;
        TotalVirtualMB = UsedVirtualMB = FreeVirtualMB = 0;
    }
}

void USystemInfoBPLibrary::GetCPUInfo(FString& DeviceName, ECPUVendor& Vendor, int32& PhysicalCores, int32& LogicalThreads)
{
    DeviceName = FPlatformMisc::GetCPUBrand();

    if (DeviceName.IsEmpty())
    {
        DeviceName = TEXT("Unknown Processor");
    }

    PhysicalCores = FPlatformMisc::NumberOfCores();

    LogicalThreads = FPlatformMisc::NumberOfCoresIncludingHyperthreads();

    FString VendorString = FPlatformMisc::GetCPUVendor();

    if (VendorString.Equals(TEXT("GenuineIntel"), ESearchCase::IgnoreCase) || DeviceName.Contains(TEXT("Intel")))
    {
        Vendor = ECPUVendor::Intel;
    }
    else if (VendorString.Equals(TEXT("AuthenticAMD"), ESearchCase::IgnoreCase) || DeviceName.Contains(TEXT("AMD")))
    {
        Vendor = ECPUVendor::AMD;
    }
    else if (DeviceName.Contains(TEXT("Apple")))
    {
        Vendor = ECPUVendor::Apple;
    }
    else if (DeviceName.Contains(TEXT("Snapdragon")) || DeviceName.Contains(TEXT("Qualcomm")))
    {
        Vendor = ECPUVendor::Qualcomm;
    }
    else
    {
        Vendor = ECPUVendor::Generic;
    }
}

void USystemInfoBPLibrary::GetGPUInfo(FString& Name, FString& Manufacturer, int32& TotalVRAMMB, int32& UsedVRAMMB, int32& FreeVRAMMB)
{
    Name = TEXT("Unknown");
    Manufacturer = TEXT("Unknown");
    TotalVRAMMB = 0;
    UsedVRAMMB = 0;
    FreeVRAMMB = 0;

    IDXGIFactory* pFactory = nullptr;
    if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory)))
    {
        return;
    }
    IDXGIAdapter* pAdapter = nullptr;
    if (SUCCEEDED(pFactory->EnumAdapters(0, &pAdapter)))
    {
        DXGI_ADAPTER_DESC desc;
        if (SUCCEEDED(pAdapter->GetDesc(&desc)))
        {
            Name = FString(desc.Description);
            TotalVRAMMB = static_cast<int32>(desc.DedicatedVideoMemory >> 20);
            switch (desc.VendorId)
            {
            case 0x10DE: Manufacturer = TEXT("NVIDIA"); break;
            case 0x1002: Manufacturer = TEXT("AMD"); break;
            case 0x8086: Manufacturer = TEXT("Intel"); break;
            case 0x1414: Manufacturer = TEXT("Microsoft"); break;
            default: Manufacturer = TEXT("Unknown"); break;
            }
            IDXGIAdapter3* pAdapter3 = nullptr;
            if (SUCCEEDED(pAdapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&pAdapter3)))
            {
                DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo;
                if (SUCCEEDED(pAdapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo)))
                {
                    UsedVRAMMB = static_cast<int32>(memoryInfo.CurrentUsage >> 20);
                    FreeVRAMMB = TotalVRAMMB - UsedVRAMMB;
                }
                pAdapter3->Release();
            }
        }
        pAdapter->Release();
    }
    pFactory->Release();
}

void USystemInfoBPLibrary::GetGPUNameAndManufacturer(FString& DeviceName, EGPUVendor& Manufacturer)
{
    DeviceName = GRHIAdapterName;

    if (DeviceName.IsEmpty())
    {
        DeviceName = TEXT("Unknown Graphics Adapter");
    }

    const uint32 VendorId = GRHIVendorId;

    switch (VendorId)
    {
    case 0x10DE:
        Manufacturer = EGPUVendor::Nvidia;
        break;

    case 0x1002:
        Manufacturer = EGPUVendor::AMD;
        break;

    case 0x8086:
        Manufacturer = EGPUVendor::Intel;
        break;

    case 0x5143:
        Manufacturer = EGPUVendor::Qualcomm;
        break;

    default:
        Manufacturer = EGPUVendor::Unknown;
        break;
    }
}

int32 USystemInfoBPLibrary::GetTotalVRAMMB()
{
    // We cache this value because Total VRAM never changes while the game is running.
    static int32 CachedTotalVRAM = -1;

    if (CachedTotalVRAM < 0)
    {
        int32 Budget, Usage;
        QueryGPUStats(CachedTotalVRAM, Budget, Usage);
    }

    return CachedTotalVRAM;
}

int32 USystemInfoBPLibrary::GetGameVRAMUsageMB()
{
    int32 Total, Budget, Usage;
    if (QueryGPUStats(Total, Budget, Usage))
    {
        return Usage;
    }
    return 0;
}

int32 USystemInfoBPLibrary::GetUsedVRAMMB()
{
    int32 Total, Budget, Usage;
    if (QueryGPUStats(Total, Budget, Usage))
    {


        int32 SystemOverhead = Total - Budget;
        int32 GlobalUsed = SystemOverhead + Usage;

        if (GlobalUsed > Total) GlobalUsed = Total;
        if (GlobalUsed < 0) GlobalUsed = 0;

        return GlobalUsed;
    }
    return 0;
}

void USystemInfoBPLibrary::GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard)
{
    UINT nDevices = 0;
    // Get the number of devices
    GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST));

    if (nDevices == 0)
    {
        HasMouse = false;
        HasKeyboard = false;
    }
    else
    {
        // Allocate memory for the device list
        TArray<RAWINPUTDEVICELIST> DeviceList;
        DeviceList.SetNumUninitialized(nDevices);

        // Retrieve the device list
        GetRawInputDeviceList(DeviceList.GetData(), &nDevices, sizeof(RAWINPUTDEVICELIST));

        HasMouse = false;
        HasKeyboard = false;

        for (UINT i = 0; i < nDevices; i++)
        {
            if (DeviceList[i].dwType == RIM_TYPEMOUSE)
            {

                HasMouse = true;
            }
            else if (DeviceList[i].dwType == RIM_TYPEKEYBOARD)
            {
                HasKeyboard = true;
            }
        }
    }
    HasGamepad = false;

    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
    {
        XINPUT_STATE State;
        ZeroMemory(&State, sizeof(XINPUT_STATE));

        if (XInputGetState(i, &State) == ERROR_SUCCESS)
        {
            HasGamepad = true;
            break; // Found at least one controller
        }
    }

    // NOTE: This still only finds XInput (Xbox) devices. 
    // To find PlayStation (DirectInput) controllers without Steam/DS4Windows, 
    // you would need to parse the RawInput list above for HID devices with "Joystick" usage pages.
}

EGraphicsRHI USystemInfoBPLibrary::GetRHIName()
{
    // Safety Check: If running on a server or commandlet with no RHI
    if (!GDynamicRHI)
    {
        return EGraphicsRHI::Unknown;
    }

    // Get the authoritative name directly from the driver
    FString RHIName = FString(GDynamicRHI->GetName());

    // Map to Enum
    if (RHIName == TEXT("D3D12"))           return EGraphicsRHI::DirectX12;
    if (RHIName == TEXT("D3D11"))           return EGraphicsRHI::DirectX11;
    if (RHIName.StartsWith(TEXT("Vulkan"))) return EGraphicsRHI::Vulkan;
    if (RHIName == TEXT("Metal"))           return EGraphicsRHI::Metal;
    if (RHIName.Contains(TEXT("OpenGL")))   return EGraphicsRHI::OpenGL;

    return EGraphicsRHI::Unknown;
}

void USystemInfoBPLibrary::RestartGameWithCommandLine(const FString& ExtraCommandLine)
{
#if PLATFORM_WINDOWS
    static bool bIsRestarting = false;
    if (bIsRestarting) { return; }

    const FString RestartSentinel = TEXT("--restarted");
    if (ExtraCommandLine.Contains(RestartSentinel, ESearchCase::IgnoreCase)) { return; }

    FString Sanitized;
    Sanitized.Reserve(ExtraCommandLine.Len());
    for (TCHAR C : ExtraCommandLine)
    {
        if (FChar::IsAlpha(C) || FChar::IsDigit(C) ||
            C == TEXT(' ') || C == TEXT('-') || C == TEXT('_') ||
            C == TEXT('=') || C == TEXT(':') || C == TEXT('/') ||
            C == TEXT('\\') || C == TEXT('.') || C == TEXT(','))
        {
            Sanitized.AppendChar(C);
        }
        else
        {
            Sanitized.AppendChar(TEXT(' '));
        }
    }

    FString CmdLine = Sanitized;
    if (!CmdLine.IsEmpty())
    {
        CmdLine.TrimEndInline();
        CmdLine += TEXT(" ");
    }

    CmdLine += RestartSentinel;
    bIsRestarting = true;

    FCoreDelegates::OnPreExit.Broadcast();
    if (GConfig) { GConfig->Flush(false, GEngineIni); }

    const FString ExePath = FPlatformProcess::ExecutablePath();
    if (ExePath.IsEmpty())
    {
        bIsRestarting = false;
        return;
    }

    FProcHandle ProcHandle = FPlatformProcess::CreateProc(*ExePath, *CmdLine, true, false, false, nullptr, 0, nullptr, nullptr);
    if (!ProcHandle.IsValid())
    {
        bIsRestarting = false;
        return;
    }

    const double StartTime = FPlatformTime::Seconds();
    const double TimeoutSeconds = 5.0;
    bool bChildRunning = false;

    while ((FPlatformTime::Seconds() - StartTime) < TimeoutSeconds)
    {
        if (FPlatformProcess::IsProcRunning(ProcHandle))
        {
            bChildRunning = true;
            break;
        }
        FPlatformProcess::Sleep(0.05f);
    }

    if (bChildRunning)
    {
        FPlatformProcess::CloseProc(ProcHandle);
        FPlatformMisc::RequestExit(true);
        return;
    }

    FPlatformProcess::CloseProc(ProcHandle);
    bIsRestarting = false;
#endif
}