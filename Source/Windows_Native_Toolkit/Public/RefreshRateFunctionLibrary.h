#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RefreshRateFunctionLibrary.generated.h"

/**
 * Blueprint function library for managing display refresh rates on Windows.
 * Provides functions to get/set the refresh rate and query supported rates.
 * Note: Functions are Windows-only; unsupported platforms return default values.
 */
UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API URefreshRateFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Retrieves the current refresh rate of the primary display in Hertz.
     * @return Refresh rate in Hz (e.g., 60) or -1 if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "Display")
    static int32 GetCurrentRefreshRate();

    /**
     * Sets the refresh rate of the primary display to the specified value in Hertz.
     * Optionally sets the resolution if Width and Height are provided (use -1 to keep current resolution).
     * @param NewRefreshRate Desired refresh rate in Hz (must be positive and supported).
     * @param Width Desired width in pixels (default -1 keeps current width).
     * @param Height Desired height in pixels (default -1 keeps current height).
     * @return True if the refresh rate (and resolution, if specified) was successfully changed, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "Display")
    static bool SetRefreshRate(int32 NewRefreshRate, int32 Width = -1, int32 Height = -1);

    /**
     * Retrieves an array of supported refresh rates for the current display at the current resolution.
     * @return Array of supported refresh rates in Hz, sorted in ascending order, or empty if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "Display")
    static TArray<int32> GetSupportedRefreshRates();
};