// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "BatteryUtility.h"
#include "SystemUtilityModule.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"

static bool ReadWindowsPowerStatus(SYSTEM_POWER_STATUS& OutStatus)
{
    ZeroMemory(&OutStatus, sizeof(SYSTEM_POWER_STATUS));
    return GetSystemPowerStatus(&OutStatus) != 0;
}
#endif

FWNTBatteryStatus UBatteryUtility::GetBatteryStatus()
{
    FWNTBatteryStatus Result;
#if PLATFORM_WINDOWS
    SYSTEM_POWER_STATUS Status;
    if (!ReadWindowsPowerStatus(Status))
    {
        UE_LOG(LogWNT, Error, TEXT("Get Battery Status failed because Windows did not return power status."));
        return Result;
    }

    Result.bSuccess = true;
    Result.bHasBattery = !(Status.BatteryFlag & BATTERY_FLAG_NO_BATTERY) && Status.BatteryFlag != 255;
    Result.Level = Status.BatteryLifePercent == 255 ? 0 : FMath::Clamp(static_cast<int32>(Status.BatteryLifePercent), 0, 100);
    Result.bIsCharging = (Status.BatteryFlag & BATTERY_FLAG_CHARGING) != 0;
    Result.bIsFullyCharged = Status.ACLineStatus == 1 && Status.BatteryLifePercent == 100;
#else
    UE_LOG(LogWNT, Error, TEXT("Get Battery Status is only available on Windows."));
#endif
    return Result;
}

