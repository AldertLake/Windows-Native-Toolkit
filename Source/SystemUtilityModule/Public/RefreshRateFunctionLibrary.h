// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RefreshRateFunctionLibrary.generated.h"

/** Summary of one Windows monitor and its active display mode. */
USTRUCT(BlueprintType)
struct FWNTMonitorInfo
{
    GENERATED_BODY()

    /** Windows monitor device ID, such as \\.\DISPLAY1. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    FString MonitorId;

    /** Human-readable monitor or device name when Windows provides one. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    FString Name;

    /** Current resolution used by this monitor. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    FIntPoint CurrentResolution = FIntPoint::ZeroValue;

    /** Current refresh rate used by this monitor. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    int32 CurrentRefreshRate = 0;

    /** Recommended resolution shown by Windows Settings for this monitor. Falls back to the highest supported resolution when Windows does not report a recommended mode. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    FIntPoint NativeResolution = FIntPoint::ZeroValue;

    /** True when this monitor is the primary Windows display. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    bool bIsPrimary = false;

    /** True when the current Unreal game window is running on this monitor. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    bool bIsGameWindowMonitor = false;
};

/** Resolution and refresh-rate pair for one specific monitor. */
USTRUCT(BlueprintType)
struct FWNTDisplayMode
{
    GENERATED_BODY()

    /** Windows monitor device ID. Empty uses the game window monitor. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    FString MonitorId;

    /** Display mode resolution. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    FIntPoint Resolution = FIntPoint::ZeroValue;

    /** Display mode refresh rate. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    int32 RefreshRate = 0;

    /** True when Windows returned a valid display mode. */
    UPROPERTY(BlueprintReadOnly, Category = "Display")
    bool bIsValid = false;
};

/** Native Windows monitor, resolution, and refresh-rate helper nodes for Blueprints. */
UCLASS()
class SYSTEMUTILITYMODULE_API URefreshRateFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Returns all Windows monitors with their current and native display modes. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations", meta = (DisplayName = "Get Monitors"))
    static TArray<FWNTMonitorInfo> GetMonitors();

    /** Returns the Windows monitor device ID used by the current Unreal game window. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations", meta = (DisplayName = "Get Game Window Monitor ID"))
    static FString GetGameWindowMonitorId();

    /** Returns full monitor information for the display used by the current Unreal game window. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations", meta = (DisplayName = "Get Game Window Monitor"))
    static FWNTMonitorInfo GetGameWindowMonitor();

    /** Returns the current mode for a monitor. Leave Monitor Id empty to use the game window monitor. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations", meta = (DisplayName = "Get Display Mode"))
    static FWNTDisplayMode GetCurrentDisplayMode(const FString& MonitorId);

    /** Tests whether Windows accepts a monitor resolution and refresh rate without applying it. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Display Operations", meta = (DisplayName = "Test Display Mode"))
    static bool TestDisplayMode(const FString& MonitorId, FIntPoint Resolution, int32 RefreshRate, FString& OutError);

    /** Applies a monitor display mode. RevertAfterSeconds restores the previous mode after a delay when greater than zero. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Display Operations", meta = (DisplayName = "Apply Display Mode", AdvancedDisplay = "RevertAfterSeconds"))
    static bool ApplyDisplayMode(const FString& MonitorId, FIntPoint Resolution, int32 RefreshRate, float RevertAfterSeconds, FString& OutError);

    /** Returns the recommended resolution for a monitor. Leave Monitor Id empty to use the game window monitor. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations|Resolution Management", meta = (DisplayName = "Get Native Resolution"))
    static FIntPoint GetNativeResolution(const FString& MonitorId);

    /** Returns supported resolutions for a monitor. Leave Monitor Id empty to use the game window monitor. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations|Resolution Management", meta = (DisplayName = "Get Resolutions"))
    static TArray<FIntPoint> GetSupportedDisplayResolutions(const FString& MonitorId);

    /** Returns the current refresh rate of a monitor. Leave Monitor Id empty to use the game window monitor. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations|Refresh Rate Management", meta = (DisplayName = "Get Refresh Rate"))
    static int32 GetCurrentRefreshRate(const FString& MonitorId);

    /** Sets the refresh rate of a monitor while keeping the current resolution. Leave Monitor Id empty to use the game window monitor. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Display Operations|Refresh Rate Management", meta = (DisplayName = "Set Refresh Rate"))
    static bool SetRefreshRate(int32 NewRefreshRate, const FString& MonitorId, FString& OutError);

    /** Returns supported refresh rates for a monitor resolution. Leave Monitor Id empty to use the game window monitor. Leave Resolution at zero to use the current monitor resolution. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Display Operations|Refresh Rate Management", meta = (DisplayName = "Get Refresh Rates"))
    static TArray<int32> GetSupportedRefreshRates(const FString& MonitorId, FIntPoint Resolution);
};

