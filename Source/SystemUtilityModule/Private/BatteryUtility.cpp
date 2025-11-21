// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "BatteryUtility.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h> 
#include "Windows/HideWindowsPlatformTypes.h"
#endif


static bool GetWindowsPowerStatus(SYSTEM_POWER_STATUS& OutStatus)
{
#if PLATFORM_WINDOWS
    FMemory::Memzero(OutStatus);

    if (GetSystemPowerStatus(&OutStatus))
    {
        return true;
    }
#endif
    return false;
}

bool UBatteryUtility::HasBattery()
{
#if PLATFORM_WINDOWS
    SYSTEM_POWER_STATUS Status;
    if (GetWindowsPowerStatus(Status))
    {

        if (Status.BatteryFlag & BATTERY_FLAG_NO_BATTERY)
        {
            return false;
        }

        if (Status.BatteryFlag == 255)
        {
            return false;
        }

        return true;
    }
#endif
    return false;
}

int32 UBatteryUtility::GetBatteryLevel()
{
#if PLATFORM_WINDOWS
    SYSTEM_POWER_STATUS Status;
    if (GetWindowsPowerStatus(Status))
    {

        if (Status.BatteryLifePercent == 255)
        {
            return 0; 
        }

        return FMath::Clamp((int32)Status.BatteryLifePercent, 0, 100);
    }
#endif
    return 0;
}

bool UBatteryUtility::IsCharging()
{
#if PLATFORM_WINDOWS
    SYSTEM_POWER_STATUS Status;
    if (GetWindowsPowerStatus(Status))
    {
        return (Status.BatteryFlag & BATTERY_FLAG_CHARGING) != 0;
    }
#endif
    return false;
}

bool UBatteryUtility::IsFullyCharged()
{
#if PLATFORM_WINDOWS
    SYSTEM_POWER_STATUS Status;
    if (GetWindowsPowerStatus(Status))
    {
        bool bIsPluggedIn = (Status.ACLineStatus == 1);
        bool bIsAtMax = (Status.BatteryLifePercent == 100);

        return bIsPluggedIn && bIsAtMax;
    }
#endif
    return false;
}