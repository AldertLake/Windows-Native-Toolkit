#include "RefreshRateFunctionLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include <winuser.h>
#endif

int32 URefreshRateFunctionLibrary::GetCurrentRefreshRate()
{
#if PLATFORM_WINDOWS
    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &DevMode) && DevMode.dmDisplayFrequency > 0)
    {
        return static_cast<int32>(DevMode.dmDisplayFrequency);
    }
    return -1; // Indicate failure
#else
    return -1; // Non-Windows platforms
#endif
}

bool URefreshRateFunctionLibrary::SetRefreshRate(int32 NewRefreshRate, int32 Width, int32 Height)
{
#if PLATFORM_WINDOWS
    // Validate inputs
    if (NewRefreshRate <= 0 || (Width <= 0 && Width != -1) || (Height <= 0 && Height != -1))
    {
        return false;
    }

    // Get current display settings
    DEVMODE CurrentMode = { 0 };
    CurrentMode.dmSize = sizeof(DEVMODE);
    if (!EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &CurrentMode))
    {
        return false;
    }

    // Use current resolution if Width/Height are -1
    int32 TargetWidth = (Width == -1) ? CurrentMode.dmPelsWidth : Width;
    int32 TargetHeight = (Height == -1) ? CurrentMode.dmPelsHeight : Height;

    // Find matching display mode
    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;
    bool bFound = false;

    while (EnumDisplaySettings(nullptr, ModeIndex, &DevMode))
    {
        if (DevMode.dmPelsWidth == static_cast<DWORD>(TargetWidth) &&
            DevMode.dmPelsHeight == static_cast<DWORD>(TargetHeight) &&
            DevMode.dmDisplayFrequency == static_cast<DWORD>(NewRefreshRate))
        {
            bFound = true;
            break;
        }
        ModeIndex++;
    }

    if (!bFound)
    {
        return false;
    }

    // Set display settings
    DevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
    DevMode.dmPelsWidth = TargetWidth;
    DevMode.dmPelsHeight = TargetHeight;
    DevMode.dmDisplayFrequency = NewRefreshRate;

    LONG Result = ChangeDisplaySettings(&DevMode, CDS_UPDATEREGISTRY);
    return Result == DISP_CHANGE_SUCCESSFUL;
#else
    return false; // Non-Windows platforms
#endif
}

TArray<int32> URefreshRateFunctionLibrary::GetSupportedRefreshRates()
{
    TArray<int32> SupportedRates;

#if PLATFORM_WINDOWS
    // Get current display settings
    DEVMODE CurrentMode = { 0 };
    CurrentMode.dmSize = sizeof(DEVMODE);
    if (!EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &CurrentMode))
    {
        return SupportedRates; // Empty array on failure
    }

    int32 CurrentWidth = CurrentMode.dmPelsWidth;
    int32 CurrentHeight = CurrentMode.dmPelsHeight;

    // Enumerate supported modes
    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;

    while (EnumDisplaySettings(nullptr, ModeIndex, &DevMode))
    {
        if (DevMode.dmPelsWidth == static_cast<DWORD>(CurrentWidth) &&
            DevMode.dmPelsHeight == static_cast<DWORD>(CurrentHeight) &&
            DevMode.dmDisplayFrequency > 0)
        {
            SupportedRates.AddUnique(static_cast<int32>(DevMode.dmDisplayFrequency));
        }
        ModeIndex++;
    }

    SupportedRates.Sort(); // Sort in ascending order
#endif

    return SupportedRates;
}