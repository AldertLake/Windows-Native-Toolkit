// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NetworkUtilities.generated.h"

UCLASS()
class NETWORKUTILITYMODULE_API UNetworkUtilities : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //Get Player Local IP Adress
    UFUNCTION(BlueprintPure, Category = "Network Info")
    static FString GetLocalIpAddress();

    //Is User Connected To Internet
    UFUNCTION(BlueprintPure, Category = "Network Info")
    static bool IsConnectedToInternet();

    //Get Type Of Connection Type (Ethernet/Wifi)
    UFUNCTION(BlueprintPure, Category = "Network Info")
    static FString GetConnectionType();

    //Get the wifi Network Name
    UFUNCTION(BlueprintPure, Category = "Network Info")
    static FString GetWifiNetworkName();

    //Get The Active Network Card Name
    UFUNCTION(BlueprintPure, Category = "Network Info")
    static FString GetActiveNetworkCard();
};