// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "NetworkUtilities.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <wininet.h>
#include <wlanapi.h>
#include <objbase.h>
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

//Static BS Used For Public IP Query... Note that each index is tied with element
//In the EPublicIPProvider Enumeration.
static const TArray<FString> PROVIDER_URLS = {
    TEXT("https://ifconfig.me/ip"),        
    TEXT("https://checkip.amazonaws.com"),
    TEXT("https://icanhazip.com/")          
};

// --- INTERNET SUPPORT OR CONNECTION ---

bool UNetworkUtilities::IsConnectedToInternet()
{
    DWORD flags;
    return InternetGetConnectedState(&flags, 0);
}

void UNetworkUtilities::QueryInternetAccess(FString TargetURL, float Timeout, FOnInternetAccessResult OnResult)
{
    if (TargetURL.IsEmpty())
    {
        TargetURL = TEXT("http://clients3.google.com/generate_204");
    }

    if (Timeout <= 0.1f)
    {
        Timeout = 2.0f;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL(TargetURL);
    Request->SetVerb("HEAD");
    Request->SetTimeout(Timeout);

    Request->OnProcessRequestComplete().BindLambda(
        [OnResult](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
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
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    DWORD dwRetVal = 0;

    pAddresses = (PIP_ADAPTER_ADDRESSES)FMemory::Malloc(OutBufLen);
    if (pAddresses == nullptr)
    {
        return ENetworkWindowsType::None;
    }

    dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, pAddresses, &OutBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW)
    {
        FMemory::Free(pAddresses);
        pAddresses = (PIP_ADAPTER_ADDRESSES)FMemory::Malloc(OutBufLen);
        if (pAddresses == nullptr) return ENetworkWindowsType::None;

        dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, pAddresses, &OutBufLen);
    }

    if (dwRetVal != NO_ERROR)
    {
        FMemory::Free(pAddresses);
        return ENetworkWindowsType::None;
    }

    bool bHasWifi = false;
    bool bHasEthernet = false;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
    while (pCurrAddresses)
    {
        if (pCurrAddresses->OperStatus == IfOperStatusUp)
        {
            if (pCurrAddresses->FirstGatewayAddress != nullptr)
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
        }
        pCurrAddresses = pCurrAddresses->Next;
    }

    FMemory::Free(pAddresses);

    if (bHasEthernet && bHasWifi) return ENetworkWindowsType::Both;
    if (bHasEthernet) return ENetworkWindowsType::Ethernet;
    if (bHasWifi) return ENetworkWindowsType::WiFi;

    return ENetworkWindowsType::None;
#else
    return ENetworkWindowsType::None;
#endif
}

// --- PUBLIC IP ADRESS HANDLING ---

void UNetworkUtilities::GetPublicIPAddress(EPublicIPProvider Mode, float Timeout, FOnPublicIPFound OnResult)
{
    if (Timeout <= 0.1f) Timeout = 4.0f;

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

void UNetworkUtilities::ProcessIPRequest(int32 Index, bool bIsAutoMode, float Timeout, FOnPublicIPFound Callback)
{
    if (!PROVIDER_URLS.IsValidIndex(Index))
    {
        Callback.ExecuteIfBound(TEXT("Error: All providers failed"));
        return;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL(PROVIDER_URLS[Index]);
    Request->SetVerb("GET");
    Request->SetTimeout(Timeout);

    Request->OnProcessRequestComplete().BindLambda(
        [Index, bIsAutoMode, Timeout, Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {

            if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
            {
                FString ResultIP = Response->GetContentAsString();

                ResultIP = ResultIP.Replace(TEXT("\n"), TEXT("")).Replace(TEXT("\r"), TEXT(""));
                ResultIP.TrimStartAndEndInline();

                if (ResultIP.Len() >= 7 && ResultIP.Len() <= 45)
                {
                    Callback.ExecuteIfBound(ResultIP);
                    return;
                }
            }

            if (bIsAutoMode)
            {

                ProcessIPRequest(Index + 1, true, Timeout, Callback);
            }
            else
            {
                Callback.ExecuteIfBound(TEXT("Error: Provider Unreachable"));
            }
        });

    Request->ProcessRequest();
}

// --- Deprecated Functions --- Never Use Them Or Replace Them As Soon As Possible.

FString UNetworkUtilities::GetLocalIpAddress()
{
    ULONG bufferSize = 0;
    PIP_ADAPTER_ADDRESSES adapters = nullptr;

    GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, nullptr, &bufferSize);
    if (bufferSize == 0) return FString();

    adapters = (PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), 0, bufferSize);
    if (!adapters) return FString();

    DWORD result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, adapters, &bufferSize);
    if (result != NO_ERROR)
    {
        HeapFree(GetProcessHeap(), 0, adapters);
        return FString();
    }

    FString ipAddress;
    for (PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next)
    {
        if (adapter->OperStatus == IfOperStatusUp && adapter->FirstGatewayAddress)
        {
            PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress;
            if (unicast && unicast->Address.lpSockaddr->sa_family == AF_INET)
            {
                sockaddr_in* addr = (sockaddr_in*)unicast->Address.lpSockaddr;
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(addr->sin_addr), ipStr, INET_ADDRSTRLEN);
                ipAddress = FString(UTF8_TO_TCHAR(ipStr));
                break;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, adapters);
    return ipAddress;
}

FString UNetworkUtilities::GetActiveNetworkCard()
{
    ULONG bufferSize = 0;
    PIP_ADAPTER_ADDRESSES adapters = nullptr;

    GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, nullptr, &bufferSize);
    if (bufferSize == 0) return FString();

    adapters = (PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), 0, bufferSize);
    if (!adapters) return FString();

    DWORD result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, adapters, &bufferSize);
    if (result != NO_ERROR)
    {
        HeapFree(GetProcessHeap(), 0, adapters);
        return FString();
    }

    FString networkCardName;
    for (PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next)
    {
        if (adapter->OperStatus == IfOperStatusUp && adapter->FirstGatewayAddress)
        {
            if (adapter->Description)
            {
                networkCardName = FString(adapter->Description);
                break; // Use the first active adapter with a gateway
                // if you have ethernet and wifi and wanna test,
                // there is a way to change active adapter i dont clearly remember it in windwos, somthing like metric value or idk..
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, adapters);
    return networkCardName.IsEmpty() ? FString() : networkCardName;
}

// --- NETWORK INTERFACE PROPERTIES ---

TArray<FNetworkInterfaceInfo> UNetworkUtilities::GetAvailableInterfaces()
{
    TArray<FNetworkInterfaceInfo> ResultArray;

#if PLATFORM_WINDOWS
    ULONG OutBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;


    pAddresses = (PIP_ADAPTER_ADDRESSES)FMemory::Malloc(OutBufLen);
    if (pAddresses == nullptr) return ResultArray;

    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, nullptr, pAddresses, &OutBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW)
    {
        FMemory::Free(pAddresses);
        pAddresses = (PIP_ADAPTER_ADDRESSES)FMemory::Malloc(OutBufLen);
        if (pAddresses == nullptr) return ResultArray;
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

    FMemory::Free(pAddresses);
#endif

    return ResultArray;
}

FString UNetworkUtilities::GetWifiNetworkName(FString InterfaceID)
{
#if PLATFORM_WINDOWS
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    DWORD dwResult = 0;
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
    FString FoundSSID = TEXT("None");

    dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
    if (dwResult != ERROR_SUCCESS) return TEXT("Error: WlanOpenHandle Failed");

    dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
    if (dwResult != ERROR_SUCCESS)
    {
        WlanCloseHandle(hClient, NULL);
        return TEXT("Error: EnumInterfaces Failed");
    }

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

                            FoundSSID = FString(UTF8_TO_TCHAR(SSIDString));
                        }
                    }
                    else
                    {
                        FoundSSID = TEXT("Disconnected");
                    }


                    if (pConnectInfo)
                    {
                        WlanFreeMemory(pConnectInfo);
                        pConnectInfo = NULL;
                    }
                }
                break; 
            }
        }
    }

    if (pIfList != NULL)
    {
        WlanFreeMemory(pIfList);
    }
    WlanCloseHandle(hClient, NULL);

    return FoundSSID;

#else
    return TEXT("None"); 
#endif
}

FString UNetworkUtilities::GetLocalIpForInterface(FString InterfaceID)
{

#if PLATFORM_WINDOWS
    ULONG OutBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;

    pAddresses = (PIP_ADAPTER_ADDRESSES)FMemory::Malloc(OutBufLen);
    if (pAddresses == nullptr) return TEXT("");

    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, nullptr, pAddresses, &OutBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW)
    {
        FMemory::Free(pAddresses);
        pAddresses = (PIP_ADAPTER_ADDRESSES)FMemory::Malloc(OutBufLen);
        if (pAddresses == nullptr) return TEXT("");
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
                            inet_ntop(AF_INET, &(pSockAddr->sin_addr), ipStr, INET_ADDRSTRLEN);
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

    FMemory::Free(pAddresses);
    return FoundIP;
#else
    return TEXT("");
#endif
}