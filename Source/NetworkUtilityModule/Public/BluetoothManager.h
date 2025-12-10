// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BluetoothManager.generated.h"

UCLASS()
class NETWORKUTILITYMODULE_API UBluetoothManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //Verify if bluetooth is enabled by user or still disabled.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Bluetooth Library")
    static bool IsBluetoothEnabled();

    // Number of paired devices (returns 0 on failure)
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Bluetooth Library")
    static int32 GetPairedDeviceCount();

    // Get the paired device name by index. Returns empty FString on invalid index or error.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Bluetooth Library")
    static FString GetPairedDeviceName(int32 DeviceIndex);
 
};
