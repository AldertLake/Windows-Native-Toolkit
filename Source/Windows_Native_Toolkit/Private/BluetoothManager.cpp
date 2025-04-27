/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#include "BluetoothManager.h"

bool UBluetoothManager::IsBluetoothEnabled()
{
    HANDLE hRadio = nullptr;
    BLUETOOTH_FIND_RADIO_PARAMS btfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

    // attempt to find a Bluetooth radio ---------
    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&btfrp, &hRadio);
    if (hFind != nullptr)
    {
        BluetoothFindRadioClose(hFind);
        if (hRadio != nullptr)
        {
            CloseHandle(hRadio);
        }
        return true;
    }
    return false;
}


int32 UBluetoothManager::GetPairedDeviceCount()
{
    HANDLE hRadio = nullptr;
    BLUETOOTH_FIND_RADIO_PARAMS btfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

    if (!InitializeBluetooth(hRadio, btfrp))
    {
        return 0;
    }

    int32 deviceCount = 0;
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = { sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS) };
    searchParams.fReturnAuthenticated = TRUE; 
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

    if (hRadio != nullptr)
    {
        CloseHandle(hRadio);
    }
    return deviceCount;
}

FString UBluetoothManager::GetPairedDeviceName(int32 DeviceIndex)
{
    HANDLE hRadio = nullptr;
    BLUETOOTH_FIND_RADIO_PARAMS btfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

    if (!InitializeBluetooth(hRadio, btfrp))
    {
        return FString();
    }

    int32 currentIndex = 0;
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = { sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS) };
    searchParams.fReturnAuthenticated = TRUE; 
    searchParams.hRadio = hRadio;

    BLUETOOTH_DEVICE_INFO deviceInfo = { sizeof(BLUETOOTH_DEVICE_INFO) };
    HBLUETOOTH_DEVICE_FIND hDeviceFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    if (hDeviceFind != nullptr)
    {
        do
        {
            if (currentIndex == DeviceIndex)
            {
                FString deviceName = FString(deviceInfo.szName);
                BluetoothFindDeviceClose(hDeviceFind);
                if (hRadio != nullptr)
                {
                    CloseHandle(hRadio);
                }
                return deviceName;
            }
            currentIndex++;
        } while (BluetoothFindNextDevice(hDeviceFind, &deviceInfo));
        BluetoothFindDeviceClose(hDeviceFind);
    }

    if (hRadio != nullptr)
    {
        CloseHandle(hRadio);
    }
    return FString(); // Return empty string if index is invalid, Uncomment the code below for custom mssg.

   // return FString(TEXT("invalid index")); // Use this to return an custom text for invalid index
   // if using this, delete the first return value i mean u really dont wanna get out with two string return values LOL
}

bool UBluetoothManager::InitializeBluetooth(HANDLE& hRadio, BLUETOOTH_FIND_RADIO_PARAMS& btfrp)
{
    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&btfrp, &hRadio);
    if (hFind == nullptr)
    {
        return false;
    }
    BluetoothFindRadioClose(hFind);
    return true;
}


