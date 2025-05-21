/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

#include "Hardware_FrameworkBPLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <dxgi1_4.h>  // For IDXGIAdapter3
#include <XInput.h>
#include <winreg.h>
#include <vector>

#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "CoreMinimal.h"
#include "Runtime/Core/Public/Windows/WindowsPlatformMisc.h"
#include "Logging/LogMacros.h"

// Log category for system info
DEFINE_LOG_CATEGORY_STATIC(LogSystemInfo, Log, All);

void USystemInfoBPLibrary::GetMemoryInfo(int32& TotalPhysicalMB, int32& UsedPhysicalMB, int32& FreePhysicalMB,
    int32& TotalVirtualMB, int32& UsedVirtualMB, int32& FreeVirtualMB)
{
#if PLATFORM_WINDOWS
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
        UE_LOG(LogSystemInfo, Warning, TEXT("Failed to get memory info: %d"), GetLastError());
        TotalPhysicalMB = UsedPhysicalMB = FreePhysicalMB = 0;
        TotalVirtualMB = UsedVirtualMB = FreeVirtualMB = 0;
    }
#else
    UE_LOG(LogSystemInfo, Warning, TEXT("GetMemoryInfo is not supported on this platform"));
    TotalPhysicalMB = UsedPhysicalMB = FreePhysicalMB = 0;
    TotalVirtualMB = UsedVirtualMB = FreeVirtualMB = 0;
#endif
}

void USystemInfoBPLibrary::GetCPUInfo(FString& Name, FString& Manufacturer, int32& Cores, int32& Threads)
{
#if PLATFORM_WINDOWS
    Name = TEXT("Unknown");
    Manufacturer = TEXT("Unknown");
    Cores = Threads = 0;

    // Retrieve CPU name from registry
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        WCHAR Buffer[512];
        DWORD BufferSize = sizeof(Buffer);
        if (RegQueryValueExW(hKey, TEXT("ProcessorNameString"), nullptr, nullptr, (LPBYTE)Buffer, &BufferSize) == ERROR_SUCCESS)
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
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        WCHAR Buffer[512];
        DWORD BufferSize = sizeof(Buffer);
        if (RegQueryValueExW(hKey, TEXT("VendorIdentifier"), nullptr, nullptr, (LPBYTE)Buffer, &BufferSize) == ERROR_SUCCESS)
        {
            Manufacturer = FString(Buffer).TrimStartAndEnd();
        }
        RegCloseKey(hKey);
    }
    else
    {
        Manufacturer = FPlatformMisc::GetCPUVendor();
    }
#else
    UE_LOG(LogSystemInfo, Warning, TEXT("GetCPUInfo is not supported on this platform"));
    Name = TEXT("Unsupported");
    Manufacturer = TEXT("Unsupported");
    Cores = Threads = 0;
#endif
}

void USystemInfoBPLibrary::GetGPUInfo(FString& Name, FString& Manufacturer, int32& TotalVRAMMB,
    int32& UsedVRAMMB, int32& FreeVRAMMB)
{
#if PLATFORM_WINDOWS
    Name = TEXT("Unknown");
    Manufacturer = TEXT("Unknown");
    TotalVRAMMB = UsedVRAMMB = FreeVRAMMB = 0;

    IDXGIFactory* pFactory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    if (SUCCEEDED(hr))
    {
        IDXGIAdapter* pAdapter = nullptr;
        if (SUCCEEDED(pFactory->EnumAdapters(0, &pAdapter)))
        {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(pAdapter->GetDesc(&desc)))
            {
                Name = desc.Description;
                TotalVRAMMB = static_cast<int32>(desc.DedicatedVideoMemory >> 20);

                // Detect manufacturer based on VendorId
                switch (desc.VendorId)
                {
                case 0x10DE: Manufacturer = TEXT("NVIDIA"); break;
                case 0x1002: Manufacturer = TEXT("AMD"); break;
                case 0x8086: Manufacturer = TEXT("Intel"); break;
                case 0x1414: Manufacturer = TEXT("Microsoft"); break;
                default: Manufacturer = TEXT("Unknown"); break;
                }

                // Query VRAM usage (DXGI 1.4)
                IDXGIAdapter3* pAdapter3 = nullptr;
                if (SUCCEEDED(pAdapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&pAdapter3)))
                {
                    DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo;
                    if (SUCCEEDED(pAdapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo)))
                    {
                        UsedVRAMMB = static_cast<int32>(memoryInfo.CurrentUsage >> 20);
                        FreeVRAMMB = static_cast<int32>((memoryInfo.Budget - memoryInfo.CurrentUsage) >> 20);
                    }
                    pAdapter3->Release();
                }
            }
            pAdapter->Release();
        }
        pFactory->Release();
    }
    else
    {
        UE_LOG(LogSystemInfo, Warning, TEXT("Failed to create DXGI factory: 0x%08X"), hr);
    }
#else
    UE_LOG(LogSystemInfo, Warning, TEXT("GetGPUInfo is not supported on this platform"));
    Name = TEXT("Unsupported");
    Manufacturer = TEXT("Unsupported");
    TotalVRAMMB = UsedVRAMMB = FreeVRAMMB = 0;
#endif
}

void USystemInfoBPLibrary::GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard)
{
#if PLATFORM_WINDOWS
    // Gamepad detection using XInput
    XINPUT_STATE state;
    HasGamepad = (XInputGetState(0, &state) == ERROR_SUCCESS);

    // Mouse detection
    HasMouse = (GetSystemMetrics(SM_MOUSEPRESENT) != 0);

    // Keyboard detection (assumed true on Windows)
    HasKeyboard = true;
#else
    UE_LOG(LogSystemInfo, Warning, TEXT("GetInputDevices is not supported on this platform"));
    HasGamepad = HasMouse = HasKeyboard = false;
#endif
}