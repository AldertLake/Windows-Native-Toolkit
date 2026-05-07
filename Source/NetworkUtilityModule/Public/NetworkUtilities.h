// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NetworkUtilities.generated.h"

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnPublicIPResult, bool, bSuccess, FString, PublicIP, FString, ErrorMessage);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnInternetAccessResult, bool, bHasInternet);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnPingResult, bool, bSuccess, int32, PingMs);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnDNSResult, bool, bSuccess, FString, ResolvedIP);

/** Selects which public-IP provider to query. */
UENUM(BlueprintType)
enum class EPublicIPProvider : uint8
{
    Auto        UMETA(DisplayName = "Auto (Best Available)"),
    IfConfig    UMETA(DisplayName = "IfConfig"),
    Amazon      UMETA(DisplayName = "Amazon AWS"),
    ICanHazIP   UMETA(DisplayName = "ICanHazIP")
};

/** Describes the machine's current Windows network connection type. */
UENUM(BlueprintType)
enum class ENetworkWindowsType : uint8
{
    None        UMETA(DisplayName = "No Connection"),
    Ethernet    UMETA(DisplayName = "Ethernet"),
    WiFi        UMETA(DisplayName = "Wi-Fi"),
    Both        UMETA(DisplayName = "Both (Wi-Fi & Ethernet)"),
    Other       UMETA(DisplayName = "Other (Cellular/Unknown)")
};

/** Basic details for one active Windows network interface. */
USTRUCT(BlueprintType)
struct FNetworkInterfaceInfo
{
    GENERATED_BODY()

    /** Readable adapter name reported by Windows. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString InterfaceName;

    /** Hardware description reported by the adapter driver. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString HardwareName;

    /** Current connection type associated with this adapter. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    ENetworkWindowsType Type = ENetworkWindowsType::None;

    /** Stable Windows interface identifier used by other Wi-Fi nodes. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString InterfaceID;
};

/** Detailed Wi-Fi information for one active interface. */
USTRUCT(BlueprintType)
struct FWiFiNetworkInfo
{
    GENERATED_BODY()

    /** Broadcast Wi-Fi network name. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString SSID;

    /** Access-point hardware address. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString BSSID;

    /** Reported signal quality from 0 to 100. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    int32 SignalQuality = 0;

    /** Authentication mode such as WPA2-Personal. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString AuthAlgorithm;

    /** True when Windows returned valid Wi-Fi details. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    bool bSuccess = false;

    /** Readable failure reason when bSuccess is false. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString ErrorMessage;
};

/** Native Windows networking helper nodes for Blueprints. */
UCLASS()
class NETWORKUTILITYMODULE_API UNetworkUtilities : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Returns true when Windows reports active internet connectivity for the current machine. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Is Connected To Internet"))
    static bool IsConnectedToInternet();

    /** Sends a lightweight request to confirm that the machine can reach the internet, not just a local network. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Query Player Internet Access"))
    static void QueryInternetAccess(FString TargetURL, float Timeout, FOnInternetAccessResult OnResult);

    /** Returns the active Windows network connection type, such as Wi-Fi, Ethernet, both, or none. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Wireless Operations|Wi-Fi", meta = (DisplayName = "Get Network Connection Type"))
    static ENetworkWindowsType GetConnectionType();
    
    /** Pings a host name or IP address asynchronously and returns success plus round-trip time. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Ping URL or IP"))
    static void PingAddress(FString Address, float Timeout, FOnPingResult OnResult);

    /** Resolves a host name to an IPv4 address asynchronously. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Resolve Domain Name"))
    static void ResolveDomain(FString Hostname, FOnDNSResult OnResult);

    /** Returns all active network interfaces that Windows currently reports for connectivity queries. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Available Network Interfaces"))
    static TArray<FNetworkInterfaceInfo> GetAvailableInterfaces();

    /** Returns the Wi-Fi SSID for a specific interface ID from Get Available Network Interfaces. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Wi-Fi Network SSID"))
    static FString GetWifiNetworkName(FString InterfaceID);

    /** Returns detailed Wi-Fi information, including SSID, BSSID, signal quality, and authentication mode. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Detailed Wi-Fi Info"))
    static FWiFiNetworkInfo GetDetailedWiFiInfo(FString InterfaceID);

    /** Returns the private IPv4 address currently assigned to a specific network interface. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Private IPv4 Address"))
    static FString GetLocalIpForInterface(FString InterfaceID);

    /** Returns the public IPv4 address with explicit success and error pins. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Public IP"))
    static void GetPublicIP(EPublicIPProvider Mode, float Timeout, FOnPublicIPResult OnResult);

private:

    static void ProcessIPRequest(int32 Index, bool bIsAutoMode, float Timeout, FOnPublicIPResult Callback);
};

