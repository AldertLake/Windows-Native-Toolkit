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
#include "AudioSystemLibrary.generated.h"

UCLASS()
class HARDWARE_FRAMEWORK_API UAudioSystemLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Audio|System", meta = (DisplayName = "Get System Volume"))
    static float GetSystemVolume();

    UFUNCTION(BlueprintCallable, Category = "Audio|System", meta = (DisplayName = "Set System Volume"))
    static void SetSystemVolume(float Volume);

    UFUNCTION(BlueprintPure, Category = "Audio|System", meta = (DisplayName = "Get Audio Device Name"))
    static FString GetCurrentAudioDeviceName();
};