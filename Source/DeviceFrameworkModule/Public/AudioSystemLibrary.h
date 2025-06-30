// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AudioSystemLibrary.generated.h"

UCLASS()
class DEVICEFRAMEWORKMODULE_API UAudioSystemLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //Get System Current Volume From Range 0 to 1.0 As Integer
    UFUNCTION(BlueprintPure, Category = "Audio Control", meta = (DisplayName = "Get System Volume"))
    static float GetSystemVolume();

    //Change Device Volume From 0 To 1.0 As Integer
    UFUNCTION(BlueprintCallable, Category = "Audio Control", meta = (DisplayName = "Set System Volume", BlueprintAuthorityOnly))
    static void SetSystemVolume(float Volume);

    //Get Current Audio Outpu Device Name
    UFUNCTION(BlueprintPure, Category = "Audio Control", meta = (DisplayName = "Get Audio Device Name"))
    static FString GetCurrentAudioDeviceName();
};