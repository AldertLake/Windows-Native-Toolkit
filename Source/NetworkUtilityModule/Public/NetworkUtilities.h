/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NetworkUtilities.generated.h"

UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UNetworkUtilities : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "Network Info")
    static FString GetLocalIpAddress();

    UFUNCTION(BlueprintPure, Category = "Network Info")
    static bool IsConnectedToInternet();

    UFUNCTION(BlueprintPure, Category = "Network Info")
    static FString GetConnectionType();

    UFUNCTION(BlueprintPure, Category = "Network Info")
    static FString GetWifiNetworkName();

    UFUNCTION(BlueprintPure, Category = "Network Info")
    static FString GetActiveNetworkCard();
};