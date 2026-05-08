// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
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

/** One shared-mode format option that an audio endpoint accepts. */
USTRUCT(BlueprintType)
struct FWNTAudioDeviceFormat
{
    GENERATED_BODY()

    /** Index used by Set Audio Device Default Format. */
    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    int32 FormatIndex = -1;

    /** Number of audio channels, such as 2 for stereo. */
    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    int32 ChannelCount = 0;

    /** Audio sample rate in hertz. */
    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    int32 SampleRate = 0;

    /** Container bit depth reported by the format. */
    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    int32 BitsPerSample = 0;

    /** Readable summary, for example Stereo, 24-bit, 48000 Hz. */
    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    FString Description;
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
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Default Audio Device"))
    static bool GetDefaultAudioDevice(EWNTAudioDeviceFlow Flow, EWNTAudioDeviceRole Role, FAudioDeviceInfo& OutDevice);

    /** Sets the default Windows audio device for normal playback or recording. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Default Audio Device"))
    static bool SetDefaultAudioDevice(const FString& DeviceID);

    /** Sets the default Windows communications device for calls and chat apps. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Communication Audio Device"))
    static bool SetCommunicationAudioDevice(const FString& DeviceID);

    /** Sets endpoint volume from 0.0 to 1.0 for any input or output device ID. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Audio Volume"))
    static bool SetAudioDeviceVolume(const FString& DeviceID, float Volume);

    /** Reads endpoint volume from 0.0 to 1.0 for any input or output device ID. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Audio Volume"))
    static bool GetAudioDeviceVolume(const FString& DeviceID, float& OutVolume);

    /** Sets endpoint mute state for any input or output device ID. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Audio Muted"))
    static bool SetAudioDeviceMuted(const FString& DeviceID, bool bMuted);

    /** Reads endpoint mute state for any input or output device ID. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Is Audio Muted"))
    static bool GetAudioDeviceMuted(const FString& DeviceID, bool& bMuted);

    /** Reads the current endpoint peak level from 0.0 to 1.0 for visualization. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Audio Peak"))
    static bool GetAudioDevicePeak(const FString& DeviceID, float& OutPeakValue);

    /** Returns the shared-mode formats that Windows currently accepts for the device. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Supported Audio Device Default Formats"))
    static TArray<FWNTAudioDeviceFormat> GetSupportedAudioDeviceFormats(const FString& DeviceID);

    /** Sets the shared-mode default format by using an index returned from Get Supported Audio Device Default Formats. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Audio Device Default Format"))
    static bool SetAudioDeviceDefaultFormat(const FString& DeviceID, int32 FormatIndex);

    /** Shows or hides an audio endpoint in the Windows audio-device list. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Audio Device Visibility"))
    static bool SetAudioDeviceEnabled(const FString& DeviceID, UPARAM(DisplayName = "Visible") bool bEnabled);
};

