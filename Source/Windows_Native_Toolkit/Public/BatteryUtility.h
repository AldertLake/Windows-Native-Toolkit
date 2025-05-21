/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BatteryUtility.generated.h"

/**
 * Blueprint function library for retrieving battery information on Windows.
 * Provides functions to check battery presence, level, charging status, and full charge state.
 * Note: Functions are Windows-only; unsupported platforms return default/error values.
 */
UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UBatteryUtility : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Checks if the system has a battery.
     * @return True if a battery is present, false otherwise or if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "Battery", meta = (DisplayName = "Has Battery"))
    static bool HasBattery();

    /**
     * Gets the current battery level as a percentage (0-100).
     * @return Battery level (0-100) or -1 if unavailable or no battery.
     */
    UFUNCTION(BlueprintPure, Category = "Battery", meta = (DisplayName = "Get Battery Level"))
    static int32 GetBatteryLevel();

    /**
     * Checks if the battery is currently charging.
     * @return True if charging and not fully charged, false otherwise or if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "Battery", meta = (DisplayName = "Is Battery Charging"))
    static bool IsCharging();

    /**
     * Checks if the battery is fully charged.
     * @return True if fully charged, false otherwise or if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "Battery", meta = (DisplayName = "Is Battery Fully Charged"))
    static bool IsFullyCharged();
};