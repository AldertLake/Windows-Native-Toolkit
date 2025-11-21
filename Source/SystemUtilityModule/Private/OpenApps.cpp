// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "OpenApps.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h" 
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include "Windows/HideWindowsPlatformTypes.h"
struct FHandlePtr
{
    HANDLE Handle = nullptr;
    FHandlePtr() = default;
    explicit FHandlePtr(HANDLE InHandle) : Handle(InHandle) {}
    ~FHandlePtr() { if (Handle && Handle != INVALID_HANDLE_VALUE) CloseHandle(Handle); }
    HANDLE* operator&() { return &Handle; }
    bool IsValid() const { return Handle != nullptr && Handle != INVALID_HANDLE_VALUE; }
};
#endif

bool UOpenApps::OpenApplication(const FString& ExePath)
{
#if PLATFORM_WINDOWS
    if (ExePath.IsEmpty())
    {
        return false;
    }

    // Validate file existence
    FString NormalizedPath = FPaths::ConvertRelativePathToFull(ExePath);
    if (!FPaths::FileExists(NormalizedPath))
    {
        return false;
    }

    // Use TCHAR for wide-character path
    const TCHAR* WidePath = *NormalizedPath;

    HINSTANCE Result = ShellExecuteW(
        nullptr,        // No parent window
        L"open",       // Operation to perform
        WidePath,      // Path to executable
        nullptr,       // No parameters
        nullptr,       // Default directory
        SW_SHOWNORMAL  // Show window normally
    );

    return reinterpret_cast<intptr_t>(Result) > 32;
#else
    return false; // Non-Windows platforms
#endif
}

bool UOpenApps::IsAppRunning(const FString& ExePath)
{
#if PLATFORM_WINDOWS
    if (ExePath.IsEmpty())
    {
        return false;
    }

    // Extract filename for matching
    FString FileName = FPaths::GetCleanFilename(ExePath);
    const TCHAR* WideFileName = *FileName;

    FHandlePtr Snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!Snapshot.IsValid())
    {
        return false;
    }

    PROCESSENTRY32W ProcessEntry = { 0 };
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32W);

    bool bIsRunning = false;
    if (Process32FirstW(Snapshot.Handle, &ProcessEntry))
    {
        do
        {
            if (_wcsicmp(ProcessEntry.szExeFile, WideFileName) == 0)
            {
                bIsRunning = true;
                break; // Early exit on match
            }
        } while (Process32NextW(Snapshot.Handle, &ProcessEntry));
    }

    return bIsRunning;
#else
    return false; // Non-Windows platforms
#endif
}

static int32 FindChildProcess(int32 ParentPID)
{
#if PLATFORM_WINDOWS
    int32 FoundPID = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W ProcessEntry;
        ProcessEntry.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &ProcessEntry))
        {
            do
            {
                // Check if this process was created by our ParentPID
                if (ProcessEntry.th32ParentProcessID == static_cast<DWORD>(ParentPID))
                {
                    FoundPID = static_cast<int32>(ProcessEntry.th32ProcessID);

                    // Heuristic: We usually want the child that is DIFFERENT from the parent
                    // but in most launcher cases, it's the only child.
                    break;
                }
            } while (Process32NextW(hSnapshot, &ProcessEntry));
        }
        CloseHandle(hSnapshot);
    }
    return FoundPID;
#else
    return 0;
#endif
}

// --- INTERNAL HELPER: Recursive Tree Kill ---
static void RecursiveKill(int32 PID)
{
#if PLATFORM_WINDOWS
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W ProcessEntry;
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32W);

    // 1. Find all children of this PID
    if (Process32FirstW(hSnapshot, &ProcessEntry))
    {
        do
        {
            if (ProcessEntry.th32ParentProcessID == static_cast<DWORD>(PID))
            {
                // Recursively kill the child's children first
                RecursiveKill(static_cast<int32>(ProcessEntry.th32ProcessID));
            }
        } while (Process32NextW(hSnapshot, &ProcessEntry));
    }
    CloseHandle(hSnapshot);

    // 2. Kill the Target PID
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, false, static_cast<DWORD>(PID));
    if (hProcess)
    {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
    }
#endif
}

int32 UOpenApps::LaunchExternalProcess(const FString& ExePath, const FString& Arguments, bool bHidden)
{
    if (ExePath.IsEmpty()) return 0;

    FString CleanPath = FPaths::ConvertRelativePathToFull(ExePath);
    if (!FPaths::FileExists(CleanPath)) return 0;

    // 1. Launch Initial Process
    uint32 InitialPID = 0;
    FProcHandle Handle = FPlatformProcess::CreateProc(
        *CleanPath, *Arguments, true, bHidden, bHidden, &InitialPID, 0, nullptr, nullptr
    );

    if (!Handle.IsValid()) return 0;
    FPlatformProcess::CloseProc(Handle);



    int32 BestPID = static_cast<int32>(InitialPID);

    for (int i = 0; i < 10; i++)
    {
        FPlatformProcess::Sleep(0.05f); // Wait 50ms

        // Check if a child was born
        int32 ChildPID = FindChildProcess(BestPID);

        if (ChildPID > 0)
        {

            BestPID = ChildPID;
        }

        // Check if current BestPID is dead
        if (!IsProcessRunning(BestPID))
        {

            if (ChildPID > 0)
            {
                BestPID = ChildPID;
            }
            else
            {

            }
        }
    }

    return BestPID;
}

bool UOpenApps::IsProcessRunning(int32 ProcessID)
{
    if (ProcessID <= 0) return false;

#if PLATFORM_WINDOWS
    // Use QUERY_LIMITED_INFORMATION for maximum compatibility
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, static_cast<DWORD>(ProcessID));

    if (hProcess == NULL) hProcess = OpenProcess(SYNCHRONIZE, false, static_cast<DWORD>(ProcessID));

    if (hProcess != NULL)
    {
        DWORD ExitCode = 0;
        if (GetExitCodeProcess(hProcess, &ExitCode))
        {
            CloseHandle(hProcess);
            return (ExitCode == STILL_ACTIVE);
        }
        CloseHandle(hProcess);
    }
#endif
    return false;
}

bool UOpenApps::KillProcessTree(int32 ProcessID)
{
    if (ProcessID <= 0) return false;

#if PLATFORM_WINDOWS
    // Instead of just killing one, we kill the family tree.
    RecursiveKill(ProcessID);

    // Double check if it's actually dead
    return !IsProcessRunning(ProcessID);
#else
    return false;
#endif
}
