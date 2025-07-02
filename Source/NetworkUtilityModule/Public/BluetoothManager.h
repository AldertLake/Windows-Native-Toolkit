// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BluetoothManager.generated.h"

// Forward-declare Windows types to avoid including heavy headers here
struct _BLUETOOTH_FIND_RADIO_PARAMS;
using HBLUETOOTH_RADIO_FIND = void*;
using HANDLE = void*;

UCLASS()
class NETWORKUTILITYMODULE_API UBluetoothManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Checks if a Bluetooth adapter is enabled and available on the system. */
    UFUNCTION(BlueprintPure, Category = "Bluetooth Functions", meta = (DisplayName = "Is Bluetooth Enabled"))
    static bool IsBluetoothEnabled();

    /** Gets the number of paired (authenticated) Bluetooth devices. */
    UFUNCTION(BlueprintPure, Category = "Bluetooth Functions", meta = (DisplayName = "Get Paired Bluetooth Device Count"))
    static int32 GetPairedDeviceCount();

    /**
     * Gets the name of a paired Bluetooth device by its index.
     * @param DeviceIndex The index of the device to query.
     * @param OutDeviceName The name of the found device.
     * @return True if a device was found at that index, false otherwise.
     */
    UFUNCTION(BlueprintPure, Category = "Bluetooth Functions", meta = (DisplayName = "Get Paired Bluetooth Device Name"))
    static bool GetPairedDeviceName(int32 DeviceIndex, FString& OutDeviceName);
};