// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "RefreshRateFunctionLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

static bool GetCurrentMonitorDeviceName(FString& OutDeviceName)
{
#if PLATFORM_WINDOWS
    if (!GEngine || !GEngine->GameViewport) return false;

    TSharedPtr<SWindow> WindowPtr = GEngine->GameViewport->GetWindow();
    if (!WindowPtr.IsValid()) return false;

    auto NativeWindow = WindowPtr->GetNativeWindow();
    if (!NativeWindow.IsValid()) return false;

    HWND WindowHandle = (HWND)NativeWindow->GetOSWindowHandle();
    if (!WindowHandle) return false;

    HMONITOR MonitorHandle = MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTONEAREST);
    if (!MonitorHandle) return false;

    MONITORINFOEX MonitorInfo;
    MonitorInfo.cbSize = sizeof(MONITORINFOEX);

    if (GetMonitorInfo(MonitorHandle, &MonitorInfo))
    {
        OutDeviceName = FString(MonitorInfo.szDevice);
        return !OutDeviceName.IsEmpty();
    }
#endif
    return false;
}

TArray<FIntPoint> URefreshRateFunctionLibrary::GetSupportedDisplayResolutions()
{
    TArray<FIntPoint> Resolutions;

#if PLATFORM_WINDOWS
    FString DeviceName;
    if (!GetCurrentMonitorDeviceName(DeviceName)) return Resolutions;

    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;

    TSet<FIntPoint> UniqueRes;

    while (EnumDisplaySettings(*DeviceName, ModeIndex++, &DevMode))
    {

        if (DevMode.dmBitsPerPel != 32) continue;

        FIntPoint Res(DevMode.dmPelsWidth, DevMode.dmPelsHeight);
        UniqueRes.Add(Res);
    }

    Resolutions = UniqueRes.Array();

    // Sort: Low to High
    Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) {
        return (A.X * A.Y) < (B.X * B.Y);
        });
#else
    // Fallback for non-Windows
    Resolutions.Add(FIntPoint(1920, 1080));
#endif

    return Resolutions;
}

FIntPoint URefreshRateFunctionLibrary::GetNativeResolution()
{
#if PLATFORM_WINDOWS
    FString DeviceName;
    if (GetCurrentMonitorDeviceName(DeviceName))
    {
        DEVMODE DevMode = { 0 };
        DevMode.dmSize = sizeof(DEVMODE);

        if (EnumDisplaySettings(*DeviceName, ENUM_REGISTRY_SETTINGS, &DevMode))
        {
            return FIntPoint(DevMode.dmPelsWidth, DevMode.dmPelsHeight);
        }
    }
#endif
    return FIntPoint(1920, 1080);
}

int32 URefreshRateFunctionLibrary::GetCurrentRefreshRate()
{
#if PLATFORM_WINDOWS
    FString DeviceName;
    if (GetCurrentMonitorDeviceName(DeviceName))
    {
        DEVMODE DevMode = { 0 };
        DevMode.dmSize = sizeof(DEVMODE);

        if (EnumDisplaySettings(*DeviceName, ENUM_CURRENT_SETTINGS, &DevMode))
        {
            return (int32)DevMode.dmDisplayFrequency;
        }
    }
#endif
    return 60; // Safe fallback
}

TArray<int32> URefreshRateFunctionLibrary::GetSupportedRefreshRates()
{
    TArray<int32> Rates;

#if PLATFORM_WINDOWS
    FString DeviceName;
    if (!GetCurrentMonitorDeviceName(DeviceName)) return Rates;


    DEVMODE CurrentMode = { 0 };
    CurrentMode.dmSize = sizeof(DEVMODE);
    if (!EnumDisplaySettings(*DeviceName, ENUM_CURRENT_SETTINGS, &CurrentMode)) return Rates;

    int32 TargetW = CurrentMode.dmPelsWidth;
    int32 TargetH = CurrentMode.dmPelsHeight;

    DEVMODE TempMode = { 0 };
    TempMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;
    TSet<int32> UniqueRates;

    while (EnumDisplaySettings(*DeviceName, ModeIndex++, &TempMode))
    {
        if (TempMode.dmBitsPerPel != 32) continue;

        if (TempMode.dmPelsWidth == TargetW && TempMode.dmPelsHeight == TargetH)
        {
            if (TempMode.dmDisplayFrequency > 1) 
            {
                UniqueRates.Add(TempMode.dmDisplayFrequency);
            }
        }
    }

    Rates = UniqueRates.Array();
    Rates.Sort();
#endif

    return Rates;
}

bool URefreshRateFunctionLibrary::SetRefreshRate(int32 NewRefreshRate)
{
#if PLATFORM_WINDOWS
    FString DeviceName;
    if (!GetCurrentMonitorDeviceName(DeviceName)) return false;


    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    if (!EnumDisplaySettings(*DeviceName, ENUM_CURRENT_SETTINGS, &DevMode)) return false;

    DevMode.dmDisplayFrequency = NewRefreshRate;
    DevMode.dmFields = DM_DISPLAYFREQUENCY; 

    LONG TestResult = ChangeDisplaySettingsEx(*DeviceName, &DevMode, NULL, CDS_TEST, NULL);
    if (TestResult != DISP_CHANGE_SUCCESSFUL)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetRefreshRate: Mode %d Hz not supported by monitor."), NewRefreshRate);
        return false;
    }

    LONG Result = ChangeDisplaySettingsEx(*DeviceName, &DevMode, NULL, CDS_FULLSCREEN, NULL);

    return (Result == DISP_CHANGE_SUCCESSFUL);
#else
    return false;
#endif
}