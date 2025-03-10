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


#include "NetworkUtilities.h"
#include "Windows/AllowWindowsPlatformTypes.h" // For Windows API usage
#include <winsock2.h>
#include <iphlpapi.h>
#include <wininet.h>
#include <ws2tcpip.h>
#pragma comment(lib, "iphlpapi.lib") // Link IP Helper library
#pragma comment(lib, "wininet.lib")  // Link WinINet library
#pragma comment(lib, "ws2_32.lib")   // Link Winsock library
#include "Windows/HideWindowsPlatformTypes.h"

FString UNetworkUtilities::GetLocalIpAddress()
{
    // Initialize variables for adapter info
    ULONG bufferSize = 0;
    PIP_ADAPTER_ADDRESSES adapters = nullptr;

    // First call to get the required buffer size
    GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, nullptr, &bufferSize);
    if (bufferSize == 0) return FString();

    // Allocate buffer and get adapter info
    adapters = (PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), 0, bufferSize);
    if (!adapters) return FString();

    DWORD result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, adapters, &bufferSize);
    if (result != NO_ERROR)
    {
        HeapFree(GetProcessHeap(), 0, adapters);
        return FString();
    }

    // Iterate through adapters to find the one with a default gateway
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
                break; // Use the first valid IP with a gateway
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
    // Initialize variables for adapter info
    ULONG bufferSize = 0;
    PIP_ADAPTER_ADDRESSES adapters = nullptr;

    // First call to get the required buffer size
    GetAdaptersAddresses(AF_INET, 0, nullptr, nullptr, &bufferSize);
    if (bufferSize == 0) return TEXT("None");

    // Allocate buffer and get adapter info
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

    // Iterate through adapters to check connection types
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

    // Determine connection type based on active adapters
    if (hasWifiUp && hasEthernetUp) return TEXT("Both");
    if (hasWifiUp) return TEXT("Wi-Fi");
    if (hasEthernetUp) return TEXT("Ethernet");
    return TEXT("None");
}