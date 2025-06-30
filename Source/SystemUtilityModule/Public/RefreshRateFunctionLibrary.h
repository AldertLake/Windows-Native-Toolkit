// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RefreshRateFunctionLibrary.generated.h"

UCLASS()
class
    SYSTEMUTILITYMODULE_API URefreshRateFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    // Returns the native (recommended) resolution of the primary display
    UFUNCTION(BlueprintPure, Category = "FMM Core")
    static FIntPoint GetNativeResolution();

    //Ouptut all supported resolutions by user display as an array
    UFUNCTION(BlueprintPure, Category = "FMM Core")
    static TArray<FIntPoint> GetSupportedDisplayResolutions();

    //Output the current used refresh rate by user display
    UFUNCTION(BlueprintPure, Category = "FMM Core")
    static int32 GetCurrentRefreshRate();

    UFUNCTION(BlueprintCallable, Category = "Display Settings")
    static bool SetRefreshRate(int32 NewRefreshRate);

    UFUNCTION(BlueprintPure, Category = "FMM Core")
    static TArray<int32> GetSupportedRefreshRates();
};