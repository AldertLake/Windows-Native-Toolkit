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

#include "RefreshRateFunctionLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

int32 URefreshRateFunctionLibrary::GetCurrentRefreshRate()
{
#if PLATFORM_WINDOWS
    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DevMode) && DevMode.dmDisplayFrequency > 0)
    {
        return DevMode.dmDisplayFrequency;
    }
    return 0; // Fallback if retrieval fails
#else
    return 0; // Non-Windows platforms not supported
#endif
}

bool URefreshRateFunctionLibrary::SetRefreshRate(int32 NewRefreshRate, int32 Width, int32 Height)
{
#if PLATFORM_WINDOWS
    DEVMODE CurrentMode = { 0 };
    CurrentMode.dmSize = sizeof(DEVMODE);

    // Get current display settings to preserve resolution if not overridden
    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &CurrentMode))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to retrieve current display settings."));
        return false;
    }

    // Use current resolution if Width or Height is -1
    int32 TargetWidth = (Width == -1) ? CurrentMode.dmPelsWidth : Width;
    int32 TargetHeight = (Height == -1) ? CurrentMode.dmPelsHeight : Height;

    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;
    bool bFound = false;

    // Find a display mode matching the target resolution and refresh rate
    while (EnumDisplaySettings(NULL, ModeIndex, &DevMode))
    {
        if (DevMode.dmPelsWidth == (DWORD)TargetWidth &&
            DevMode.dmPelsHeight == (DWORD)TargetHeight &&
            DevMode.dmDisplayFrequency == (DWORD)NewRefreshRate)
        {
            bFound = true;
            break;
        }
        ModeIndex++;
    }

    if (!bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("No display mode found for %dx%d at %d Hz."), TargetWidth, TargetHeight, NewRefreshRate);
        return false;
    }

    // Set the fields explicitly to ensure only desired changes are applied
    DevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
    DevMode.dmPelsWidth = TargetWidth;
    DevMode.dmPelsHeight = TargetHeight;
    DevMode.dmDisplayFrequency = NewRefreshRate;

    // Attempt to change the display settings
    LONG Result = ChangeDisplaySettings(&DevMode, CDS_FULLSCREEN);
    if (Result == DISP_CHANGE_SUCCESSFUL)
    {
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to set %dx%d at %d Hz. Error code: %d"), TargetWidth, TargetHeight, NewRefreshRate, Result);
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
    DEVMODE CurrentMode = { 0 };
    CurrentMode.dmSize = sizeof(DEVMODE);

    // Get the current display settings to determine the resolution
    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &CurrentMode))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to retrieve current display settings."));
        return SupportedRates; // Return empty array if we can�t get settings
    }

    int32 CurrentWidth = CurrentMode.dmPelsWidth;
    int32 CurrentHeight = CurrentMode.dmPelsHeight;

    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;
    TSet<int32> UniqueRates;

    // Enumerate all display modes and collect refresh rates for the current resolution
    while (EnumDisplaySettings(NULL, ModeIndex, &DevMode))
    {
        if (DevMode.dmPelsWidth == (DWORD)CurrentWidth && DevMode.dmPelsHeight == (DWORD)CurrentHeight)
        {
            UniqueRates.Add(DevMode.dmDisplayFrequency);
        }
        ModeIndex++;
    }

    // Convert set to sorted array
    SupportedRates = UniqueRates.Array();
    SupportedRates.Sort();
#endif

    return SupportedRates;
}