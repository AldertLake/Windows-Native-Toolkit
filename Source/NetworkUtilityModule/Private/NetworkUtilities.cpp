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
#include "WindowsNativeToolkitSettings.h"

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

void UNetworkUtilities::QueryInternetAccess(FString TargetURL, float Timeout, FOnInternetAccessResult OnResult)
{
    if (TargetURL.IsEmpty())
    {
        TargetURL = GetDefaultInternetAccessUrl();
    }

    if (Timeout <= 0.1f)
    {
        Timeout = GetDefaultInternetAccessTimeoutSeconds();
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL(TargetURL);
    Request->SetVerb("HEAD");
    Request->SetTimeout(Timeout);

    Request->OnProcessRequestComplete().BindLambda(
        [OnResult](FHttpRequestPtr HttpRequest, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            bool bRealConnection = false;

            if (bWasSuccessful && Response.IsValid())
            {
                int32 Code = Response->GetResponseCode();

                if (Code >= 200 && Code < 400)
                {
                    bRealConnection = true;
                }
            }
            OnResult.ExecuteIfBound(bRealConnection);
        });

    Request->ProcessRequest();
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

void UNetworkUtilities::PingAddress(FString Address, float Timeout, FOnPingResult OnResult)
{
#if PLATFORM_WINDOWS
    if (Timeout <= 0.1f)
    {
        Timeout = GetDefaultPingTimeoutSeconds();
    }

    Async(EAsyncExecution::Thread, [Address, Timeout, OnResult]()
    {
        bool bSuccess = false;
        int32 PingMs = -1;

        HANDLE hIcmpFile = IcmpCreateFile();
        if (hIcmpFile != INVALID_HANDLE_VALUE)
        {
            ADDRINFOA hints = {};
            PADDRINFOA res = nullptr;
            hints.ai_family = AF_INET;

            if (GetAddrInfoA(TCHAR_TO_ANSI(*Address), nullptr, &hints, &res) == 0 && res != nullptr)
            {
                sockaddr_in* ipv4 = (sockaddr_in*)res->ai_addr;
                IPAddr destIp = ipv4->sin_addr.S_un.S_addr;
                FreeAddrInfoA(res);

                char SendData[32] = "Ping Buffer Data for Testing";
                DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData) + 8;
                TArray<uint8> ReplyBuffer;
                ReplyBuffer.SetNumZeroed(ReplySize);

                DWORD dwRetVal = IcmpSendEcho(hIcmpFile, destIp, SendData, sizeof(SendData),
                    nullptr, ReplyBuffer.GetData(), ReplySize, FMath::Max(100.0f, Timeout * 1000.0f));

                if (dwRetVal != 0)
                {
                    PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer.GetData();
                    if (pEchoReply->Status == IP_SUCCESS)
                    {
                        bSuccess = true;
                        PingMs = pEchoReply->RoundTripTime;
                    }
                }
            }
            IcmpCloseHandle(hIcmpFile);
        }

        AsyncTask(ENamedThreads::GameThread, [bSuccess, PingMs, OnResult]()
        {
            OnResult.ExecuteIfBound(bSuccess, PingMs);
        });
    });
#else
    AsyncTask(ENamedThreads::GameThread, [OnResult]() { OnResult.ExecuteIfBound(false, -1); });
#endif
}

void UNetworkUtilities::ResolveDomain(FString Hostname, FOnDNSResult OnResult)
{
#if PLATFORM_WINDOWS
    Async(EAsyncExecution::Thread, [Hostname, OnResult]()
    {
        bool bSuccess = false;
        FString ResolvedIP = TEXT("");

        ADDRINFOA hints = {};
        PADDRINFOA res = nullptr;
        hints.ai_family = AF_INET;

        if (GetAddrInfoA(TCHAR_TO_ANSI(*Hostname), nullptr, &hints, &res) == 0 && res != nullptr)
        {
            sockaddr_in* ipv4 = (sockaddr_in*)res->ai_addr;
            char ipStr[INET_ADDRSTRLEN];
            InetNtopA(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
            ResolvedIP = FString(UTF8_TO_TCHAR(ipStr));
            bSuccess = true;
            FreeAddrInfoA(res);
        }

        AsyncTask(ENamedThreads::GameThread, [bSuccess, ResolvedIP, OnResult]()
        {
            OnResult.ExecuteIfBound(bSuccess, ResolvedIP);
        });
    });
#else
    AsyncTask(ENamedThreads::GameThread, [OnResult]() { OnResult.ExecuteIfBound(false, TEXT("")); });
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

FString UNetworkUtilities::GetWifiNetworkName(FString InterfaceID)
{
    FWiFiNetworkInfo Info = GetDetailedWiFiInfo(InterfaceID);
    return Info.SSID.IsEmpty() ? TEXT("None") : Info.SSID;
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
        Info.ErrorMessage = TEXT("WlanOpenHandle failed.");
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
                            Info.ErrorMessage = TEXT("Wi-Fi interface is disconnected.");
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
    if (!Info.bSuccess && Info.ErrorMessage.IsEmpty() && Info.SSID.Equals(TEXT("None"), ESearchCase::IgnoreCase))
    {
        Info.ErrorMessage = TEXT("Wi-Fi information was not found.");
    }

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

void UNetworkUtilities::GetPublicIP(EPublicIPProvider Mode, float Timeout, FOnPublicIPResult OnResult)
{
    if (Timeout <= 0.1f)
    {
        Timeout = GetDefaultPublicIPTimeoutSeconds();
    }

    int32 StartIndex = 0;
    bool bIsAuto = false;

    switch (Mode)
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

    ProcessIPRequest(StartIndex, bIsAuto, Timeout, OnResult);
}

void UNetworkUtilities::ProcessIPRequest(int32 Index, bool bIsAutoMode, float Timeout, FOnPublicIPResult Callback)
{
    const TArray<FString> ProviderUrls = GetPublicIPProviderUrls();
    if (!ProviderUrls.IsValidIndex(Index))
    {
        Callback.ExecuteIfBound(false, TEXT(""), TEXT("All providers failed."));
        return;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL(ProviderUrls[Index]);
    Request->SetVerb("GET");
    Request->SetTimeout(Timeout);

    Request->OnProcessRequestComplete().BindLambda(
        [Index, bIsAutoMode, Timeout, Callback](FHttpRequestPtr HttpRequest, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
            {
                FString ResultIP = Response->GetContentAsString();
                ResultIP = ResultIP.Replace(TEXT("\n"), TEXT("")).Replace(TEXT("\r"), TEXT(""));
                ResultIP.TrimStartAndEndInline();

                if (ResultIP.Len() >= 7 && ResultIP.Len() <= 45)
                {
                    Callback.ExecuteIfBound(true, ResultIP, TEXT(""));
                    return;
                }
            }

            if (bIsAutoMode)
            {
                ProcessIPRequest(Index + 1, true, Timeout, Callback);
            }
            else
            {
                Callback.ExecuteIfBound(false, TEXT(""), TEXT("Provider unreachable."));
            }
        });

    Request->ProcessRequest();
}


