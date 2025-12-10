// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NetworkUtilities.generated.h"

// --- DELEGATES ---
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPublicIPFound, FString, PublicIP);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnInternetAccessResult, bool, bHasInternet);

// --- ENUMERATIONS ---
UENUM(BlueprintType)
enum class EPublicIPProvider : uint8
{
    Auto        UMETA(DisplayName = "Auto (Best Available)"),
    IfConfig    UMETA(DisplayName = "IfConfig"),
    Amazon      UMETA(DisplayName = "Amazon AWS"),
    ICanHazIP   UMETA(DisplayName = "ICanHazIP")
};

UENUM(BlueprintType)
enum class ENetworkWindowsType : uint8
{
    None        UMETA(DisplayName = "No Connection"),
    Ethernet    UMETA(DisplayName = "Ethernet"),
    WiFi        UMETA(DisplayName = "Wi-Fi"),
    Both        UMETA(DisplayName = "Both (Wi-Fi & Ethernet)"),
    Other       UMETA(DisplayName = "Other (Cellular/Unknown)")
};

USTRUCT(BlueprintType)
struct FNetworkInterfaceInfo
{
    GENERATED_BODY()

    // The name the user sees ("Wi-Fi" or "Ethernet 2")
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString InterfaceName;

    // The hardware model (Intel(R) Wi-Fi 6 AX200 160MHz)
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString HardwareName;

    UPROPERTY(BlueprintReadOnly, Category = "Network")
    ENetworkWindowsType Type;

    // The unique GUID for this network interface
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString InterfaceID;
};

UCLASS()
class NETWORKUTILITYMODULE_API UNetworkUtilities : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    // --- Check Internet Acces --- 

    //Used to check if user is connected to internet, basic & will never tell you if player has real connection to the internet !
    //Rely On WineInit (unreliable to report real internet acces).
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Is Connected To Internet"))
    static bool IsConnectedToInternet();

    //This function is very reliable to check if player has an active & working conection to the internet
    // Leaving the URL Empty is recommanded to automatiacly use 
    // Note that timeout mean the maximuim time to wait in case of failure before canceling the query.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Query Player Internet Acces"))
    static void QueryInternetAccess(FString TargetURL, float Timeout, FOnInternetAccessResult OnResult);

    //Returnt the type of internet conection in forme of enumeration, exemple : WI-FI, Ethernet or maybe both...
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Wirless Operations|Wi-fi", meta = (DisplayName = "Get Network Conection Type"))
    static ENetworkWindowsType GetConnectionType();

    // --- Get Network Interfaces & Informations About Them --- 

    //Gather all available network interfaces from windows system, will return array of a structure holding informations about the interface.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Available Network Interfaces"))
    static TArray<FNetworkInterfaceInfo> GetAvailableInterfaces();

    //Get the Wifi Name (SSID), need ID that will be gathered using GetAvailableInterfaces(), Verify if interface is a WIFI before running it.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Wifi Network SSID"))
    static FString GetWifiNetworkName(FString InterfaceID);

    //Get Local IP Adress For network interface - require ID meaning GUID of the network interface that will be gathered using GetAvailableInterfaces()
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Private IPv4 Adress"))
    static FString GetLocalIpForInterface(FString InterfaceID);

    // --- Public IP Adress --- 

    //Return the player public ip adress using famous ready providers - recommanded is amazon or auto.
    //For timout it's just the amount of time to keep retrying until we get a response.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Public IPv4 Adress"))
    static void GetPublicIPAddress(EPublicIPProvider Mode, float Timeout, FOnPublicIPFound OnResult);

    // --- DEPRECATED FUNCTIONS WARNING --- NEVER USE THEM OR ACCEPT CONSEQUENCES

    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Deprecated Functions", meta = (DeprecatedFunction, DeprecationMessage = "This function was replaced by Get Private IPv4 Adress"))
    static FString GetLocalIpAddress();

    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Deprecated Functions", meta = (DeprecatedFunction, DeprecationMessage = "This function was replaced by Get Available Network Interfaces"))
    static FString GetActiveNetworkCard();

private:

    static void ProcessIPRequest(int32 Index, bool bIsAutoMode, float Timeout, FOnPublicIPFound Callback);
};