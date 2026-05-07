// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BatteryUtility.generated.h"

/** Current Windows battery and charging state. */
USTRUCT(BlueprintType)
struct FWNTBatteryStatus
{
    GENERATED_BODY()

    /** True when Windows reports a real battery. */
    UPROPERTY(BlueprintReadOnly, Category = "Battery")
    bool bHasBattery = false;

    /** Battery charge level from 0 to 100. */
    UPROPERTY(BlueprintReadOnly, Category = "Battery")
    int32 Level = 0;

    /** True when AC power is connected and the battery is charging. */
    UPROPERTY(BlueprintReadOnly, Category = "Battery")
    bool bIsCharging = false;

    /** True when AC power is connected and charge is 100 percent. */
    UPROPERTY(BlueprintReadOnly, Category = "Battery")
    bool bIsFullyCharged = false;

    /** True when Windows returned a valid power status. */
    UPROPERTY(BlueprintReadOnly, Category = "Battery")
    bool bSuccess = false;

    /** Readable error when bSuccess is false. */
    UPROPERTY(BlueprintReadOnly, Category = "Battery")
    FString ErrorMessage;
};

/** Native Windows battery helper nodes for Blueprints. */
UCLASS()
class SYSTEMUTILITYMODULE_API UBatteryUtility : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Returns complete battery information in one Blueprint node. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Battery", meta = (DisplayName = "Get Battery Status"))
    static FWNTBatteryStatus GetBatteryStatus();
};

