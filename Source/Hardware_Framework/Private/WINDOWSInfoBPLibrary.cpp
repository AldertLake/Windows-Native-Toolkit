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

#include "WINDOWSInfoBPLibrary.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsPlatformMisc.h"
#include <Lmcons.h>
#include <winreg.h>
#include "Windows/HideWindowsPlatformTypes.h"

FString UWINDOWSInfoBPLibrary::GetWindowsVersion()
{
    FString VersionName;

    // Use Unreal's built-in version detection
    FString OSVersion = FPlatformMisc::GetOSVersion();

    if (OSVersion.Contains(TEXT("Windows 11")))
    {
        VersionName = TEXT("Windows 11");
    }
    else if (OSVersion.Contains(TEXT("Windows 10")))
    {
        VersionName = TEXT("Windows 10");
    }
    else
    {
        // Fallback to registry for older versions
        const uint32 MajorVersion = ReadRegistryDWORD(
            TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
            TEXT("CurrentMajorVersionNumber")
        );

        if (MajorVersion == 6)
        {
            const uint32 MinorVersion = ReadRegistryDWORD(
                TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                TEXT("CurrentMinorVersionNumber")
            );

            if (MinorVersion == 1) VersionName = TEXT("Windows 7");
            else if (MinorVersion == 2) VersionName = TEXT("Windows 8");
            else if (MinorVersion == 3) VersionName = TEXT("Windows 8.1");
        }
    }

    return VersionName.IsEmpty() ? OSVersion : VersionName;
}

FString UWINDOWSInfoBPLibrary::GetWindowsBuild()
{
    return ReadRegistryString(
        TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        TEXT("CurrentBuildNumber")
    ) + TEXT(".") + FString::FromInt(ReadRegistryDWORD(
        TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        TEXT("UBR")
    ));
}

FString UWINDOWSInfoBPLibrary::GetWindowsEdition()
{
    return ReadRegistryString(
        TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        TEXT("EditionID")
    );
}

FString UWINDOWSInfoBPLibrary::GetPCName()
{
    return FPlatformProcess::ComputerName();
}

FString UWINDOWSInfoBPLibrary::GetLocalUserName()
{
    return FPlatformProcess::UserName();
}

FString UWINDOWSInfoBPLibrary::ReadRegistryString(const FString& KeyPath, const FString& ValueName, bool bLocalMachine)
{
    FString Result;
    HKEY RootKey = bLocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    HKEY hKey;

    if (RegOpenKeyEx(RootKey, *KeyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD Type, Size;
        if (RegQueryValueEx(hKey, *ValueName, nullptr, &Type, nullptr, &Size) == ERROR_SUCCESS && Type == REG_SZ)
        {
            TArray<TCHAR> Buffer;
            Buffer.SetNum(Size / sizeof(TCHAR));
            if (RegQueryValueEx(hKey, *ValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(Buffer.GetData()), &Size) == ERROR_SUCCESS)
            {
                Result = FString(Buffer.GetData());
                Result.TrimEndInline();
            }
        }
        RegCloseKey(hKey);
    }
    return Result;
}

uint32 UWINDOWSInfoBPLibrary::ReadRegistryDWORD(const FString& KeyPath, const FString& ValueName, bool bLocalMachine)
{
    DWORD Value = 0;
    HKEY RootKey = bLocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    HKEY hKey;

    if (RegOpenKeyEx(RootKey, *KeyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD Size = sizeof(DWORD);
        RegQueryValueEx(hKey, *ValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(&Value), &Size);
        RegCloseKey(hKey);
    }
    return Value;
}