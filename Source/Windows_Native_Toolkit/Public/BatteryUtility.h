/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "BatteryUtility.generated.h"

UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UBatteryUtility : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "Hardware Info")
    static bool HasBattery();

    UFUNCTION(BlueprintPure, Category = "Hardware Info")
    static int32 GetBatteryLevel();

    UFUNCTION(BlueprintPure, Category = "Hardware Info")
    static bool IsCharging();

    UFUNCTION(BlueprintPure, Category = "Hardware Info")
    static bool IsFullyCharged();

};