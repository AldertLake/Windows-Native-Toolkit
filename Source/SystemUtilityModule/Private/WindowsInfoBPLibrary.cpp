// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "WindowsInfoBPLibrary.h"
#include "HAL/PlatformProcess.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsPlatformMisc.h"
#include <windows.h>
#include <lmcons.h>
#include <winreg.h>
#include "Windows/HideWindowsPlatformTypes.h"

struct FRegistryKeyPtr
{
    HKEY Key = nullptr;
    FRegistryKeyPtr() = default;
    explicit FRegistryKeyPtr(HKEY InKey) : Key(InKey) {}
    ~FRegistryKeyPtr() { if (Key) RegCloseKey(Key); }
    FRegistryKeyPtr(const FRegistryKeyPtr&) = delete;
    FRegistryKeyPtr& operator=(const FRegistryKeyPtr&) = delete;
    HKEY* operator&() { return &Key; }
    bool IsValid() const { return Key != nullptr; }
};
#endif

FString UWindowsInfoBPLibrary::GetWindowsVersion()
{
#if PLATFORM_WINDOWS

    typedef LONG(WINAPI* RtlGetVersionPtr)(POSVERSIONINFOW);
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod)
    {
#pragma warning(push)
#pragma warning(disable: 4191)
        RtlGetVersionPtr GetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(hMod, "RtlGetVersion"));
#pragma warning(pop)

        if (GetVersion)
        {
            OSVERSIONINFOW ROVI = { 0 };
            ROVI.dwOSVersionInfoSize = sizeof(ROVI);
            if (GetVersion(&ROVI) == 0)
            {
                if (ROVI.dwMajorVersion == 10 && ROVI.dwBuildNumber >= 22000)
                {
                    return TEXT("Windows 11");
                }
                else if (ROVI.dwMajorVersion == 10)
                {
                    return TEXT("Windows 10");
                }
                else if (ROVI.dwMajorVersion == 6)
                {
                    if (ROVI.dwMinorVersion == 3) return TEXT("Windows 8.1");
                    if (ROVI.dwMinorVersion == 2) return TEXT("Windows 8");
                    if (ROVI.dwMinorVersion == 1) return TEXT("Windows 7");
                }
            }
        }
    }


    FString OSVersion = FPlatformMisc::GetOSVersion();
    if (OSVersion.Contains(TEXT("Windows 11"))) return TEXT("Windows 11");
    if (OSVersion.Contains(TEXT("Windows 10"))) return TEXT("Windows 10");

    return OSVersion.IsEmpty() ? TEXT("Unknown") : OSVersion;
#else
    return FPlatformMisc::GetOSVersion();
#endif
}

FString UWindowsInfoBPLibrary::GetWindowsBuild()
{
#if PLATFORM_WINDOWS
    FRegistryKeyPtr hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR Buffer[32] = { 0 };
        DWORD Size = sizeof(Buffer) - sizeof(TCHAR);
        FString BuildNumber;
        if (RegQueryValueEx(hKey.Key, TEXT("CurrentBuildNumber"), nullptr, nullptr, reinterpret_cast<LPBYTE>(Buffer), &Size) == ERROR_SUCCESS)
        {
            Buffer[31] = 0;
            BuildNumber = Buffer;
        }

        uint32 UBR = 0;
        Size = sizeof(DWORD);
        if (RegQueryValueEx(hKey.Key, TEXT("UBR"), nullptr, nullptr, reinterpret_cast<LPBYTE>(&UBR), &Size) == ERROR_SUCCESS)
        {
            return BuildNumber + TEXT(".") + FString::FromInt(UBR);
        }
        return BuildNumber.IsEmpty() ? TEXT("Unknown") : BuildNumber;
    }
    return TEXT("Unknown");
#else
    return TEXT("Unknown");
#endif
}

FString UWindowsInfoBPLibrary::GetWindowsEdition()
{
#if PLATFORM_WINDOWS
    FString Edition = ReadRegistryString(TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), TEXT("EditionID"));
    return Edition.IsEmpty() ? TEXT("Unknown") : Edition;
#else
    return TEXT("Unknown");
#endif
}

FString UWindowsInfoBPLibrary::GetPCName()
{
    FString ComputerName = FPlatformProcess::ComputerName();
    return ComputerName.IsEmpty() ? TEXT("Unknown") : ComputerName;
}

FString UWindowsInfoBPLibrary::GetLocalUserName()
{
    FString UserName = FPlatformProcess::UserName();
    return UserName.IsEmpty() ? TEXT("Unknown") : UserName;
}

float UWindowsInfoBPLibrary::GetTotalSystemMemoryGB()
{
#if PLATFORM_WINDOWS
    MEMORYSTATUSEX MemStatus;
    MemStatus.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&MemStatus))
    {
        return static_cast<float>(MemStatus.ullTotalPhys) / (1024.0f * 1024.0f * 1024.0f);
    }
#endif
    return 0.0f;
}

FString UWindowsInfoBPLibrary::ReadRegistryString(const FString& KeyPath, const FString& ValueName, bool bLocalMachine)
{
#if PLATFORM_WINDOWS
    FString Result;
    HKEY RootKey = bLocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    FRegistryKeyPtr hKey;

    if (RegOpenKeyEx(RootKey, *KeyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD Type, Size;
        if (RegQueryValueEx(hKey.Key, *ValueName, nullptr, &Type, nullptr, &Size) == ERROR_SUCCESS && Type == REG_SZ)
        {
            TArray<TCHAR> Buffer;
            Buffer.SetNumZeroed((Size / sizeof(TCHAR)) + 1);

            if (RegQueryValueEx(hKey.Key, *ValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(Buffer.GetData()), &Size) == ERROR_SUCCESS && Buffer.Num() > 0 && Buffer[0])
            {
                Result = Buffer.GetData();
                Result.TrimEndInline();
            }
        }
    }
    return Result;
#else
    return FString();
#endif
}

uint32 UWindowsInfoBPLibrary::ReadRegistryDWORD(const FString& KeyPath, const FString& ValueName, bool bLocalMachine)
{
#if PLATFORM_WINDOWS
    DWORD Value = 0;
    HKEY RootKey = bLocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    FRegistryKeyPtr hKey;

    if (RegOpenKeyEx(RootKey, *KeyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD Size = sizeof(DWORD);
        if (RegQueryValueEx(hKey.Key, *ValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(&Value), &Size) != ERROR_SUCCESS)
        {
            Value = 0;
        }
    }
    return Value;
#else
    return 0;
#endif
}


