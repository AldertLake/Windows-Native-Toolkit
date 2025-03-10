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
#include "NetworkUtilities.generated.h"

UCLASS()
class HARDWARE_FRAMEWORK_API UNetworkUtilities : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Returns the local IPv4 address of the user as a string (e.g., "192.168.1.1"), or an empty string if not found
    UFUNCTION(BlueprintPure, Category = "Network Utilities")
    static FString GetLocalIpAddress();

    // Returns true if the user is connected to the internet, false otherwise
    UFUNCTION(BlueprintPure, Category = "Network Utilities")
    static bool IsConnectedToInternet();

    // Returns the connection type: "Wi-Fi", "Ethernet", "Both", or "None"
    UFUNCTION(BlueprintPure, Category = "Network Utilities")
    static FString GetConnectionType();
};