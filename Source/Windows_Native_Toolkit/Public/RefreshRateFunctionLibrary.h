/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RefreshRateFunctionLibrary.generated.h"

UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API URefreshRateFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Retrieves the current refresh rate of the primary display in Hertz.
    UFUNCTION(BlueprintPure, Category = "Display")
    static int32 GetCurrentRefreshRate();

    // Sets the refresh rate of the primary display to the specified value in Hertz.
    // Optionally sets the resolution if Width and Height are provided (default -1 means keep current resolution).
    // Returns true if the refresh rate (and resolution, if specified) was successfully changed, false otherwise.
    UFUNCTION(BlueprintCallable, Category = "Display")
    static bool SetRefreshRate(int32 NewRefreshRate, int32 Width = -1, int32 Height = -1);

    // Retrieves an array of supported refresh rates for the current display at the current resolution.
    UFUNCTION(BlueprintPure, Category = "Display")
    static TArray<int32> GetSupportedRefreshRates();
};