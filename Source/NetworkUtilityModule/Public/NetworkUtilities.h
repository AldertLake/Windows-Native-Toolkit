// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NetworkUtilities.generated.h"

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
};

/** Detailed Ethernet information for one active interface. */
USTRUCT(BlueprintType)
struct FEthernetNetworkInfo
{
    GENERATED_BODY()

    /** Readable adapter name reported by Windows. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString InterfaceName;

    /** Stable Windows interface identifier used by other network nodes. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString InterfaceID;

    /** Current IPv4 address when one is assigned. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString IPv4Address;

    /** Physical MAC address formatted with colons. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    FString MACAddress;

    /** Best reported link speed in megabits per second. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    int64 LinkSpeedMbps = 0;

    /** True when the interface appears to use DHCP for IPv4. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    bool bDhcpEnabled = false;

    /** True when Windows returned valid Ethernet details. */
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    bool bSuccess = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWNTInternetAccessCompleted, bool, bHasInternet);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWNTPingCompleted, int32, PingMs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWNTDomainResolved, FString, ResolvedIP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWNTPublicIPResolved, FString, PublicIP);

/** Async internet-access probe node with direct success and fail execution pins. */
UCLASS()
class NETWORKUTILITYMODULE_API UAsyncQueryInternetAccessAction : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FWNTInternetAccessCompleted OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FWNTInternetAccessCompleted OnFail;

    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Query Player Internet Access"))
    static UAsyncQueryInternetAccessAction* QueryInternetAccess(const UObject* WorldContextObject, FString TargetURL, float Timeout);

    virtual void Activate() override;

private:
    void Finalize(bool bSuccess, bool bHasInternet);

    FString RequestedUrl;
    float TimeoutSeconds = 0.0f;
    bool bAddedToRootForCompatibility = false;
};

/** Async ping node with direct success and fail execution pins. */
UCLASS()
class NETWORKUTILITYMODULE_API UAsyncPingAddressAction : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FWNTPingCompleted OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FWNTPingCompleted OnFail;

    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Ping URL or IP"))
    static UAsyncPingAddressAction* PingAddress(const UObject* WorldContextObject, FString Address, float Timeout);

    virtual void Activate() override;

private:
    void Finalize(bool bSuccess, int32 PingMs);

    FString AddressToPing;
    float TimeoutSeconds = 0.0f;
    bool bAddedToRootForCompatibility = false;
};

/** Async domain-resolution node with direct success and fail execution pins. */
UCLASS()
class NETWORKUTILITYMODULE_API UAsyncResolveDomainAction : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FWNTDomainResolved OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FWNTDomainResolved OnFail;

    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Resolve Domain Name"))
    static UAsyncResolveDomainAction* ResolveDomain(const UObject* WorldContextObject, FString Hostname);

    virtual void Activate() override;

private:
    void Finalize(bool bSuccess, const FString& ResolvedIP);

    FString HostnameToResolve;
    bool bAddedToRootForCompatibility = false;
};

/** Async public-IP lookup node with direct success and fail execution pins. */
UCLASS()
class NETWORKUTILITYMODULE_API UAsyncGetPublicIPAction : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FWNTPublicIPResolved OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FWNTPublicIPResolved OnFail;

    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Public IP"))
    static UAsyncGetPublicIPAction* GetPublicIP(const UObject* WorldContextObject, EPublicIPProvider Mode, float Timeout);

    virtual void Activate() override;

private:
    void Finalize(bool bSuccess, const FString& PublicIP);

    EPublicIPProvider ProviderMode = EPublicIPProvider::Auto;
    float TimeoutSeconds = 0.0f;
    bool bAddedToRootForCompatibility = false;
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

    /** Returns the active Windows network connection type, such as Wi-Fi, Ethernet, both, or none. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Wireless Operations|Wi-Fi", meta = (DisplayName = "Get Network Connection Type"))
    static ENetworkWindowsType GetConnectionType();
    
    /** Returns all active network interfaces that Windows currently reports for connectivity queries. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Available Network Interfaces"))
    static TArray<FNetworkInterfaceInfo> GetAvailableInterfaces();

    /** Returns detailed Wi-Fi information, including SSID, BSSID, signal quality, and authentication mode. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Detailed Wi-Fi Info"))
    static FWiFiNetworkInfo GetDetailedWiFiInfo(FString InterfaceID);

    /** Returns detailed Ethernet information for a specific interface ID. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Detailed Ethernet Info"))
    static FEthernetNetworkInfo GetDetailedEthernetInfo(FString InterfaceID);

    /** Returns the private IPv4 address currently assigned to a specific network interface. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Network & Connectivity|Internet Network", meta = (DisplayName = "Get Private IPv4 Address"))
    static FString GetLocalIpForInterface(FString InterfaceID);

};

