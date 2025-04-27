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

UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UAudioSystemLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Audio Control", meta = (DisplayName = "Get System Volume"))
    static float GetSystemVolume();

    UFUNCTION(BlueprintCallable, Category = "Audio Control", meta = (DisplayName = "Set System Volume"))
    static void SetSystemVolume(float Volume);

    UFUNCTION(BlueprintPure, Category = "Audio Control", meta = (DisplayName = "Get Audio Device Name"))
    static FString GetCurrentAudioDeviceName();
};