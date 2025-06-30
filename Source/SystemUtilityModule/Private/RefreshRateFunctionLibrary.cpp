// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "RefreshRateFunctionLibrary.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"

#include "Windows/WindowsWindow.h"
#include "Windows/WindowsHWrapper.h"

static HMONITOR GetGameMonitorHandle()
{
    // Get the active top-level window from Slate
    TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
    if (!ActiveWindow.IsValid())
    {
        return nullptr;
    }

    // Get the native window handle
    TSharedPtr<FGenericWindow> NativeWindow = ActiveWindow->GetNativeWindow();
    if (!NativeWindow.IsValid())
    {
        return nullptr;
    }

    HWND WindowHandle = static_cast<HWND>(NativeWindow->GetOSWindowHandle());
    if (!WindowHandle)
    {
        return nullptr;
    }

    // Get the monitor hosting the game window
    return MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTOPRIMARY);
}

static FString GetMonitorDeviceName(HMONITOR MonitorHandle)
{
    if (!MonitorHandle)
    {
        return FString();
    }

    MONITORINFOEX MonitorInfo = { 0 };
    MonitorInfo.cbSize = sizeof(MONITORINFOEX);
    if (GetMonitorInfo(MonitorHandle, &MonitorInfo))
    {
        return FString(MonitorInfo.szDevice);
    }
    return FString();
}

TArray<FIntPoint> URefreshRateFunctionLibrary::GetSupportedDisplayResolutions()
{
    TArray<FIntPoint> Resolutions;

    // Get the monitor where the game is running
    HMONITOR MonitorHandle = GetGameMonitorHandle();
    if (!MonitorHandle)
    {
        // Fallback to primary monitor
        Resolutions.Add(FIntPoint(1920, 1080)); // Default resolution
        return Resolutions;
    }

    // Get the monitor's device name
    FString DeviceName = GetMonitorDeviceName(MonitorHandle);
    if (DeviceName.IsEmpty())
    {
        Resolutions.Add(FIntPoint(1920, 1080)); // Default resolution
        return Resolutions;
    }

    // Enumerate all display settings for the specific monitor
    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;

    while (EnumDisplaySettings(*DeviceName, ModeIndex++, &DevMode))
    {
        FIntPoint Resolution(DevMode.dmPelsWidth, DevMode.dmPelsHeight);
        if (!Resolutions.Contains(Resolution)) // Avoid duplicates
        {
            Resolutions.Add(Resolution);
        }
    }

    // Sort resolutions by total pixels (optional, for consistency)
    Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) {
        return (A.X * A.Y) < (B.X * B.Y);
        });

    // If no resolutions found, add a default
    if (Resolutions.Num() == 0)
    {
        Resolutions.Add(FIntPoint(1920, 1080));
    }

    return Resolutions;
}

FIntPoint URefreshRateFunctionLibrary::GetNativeResolution()
{
    FIntPoint NativeResolution(1920, 1080); // Default fallback

    // Get the monitor where the game is running
    HMONITOR MonitorHandle = GetGameMonitorHandle();
    if (!MonitorHandle)
    {
        return NativeResolution;
    }

    // Get the monitor's device name
    FString DeviceName = GetMonitorDeviceName(MonitorHandle);
    if (DeviceName.IsEmpty())
    {
        return NativeResolution;
    }

    // Query the native resolution (recommended) for the specific monitor
    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    if (EnumDisplaySettings(*DeviceName, ENUM_REGISTRY_SETTINGS, &DevMode))
    {
        NativeResolution.X = DevMode.dmPelsWidth;
        NativeResolution.Y = DevMode.dmPelsHeight;
    }
    else
    {
        // Fallback: Use the highest supported resolution
        TArray<FIntPoint> Resolutions = GetSupportedDisplayResolutions();
        if (Resolutions.Num() > 0)
        {
            NativeResolution = Resolutions.Last(); // Highest is typically native
        }
    }

    return NativeResolution;
}



int32 URefreshRateFunctionLibrary::GetCurrentRefreshRate()
{
#if PLATFORM_WINDOWS
    HMONITOR MonitorHandle = GetGameMonitorHandle();
    if (!MonitorHandle)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetCurrentRefreshRate: Failed to get monitor handle."));
        return 0; // Fallback if monitor handle is invalid
    }

    FString DeviceName = GetMonitorDeviceName(MonitorHandle);
    if (DeviceName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("GetCurrentRefreshRate: Failed to get monitor device name."));
        return 0;
    }

    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    if (EnumDisplaySettings(*DeviceName, ENUM_CURRENT_SETTINGS, &DevMode) && DevMode.dmDisplayFrequency > 0)
    {
        return DevMode.dmDisplayFrequency;
    }

    UE_LOG(LogTemp, Warning, TEXT("GetCurrentRefreshRate: Failed to retrieve refresh rate for monitor %s."), *DeviceName);
    return 0; // Fallback if retrieval fails
#else
    UE_LOG(LogTemp, Warning, TEXT("GetCurrentRefreshRate is only supported on Windows."));
    return 0; // Non-Windows platforms not supported
#endif
}

bool URefreshRateFunctionLibrary::SetRefreshRate(int32 NewRefreshRate)
{
#if PLATFORM_WINDOWS
    HMONITOR MonitorHandle = GetGameMonitorHandle();
    if (!MonitorHandle)
    {
        UE_LOG(LogTemp, Error, TEXT("SetRefreshRate: Failed to get monitor handle."));
        return false;
    }

    FString DeviceName = GetMonitorDeviceName(MonitorHandle);
    if (DeviceName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("SetRefreshRate: Failed to get monitor device name."));
        return false;
    }

    // Get current display settings to preserve resolution
    DEVMODE CurrentMode = { 0 };
    CurrentMode.dmSize = sizeof(DEVMODE);
    if (!EnumDisplaySettings(*DeviceName, ENUM_CURRENT_SETTINGS, &CurrentMode))
    {
        UE_LOG(LogTemp, Error, TEXT("SetRefreshRate: Failed to retrieve current display settings for monitor %s."), *DeviceName);
        return false;
    }

    int32 CurrentWidth = CurrentMode.dmPelsWidth;
    int32 CurrentHeight = CurrentMode.dmPelsHeight;

    // Find a display mode matching the current resolution and desired refresh rate
    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;
    bool bFound = false;

    while (EnumDisplaySettings(*DeviceName, ModeIndex, &DevMode))
    {
        if (DevMode.dmPelsWidth == (DWORD)CurrentWidth &&
            DevMode.dmPelsHeight == (DWORD)CurrentHeight &&
            DevMode.dmDisplayFrequency == (DWORD)NewRefreshRate)
        {
            bFound = true;
            break;
        }
        ModeIndex++;
    }

    if (!bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetRefreshRate: No display mode found for %dx%d at %d Hz on monitor %s."),
            CurrentWidth, CurrentHeight, NewRefreshRate, *DeviceName);
        return false;
    }

    // Set only the refresh rate field
    DevMode.dmFields = DM_DISPLAYFREQUENCY;
    DevMode.dmDisplayFrequency = NewRefreshRate;

    // Apply the change to the specific monitor
    LONG Result = ChangeDisplaySettingsEx(*DeviceName, &DevMode, NULL, CDS_FULLSCREEN, NULL);
    if (Result == DISP_CHANGE_SUCCESSFUL)
    {
        UE_LOG(LogTemp, Log, TEXT("SetRefreshRate: Successfully set refresh rate to %d Hz on monitor %s."), NewRefreshRate, *DeviceName);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SetRefreshRate: Failed to set refresh rate to %d Hz on monitor %s. Error code: %d"),
            NewRefreshRate, *DeviceName, Result);
        return false;
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("SetRefreshRate is only supported on Windows."));
    return false;
#endif
}

TArray<int32> URefreshRateFunctionLibrary::GetSupportedRefreshRates()
{
    TArray<int32> SupportedRates;

#if PLATFORM_WINDOWS
    HMONITOR MonitorHandle = GetGameMonitorHandle();
    if (!MonitorHandle)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetSupportedRefreshRates: Failed to get monitor handle."));
        return SupportedRates; // Return empty array
    }

    FString DeviceName = GetMonitorDeviceName(MonitorHandle);
    if (DeviceName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("GetSupportedRefreshRates: Failed to get monitor device name."));
        return SupportedRates;
    }

    // Get current resolution
    DEVMODE CurrentMode = { 0 };
    CurrentMode.dmSize = sizeof(DEVMODE);
    if (!EnumDisplaySettings(*DeviceName, ENUM_CURRENT_SETTINGS, &CurrentMode))
    {
        UE_LOG(LogTemp, Error, TEXT("GetSupportedRefreshRates: Failed to retrieve current display settings for monitor %s."), *DeviceName);
        return SupportedRates;
    }

    int32 CurrentWidth = CurrentMode.dmPelsWidth;
    int32 CurrentHeight = CurrentMode.dmPelsHeight;

    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;
    TSet<int32> UniqueRates;

    // Enumerate display modes and collect refresh rates for the current resolution
    while (EnumDisplaySettings(*DeviceName, ModeIndex, &DevMode))
    {
        if (DevMode.dmPelsWidth == (DWORD)CurrentWidth && DevMode.dmPelsHeight == (DWORD)CurrentHeight)
        {
            UniqueRates.Add(DevMode.dmDisplayFrequency);
        }
        ModeIndex++;
    }

    // Convert to sorted array
    SupportedRates = UniqueRates.Array();
    SupportedRates.Sort();
    UE_LOG(LogTemp, Log, TEXT("GetSupportedRefreshRates: Found %d supported refresh rates for %dx%d on monitor %s."),
        SupportedRates.Num(), CurrentWidth, CurrentHeight, *DeviceName);
#endif

    return SupportedRates;
}

