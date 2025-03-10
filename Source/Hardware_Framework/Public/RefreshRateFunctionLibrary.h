/*

 * Copyright (C) 2025 AldertLake. All rights reserved.
 *
 * This file is part of the Windows Native ToolKit, an Unreal Engine Plugin.
 *
 * Unauthorized copying, distribution, or modification of this file is strictly prohibited.
 *
 * Anyone who bought this project has the full right to modify it like he want.
 *
 *
 * Author: AldertLake
 * Date: 2025/1/9
 ______________________________________________________________________________________________________________

  AAAAAAA     L          DDDDDDD     EEEEEEE     RRRRRRR    TTTTTTT    L          AAAAAAA     KKKKKK    EEEEEEE
 A       A    L          D       D   E           R     R       T       L         A       A    K     K   E
 AAAAAAAAA    L          D       D   EEEEE       RRRRRRR       T       L         AAAAAAAAA    KKKKKK    EEEE
 A       A    L          D       D   E           R   R         T       L         A       A    K   K     E
 A       A    LLLLLLL    DDDDDDD     EEEEEEE     R    R        T       LLLLLLL   A       A    K    K    EEEEEEE
 ______________________________________________________________________________________________________________
*/
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RefreshRateFunctionLibrary.generated.h"

UCLASS()
class HARDWARE_FRAMEWORK_API URefreshRateFunctionLibrary : public UBlueprintFunctionLibrary
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