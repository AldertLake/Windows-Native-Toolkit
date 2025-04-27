/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#include "OpenApps.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <shellapi.h>
#include <tlhelp32.h>
#include "Misc/Paths.h"

bool UOpenApps::OpenApps(const FString& ExePath)
{
    if (ExePath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("OpenApps: Empty executable path provided"));
        return false;
    }

    std::wstring WidePath = std::wstring(*ExePath);

    HINSTANCE Result = ShellExecuteW(
        NULL,           // No parent window
        L"open",        // Operation to perform
        WidePath.c_str(), // Path to executable
        NULL,           // No parameters
        NULL,           // Default directory
        SW_SHOWNORMAL   // Show window normally
    );

    bool bSuccess = reinterpret_cast<intptr_t>(Result) > 32;

    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open application: %s"), *ExePath);
    }

    return bSuccess;
}

bool UOpenApps::IsAppRunning(const FString& ExePath)
{
    if (ExePath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("IsAppRunning: Empty executable path provided"));
        return false;
    }

    FString FileName = FPaths::GetCleanFilename(ExePath);
    std::wstring WideFileName = std::wstring(*FileName);

    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    PROCESSENTRY32W ProcessEntry = { 0 };
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32W);

    bool bIsRunning = false;
    if (Process32FirstW(Snapshot, &ProcessEntry))
    {
        do
        {
            if (_wcsicmp(ProcessEntry.szExeFile, WideFileName.c_str()) == 0)
            {
                bIsRunning = true;
                break;
            }
        } while (Process32NextW(Snapshot, &ProcessEntry));
    }

    CloseHandle(Snapshot);
    return bIsRunning;
}

#include "Windows/HideWindowsPlatformTypes.h"