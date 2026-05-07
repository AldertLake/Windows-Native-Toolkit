// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "OpenApps.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
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

                if (ProcessEntry.th32ParentProcessID == static_cast<DWORD>(ParentPID))
                {
                    FoundPID = static_cast<int32>(ProcessEntry.th32ProcessID);



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


static void BuildProcessTreeAndKill(int32 RootPID)
{
#if PLATFORM_WINDOWS
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    TMultiMap<int32, int32> ParentToChildren;

    PROCESSENTRY32W ProcessEntry;
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &ProcessEntry))
    {
        do
        {
            ParentToChildren.Add(static_cast<int32>(ProcessEntry.th32ParentProcessID), static_cast<int32>(ProcessEntry.th32ProcessID));
        } while (Process32NextW(hSnapshot, &ProcessEntry));
    }
    CloseHandle(hSnapshot);


    TArray<int32> ToKill;
    ToKill.Add(RootPID);

    for (int32 i = 0; i < ToKill.Num(); ++i)
    {
        int32 CurrentPID = ToKill[i];
        TArray<int32> Children;
        ParentToChildren.MultiFind(CurrentPID, Children);
        ToKill.Append(Children);
    }


    for (int32 i = ToKill.Num() - 1; i >= 0; --i)
    {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, false, static_cast<DWORD>(ToKill[i]));
        if (hProcess)
        {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
    }
#endif
}

int32 UOpenApps::LaunchExternalProcess(const FString& ExePath, const FString& Arguments, bool bHidden)
{
    if (ExePath.IsEmpty()) return 0;

    FString CleanPath = FPaths::ConvertRelativePathToFull(ExePath);
    if (!FPaths::FileExists(CleanPath)) return 0;

    int32 BestPID = 0;

#if PLATFORM_WINDOWS
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = bHidden ? SW_HIDE : SW_SHOWNORMAL;
    ZeroMemory(&pi, sizeof(pi));

    FString CmdLine = FString::Printf(TEXT("\"%s\" %s"), *CleanPath, *Arguments);

    if (CreateProcessW(nullptr, CmdLine.GetCharArray().GetData(), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        BestPID = static_cast<int32>(pi.dwProcessId);
    }
#else
    uint32 InitialPID = 0;
    FProcHandle Handle = FPlatformProcess::CreateProc(
        *CleanPath, *Arguments, true, bHidden, bHidden, &InitialPID, 0, nullptr, nullptr
    );
    if (Handle.IsValid())
    {
        FPlatformProcess::CloseProc(Handle);
        BestPID = static_cast<int32>(InitialPID);
    }
#endif

    if (BestPID > 0)
    {
        for (int i = 0; i < 10; i++)
        {
            FPlatformProcess::Sleep(0.05f);


            int32 ChildPID = FindChildProcess(BestPID);

            if (ChildPID > 0)
            {
                BestPID = ChildPID;
            }


            if (!IsProcessRunning(BestPID))
            {
                if (ChildPID > 0)
                {
                    BestPID = ChildPID;
                }
            }
        }
    }

    return BestPID;
}

bool UOpenApps::IsProcessRunning(int32 ProcessID)
{
    if (ProcessID <= 0) return false;

#if PLATFORM_WINDOWS

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

    BuildProcessTreeAndKill(ProcessID);


    return !IsProcessRunning(ProcessID);
#else
    return false;
#endif
}

#if PLATFORM_WINDOWS
struct FEnumWindowsData
{
    DWORD TargetProcessId;
    HWND FoundHwnd;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    FEnumWindowsData* Data = reinterpret_cast<FEnumWindowsData*>(lParam);
    DWORD ProcessId = 0;
    GetWindowThreadProcessId(hwnd, &ProcessId);

    if (ProcessId == Data->TargetProcessId)
    {
        if (IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == NULL)
        {
            Data->FoundHwnd = hwnd;
            return false;
        }
    }
    return true;
}
#endif

bool UOpenApps::BringAppToFront(int32 ProcessID)
{
#if PLATFORM_WINDOWS
    if (ProcessID <= 0) return false;

    FEnumWindowsData Data;
    Data.TargetProcessId = static_cast<DWORD>(ProcessID);
    Data.FoundHwnd = NULL;

    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&Data));

    if (Data.FoundHwnd != NULL)
    {
        if (IsIconic(Data.FoundHwnd))
        {
            ShowWindow(Data.FoundHwnd, SW_RESTORE);
        }
        SetForegroundWindow(Data.FoundHwnd);
        return true;
    }
#endif
    return false;
}


