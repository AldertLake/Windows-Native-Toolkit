/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#include "BatteryUtility.h"
#include "Windows.h"

bool UBatteryUtility::HasBattery()
{
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status))
    {
        return (status.BatteryFlag & 128) == 0; // 128 = no battery
    }
    return false;
}

int32 UBatteryUtility::GetBatteryLevel()
{
    if (!HasBattery()) return -1;

    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status))
    {
        // there was an issue when the batterylevel return 100 when laptop plugged to power, guess what ? i fix it, yup
        if (status.ACLineStatus == 1 && status.BatteryFlag & 8) // 8 = fully charged flag, d'ont like it ? who cares lol
        {
            return 100;
        }
        else if (status.ACLineStatus == 1)
        {

            return status.BatteryLifePercent;
        }
        return status.BatteryLifePercent; 
    }
    return -1;
}

bool UBatteryUtility::IsCharging()
{
    if (!HasBattery()) return false;

    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status))
    {
        return status.ACLineStatus == 1 && !(status.BatteryFlag & 8); 
    }
    return false;
}


bool UBatteryUtility::IsFullyCharged()
{
    if (!HasBattery()) return false;

    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status))
    {
        return (status.BatteryFlag & 8) != 0; // 8 = fully charged flag
    }
    return false;
}
