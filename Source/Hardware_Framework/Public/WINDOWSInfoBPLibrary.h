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

#include "Kismet/BlueprintFunctionLibrary.h"
#include "WINDOWSInfoBPLibrary.generated.h"

UCLASS()
class UWINDOWSInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Windows Info", meta = (Platforms = "Win64"))
    static FString GetWindowsVersion();

    UFUNCTION(BlueprintPure, Category = "Windows Info", meta = (Platforms = "Win64"))
    static FString GetWindowsBuild();

    UFUNCTION(BlueprintPure, Category = "Windows Info", meta = (Platforms = "Win64"))
    static FString GetWindowsEdition();

    UFUNCTION(BlueprintPure, Category = "Windows Info", meta = (Platforms = "Win64"))
    static FString GetPCName();

    UFUNCTION(BlueprintPure, Category = "Windows Info", meta = (Platforms = "Win64"))
    static FString GetLocalUserName();

private:
    static FString ReadRegistryString(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);
    static uint32 ReadRegistryDWORD(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);
};