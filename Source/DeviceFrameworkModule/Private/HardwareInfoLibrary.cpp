// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "HardwareInfoLibrary.h"
#include "Runtime/Core/Public/Windows/WindowsPlatformMisc.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinWindows.h"
#include <dxgi1_4.h> 
#include <Xinput.h>    
#include <winreg.h>    
#include <vector>
#include <pdh.h>
#include <wchar.h>
#pragma comment(lib, "pdh.lib")
//DXGI & PDH
#include <dxgi.h>
#include <dxgi1_6.h>
#include <pdhmsg.h>
#pragma comment(lib, "dxgi.lib")
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "HardwareInfo.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"


// Memory Information
void USystemInfoBPLibrary::GetMemoryInfo(int32& TotalPhysicalMB, int32& UsedPhysicalMB, int32& FreePhysicalMB,
    int32& TotalVirtualMB, int32& UsedVirtualMB, int32& FreeVirtualMB)
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo))
    {
        TotalPhysicalMB = static_cast<int32>(memInfo.ullTotalPhys >> 20);
        FreePhysicalMB = static_cast<int32>(memInfo.ullAvailPhys >> 20);
        UsedPhysicalMB = TotalPhysicalMB - FreePhysicalMB;

        TotalVirtualMB = static_cast<int32>(memInfo.ullTotalVirtual >> 20);
        FreeVirtualMB = static_cast<int32>(memInfo.ullAvailVirtual >> 20);
        UsedVirtualMB = TotalVirtualMB - FreeVirtualMB;
    }
    else
    {
        TotalPhysicalMB = UsedPhysicalMB = FreePhysicalMB = 0;
        TotalVirtualMB = UsedVirtualMB = FreeVirtualMB = 0;
    }
}

// CPU Information
void USystemInfoBPLibrary::GetCPUInfo(FString& Name, FString& Manufacturer, int32& Cores, int32& Threads)
{
    Name = TEXT("Unknown");
    Manufacturer = TEXT("Unknown");
    Cores = Threads = 0;

    // Retrieve CPU name from registry
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR Buffer[512];
        DWORD BufferSize = sizeof(Buffer);
        if (RegQueryValueEx(hKey, TEXT("ProcessorNameString"), NULL, NULL, (LPBYTE)Buffer, &BufferSize) == ERROR_SUCCESS)
        {
            Name = FString(Buffer).TrimStartAndEnd();
        }
        RegCloseKey(hKey);
    }

    // Get logical processor count
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    Threads = sysInfo.dwNumberOfProcessors;

    // Get physical core count
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        std::vector<BYTE> buffer(length);
        if (GetLogicalProcessorInformationEx(RelationProcessorCore, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &length))
        {
            Cores = 0;
            DWORD offset = 0;
            while (offset < length)
            {
                auto info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data() + offset);
                if (info->Relationship == RelationProcessorCore)
                {
                    Cores++;
                }
                offset += info->Size;
            }
        }
    }

    // Retrieve manufacturer from registry with fallback
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR Buffer[512];
        DWORD BufferSize = sizeof(Buffer);
        if (RegQueryValueEx(hKey, TEXT("VendorIdentifier"), NULL, NULL, (LPBYTE)Buffer, &BufferSize) == ERROR_SUCCESS)
        {
            Manufacturer = FString(Buffer).TrimStartAndEnd();
        }
        RegCloseKey(hKey);
    }
    else
    {
        Manufacturer = FPlatformMisc::GetCPUVendor();
    }
}

//Decapreted function to get all in one gpu info (Using Only DXGI)
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

//Get GPU Name & Manufacturer
void USystemInfoBPLibrary::GetGPUNameAndManufacturer(FString& Name, FString& Manufacturer)
{
    Name = TEXT("Unknown");
    Manufacturer = TEXT("Unknown");

#if PLATFORM_WINDOWS
    IDXGIFactory* pFactory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory)))
    {
        IDXGIAdapter* pAdapter = nullptr;
        if (SUCCEEDED(pFactory->EnumAdapters(0, &pAdapter)))
        {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(pAdapter->GetDesc(&desc)))
            {
                Name = FString(desc.Description);
                switch (desc.VendorId)
                {
                case 0x10DE: Manufacturer = TEXT("NVIDIA"); break;
                case 0x1002: Manufacturer = TEXT("AMD"); break;
                case 0x8086: Manufacturer = TEXT("Intel"); break;
                case 0x1414: Manufacturer = TEXT("Microsoft"); break;
                default:     Manufacturer = TEXT("Unknown"); break;
                }
            }
            pAdapter->Release();
        }
        pFactory->Release();
    }
#endif
}

//Get Total VRAM in MB (Using DXGI & PDH)
int32 USystemInfoBPLibrary::GetTotalVRAMMB()
{
    int32 TotalVRAMMB = 0;

#if PLATFORM_WINDOWS
    IDXGIFactory* pFactory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory)))
    {
        IDXGIAdapter* pAdapter = nullptr;
        if (SUCCEEDED(pFactory->EnumAdapters(0, &pAdapter)))
        {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(pAdapter->GetDesc(&desc)))
            {
                TotalVRAMMB = static_cast<int32>(desc.DedicatedVideoMemory / 1024 / 1024);
            }
            pAdapter->Release();
        }
        pFactory->Release();
    }
#endif

    return TotalVRAMMB;
}

//Get Used VRAM in MB (adapter-level) (Using DXGI & PDH)
int32 USystemInfoBPLibrary::GetUsedVRAMMB()
{
    int32 UsedVRAMMB = 0;

#if PLATFORM_WINDOWS
    const wchar_t* pattern = L"\\GPU Adapter Memory(*)\\Dedicated Usage";

    DWORD requiredChars = 0;
    PDH_STATUS ps = PdhExpandWildCardPathW(nullptr, pattern, nullptr, &requiredChars, 0);
    const PDH_STATUS PDH_MORE_DATA_NUM = 0x800007D2u;

    if (ps == PDH_MORE_DATA_NUM && requiredChars > 0)
    {
        std::vector<wchar_t> buffer(requiredChars);
        if (PdhExpandWildCardPathW(nullptr, pattern, buffer.data(), &requiredChars, 0) == ERROR_SUCCESS)
        {
            wchar_t* entry = buffer.data();
            if (*entry != L'\0')
            {
                PDH_HQUERY pdhQuery = nullptr;
                PDH_HCOUNTER pdhCounter = nullptr;
                if (PdhOpenQueryW(nullptr, 0, &pdhQuery) == ERROR_SUCCESS)
                {
                    if (PdhAddCounterW(pdhQuery, entry, 0, &pdhCounter) == ERROR_SUCCESS)
                    {
                        if (PdhCollectQueryData(pdhQuery) == ERROR_SUCCESS)
                        {
                            FPlatformProcess::Sleep(0.1f);
                            if (PdhCollectQueryData(pdhQuery) == ERROR_SUCCESS)
                            {
                                PDH_FMT_COUNTERVALUE counterValue;
                                if (PdhGetFormattedCounterValue(pdhCounter, PDH_FMT_LARGE, nullptr, &counterValue) == ERROR_SUCCESS
                                    && counterValue.CStatus == ERROR_SUCCESS)
                                {
                                    UsedVRAMMB = static_cast<int32>(counterValue.largeValue / 1024 / 1024);
                                }
                            }
                        }
                        PdhRemoveCounter(pdhCounter);
                    }
                    PdhCloseQuery(pdhQuery);
                }
            }
        }
    }
#endif

    return UsedVRAMMB;
}

//Get Game VRAM Usage in MB (process-level) (Using DXGI & PDH)
int32 USystemInfoBPLibrary::GetGameVRAMUsageMB()
{
    int32 GameVRAMMB = 0;

#if PLATFORM_WINDOWS
    const wchar_t* pattern = L"\\GPU Process Memory(*)\\Dedicated Usage";

    DWORD pid = GetCurrentProcessId();
    wchar_t pidBuf[32];
    swprintf_s(pidBuf, _countof(pidBuf), L"pid_%lu", static_cast<unsigned long>(pid));

    DWORD requiredChars = 0;
    PDH_STATUS ps = PdhExpandWildCardPathW(nullptr, pattern, nullptr, &requiredChars, 0);
    const PDH_STATUS PDH_MORE_DATA_NUM = 0x800007D2u;

    if (ps == PDH_MORE_DATA_NUM && requiredChars > 0)
    {
        std::vector<wchar_t> buffer(requiredChars);
        if (PdhExpandWildCardPathW(nullptr, pattern, buffer.data(), &requiredChars, 0) == ERROR_SUCCESS)
        {
            wchar_t* entry = buffer.data();
            while (*entry != L'\0')
            {
                if (wcsstr(entry, pidBuf) != nullptr) // match our process
                {
                    PDH_HQUERY pdhQuery = nullptr;
                    PDH_HCOUNTER pdhCounter = nullptr;
                    if (PdhOpenQueryW(nullptr, 0, &pdhQuery) == ERROR_SUCCESS)
                    {
                        if (PdhAddCounterW(pdhQuery, entry, 0, &pdhCounter) == ERROR_SUCCESS)
                        {
                            if (PdhCollectQueryData(pdhQuery) == ERROR_SUCCESS)
                            {
                                FPlatformProcess::Sleep(0.1f);
                                if (PdhCollectQueryData(pdhQuery) == ERROR_SUCCESS)
                                {
                                    PDH_FMT_COUNTERVALUE counterValue;
                                    if (PdhGetFormattedCounterValue(pdhCounter, PDH_FMT_LARGE, nullptr, &counterValue) == ERROR_SUCCESS
                                        && counterValue.CStatus == ERROR_SUCCESS)
                                    {
                                        GameVRAMMB = static_cast<int32>(counterValue.largeValue / 1024 / 1024);
                                    }
                                }
                            }
                            PdhRemoveCounter(pdhCounter);
                        }
                        PdhCloseQuery(pdhQuery);
                    }
                    break;
                }
                entry += wcslen(entry) + 1;
            }
        }
    }
#endif

    return GameVRAMMB;
}

// Input Devices Reporter
void USystemInfoBPLibrary::GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard)
{
    // Gamepad detection using XInput
    XINPUT_STATE state;
    HasGamepad = (XInputGetState(0, &state) == ERROR_SUCCESS);

    // Mouse detection
    HasMouse = (GetSystemMetrics(SM_MOUSEPRESENT) != 0);

    // Keyboard detection (assumed true on Windows)
    HasKeyboard = true;
}

//Retun the current in use RHI
FString USystemInfoBPLibrary::GetRHIName()
{
    FString RHI_Info = FHardwareInfo::GetHardwareInfo(NAME_RHI);
    int32 SpacePos = RHI_Info.Find(TEXT(" "));
    if (SpacePos != INDEX_NONE)
    {
        return RHI_Info.Left(SpacePos);
    }
    return RHI_Info.IsEmpty() ? FString(TEXT("Unknown")) : RHI_Info;
}

//Restart the game, it work by simply launching new game instance then close the current one.
void USystemInfoBPLibrary::RestartGameWithCommandLine(const FString& ExtraCommandLine)
{
    const FString RestartSentinel = TEXT("--restarted");

    if (ExtraCommandLine.Contains(RestartSentinel, ESearchCase::IgnoreCase) ||
        FString(FCommandLine::Get()).Contains(RestartSentinel, ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Warning, TEXT("Restart aborted: restart sentinel already present."));
        return;
    }

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

    FCoreDelegates::OnPreExit.Broadcast();
    if (GConfig)
    {
        GConfig->Flush(false, GEngineIni);
    }

    const FString ExePath = FPlatformProcess::ExecutablePath();
    if (ExePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Restart aborted: cannot determine executable path."));
        return;
    }

    FProcHandle ProcHandle = FPlatformProcess::CreateProc(
        *ExePath,
        *CmdLine,
        true,
        false,
        false,
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (!ProcHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Restart failed: CreateProc returned invalid handle."));
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
        UE_LOG(LogTemp, Log, TEXT("Restart successful: spawned child process, exiting gracefully."));
        FPlatformProcess::CloseProc(ProcHandle);
        FPlatformMisc::RequestExit(true);
        return;
    }

    int32 ReturnCode = 0;
    bool bHasReturn = FPlatformProcess::GetProcReturnCode(ProcHandle, &ReturnCode);
    if (bHasReturn)
    {
        UE_LOG(LogTemp, Error, TEXT("Restart failed: child exited immediately with code %d."), ReturnCode);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Restart failed: child not running after %.1f seconds (timeout)."), TimeoutSeconds);
    }
    FPlatformProcess::CloseProc(ProcHandle);
}