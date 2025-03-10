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
#include "Hardware_FrameworkBPLibrary.generated.h"

UCLASS()
class USystemInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Memory Info
    UFUNCTION(BlueprintPure, Category = "System Info|Memory")
    static void GetMemoryInfo(int32& TotalPhysicalMB, int32& UsedPhysicalMB, int32& FreePhysicalMB,
        int32& TotalVirtualMB, int32& UsedVirtualMB, int32& FreeVirtualMB);

    // CPU Info
    UFUNCTION(BlueprintPure, Category = "System Info|CPU")
    static void GetCPUInfo(FString& Name, FString& Manufacturer, int32& Cores, int32& Threads);

    // GPU Info
    UFUNCTION(BlueprintPure, Category = "System Info|GPU")
    static void GetGPUInfo(FString& Name, FString& Manufacturer, int32& TotalVRAMMB,
        int32& UsedVRAMMB, int32& FreeVRAMMB);

    // Input Devices
    UFUNCTION(BlueprintPure, Category = "System Info|Input")
    static void GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard);
};