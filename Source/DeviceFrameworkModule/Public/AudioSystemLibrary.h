/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AudioSystemLibrary.generated.h"

 /**
  * Blueprint function library for controlling system audio settings on Windows.
  * Provides functions to get/set system volume and retrieve the current audio device name.
  * Note: Functions are Windows-only; unsupported platforms return default/error values.
  */
UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UAudioSystemLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Gets the current system master volume (0.0 to 1.0).
     * @return Volume level or -1.0 if unavailable or unsupported.
     */
    UFUNCTION(BlueprintPure, Category = "Audio Control", meta = (DisplayName = "Get System Volume"))
    static float GetSystemVolume();

    /**
     * Sets the system master volume (clamped to 0.0 to 1.0).
     * @param Volume Desired volume level.
     */
    UFUNCTION(BlueprintCallable, Category = "Audio Control", meta = (DisplayName = "Set System Volume", BlueprintAuthorityOnly))
    static void SetSystemVolume(float Volume);

    /**
     * Gets the friendly name of the current default audio output device.
     * @return Device name or "Unsupported platform" if not available.
     */
    UFUNCTION(BlueprintPure, Category = "Audio Control", meta = (DisplayName = "Get Audio Device Name"))
    static FString GetCurrentAudioDeviceName();
};