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
class UBluetoothManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "Bluetooth Functions")
    static bool IsBluetoothEnabled();

    // Number of paired devices (returns 0 on failure)
    UFUNCTION(BlueprintPure, Category = "Bluetooth Functions")
    static int32 GetPairedDeviceCount();

    // Get the paired device name by index. Returns empty FString on invalid index or error.
    UFUNCTION(BlueprintPure, Category = "Bluetooth Functions")
    static FString GetPairedDeviceName(int32 DeviceIndex);
 
};
