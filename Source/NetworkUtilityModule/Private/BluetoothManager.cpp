// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "BluetoothManager.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinWindows.h"
#include <bluetoothapis.h>
#include <setupapi.h>
#include <bthsdpdef.h>
#include <bthdef.h>
#include "Windows/HideWindowsPlatformTypes.h"

// Verify if Bluetooth is currently enabled
bool UBluetoothManager::IsBluetoothEnabled()
{
    BLUETOOTH_FIND_RADIO_PARAMS radioFindParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
    HANDLE hRadio = nullptr;
    HBLUETOOTH_RADIO_FIND hRadioFind = BluetoothFindFirstRadio(&radioFindParams, &hRadio);

    if (hRadioFind == nullptr)
    {
        return false; // No radios found
    }

    BluetoothFindRadioClose(hRadioFind);

    if (hRadio != nullptr)
    {
        CloseHandle(hRadio);
    }

    return true;
}

// Get number of paired devices
int32 UBluetoothManager::GetPairedDeviceCount()
{
    int32 deviceCount = 0;

    // First, get a Bluetooth radio handle
    BLUETOOTH_FIND_RADIO_PARAMS radioFindParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
    HANDLE hRadio = nullptr;
    HBLUETOOTH_RADIO_FIND hRadioFind = BluetoothFindFirstRadio(&radioFindParams, &hRadio);

    if (hRadioFind == nullptr || hRadio == nullptr)
    {
        return 0;
    }

    // Set up device search parameters
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams{};
    searchParams.dwSize = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS);
    searchParams.fReturnAuthenticated = 1;  
    searchParams.fReturnRemembered = 1;   
    searchParams.fReturnUnknown = 0;   
    searchParams.fReturnConnected = 0;   
    searchParams.fIssueInquiry = 0;  
    searchParams.cTimeoutMultiplier = 0;
    searchParams.hRadio = hRadio;


    BLUETOOTH_DEVICE_INFO deviceInfo = { 0 };
    deviceInfo.dwSize = sizeof(BLUETOOTH_DEVICE_INFO);

    // Find devices
    HBLUETOOTH_DEVICE_FIND hDeviceFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);

    if (hDeviceFind != nullptr)
    {
        do
        {
            // Count only authenticated (paired) devices
            if (deviceInfo.fAuthenticated)
            {
                deviceCount++;
            }
        } while (BluetoothFindNextDevice(hDeviceFind, &deviceInfo));

        BluetoothFindDeviceClose(hDeviceFind);
    }

    BluetoothFindRadioClose(hRadioFind);
    CloseHandle(hRadio);

    return deviceCount;
}

// Get paired device name by index
FString UBluetoothManager::GetPairedDeviceName(int32 DeviceIndex)
{
    if (DeviceIndex < 0)
    {
        return FString();
    }

    // First, get a Bluetooth radio handle
    BLUETOOTH_FIND_RADIO_PARAMS radioFindParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
    HANDLE hRadio = nullptr;
    HBLUETOOTH_RADIO_FIND hRadioFind = BluetoothFindFirstRadio(&radioFindParams, &hRadio);

    if (hRadioFind == nullptr || hRadio == nullptr)
    {
        return FString();
    }

    // Set up device search parameters
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams{};
    searchParams.dwSize = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS);
    searchParams.fReturnAuthenticated = 1;  
    searchParams.fReturnRemembered = 1;   
    searchParams.fReturnUnknown = 0;   
    searchParams.fReturnConnected = 0;   
    searchParams.fIssueInquiry = 0;  
    searchParams.cTimeoutMultiplier = 0;
    searchParams.hRadio = hRadio;


    BLUETOOTH_DEVICE_INFO deviceInfo = { 0 };
    deviceInfo.dwSize = sizeof(BLUETOOTH_DEVICE_INFO);

    HBLUETOOTH_DEVICE_FIND hDeviceFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    int32 currentIndex = 0;
    FString deviceName;

    if (hDeviceFind != nullptr)
    {
        do
        {
            if (deviceInfo.fAuthenticated)
            {
                if (currentIndex == DeviceIndex)
                {
                    // Convert the device name to FString
                    deviceName = FString(deviceInfo.szName);
                    break;
                }
                currentIndex++;
            }
        } while (BluetoothFindNextDevice(hDeviceFind, &deviceInfo));

        BluetoothFindDeviceClose(hDeviceFind);
    }

    BluetoothFindRadioClose(hRadioFind);
    CloseHandle(hRadio);

    return deviceName;
}