// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AudioSystemLibrary.generated.h"

/** Selects whether an audio node should work with playback or recording devices. */
UENUM(BlueprintType)
enum class EWNTAudioDeviceFlow : uint8
{
    Output UMETA(DisplayName = "Output"),
    Input UMETA(DisplayName = "Input")
};

/** Selects the Windows default audio-device role to query. */
UENUM(BlueprintType)
enum class EWNTAudioDeviceRole : uint8
{
    System UMETA(DisplayName = "System Default"),
    Communication UMETA(DisplayName = "Communication Default")
};

/** Basic details for one Windows audio endpoint device. */
USTRUCT(BlueprintType)
struct FAudioDeviceInfo
{
    GENERATED_BODY()

    /** True when this is the default system device for its input or output flow. */
    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    bool bIsDefaultDevice = false;

    /** True when this is the default communications device for its input or output flow. */
    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    bool bIsCommunicationDevice = false;

    /** Friendly device name reported by Windows. */
    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    FString DeviceName;

    /** Stable Windows endpoint identifier used by volume, mute, and peak functions. */
    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    FString DeviceID;
};

/** Native Windows audio endpoint helper nodes for Blueprints. */
UCLASS()
class DEVICEFRAMEWORKMODULE_API UAudioSystemLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Returns all active audio devices for the selected input or output flow. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Audio Devices"))
    static TArray<FAudioDeviceInfo> GetAudioDevices(EWNTAudioDeviceFlow Flow);

    /** Returns the default Windows endpoint for the selected flow and role. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Default Audio Device"))
    static bool GetDefaultAudioDevice(EWNTAudioDeviceFlow Flow, EWNTAudioDeviceRole Role, FAudioDeviceInfo& OutDevice, FString& OutError);

    /** Sets endpoint volume from 0.0 to 1.0 for any input or output device ID. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Audio Volume"))
    static bool SetAudioDeviceVolume(const FString& DeviceID, float Volume, FString& OutError);

    /** Reads endpoint volume from 0.0 to 1.0 for any input or output device ID. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Audio Volume"))
    static bool GetAudioDeviceVolume(const FString& DeviceID, float& OutVolume, FString& OutError);

    /** Sets endpoint mute state for any input or output device ID. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Audio Muted"))
    static bool SetAudioDeviceMuted(const FString& DeviceID, bool bMuted, FString& OutError);

    /** Reads endpoint mute state for any input or output device ID. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Is Audio Muted"))
    static bool GetAudioDeviceMuted(const FString& DeviceID, bool& bMuted, FString& OutError);

    /** Reads the current endpoint peak level from 0.0 to 1.0 for visualization. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Audio Peak"))
    static bool GetAudioDevicePeak(const FString& DeviceID, float& OutPeakValue, FString& OutError);
};

