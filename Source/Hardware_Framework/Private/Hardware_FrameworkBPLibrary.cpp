/*

 * Copyright (C) 2025 AldertLake. All rights reserved.
 *
 * This file is part of the Windows Native ToolKit, an Unreal Engine Plugin.
 *
 * Unauthorized copying, distribution, or modification of this file is strictly prohibited.
 *
 * Anyone who bought this project has the full right to modify it like he want.
 *
 *
 * Author: AldertLake
 * Date: 2025/1/9
 ______________________________________________________________________________________________________________

  AAAAAAA     L          DDDDDDD     EEEEEEE     RRRRRRR    TTTTTTT    L          AAAAAAA     KKKKKK    EEEEEEE
 A       A    L          D       D   E           R     R       T       L         A       A    K     K   E
 AAAAAAAAA    L          D       D   EEEEE       RRRRRRR       T       L         AAAAAAAAA    KKKKKK    EEEE
 A       A    L          D       D   E           R   R         T       L         A       A    K   K     E
 A       A    LLLLLLL    DDDDDDD     EEEEEEE     R    R        T       LLLLLLL   A       A    K    K    EEEEEEE
 ______________________________________________________________________________________________________________
*/

#include "Hardware_FrameworkBPLibrary.h"
#include "Runtime/Core/Public/Windows/WindowsPlatformMisc.h"

// Windows-specific includes with proper defines
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include <dxgi1_4.h>  // For IDXGIAdapter3
#include <XInput.h>
#include <winreg.h>
#include <Psapi.h>
#include <thread>
#include <vector>
#include "Windows/HideWindowsPlatformTypes.h"
#include <d3d12.h>

// Windows version definitions
#define NTDDI_VERSION NTDDI_WIN10
#define _WIN32_WINNT _WIN32_WINNT_WIN10

// Memory Information
void USystemInfoBPLibrary::GetMemoryInfo(int32& TotalPhysicalMB, int32& UsedPhysicalMB, int32& FreePhysicalMB,
    int32& TotalVirtualMB, int32& UsedVirtualMB, int32& FreeVirtualMB)
{
#if PLATFORM_WINDOWS
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    TotalPhysicalMB = static_cast<int32>(memInfo.ullTotalPhys / (1024 * 1024));
    FreePhysicalMB = static_cast<int32>(memInfo.ullAvailPhys / (1024 * 1024));
    UsedPhysicalMB = TotalPhysicalMB - FreePhysicalMB;

    TotalVirtualMB = static_cast<int32>(memInfo.ullTotalVirtual / (1024 * 1024));
    FreeVirtualMB = static_cast<int32>(memInfo.ullAvailVirtual / (1024 * 1024));
    UsedVirtualMB = TotalVirtualMB - FreeVirtualMB;
#else
    TotalPhysicalMB = UsedPhysicalMB = FreePhysicalMB = 0;
    TotalVirtualMB = UsedVirtualMB = FreeVirtualMB = 0;
#endif
}

// CPU Information
void USystemInfoBPLibrary::GetCPUInfo(FString& Name, FString& Manufacturer, int32& Cores, int32& Threads)
{
#if PLATFORM_WINDOWS
    Name = TEXT("Unknown");
    Manufacturer = TEXT("Unknown");
    Cores = Threads = 0;

    // Get CPU name from registry
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
        0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR Buffer[512];
        DWORD BufferSize = sizeof(Buffer);
        if (RegQueryValueEx(hKey, TEXT("ProcessorNameString"), NULL, NULL,
            (LPBYTE)Buffer, &BufferSize) == ERROR_SUCCESS)
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
        if (GetLogicalProcessorInformationEx(RelationProcessorCore,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &length))
        {
            Cores = 0;
            DWORD offset = 0;
            while (offset < length)
            {
                auto info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
                    buffer.data() + offset);
                if (info->Relationship == RelationProcessorCore)
                {
                    Cores++;
                }
                offset += info->Size;
            }
        }
    }

    // Get manufacturer
    Manufacturer = FPlatformMisc::GetCPUVendor();
#else
    Name = Manufacturer = TEXT("Unsupported Platform");
    Cores = Threads = 0;
#endif
}

// GPU Information
void USystemInfoBPLibrary::GetGPUInfo(FString& Name, FString& Manufacturer, int32& TotalVRAMMB,
    int32& UsedVRAMMB, int32& FreeVRAMMB)
{
#if PLATFORM_WINDOWS
    Name = TEXT("Unknown");
    Manufacturer = TEXT("Unknown");
    TotalVRAMMB = UsedVRAMMB = FreeVRAMMB = 0;

    IDXGIFactory* pFactory = nullptr;
    if (CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory) == S_OK)
    {
        IDXGIAdapter* pAdapter = nullptr;
        if (pFactory->EnumAdapters(0, &pAdapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC desc;
            if (pAdapter->GetDesc(&desc) == S_OK)
            {
                Name = desc.Description;
                TotalVRAMMB = static_cast<int32>(desc.DedicatedVideoMemory / (1024 * 1024));

                switch (desc.VendorId)
                {
                case 0x10DE: Manufacturer = TEXT("NVIDIA"); break;
                case 0x1002: Manufacturer = TEXT("AMD"); break;
                case 0x8086: Manufacturer = TEXT("Intel"); break;
                default: Manufacturer = TEXT("Unknown"); break;
                }

                // Query VRAM usage
                IDXGIAdapter3* pAdapter3 = nullptr;
                if (SUCCEEDED(pAdapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&pAdapter3)))
                {
                    DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo;
                    if (SUCCEEDED(pAdapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo)))
                    {
                        UsedVRAMMB = static_cast<int32>(memoryInfo.CurrentUsage / (1024 * 1024));
                        FreeVRAMMB = static_cast<int32>((memoryInfo.Budget - memoryInfo.CurrentUsage) / (1024 * 1024));
                    }
                    pAdapter3->Release();
                }
            }
            pAdapter->Release();
        }
        pFactory->Release();
    }
#else
    Name = Manufacturer = TEXT("Unsupported Platform");
    TotalVRAMMB = UsedVRAMMB = FreeVRAMMB = 0;
#endif
}

// Input Devices
void USystemInfoBPLibrary::GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard)
{
#if PLATFORM_WINDOWS
    // Gamepad detection
    XINPUT_STATE state;
    HasGamepad = (XInputGetState(0, &state) == ERROR_SUCCESS);

    // Mouse detection
    HasMouse = (GetSystemMetrics(SM_MOUSEPRESENT) != 0);

    // Keyboard detection
    HasKeyboard = true;  // Windows always has virtual keyboard
#else
    HasGamepad = HasMouse = HasKeyboard = false;
#endif
}