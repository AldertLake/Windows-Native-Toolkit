// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "BluetoothManager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h"

#include <bthdef.h>
#include <BluetoothAPIs.h>
#pragma comment(lib, "Bthprops.lib")

#include "Windows/HideWindowsPlatformTypes.h"
#endif

class FScopedBluetoothRadioFinder
{
public:
    // Constructor finds the first radio
    FScopedBluetoothRadioFinder()
    {
        BLUETOOTH_FIND_RADIO_PARAMS RadioParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
        FindHandle = BluetoothFindFirstRadio(&RadioParams, &RadioHandle);
    }

    // Destructor ensures handles are closed
    ~FScopedBluetoothRadioFinder()
    {
        if (RadioHandle)
        {
            CloseHandle(RadioHandle);
        }
        if (FindHandle)
        {
            BluetoothFindRadioClose(FindHandle);
        }
    }

    // Check if the finder is valid (found a radio)
    bool IsValid() const { return FindHandle != nullptr; }

    // Get the underlying radio handle
    HANDLE GetRadioHandle() const { return RadioHandle; }

private:
    HBLUETOOTH_RADIO_FIND FindHandle = nullptr;
    HANDLE RadioHandle = nullptr;
};

bool UBluetoothManager::IsBluetoothEnabled()
{
#if PLATFORM_WINDOWS
    FScopedBluetoothRadioFinder RadioFinder;
    return RadioFinder.IsValid();
#else
    return false;
#endif
}

int32 UBluetoothManager::GetPairedDeviceCount()
{
#if PLATFORM_WINDOWS
    FScopedBluetoothRadioFinder RadioFinder;
    if (!RadioFinder.IsValid())
    {
        return 0;
    }

    BLUETOOTH_DEVICE_SEARCH_PARAMS SearchParams = { sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS) };
    SearchParams.fReturnAuthenticated = true; // Use modern C++ bool
    SearchParams.hRadio = RadioFinder.GetRadioHandle();

    BLUETOOTH_DEVICE_INFO DeviceInfo = { sizeof(BLUETOOTH_DEVICE_INFO) };
    HBLUETOOTH_DEVICE_FIND DeviceFindHandle = BluetoothFindFirstDevice(&SearchParams, &DeviceInfo);

    if (!DeviceFindHandle)
    {
        return 0;
    }

    int32 DeviceCount = 0;
    do
    {
        DeviceCount++;
    } while (BluetoothFindNextDevice(DeviceFindHandle, &DeviceInfo));

    BluetoothFindDeviceClose(DeviceFindHandle);
    return DeviceCount;
#else
    return 0;
#endif
}

bool UBluetoothManager::GetPairedDeviceName(int32 DeviceIndex, FString& OutDeviceName)
{
    OutDeviceName.Empty();

#if PLATFORM_WINDOWS
    FScopedBluetoothRadioFinder RadioFinder;
    if (!RadioFinder.IsValid() || DeviceIndex < 0)
    {
        return false;
    }

    BLUETOOTH_DEVICE_SEARCH_PARAMS SearchParams = { sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS) };
    SearchParams.fReturnAuthenticated = true; // Use modern C++ bool
    SearchParams.hRadio = RadioFinder.GetRadioHandle();

    BLUETOOTH_DEVICE_INFO DeviceInfo = { sizeof(BLUETOOTH_DEVICE_INFO) };
    HBLUETOOTH_DEVICE_FIND DeviceFindHandle = BluetoothFindFirstDevice(&SearchParams, &DeviceInfo);

    if (!DeviceFindHandle)
    {
        return false;
    }

    int32 CurrentIndex = 0;
    bool bDeviceFound = false;
    do
    {
        if (CurrentIndex == DeviceIndex)
        {
            OutDeviceName = FString(DeviceInfo.szName);
            bDeviceFound = true;
            break; // Found the device, no need to continue looping
        }
        CurrentIndex++;
    } while (BluetoothFindNextDevice(DeviceFindHandle, &DeviceInfo));

    BluetoothFindDeviceClose(DeviceFindHandle);
    return bDeviceFound;
#else
    return false;
#endif
}