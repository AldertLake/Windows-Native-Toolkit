// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BluetoothManager.generated.h"

/** Blueprint-friendly summary for a paired or connected Bluetooth device. */
USTRUCT(BlueprintType)
struct FBluetoothDeviceInfo
{
    GENERATED_BODY()

    /** Friendly Bluetooth device name reported by Windows. */
    UPROPERTY(BlueprintReadOnly, Category = "Bluetooth")
    FString Name;

    /** Device address formatted by the Bluetooth stack. */
    UPROPERTY(BlueprintReadOnly, Category = "Bluetooth")
    FString Address;

    /** True when the device is currently connected. */
    UPROPERTY(BlueprintReadOnly, Category = "Bluetooth")
    bool bIsConnected = false;

    /** True when the device is paired or otherwise authenticated. */
    UPROPERTY(BlueprintReadOnly, Category = "Bluetooth")
    bool bIsAuthenticated = false;
};

/** Native Windows Bluetooth helper nodes for Blueprints. */
UCLASS()
class NETWORKUTILITYMODULE_API UBluetoothManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Returns true when Windows reports a physical Bluetooth adapter. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Bluetooth Library", meta = (DisplayName = "Has Bluetooth Adapter"))
    static bool HasBluetoothAdapter();

    /** Returns true when a Bluetooth radio is available and enabled. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Bluetooth Library", meta = (DisplayName = "Is Bluetooth Enabled"))
    static bool IsBluetoothEnabled();

    /** Returns paired and remembered Bluetooth devices. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Bluetooth Library", meta = (DisplayName = "Get Paired Bluetooth Devices"))
    static TArray<FBluetoothDeviceInfo> GetPairedDevices();

    /** Returns true when a paired Bluetooth device address is currently connected. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Bluetooth Library", meta = (DisplayName = "Is Bluetooth Device Connected"))
    static bool IsBluetoothDeviceConnected(FString DeviceAddress);
};

