/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
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

    UFUNCTION(BlueprintPure, Category = "Display")
    static int32 GetCurrentRefreshRate();


    UFUNCTION(BlueprintCallable, Category = "Display")
    static bool SetRefreshRate(int32 NewRefreshRate, int32 Width = -1, int32 Height = -1);

    UFUNCTION(BlueprintPure, Category = "Display")
    static TArray<int32> GetSupportedRefreshRates();
};