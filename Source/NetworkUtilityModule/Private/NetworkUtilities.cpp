// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "NetworkUtilities.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h"  

#include <winsock2.h>
#include <iphlpapi.h>
#include <wininet.h>
#include <ws2tcpip.h>
#include <wlanapi.h>  

#pragma comment(lib, "iphlpapi.lib")  
#pragma comment(lib, "wininet.lib")   
#pragma comment(lib, "ws2_32.lib")    
#pragma comment(lib, "wlanapi.lib")    

#include "Windows/HideWindowsPlatformTypes.h"


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

bool UNetworkUtilities::IsConnectedToInternet()
{
    DWORD flags;
    return InternetGetConnectedState(&flags, 0);
}

FString UNetworkUtilities::GetConnectionType()
{
    ULONG bufferSize = 0;
    PIP_ADAPTER_ADDRESSES adapters = nullptr;

    GetAdaptersAddresses(AF_INET, 0, nullptr, nullptr, &bufferSize);
    if (bufferSize == 0) return TEXT("None");

    adapters = (PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), 0, bufferSize);
    if (!adapters) return TEXT("None");

    DWORD result = GetAdaptersAddresses(AF_INET, 0, nullptr, adapters, &bufferSize);
    if (result != NO_ERROR)
    {
        HeapFree(GetProcessHeap(), 0, adapters);
        return TEXT("None");
    }

    bool hasWifiUp = false;
    bool hasEthernetUp = false;

    for (PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next)
    {
        if (adapter->OperStatus == IfOperStatusUp)
        {
            if (adapter->IfType == IF_TYPE_IEEE80211) // Wi-Fi
            {
                hasWifiUp = true;
            }
            else if (adapter->IfType == IF_TYPE_ETHERNET_CSMACD) // Ethernet
            {
                hasEthernetUp = true;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, adapters);

    if (hasWifiUp && hasEthernetUp) return TEXT("Both");
    if (hasWifiUp) return TEXT("Wi-Fi");
    if (hasEthernetUp) return TEXT("Ethernet");
    return TEXT("None");
}

FString UNetworkUtilities::GetWifiNetworkName()
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

    FString networkName;
    bool isWifi = false;
    PIP_ADAPTER_ADDRESSES selectedAdapter = nullptr;

    for (PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next)
    {
        if (adapter->OperStatus == IfOperStatusUp && adapter->FirstGatewayAddress)
        {
            selectedAdapter = adapter;
            isWifi = (adapter->IfType == IF_TYPE_IEEE80211);
            break;
        }
    }

    if (selectedAdapter)
    {
        if (isWifi)
        {
            HANDLE wlanHandle = nullptr;
            DWORD negotiatedVersion;
            if (WlanOpenHandle(2, nullptr, &negotiatedVersion, &wlanHandle) == ERROR_SUCCESS)
            {
                PWLAN_INTERFACE_INFO_LIST interfaceList = nullptr;
                if (WlanEnumInterfaces(wlanHandle, nullptr, &interfaceList) == ERROR_SUCCESS)
                {
                    for (DWORD i = 0; i < interfaceList->dwNumberOfItems; i++)
                    {
                        WLAN_INTERFACE_INFO& interfaceInfo = interfaceList->InterfaceInfo[i];
                        PWLAN_CONNECTION_ATTRIBUTES connectionAttrs = nullptr;
                        DWORD dataSize = 0;
                        if (WlanQueryInterface(wlanHandle, &interfaceInfo.InterfaceGuid,
                            wlan_intf_opcode_current_connection, nullptr, &dataSize,
                            (PVOID*)&connectionAttrs, nullptr) == ERROR_SUCCESS && connectionAttrs)
                        {
                            if (connectionAttrs->isState == wlan_interface_state_connected)
                            {
                                DOT11_SSID* ssid = &connectionAttrs->wlanAssociationAttributes.dot11Ssid;
                                if (ssid->uSSIDLength > 0)
                                {
                                    char ssidStr[DOT11_SSID_MAX_LENGTH + 1] = { 0 };
                                    memcpy(ssidStr, ssid->ucSSID, ssid->uSSIDLength);
                                    ssidStr[ssid->uSSIDLength] = '\0';
                                    networkName = FString(UTF8_TO_TCHAR(ssidStr));
                                }
                            }
                            WlanFreeMemory(connectionAttrs);
                            break;
                        }
                    }
                    WlanFreeMemory(interfaceList);
                }
                WlanCloseHandle(wlanHandle, nullptr);
            }
        }
        else
        {
            if (selectedAdapter->FriendlyName)
            {
                networkName = FString(selectedAdapter->FriendlyName);
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, adapters);
    return networkName.IsEmpty() ? FString() : networkName;
}

FString UNetworkUtilities::GetActiveNetworkCard()
{
    ULONG bufferSize = 0;
    PIP_ADAPTER_ADDRESSES adapters = nullptr;

    // Get buffer size
    GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, nullptr, &bufferSize);
    if (bufferSize == 0) return FString();

    // Allocate buffer
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