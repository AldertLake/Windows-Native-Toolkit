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

    // Returns the native (recommended) resolution of the primary monitor
    //It will returnt the resolution that windows recognize as native meaning if windwos get it wrong, the code will get it wrong (very rare to happen & mostly happen in HDMI to VGA cables).
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations|Resolution Management", meta = (DisplayName = "Get Windows Native Resolution"))
    static FIntPoint GetNativeResolution();

    //Ouptut all supported resolutions by user monitor as an array
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations|Resolution Management", meta = (DisplayName = "Get Supported Monitor Resolutions"))
    static TArray<FIntPoint> GetSupportedDisplayResolutions();

    //Output the current used refresh rate by user monitor
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations|Refresh Rate Management", meta = (DisplayName = "Get Game Monitor Refresh Rate"))
    static int32 GetCurrentRefreshRate();

    //Change or update the used refresh rate on user display
    //This function use to cause issues on multi display configuration & it is highly unrecommanded to control user display
    //Refresh rate in general. i investigated this & did some theoric solutions since i have only one display (i cannot test).
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Display Operations|Refresh Rate Management", meta = (DisplayName = "Set Game Monitor Refresh Rate"))
    static bool SetRefreshRate(int32 NewRefreshRate);

    //Return all supported refresh rates by the player monitor.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations|Refresh Rate Management", meta = (DisplayName = "Get Supported Refresh Rates"))
    static TArray<int32> GetSupportedRefreshRates();
};