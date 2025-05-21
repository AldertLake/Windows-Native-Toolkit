/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BluetoothManager.generated.h"

/**
 * Blueprint function library for managing Bluetooth functionality on Windows.
 * Provides functions to check Bluetooth status, count paired devices, and retrieve device names.
 * Note: Functions are Windows-only; unsupported platforms return default/error values.
 */
UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UBluetoothManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Checks if Bluetooth is enabled on the system.
     * @return True if a Bluetooth radio is found and enabled, false otherwise.
     */
    UFUNCTION(BlueprintPure, Category = "Bluetooth", meta = (DisplayName = "Is Bluetooth Enabled"))
    static bool IsBluetoothEnabled();

    /**
     * Gets the number of paired Bluetooth devices.
     * @return Number of paired devices, or 0 if none or on error.
     */
    UFUNCTION(BlueprintPure, Category = "Bluetooth", meta = (DisplayName = "Get Paired Device Count"))
    static int32 GetPairedDeviceCount();

    /**
     * Gets the name of a paired Bluetooth device at the specified index.
     * @param DeviceIndex Index of the device (0-based).
     * @return Device name, or empty string if index is invalid or on error.
     */
    UFUNCTION(BlueprintPure, Category = "Bluetooth", meta = (DisplayName = "Get Paired Device Name"))
    static FString GetPairedDeviceName(int32 DeviceIndex);

private:
    // Helper function to initialize Bluetooth and get radio handle
    static bool InitializeBluetooth();
};