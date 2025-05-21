/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <Windows.h> 
#include <bthdef.h>
#include <BluetoothAPIs.h>
#pragma comment(lib, "Bthprops.lib")

#include "BluetoothManager.generated.h"

UCLASS()
class UBluetoothManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    

public:

    UFUNCTION(BlueprintPure, Category = "Bluetooth Functions")
    static bool IsBluetoothEnabled();


    UFUNCTION(BlueprintPure, Category = "Bluetooth Functions")
    static int32 GetPairedDeviceCount();


    UFUNCTION(BlueprintPure, Category = "Bluetooth Functions")
    static FString GetPairedDeviceName(int32 DeviceIndex);

private:
    // Helper function to initialize Bluetooth and get handle
    static bool InitializeBluetooth(HANDLE& hRadio, BLUETOOTH_FIND_RADIO_PARAMS& btfrp);

    
};

