// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "WindowsInfoBPLibrary.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h" 
#include "Windows/WindowsPlatformMisc.h"
#include <lmcons.h>
#include <winreg.h>
#include "Windows/HideWindowsPlatformTypes.h"


// RAII wrapper for HKEY
struct FRegistryKeyPtr
{
    HKEY Key = nullptr;
    FRegistryKeyPtr() = default;
    explicit FRegistryKeyPtr(HKEY InKey) : Key(InKey) {}
    ~FRegistryKeyPtr() { if (Key) RegCloseKey(Key); }
    HKEY* operator&() { return &Key; }
    bool IsValid() const { return Key != nullptr; }
};

FString UWindowsInfoBPLibrary::GetWindowsVersion()
{
#if PLATFORM_WINDOWS
    // Use Unreal's built-in version detection first
    FString OSVersion = FPlatformMisc::GetOSVersion();
    if (OSVersion.Contains(TEXT("Windows 11")))
    {
        return TEXT("Windows 11");
    }
    if (OSVersion.Contains(TEXT("Windows 10")))
    {
        return TEXT("Windows 10");
    }

    // Fallback to registry for older versions
    FRegistryKeyPtr hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        uint32 MajorVersion = 0;
        DWORD Size = sizeof(DWORD);
        if (RegQueryValueEx(hKey.Key, TEXT("CurrentMajorVersionNumber"), nullptr, nullptr, reinterpret_cast<LPBYTE>(&MajorVersion), &Size) == ERROR_SUCCESS)
        {
            if (MajorVersion == 6)
            {
                uint32 MinorVersion = 0;
                Size = sizeof(DWORD);
                if (RegQueryValueEx(hKey.Key, TEXT("CurrentMinorVersionNumber"), nullptr, nullptr, reinterpret_cast<LPBYTE>(&MinorVersion), &Size) == ERROR_SUCCESS)
                {
                    if (MinorVersion == 1) return TEXT("Windows 7");
                    if (MinorVersion == 2) return TEXT("Windows 8");
                    if (MinorVersion == 3) return TEXT("Windows 8.1");
                }
            }
        }
    }
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
        TCHAR Buffer[32] = { 0 }; // Fixed-size buffer for small strings
        DWORD Size = sizeof(Buffer);
        FString BuildNumber;
        if (RegQueryValueEx(hKey.Key, TEXT("CurrentBuildNumber"), nullptr, nullptr, reinterpret_cast<LPBYTE>(Buffer), &Size) == ERROR_SUCCESS)
        {
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
            TCHAR Buffer[256] = { 0 }; // Fixed-size buffer for typical registry strings
            if (RegQueryValueEx(hKey.Key, *ValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(Buffer), &Size) == ERROR_SUCCESS && Buffer[0])
            {
                Result = Buffer;
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