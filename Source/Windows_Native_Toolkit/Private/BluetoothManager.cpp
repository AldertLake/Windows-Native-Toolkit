/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

#include "BluetoothManager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>  // Must be included before <windows.h>
#include <ws2bth.h>    // Bluetooth socket support
#include <windows.h>
#include <bthdef.h>
#include <bluetoothapis.h>

#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// Log category for Bluetooth
DEFINE_LOG_CATEGORY_STATIC(LogBluetooth, Log, All);

bool UBluetoothManager::IsBluetoothEnabled()
{
#if PLATFORM_WINDOWS
    return InitializeBluetooth();
#else
    UE_LOG(LogBluetooth, Warning, TEXT("IsBluetoothEnabled is not supported on this platform"));
    return false;
#endif
}

int32 UBluetoothManager::GetPairedDeviceCount()
{
#if PLATFORM_WINDOWS
    HANDLE hRadio = nullptr;
    BLUETOOTH_FIND_RADIO_PARAMS btfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

    if (!InitializeBluetooth())
    {
        UE_LOG(LogBluetooth, Warning, TEXT("Failed to initialize Bluetooth"));
        return 0;
    }

    // Re-find the radio handle for this function
    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&btfrp, &hRadio);
    if (hFind == nullptr)
    {
        UE_LOG(LogBluetooth, Warning, TEXT("Failed to find Bluetooth radio: %d"), GetLastError());
        return 0;
    }
    BluetoothFindRadioClose(hFind);

    int32 deviceCount = 0;
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = { sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS) };
    searchParams.fReturnAuthenticated = true;
    searchParams.hRadio = hRadio;

    BLUETOOTH_DEVICE_INFO deviceInfo = { sizeof(BLUETOOTH_DEVICE_INFO) };
    HBLUETOOTH_DEVICE_FIND hDeviceFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    if (hDeviceFind != nullptr)
    {
        do
        {
            deviceCount++;
        } while (BluetoothFindNextDevice(hDeviceFind, &deviceInfo));
        BluetoothFindDeviceClose(hDeviceFind);
    }
    else
    {
        UE_LOG(LogBluetooth, Warning, TEXT("No paired Bluetooth devices found: %d"), GetLastError());
    }

    if (hRadio != nullptr)
    {
        CloseHandle(hRadio);
    }
    return deviceCount;
#else
    UE_LOG(LogBluetooth, Warning, TEXT("GetPairedDeviceCount is not supported on this platform"));
    return 0;
#endif
}

FString UBluetoothManager::GetPairedDeviceName(int32 DeviceIndex)
{
#if PLATFORM_WINDOWS
    if (DeviceIndex < 0)
    {
        UE_LOG(LogBluetooth, Warning, TEXT("Invalid device index: %d"), DeviceIndex);
        return FString();
    }

    HANDLE hRadio = nullptr;
    BLUETOOTH_FIND_RADIO_PARAMS btfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

    if (!InitializeBluetooth())
    {
        UE_LOG(LogBluetooth, Warning, TEXT("Failed to initialize Bluetooth"));
        return FString();
    }

    // Re-find the radio handle for this function
    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&btfrp, &hRadio);
    if (hFind == nullptr)
    {
        UE_LOG(LogBluetooth, Warning, TEXT("Failed to find Bluetooth radio: %d"), GetLastError());
        return FString();
    }
    BluetoothFindRadioClose(hFind);

    int32 currentIndex = 0;
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = { sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS) };
    searchParams.fReturnAuthenticated = true;
    searchParams.hRadio = hRadio;

    BLUETOOTH_DEVICE_INFO deviceInfo = { sizeof(BLUETOOTH_DEVICE_INFO) };
    HBLUETOOTH_DEVICE_FIND hDeviceFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    if (hDeviceFind != nullptr)
    {
        do
        {
            if (currentIndex == DeviceIndex)
            {
                FString deviceName = FString(deviceInfo.szName).TrimStartAndEnd();
                BluetoothFindDeviceClose(hDeviceFind);
                if (hRadio != nullptr)
                {
                    CloseHandle(hRadio);
                }
                return deviceName.IsEmpty() ? FString(TEXT("Unnamed Device")) : deviceName;
            }
            currentIndex++;
        } while (BluetoothFindNextDevice(hDeviceFind, &deviceInfo));
        BluetoothFindDeviceClose(hDeviceFind);
    }
    else
    {
        UE_LOG(LogBluetooth, Warning, TEXT("No paired Bluetooth devices found: %d"), GetLastError());
    }

    if (hRadio != nullptr)
    {
        CloseHandle(hRadio);
    }
    UE_LOG(LogBluetooth, Warning, TEXT("Device index %d not found"), DeviceIndex);
    return FString();
#else
    UE_LOG(LogBluetooth, Warning, TEXT("GetPairedDeviceName is not supported on this platform"));
    return FString();
#endif
}

bool UBluetoothManager::InitializeBluetooth()
{
#if PLATFORM_WINDOWS
    HANDLE hRadio = nullptr;
    BLUETOOTH_FIND_RADIO_PARAMS btfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&btfrp, &hRadio);
    if (hFind == nullptr)
    {
        UE_LOG(LogBluetooth, Warning, TEXT("Failed to find Bluetooth radio: %d"), GetLastError());
        return false;
    }
    BluetoothFindRadioClose(hFind);
    if (hRadio != nullptr)
    {
        CloseHandle(hRadio);
    }
    return true;
#else
    UE_LOG(LogBluetooth, Warning, TEXT("InitializeBluetooth is not supported on this platform"));
    return false;
#endif
}