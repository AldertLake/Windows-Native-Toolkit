/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

#include "BatteryUtility.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "Windows/HideWindowsPlatformTypes.h"

// Log category for battery utility
DEFINE_LOG_CATEGORY_STATIC(LogBatteryUtility, Log, All);

// Battery flag constants
namespace BatteryFlags
{
    const BYTE NoBattery = 128;      // No battery present
    const BYTE FullyCharged = 8;     // Battery is fully charged
}

// Helper to get battery status
static bool GetBatteryStatus(SYSTEM_POWER_STATUS& OutStatus)
{
    if (GetSystemPowerStatus(&OutStatus))
    {
        return true;
    }
    UE_LOG(LogBatteryUtility, Warning, TEXT("Failed to get system power status: %d"), GetLastError());
    return false;
}
#endif

bool UBatteryUtility::HasBattery()
{
#if PLATFORM_WINDOWS
    SYSTEM_POWER_STATUS Status;
    if (GetBatteryStatus(Status))
    {
        return (Status.BatteryFlag & BatteryFlags::NoBattery) == 0;
    }
    return false;
#else
    UE_LOG(LogBatteryUtility, Warning, TEXT("HasBattery is not supported on this platform"));
    return false;
#endif
}

int32 UBatteryUtility::GetBatteryLevel()
{
#if PLATFORM_WINDOWS
    SYSTEM_POWER_STATUS Status;
    if (!GetBatteryStatus(Status) || (Status.BatteryFlag & BatteryFlags::NoBattery))
    {
        return -1;
    }

    // Return 100 if fully charged, otherwise use reported percentage
    // Handle unknown percentage (255)
    return (Status.BatteryFlag & BatteryFlags::FullyCharged) ? 100 :
           (Status.BatteryLifePercent == 255 ? -1 : Status.BatteryLifePercent);
#else
    UE_LOG(LogBatteryUtility, Warning, TEXT("GetBatteryLevel is not supported on this platform"));
    return -1;
#endif
}

bool UBatteryUtility::IsCharging()
{
#if PLATFORM_WINDOWS
    SYSTEM_POWER_STATUS Status;
    if (!GetBatteryStatus(Status) || (Status.BatteryFlag & BatteryFlags::NoBattery))
    {
        return false;
    }

    return Status.ACLineStatus == 1 && !(Status.BatteryFlag & BatteryFlags::FullyCharged);
#else
    UE_LOG(LogBatteryUtility, Warning, TEXT("IsCharging is not supported on this platform"));
    return false;
#endif
}

bool UBatteryUtility::IsFullyCharged()
{
#if PLATFORM_WINDOWS
    SYSTEM_POWER_STATUS Status;
    if (!GetBatteryStatus(Status) || (Status.BatteryFlag & BatteryFlags::NoBattery))
    {
        return false;
    }

    return (Status.BatteryFlag & BatteryFlags::FullyCharged) != 0;
#else
    UE_LOG(LogBatteryUtility, Warning, TEXT("IsFullyCharged is not supported on this platform"));
    return false;
#endif
}