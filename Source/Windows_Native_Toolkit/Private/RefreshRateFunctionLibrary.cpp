/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

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
    return 0; 
#else
    return 0; 
#endif
}

bool URefreshRateFunctionLibrary::SetRefreshRate(int32 NewRefreshRate, int32 Width, int32 Height)
{
#if PLATFORM_WINDOWS
    DEVMODE CurrentMode = { 0 };
    CurrentMode.dmSize = sizeof(DEVMODE);


    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &CurrentMode))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to retrieve current display settings."));
        return false;
    }


    int32 TargetWidth = (Width == -1) ? CurrentMode.dmPelsWidth : Width;
    int32 TargetHeight = (Height == -1) ? CurrentMode.dmPelsHeight : Height;

    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;
    bool bFound = false;


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


    DevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
    DevMode.dmPelsWidth = TargetWidth;
    DevMode.dmPelsHeight = TargetHeight;
    DevMode.dmDisplayFrequency = NewRefreshRate;


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


    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &CurrentMode))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to retrieve current display settings."));
        return SupportedRates; 
    }

    int32 CurrentWidth = CurrentMode.dmPelsWidth;
    int32 CurrentHeight = CurrentMode.dmPelsHeight;

    DEVMODE DevMode = { 0 };
    DevMode.dmSize = sizeof(DEVMODE);
    int32 ModeIndex = 0;
    TSet<int32> UniqueRates;


    while (EnumDisplaySettings(NULL, ModeIndex, &DevMode))
    {
        if (DevMode.dmPelsWidth == (DWORD)CurrentWidth && DevMode.dmPelsHeight == (DWORD)CurrentHeight)
        {
            UniqueRates.Add(DevMode.dmDisplayFrequency);
        }
        ModeIndex++;
    }

 
    SupportedRates = UniqueRates.Array();
    SupportedRates.Sort();
#endif

    return SupportedRates;
}