// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AudioSystemLibrary.generated.h"

USTRUCT(BlueprintType)
struct FAudioDeviceInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    FString DeviceName;

    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    FString DeviceID;
};

UCLASS()
class DEVICEFRAMEWORKMODULE_API UAudioSystemLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    // --- FOR SYSTEM DEVICE --- OLD ONES BAISCLY  --- 

    //Get System Current Active Default Audio Output Device Volume From Range 0 to 1.0 As Integer.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Current System Volume"))
    static float GetSystemVolume();

    //Change System Current Active Default Audio Output Device Volume From 0 To 1.0 As Integer.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Current System Volume", BlueprintAuthorityOnly))
    static void SetSystemVolume(float Volume);

    //Get Current Active Default Audio Output Device Name
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Current Audio Device Name"))
    static FString GetCurrentAudioDeviceName();


    // --- OUTPUT DEVICES ---

    //Scans for all active audio OUTPUT devices (Speakers, Headphones).
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get All Audio Output Devices"))
    static TArray<FAudioDeviceInfo> GetAllAudioOutputDevices();

    //Sets volume for a specific OUTPUT device using its ID.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Audio Output Device Volume"))
    static void SetVolumeForDevice(const FString& DeviceID, float Volume);

    // --- INPUT DEVICES ---

    //Scans for all active audio INPUT devices (Microphones, Line-In).
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get All Audio Input Devices"))
    static TArray<FAudioDeviceInfo> GetAllAudioInputDevices();

    //Sets volume for a specific INPUT device using its ID.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Audio Input Device Volume"))
    static void SetInputVolumeForDevice(const FString& DeviceID, float Volume);

    //Get output audio devices volume by device ID (Speaker..)
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Output Device Volume"))
    static float GetOutputDeviceVolume(const FString & DeviceID);

    //Get input audio devices volume by device ID (Micro....)
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Input Device Volume"))
    static float GetInputDeviceVolume(const FString& DeviceID);

    //This function will output the highest sound value as float normalized from 0 to 1.
    //Used to visualize the audio sound.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Audio Device Peak Value"))
    static float GetAudioDevicePeakValue(const FString& DeviceID);
};