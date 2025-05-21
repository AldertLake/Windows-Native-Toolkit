#include "OpenApps.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <shellapi.h>
#include <tlhelp32.h>
#include "Windows/HideWindowsPlatformTypes.h"

// RAII wrapper for HANDLE
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

bool UOpenApps::OpenApps(const FString& ExePath)
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