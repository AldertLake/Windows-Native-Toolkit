// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "NetworkUtilities.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "WindowsNativeToolkitSettings.h"
#include <atomic>

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <wininet.h>
#include <wlanapi.h>
#include <objbase.h>
#include <netlistmgr.h>
#include <icmpapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if PLATFORM_WINDOWS
namespace
{
struct FScopedComInit
{
    HRESULT Result = E_FAIL;
    bool bNeedsUninitialize = false;

    FScopedComInit()
    {
        Result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        bNeedsUninitialize = SUCCEEDED(Result);
    }

    ~FScopedComInit()
    {
        if (bNeedsUninitialize)
        {
            CoUninitialize();
        }
    }

    bool IsUsable() const
    {
        return SUCCEEDED(Result) || Result == RPC_E_CHANGED_MODE;
    }
};
}
#endif

namespace
{
    static TArray<FString> GetPublicIPProviderUrls();
    static FString GetDefaultInternetAccessUrl();
    static float GetDefaultInternetAccessTimeoutSeconds();
    static float GetDefaultPingTimeoutSeconds();
    static float GetDefaultPublicIPTimeoutSeconds();

    static void LogNetworkError(const TCHAR* Context, const FString& Message)
    {
        UE_LOG(LogTemp, Error, TEXT("Error: %s failed. %s"), Context, *Message);
    }

    static FString FormatMacAddress(const uint8* Address, uint32 Length)
    {
        if (!Address || Length == 0)
        {
            return FString();
        }

        TArray<FString> Parts;
        Parts.Reserve(static_cast<int32>(Length));
        for (uint32 Index = 0; Index < Length; ++Index)
        {
            Parts.Add(FString::Printf(TEXT("%02X"), Address[Index]));
        }
        return FString::Join(Parts, TEXT(":"));
    }

    static TArray<FString> GetPublicIPProviderUrls()
    {
        const UWindowsNativeToolkitSettings* Settings = GetDefault<UWindowsNativeToolkitSettings>();
        TArray<FString> ProviderUrls;
        ProviderUrls.Reserve(3);
        ProviderUrls.Add(Settings ? Settings->IfConfigPublicIPURL.TrimStartAndEnd() : FString(TEXT("https://ifconfig.me/ip")));
        ProviderUrls.Add(Settings ? Settings->AmazonPublicIPURL.TrimStartAndEnd() : FString(TEXT("https://checkip.amazonaws.com")));
        ProviderUrls.Add(Settings ? Settings->ICanHazIPPublicIPURL.TrimStartAndEnd() : FString(TEXT("https://icanhazip.com/")));
        return ProviderUrls;
    }

    static FString GetDefaultInternetAccessUrl()
    {
        const UWindowsNativeToolkitSettings* Settings = GetDefault<UWindowsNativeToolkitSettings>();
        const FString Url = Settings ? Settings->DefaultInternetAccessURL.TrimStartAndEnd() : FString();
        return Url.IsEmpty() ? FString(TEXT("http://clients3.google.com/generate_204")) : Url;
    }

    static float GetDefaultInternetAccessTimeoutSeconds()
    {
        const UWindowsNativeToolkitSettings* Settings = GetDefault<UWindowsNativeToolkitSettings>();
        return (Settings && Settings->DefaultInternetAccessTimeoutSeconds > 0.1f) ? Settings->DefaultInternetAccessTimeoutSeconds : 2.0f;
    }

    static float GetDefaultPingTimeoutSeconds()
    {
        const UWindowsNativeToolkitSettings* Settings = GetDefault<UWindowsNativeToolkitSettings>();
        return (Settings && Settings->DefaultPingTimeoutSeconds > 0.1f) ? Settings->DefaultPingTimeoutSeconds : 2.0f;
    }

    static float GetDefaultPublicIPTimeoutSeconds()
    {
        const UWindowsNativeToolkitSettings* Settings = GetDefault<UWindowsNativeToolkitSettings>();
        return (Settings && Settings->DefaultPublicIPTimeoutSeconds > 0.1f) ? Settings->DefaultPublicIPTimeoutSeconds : 4.0f;
    }
}

UAsyncQueryInternetAccessAction* UAsyncQueryInternetAccessAction::QueryInternetAccess(const UObject* WorldContextObject, FString TargetURL, float Timeout)
{
    UAsyncQueryInternetAccessAction* Node = NewObject<UAsyncQueryInternetAccessAction>();
    Node->RequestedUrl = MoveTemp(TargetURL);
    Node->TimeoutSeconds = Timeout;
    if (WorldContextObject)
    {
        Node->RegisterWithGameInstance(WorldContextObject);
    }
    else
    {
        Node->AddToRoot();
        Node->bAddedToRootForCompatibility = true;
    }
    return Node;
}

void UAsyncQueryInternetAccessAction::Activate()
{
    if (RequestedUrl.IsEmpty())
    {
        RequestedUrl = GetDefaultInternetAccessUrl();
    }

    if (TimeoutSeconds <= 0.1f)
    {
        TimeoutSeconds = GetDefaultInternetAccessTimeoutSeconds();
    }

    TWeakObjectPtr<UAsyncQueryInternetAccessAction> WeakThis(this);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(RequestedUrl);
    Request->SetVerb(TEXT("HEAD"));
    Request->SetTimeout(TimeoutSeconds);
    Request->OnProcessRequestComplete().BindLambda([WeakThis](FHttpRequestPtr HttpRequest, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        bool bHasInternet = false;
        bool bRequestSucceeded = false;

        if (bWasSuccessful && Response.IsValid())
        {
            bRequestSucceeded = true;
            const int32 Code = Response->GetResponseCode();
            bHasInternet = (Code >= 200 && Code < 400);
        }

        if (!bRequestSucceeded)
        {
            LogNetworkError(TEXT("Query Player Internet Access"), TEXT("The HTTP probe did not return a valid response."));
        }

        if (UAsyncQueryInternetAccessAction* Node = WeakThis.Get())
        {
            Node->Finalize(bRequestSucceeded, bHasInternet);
        }
    });

    if (!Request->ProcessRequest())
    {
        LogNetworkError(TEXT("Query Player Internet Access"), TEXT("Failed to start the HTTP probe request."));
        Finalize(false, false);
    }
}

void UAsyncQueryInternetAccessAction::Finalize(bool bSuccess, bool bHasInternet)
{
    if (bSuccess)
    {
        OnSuccess.Broadcast(bHasInternet);
    }
    else
    {
        OnFail.Broadcast(bHasInternet);
    }

    if (bAddedToRootForCompatibility)
    {
        RemoveFromRoot();
        bAddedToRootForCompatibility = false;
    }

    SetReadyToDestroy();
}

UAsyncPingAddressAction* UAsyncPingAddressAction::PingAddress(const UObject* WorldContextObject, FString Address, float Timeout)
{
    UAsyncPingAddressAction* Node = NewObject<UAsyncPingAddressAction>();
    Node->AddressToPing = MoveTemp(Address);
    Node->TimeoutSeconds = Timeout;
    if (WorldContextObject)
    {
        Node->RegisterWithGameInstance(WorldContextObject);
    }
    else
    {
        Node->AddToRoot();
        Node->bAddedToRootForCompatibility = true;
    }
    return Node;
}

void UAsyncPingAddressAction::Activate()
{
#if PLATFORM_WINDOWS
    if (TimeoutSeconds <= 0.1f)
    {
        TimeoutSeconds = GetDefaultPingTimeoutSeconds();
    }

    TWeakObjectPtr<UAsyncPingAddressAction> WeakThis(this);
    const FString AddressCopy = AddressToPing;
    const float TimeoutCopy = TimeoutSeconds;
    Async(EAsyncExecution::Thread, [WeakThis, AddressCopy, TimeoutCopy]()
    {
        bool bSuccess = false;
        int32 PingMs = -1;

        HANDLE hIcmpFile = IcmpCreateFile();
        if (hIcmpFile != INVALID_HANDLE_VALUE)
        {
            ADDRINFOA hints = {};
            PADDRINFOA res = nullptr;
            hints.ai_family = AF_INET;

            if (GetAddrInfoA(TCHAR_TO_ANSI(*AddressCopy), nullptr, &hints, &res) == 0 && res != nullptr)
            {
                sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(res->ai_addr);
                IPAddr destIp = ipv4->sin_addr.S_un.S_addr;
                FreeAddrInfoA(res);

                char SendData[32] = "Ping Buffer Data for Testing";
                DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData) + 8;
                TArray<uint8> ReplyBuffer;
                ReplyBuffer.SetNumZeroed(ReplySize);

                DWORD dwRetVal = IcmpSendEcho(hIcmpFile, destIp, SendData, sizeof(SendData),
                    nullptr, ReplyBuffer.GetData(), ReplySize, FMath::Max(100.0f, TimeoutCopy * 1000.0f));

                if (dwRetVal != 0)
                {
                    PICMP_ECHO_REPLY pEchoReply = reinterpret_cast<PICMP_ECHO_REPLY>(ReplyBuffer.GetData());
                    if (pEchoReply->Status == IP_SUCCESS)
                    {
                        bSuccess = true;
                        PingMs = pEchoReply->RoundTripTime;
                    }
                }
            }

            IcmpCloseHandle(hIcmpFile);
        }

        if (!bSuccess)
        {
            LogNetworkError(TEXT("Ping URL or IP"), TEXT("The host did not respond to ICMP echo."));
        }

        AsyncTask(ENamedThreads::GameThread, [WeakThis, bSuccess, PingMs]()
        {
            if (UAsyncPingAddressAction* Node = WeakThis.Get())
            {
                Node->Finalize(bSuccess, PingMs);
            }
        });
    });
#else
    LogNetworkError(TEXT("Ping URL or IP"), TEXT("Ping is only available on Windows."));
    Finalize(false, -1);
#endif
}

void UAsyncPingAddressAction::Finalize(bool bSuccess, int32 PingMs)
{
    if (bSuccess)
    {
        OnSuccess.Broadcast(PingMs);
    }
    else
    {
        OnFail.Broadcast(PingMs);
    }

    if (bAddedToRootForCompatibility)
    {
        RemoveFromRoot();
        bAddedToRootForCompatibility = false;
    }

    SetReadyToDestroy();
}

UAsyncResolveDomainAction* UAsyncResolveDomainAction::ResolveDomain(const UObject* WorldContextObject, FString Hostname)
{
    UAsyncResolveDomainAction* Node = NewObject<UAsyncResolveDomainAction>();
    Node->HostnameToResolve = MoveTemp(Hostname);
    if (WorldContextObject)
    {
        Node->RegisterWithGameInstance(WorldContextObject);
    }
    else
    {
        Node->AddToRoot();
        Node->bAddedToRootForCompatibility = true;
    }
    return Node;
}

void UAsyncResolveDomainAction::Activate()
{
#if PLATFORM_WINDOWS
    TWeakObjectPtr<UAsyncResolveDomainAction> WeakThis(this);
    const FString HostnameCopy = HostnameToResolve;
    Async(EAsyncExecution::Thread, [WeakThis, HostnameCopy]()
    {
        bool bSuccess = false;
        FString LocalResolvedIP;

        ADDRINFOA hints = {};
        PADDRINFOA res = nullptr;
        hints.ai_family = AF_INET;

        if (GetAddrInfoA(TCHAR_TO_ANSI(*HostnameCopy), nullptr, &hints, &res) == 0 && res != nullptr)
        {
            sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(res->ai_addr);
            char ipStr[INET_ADDRSTRLEN];
            InetNtopA(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
            LocalResolvedIP = FString(UTF8_TO_TCHAR(ipStr));
            bSuccess = true;
            FreeAddrInfoA(res);
        }

        if (!bSuccess)
        {
            LogNetworkError(TEXT("Resolve Domain Name"), TEXT("The host name could not be resolved to an IPv4 address."));
        }

        AsyncTask(ENamedThreads::GameThread, [WeakThis, bSuccess, LocalResolvedIP]()
        {
            if (UAsyncResolveDomainAction* Node = WeakThis.Get())
            {
                Node->Finalize(bSuccess, LocalResolvedIP);
            }
        });
    });
#else
    LogNetworkError(TEXT("Resolve Domain Name"), TEXT("DNS resolution is only available on Windows."));
    Finalize(false, FString());
#endif
}

void UAsyncResolveDomainAction::Finalize(bool bSuccess, const FString& ResolvedIP)
{
    if (bSuccess)
    {
        OnSuccess.Broadcast(ResolvedIP);
    }
    else
    {
        OnFail.Broadcast(ResolvedIP);
    }

    if (bAddedToRootForCompatibility)
    {
        RemoveFromRoot();
        bAddedToRootForCompatibility = false;
    }

    SetReadyToDestroy();
}

UAsyncGetPublicIPAction* UAsyncGetPublicIPAction::GetPublicIP(const UObject* WorldContextObject, EPublicIPProvider Mode, float Timeout)
{
    UAsyncGetPublicIPAction* Node = NewObject<UAsyncGetPublicIPAction>();
    Node->ProviderMode = Mode;
    Node->TimeoutSeconds = Timeout;
    if (WorldContextObject)
    {
        Node->RegisterWithGameInstance(WorldContextObject);
    }
    else
    {
        Node->AddToRoot();
        Node->bAddedToRootForCompatibility = true;
    }
    return Node;
}

void UAsyncGetPublicIPAction::Activate()
{
    if (TimeoutSeconds <= 0.1f)
    {
        TimeoutSeconds = GetDefaultPublicIPTimeoutSeconds();
    }

    int32 StartIndex = 0;
    bool bIsAuto = false;

    switch (ProviderMode)
    {
    case EPublicIPProvider::Auto:
        StartIndex = 0;
        bIsAuto = true;
        break;
    case EPublicIPProvider::IfConfig:
        StartIndex = 0;
        break;
    case EPublicIPProvider::Amazon:
        StartIndex = 1;
        break;
    case EPublicIPProvider::ICanHazIP:
        StartIndex = 2;
        break;
    default:
        StartIndex = 0;
        bIsAuto = true;
        break;
    }

    TWeakObjectPtr<UAsyncGetPublicIPAction> WeakThis(this);
    const TArray<FString> ProviderUrls = GetPublicIPProviderUrls();
    const float TimeoutCopy = TimeoutSeconds;
    TSharedRef<TFunction<void(int32)>, ESPMode::ThreadSafe> AttemptRequest = MakeShared<TFunction<void(int32)>, ESPMode::ThreadSafe>();

    *AttemptRequest = [WeakThis, ProviderUrls, TimeoutCopy, bIsAuto, AttemptRequest](int32 Index)
    {
        if (!ProviderUrls.IsValidIndex(Index))
        {
            LogNetworkError(TEXT("Get Public IP"), TEXT("All public IP providers failed."));
            if (UAsyncGetPublicIPAction* Node = WeakThis.Get())
            {
                Node->Finalize(false, FString());
            }
            return;
        }

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        Request->SetURL(ProviderUrls[Index]);
        Request->SetVerb(TEXT("GET"));
        Request->SetTimeout(TimeoutCopy);
        Request->OnProcessRequestComplete().BindLambda([WeakThis, ProviderUrls, TimeoutCopy, bIsAuto, AttemptRequest, Index](FHttpRequestPtr HttpRequest, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
            {
                FString ResultIP = Response->GetContentAsString();
                ResultIP = ResultIP.Replace(TEXT("\n"), TEXT("")).Replace(TEXT("\r"), TEXT(""));
                ResultIP.TrimStartAndEndInline();

                if (ResultIP.Len() >= 7 && ResultIP.Len() <= 45)
                {
                    if (UAsyncGetPublicIPAction* Node = WeakThis.Get())
                    {
                        Node->Finalize(true, ResultIP);
                    }
                    return;
                }
            }

            if (bIsAuto)
            {
                (*AttemptRequest)(Index + 1);
            }
            else
            {
                LogNetworkError(TEXT("Get Public IP"), TEXT("The selected public IP provider did not return a valid response."));
                if (UAsyncGetPublicIPAction* Node = WeakThis.Get())
                {
                    Node->Finalize(false, FString());
                }
            }
        });

        if (!Request->ProcessRequest())
        {
            if (bIsAuto)
            {
                (*AttemptRequest)(Index + 1);
            }
            else
            {
                LogNetworkError(TEXT("Get Public IP"), TEXT("Failed to start the HTTP request."));
                if (UAsyncGetPublicIPAction* Node = WeakThis.Get())
                {
                    Node->Finalize(false, FString());
                }
            }
        }
    };

    (*AttemptRequest)(StartIndex);
}

void UAsyncGetPublicIPAction::Finalize(bool bSuccess, const FString& PublicIP)
{
    if (bSuccess)
    {
        OnSuccess.Broadcast(PublicIP);
    }
    else
    {
        OnFail.Broadcast(PublicIP);
    }

    if (bAddedToRootForCompatibility)
    {
        RemoveFromRoot();
        bAddedToRootForCompatibility = false;
    }

    SetReadyToDestroy();
}

bool UNetworkUtilities::IsConnectedToInternet()
{
#if PLATFORM_WINDOWS
    FScopedComInit ComInit;
    bool bIsConnected = false;
    HRESULT hr = S_OK;

    INetworkListManager* pNetworkListManager = nullptr;
    hr = CoCreateInstance(CLSID_NetworkListManager, nullptr, CLSCTX_ALL, IID_INetworkListManager, (LPVOID*)&pNetworkListManager);

    if (SUCCEEDED(hr) && pNetworkListManager)
    {
        NLM_CONNECTIVITY connectivity;
        hr = pNetworkListManager->GetConnectivity(&connectivity);
        if (SUCCEEDED(hr))
        {
            if (connectivity & (NLM_CONNECTIVITY_IPV4_INTERNET | NLM_CONNECTIVITY_IPV6_INTERNET))
            {
                bIsConnected = true;
            }
        }
        pNetworkListManager->Release();
    }
    else
    {
        DWORD flags;
        bIsConnected = InternetGetConnectedState(&flags, 0);
    }

    return bIsConnected;
#else
    return false;
#endif
}

ENetworkWindowsType UNetworkUtilities::GetConnectionType()
{
#if PLATFORM_WINDOWS
    ULONG OutBufLen = 15000;
    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(OutBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)Buffer.GetData();

    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, pAddresses, &OutBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW)
    {
        Buffer.SetNumUninitialized(OutBufLen);
        pAddresses = (PIP_ADAPTER_ADDRESSES)Buffer.GetData();
        dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, pAddresses, &OutBufLen);
    }

    if (dwRetVal != NO_ERROR)
    {
        return ENetworkWindowsType::None;
    }

    bool bHasWifi = false;
    bool bHasEthernet = false;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
    while (pCurrAddresses)
    {
        if (pCurrAddresses->OperStatus == IfOperStatusUp && pCurrAddresses->FirstGatewayAddress != nullptr)
        {
            if (pCurrAddresses->IfType == IF_TYPE_IEEE80211)
            {
                bHasWifi = true;
            }
            else if (pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD)
            {
                bHasEthernet = true;
            }
        }
        pCurrAddresses = pCurrAddresses->Next;
    }

    if (bHasEthernet && bHasWifi) return ENetworkWindowsType::Both;
    if (bHasEthernet) return ENetworkWindowsType::Ethernet;
    if (bHasWifi) return ENetworkWindowsType::WiFi;

    return ENetworkWindowsType::None;
#else
    return ENetworkWindowsType::None;
#endif
}

TArray<FNetworkInterfaceInfo> UNetworkUtilities::GetAvailableInterfaces()
{
    TArray<FNetworkInterfaceInfo> ResultArray;

#if PLATFORM_WINDOWS
    ULONG OutBufLen = 15000;
    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(OutBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)Buffer.GetData();

    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, nullptr, pAddresses, &OutBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW)
    {
        Buffer.SetNumUninitialized(OutBufLen);
        pAddresses = (PIP_ADAPTER_ADDRESSES)Buffer.GetData();
        dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, nullptr, pAddresses, &OutBufLen);
    }

    if (dwRetVal == NO_ERROR)
    {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses)
        {
            if (pCurrAddresses->OperStatus == IfOperStatusUp)
            {
                ENetworkWindowsType FoundType = ENetworkWindowsType::None;

                if (pCurrAddresses->IfType == IF_TYPE_IEEE80211)
                {
                    FoundType = ENetworkWindowsType::WiFi;
                }
                else if (pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD)
                {
                    FoundType = ENetworkWindowsType::Ethernet;
                }

                if (FoundType != ENetworkWindowsType::None)
                {
                    FNetworkInterfaceInfo Info;
                    Info.Type = FoundType;

                    if (pCurrAddresses->AdapterName)
                    {
                        Info.InterfaceID = FString(UTF8_TO_TCHAR(pCurrAddresses->AdapterName));
                    }

                    if (pCurrAddresses->FriendlyName)
                    {
                        Info.InterfaceName = FString(pCurrAddresses->FriendlyName);
                    }

                    if (pCurrAddresses->Description)
                    {
                        Info.HardwareName = FString(pCurrAddresses->Description);
                    }

                    ResultArray.Add(Info);
                }
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    }
#endif

    return ResultArray;
}

FWiFiNetworkInfo UNetworkUtilities::GetDetailedWiFiInfo(FString InterfaceID)
{
    FWiFiNetworkInfo Info;
    Info.SSID = TEXT("None");

#if PLATFORM_WINDOWS
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    DWORD dwResult = 0;
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;

    dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
    if (dwResult != ERROR_SUCCESS)
    {
        LogNetworkError(TEXT("Get Detailed Wi-Fi Info"), TEXT("WlanOpenHandle failed."));
        return Info;
    }

    dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
    if (dwResult == ERROR_SUCCESS && pIfList != NULL)
    {
        for (int i = 0; i < (int)pIfList->dwNumberOfItems; i++)
        {
            PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];

            OLECHAR* GuidString;
            if (StringFromCLSID(pIfInfo->InterfaceGuid, &GuidString) == S_OK)
            {
                FString ConvertedGUID = FString(GuidString);
                ::CoTaskMemFree(GuidString);

                if (ConvertedGUID.Equals(InterfaceID, ESearchCase::IgnoreCase))
                {
                    DWORD connectSize = 0;
                    dwResult = WlanQueryInterface(
                        hClient,
                        &pIfInfo->InterfaceGuid,
                        wlan_intf_opcode_current_connection,
                        NULL,
                        &connectSize,
                        (PVOID*)&pConnectInfo,
                        NULL
                    );

                    if (dwResult == ERROR_SUCCESS && pConnectInfo != NULL)
                    {
                        if (pConnectInfo->isState == wlan_interface_state_connected)
                        {
                            DOT11_SSID ssid = pConnectInfo->wlanAssociationAttributes.dot11Ssid;

                            if (ssid.uSSIDLength > 0)
                            {
                                char SSIDString[33];
                                FMemory::Memcpy(SSIDString, ssid.ucSSID, ssid.uSSIDLength);
                                SSIDString[ssid.uSSIDLength] = '\0';
                                Info.SSID = FString(UTF8_TO_TCHAR(SSIDString));
                            }

                            Info.SignalQuality = pConnectInfo->wlanAssociationAttributes.wlanSignalQuality;
                            Info.bSuccess = true;

                            uint8* mac = pConnectInfo->wlanAssociationAttributes.dot11Bssid;
                            Info.BSSID = FString::Printf(TEXT("%02X:%02X:%02X:%02X:%02X:%02X"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

                            switch (pConnectInfo->wlanSecurityAttributes.dot11AuthAlgorithm)
                            {
                                case DOT11_AUTH_ALGO_80211_OPEN: Info.AuthAlgorithm = TEXT("Open"); break;
                                case DOT11_AUTH_ALGO_80211_SHARED_KEY: Info.AuthAlgorithm = TEXT("Shared Key"); break;
                                case DOT11_AUTH_ALGO_WPA: Info.AuthAlgorithm = TEXT("WPA"); break;
                                case DOT11_AUTH_ALGO_WPA_PSK: Info.AuthAlgorithm = TEXT("WPA-PSK"); break;
                                case DOT11_AUTH_ALGO_RSNA: Info.AuthAlgorithm = TEXT("WPA2"); break;
                                case DOT11_AUTH_ALGO_RSNA_PSK: Info.AuthAlgorithm = TEXT("WPA2-PSK"); break;
                                default: Info.AuthAlgorithm = TEXT("Unknown"); break;
                            }
                        }
                        else
                        {
                            Info.SSID = TEXT("Disconnected");
                            LogNetworkError(TEXT("Get Detailed Wi-Fi Info"), TEXT("The Wi-Fi interface is disconnected."));
                        }

                        WlanFreeMemory(pConnectInfo);
                        pConnectInfo = NULL;
                    }
                    break;
                }
            }
        }
        WlanFreeMemory(pIfList);
    }
    WlanCloseHandle(hClient, NULL);
#endif
    if (!Info.bSuccess && Info.SSID.Equals(TEXT("None"), ESearchCase::IgnoreCase))
    {
        LogNetworkError(TEXT("Get Detailed Wi-Fi Info"), TEXT("Wi-Fi information was not found."));
    }

    return Info;
}

FEthernetNetworkInfo UNetworkUtilities::GetDetailedEthernetInfo(FString InterfaceID)
{
    FEthernetNetworkInfo Info;

#if PLATFORM_WINDOWS
    ULONG OutBufLen = 15000;
    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(OutBufLen);
    PIP_ADAPTER_ADDRESSES Addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(Buffer.GetData());

    DWORD ResultCode = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, Addresses, &OutBufLen);
    if (ResultCode == ERROR_BUFFER_OVERFLOW)
    {
        Buffer.SetNumUninitialized(OutBufLen);
        Addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(Buffer.GetData());
        ResultCode = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, Addresses, &OutBufLen);
    }

    if (ResultCode != NO_ERROR)
    {
        LogNetworkError(TEXT("Get Detailed Ethernet Info"), TEXT("Windows failed to enumerate network adapters."));
        return Info;
    }

    for (PIP_ADAPTER_ADDRESSES Current = Addresses; Current; Current = Current->Next)
    {
        if (Current->IfType != IF_TYPE_ETHERNET_CSMACD || !Current->AdapterName)
        {
            continue;
        }

        const FString CurrentId = FString(UTF8_TO_TCHAR(Current->AdapterName));
        if (!CurrentId.Equals(InterfaceID, ESearchCase::IgnoreCase))
        {
            continue;
        }

        Info.InterfaceID = CurrentId;
        if (Current->FriendlyName)
        {
            Info.InterfaceName = FString(Current->FriendlyName);
        }
        Info.IPv4Address = GetLocalIpForInterface(CurrentId);
        Info.MACAddress = FormatMacAddress(Current->PhysicalAddress, Current->PhysicalAddressLength);
        Info.LinkSpeedMbps = static_cast<int64>(FMath::Max<uint64>(Current->TransmitLinkSpeed, Current->ReceiveLinkSpeed) / 1000000ULL);
        Info.bDhcpEnabled = Current->Dhcpv4Server.iSockaddrLength > 0;
        Info.bSuccess = true;
        return Info;
    }

    LogNetworkError(TEXT("Get Detailed Ethernet Info"), TEXT("Ethernet information was not found for the requested interface."));
#else
    LogNetworkError(TEXT("Get Detailed Ethernet Info"), TEXT("Ethernet information is only available on Windows."));
#endif

    return Info;
}

FString UNetworkUtilities::GetLocalIpForInterface(FString InterfaceID)
{
#if PLATFORM_WINDOWS
    ULONG OutBufLen = 15000;
    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(OutBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)Buffer.GetData();

    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, nullptr, pAddresses, &OutBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW)
    {
        Buffer.SetNumUninitialized(OutBufLen);
        pAddresses = (PIP_ADAPTER_ADDRESSES)Buffer.GetData();
        dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, nullptr, pAddresses, &OutBufLen);
    }

    FString FoundIP = TEXT("");

    if (dwRetVal == NO_ERROR)
    {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses)
        {
            if (pCurrAddresses->AdapterName)
            {
                FString CurrentID = FString(UTF8_TO_TCHAR(pCurrAddresses->AdapterName));

                if (CurrentID == InterfaceID)
                {
                    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                    while (pUnicast)
                    {
                        if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
                        {
                            sockaddr_in* pSockAddr = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                            char ipStr[INET_ADDRSTRLEN];
                            InetNtopA(AF_INET, &(pSockAddr->sin_addr), ipStr, INET_ADDRSTRLEN);
                            FoundIP = FString(UTF8_TO_TCHAR(ipStr));
                            break;
                        }
                        pUnicast = pUnicast->Next;
                    }
                    break;
                }
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    }
    return FoundIP;
#else
    return TEXT("");
#endif
}


