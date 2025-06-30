// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "BatteryUtility.h"

#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h"

// Log category for battery utility
DEFINE_LOG_CATEGORY_STATIC(LogBatteryUtility, Log, All);

// Battery flag constants
namespace BatteryFlags
{
    const BYTE NoBattery = 128;
    const BYTE FullyCharged = 8;
}

// Helper to get battery status
static bool GetBatteryStatus(SYSTEM_POWER_STATUS& OutStatus)
{
    if (GetSystemPowerStatus(&OutStatus))
    {
        return true;
    }
    UE_LOG(LogBatteryUtility, Warning, TEXT("Failed to get system power status"));
    return false;
}

#include "Windows/HideWindowsPlatformTypes.h"

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
    UE_LOG(LogBatteryUtility, Warning, TEXT("HasBattery not supported on this platform"));
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
    return (Status.BatteryFlag & BatteryFlags::FullyCharged) ? 100 : Status.BatteryLifePercent;
#else
    UE_LOG(LogBatteryUtility, Warning, TEXT("GetBatteryLevel not supported on this platform"));
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
    UE_LOG(LogBatteryUtility, Warning, TEXT("IsCharging not supported on this platform"));
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
    UE_LOG(LogBatteryUtility, Warning, TEXT("IsFullyCharged not supported on this platform"));
    return false;
#endif
}