// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BatteryUtility.generated.h"

UCLASS()
class SYSTEMUTILITYMODULE_API UBatteryUtility : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //Verify If User Has A Battery
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Battery", meta = (DisplayName = "Has Battery"))
    static bool HasBattery();

    //Get Battery Level Of User's Laptop
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Battery", meta = (DisplayName = "Get Battery Level"))
    static int32 GetBatteryLevel();

    //Verify If User Laptop is in charge mode
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Battery", meta = (DisplayName = "Is Battery Charging"))
    static bool IsCharging();

    //Verify If User Laptop Is Fully Charged
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Battery", meta = (DisplayName = "Is Battery Fully Charged"))
    static bool IsFullyCharged();
};