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
    bool bIsDefaultDevice = false;

    UPROPERTY(BlueprintReadOnly, Category = "Audio")
    bool bIsCommunicationDevice = false;

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

    // In 2.4.1 These functions are deprecated & should not be used.

    //Get System Current Active Default Audio Output Device Volume From Range 0 to 1.0 As Integer.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Current System Volume", DeprecatedFunction))
    static float GetSystemVolume();

    //Change System Current Active Default Audio Output Device Volume From 0 To 1.0 As Integer.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Set Current System Volume", DeprecatedFunction))
    static void SetSystemVolume(float Volume);

    //Get Current Active Default Audio Output Device Name
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Current Audio Device Name", DeprecatedFunction))
    static FString GetCurrentAudioDeviceName();

    //You can do what the functions in the top do by simply using the ones bellow, loop all output devices until you find the default one, cache the default one ID.
    //With the ID you can do anything related to volume...
    //With this methode you can even get the default comunication device alone and the default system device.

    //Warning: i reported that the volume functions do not work with some devices (eg: my xbox controller headset).
    //The reason is that some devices igonore IAudioEndpointVolume, you can remarke this also in capture devices sometimes...



    // --- OUTPUT DEVICES ---

    //Scans for all active audio OUTPUT devices (Speakers, Headphones).
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations|Audio Output Devices", meta = (DisplayName = "Get All Audio Output Devices"))
    static TArray<FAudioDeviceInfo> GetAllAudioOutputDevices();

    //Sets volume for a specific OUTPUT device using its ID.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations|Audio Output Devices", meta = (DisplayName = "Set Audio Output Device Volume"))
    static void SetVolumeForDevice(const FString& DeviceID, float Volume);

    //Get output audio devices volume by device ID (Speaker..)
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations|Audio Output Devices", meta = (DisplayName = "Get Audio Output Device Volume"))
    static float GetOutputDeviceVolume(const FString& DeviceID);

    // --- INPUT DEVICES ---

    //Scans for all active audio INPUT devices (Microphones, Line-In).
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations|Audio Input Devices", meta = (DisplayName = "Get All Audio Input Devices"))
    static TArray<FAudioDeviceInfo> GetAllAudioInputDevices();

    //Sets volume for a specific INPUT device using its ID.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Sound Operations|Audio Input Devices", meta = (DisplayName = "Set Audio Input Device Volume"))
    static void SetInputVolumeForDevice(const FString& DeviceID, float Volume);

    //Get input audio devices volume by device ID (Micro....)
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations|Audio Input Devices", meta = (DisplayName = "Get Audio Input Device Volume"))
    static float GetInputDeviceVolume(const FString& DeviceID);

    // --- GLOBAL FOR VISUALIZATION --

    //This function will output the highest sound value as float normalized from 0 to 1.
    //Used to visualize the audio sound.
    //Do not tick this function in your game ! also don't do some retarded moves like ticking it with a delay..lol
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Sound Operations", meta = (DisplayName = "Get Audio Device Peak Value"))
    static float GetAudioDevicePeakValue(const FString& DeviceID);

    //To the person reading please if you find any vocabulary mistake, dont get mad because english is not my first language XD.

};