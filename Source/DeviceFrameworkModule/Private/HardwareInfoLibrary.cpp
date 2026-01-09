// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "HardwareInfoLibrary.h"

#include "HardwareInfo.h"            
#include "RHI.h"
#include "Misc/ScopeLock.h"
#include "HAL/PlatformMisc.h"      
#include "Misc/ConfigCacheIni.h"    
#include "Misc/CoreDelegates.h" 

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsSystemIncludes.h"
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <Xinput.h>
#include <shellapi.h> //for cmd things..
#include "Windows/HideWindowsPlatformTypes.h"
#endif

//Important for vram query..since there is multi

// --- VRAM HELPERS & CACHE ---

using namespace Microsoft::WRL;

#if PLATFORM_WINDOWS
static FCriticalSection AdapterCacheMutex;
static ComPtr<IDXGIAdapter3> CachedAdapter = nullptr;

static bool GetCachedAdapter(ComPtr<IDXGIAdapter3>& OutAdapter)
{
    FScopeLock Lock(&AdapterCacheMutex);

    if (CachedAdapter)
    {
        OutAdapter = CachedAdapter;
        return true;
    }

    ComPtr<IDXGIFactory4> Factory;

    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&Factory))))
    {
        return false;
    }

    ComPtr<IDXGIAdapter1> TempAdapter1;
    uint32 Index = 0;
    bool bFound = false;

    while (Factory->EnumAdapters1(Index, &TempAdapter1) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC1 Desc;
        TempAdapter1->GetDesc1(&Desc);

        if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            Index++;
            continue;
        }

        if (Desc.VendorId == GRHIVendorId || GRHIVendorId == 0)
        {
            if (SUCCEEDED(TempAdapter1.As(&CachedAdapter)))
            {
                OutAdapter = CachedAdapter;
                bFound = true;
                break;
            }
        }
        Index++;
    }

    return bFound;
}

static bool QueryGPUStats(int32& OutTotalMB, int32& OutBudgetMB, int32& OutUsageMB)
{
    OutTotalMB = OutBudgetMB = OutUsageMB = 0;

    ComPtr<IDXGIAdapter3> Adapter;
    if (GetCachedAdapter(Adapter))
    {
        DXGI_ADAPTER_DESC1 Desc;
        if (SUCCEEDED(Adapter->GetDesc1(&Desc)))
        {
            OutTotalMB = (int32)(Desc.DedicatedVideoMemory / 1024 / 1024);
        }
        else
        {
            return false;
        }

        DXGI_QUERY_VIDEO_MEMORY_INFO Info;
        if (SUCCEEDED(Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &Info)))
        {
            OutUsageMB = (int32)(Info.CurrentUsage / 1024 / 1024);
            OutBudgetMB = (int32)(Info.Budget / 1024 / 1024);
            return true;
        }
    }
    return false;
}
#endif

// --- PUBLIC BP FUNCTIONS ---

void USystemInfoBPLibrary::GetMemoryInfo(int64& TotalPhysicalMB, int64& UsedPhysicalMB, int64& FreePhysicalMB,
    int64& TotalVirtualMB, int64& UsedVirtualMB, int64& FreeVirtualMB)
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memInfo))
    {

        TotalPhysicalMB = static_cast<int64>(memInfo.ullTotalPhys >> 20);
        FreePhysicalMB = static_cast<int64>(memInfo.ullAvailPhys >> 20);
        UsedPhysicalMB = TotalPhysicalMB - FreePhysicalMB;

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
    DeviceName = FPlatformMisc::GetCPUBrand().TrimStartAndEnd();

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
#if PLATFORM_WINDOWS
    static int32 CachedTotalVRAM = -1;

    static FCriticalSection VRAMCacheMutex;
    FScopeLock Lock(&VRAMCacheMutex);

    if (CachedTotalVRAM < 0)
    {
        int32 Budget, Usage;
        int32 TempTotal = 0;

        if (QueryGPUStats(TempTotal, Budget, Usage))
        {
            CachedTotalVRAM = TempTotal;
        }
    }

    return (CachedTotalVRAM < 0) ? 0 : CachedTotalVRAM;
#else
    return 0;
#endif
}

int32 USystemInfoBPLibrary::GetGameVRAMUsageMB()
{
#if PLATFORM_WINDOWS
    int32 Total, Budget, Usage;
    if (QueryGPUStats(Total, Budget, Usage))
    {
        return Usage;
    }
#endif
    return 0;
}

int32 USystemInfoBPLibrary::GetUsedVRAMMB()
{
#if PLATFORM_WINDOWS
    int32 Total, Budget, Usage;
    if (QueryGPUStats(Total, Budget, Usage))
    {

        int32 SystemOverhead = Total - Budget;
        int32 GlobalUsed = SystemOverhead + Usage;

        return FMath::Clamp(GlobalUsed, 0, Total);
    }
#endif
    return 0;
}

void USystemInfoBPLibrary::GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard)
{
    UINT nDevices = 0;

    GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST));

    if (nDevices == 0)
    {
        HasMouse = false;
        HasKeyboard = false;
    }
    else
    {

        TArray<RAWINPUTDEVICELIST> DeviceList;
        DeviceList.SetNumUninitialized(nDevices);


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

    if (!GDynamicRHI)
    {
        return EGraphicsRHI::Unknown;
    }


    FString RHIName = FString(GDynamicRHI->GetName());


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

bool USystemInfoBPLibrary::ExecuteWindowsCMD(const FString& Command, bool bRunAsAdmin, bool bHidden)
{
    if (Command.IsEmpty()) return false;

    const FString Params = FString::Printf(TEXT("/c \"%s\""), *Command);

#if PLATFORM_WINDOWS
    if (bRunAsAdmin)
    {
        SHELLEXECUTEINFO ShExecInfo = { 0 };
        ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
        ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        ShExecInfo.hwnd = NULL;
        ShExecInfo.lpVerb = TEXT("runas");
        ShExecInfo.lpFile = TEXT("cmd.exe");
        ShExecInfo.lpParameters = *Params;
        ShExecInfo.lpDirectory = NULL;
        ShExecInfo.nShow = bHidden ? SW_HIDE : SW_SHOW;

        return ShellExecuteEx(&ShExecInfo);
    }
    else
    {

        FProcHandle Handle = FPlatformProcess::CreateProc(
            TEXT("cmd.exe"), 
            *Params,        
            true,       
            bHidden,       
            bHidden,      
            nullptr,     
            0,          
            nullptr,  
            nullptr      

        );

        return Handle.IsValid();
    }
#else
    return false;
#endif
}

void USystemInfoBPLibrary::ForceKillGame()
{
#if PLATFORM_WINDOWS

    HANDLE hProcess = GetCurrentProcess();
    TerminateProcess(hProcess, 0);
#else
    FPlatformMisc::RequestExit(true);
#endif
}