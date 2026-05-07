// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "BluetoothManager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <bluetoothapis.h>
#include <setupapi.h>
#include <devguid.h>
#include <bthsdpdef.h>
#include <bthdef.h>
#include "Windows/HideWindowsPlatformTypes.h"

struct FBluetoothRadioFindHandle
{
    HBLUETOOTH_RADIO_FIND Handle;
    FBluetoothRadioFindHandle(HBLUETOOTH_RADIO_FIND InHandle) : Handle(InHandle) {}
    ~FBluetoothRadioFindHandle() { if (Handle) BluetoothFindRadioClose(Handle); }
    operator HBLUETOOTH_RADIO_FIND() const { return Handle; }
    bool IsValid() const { return Handle != nullptr; }
};

struct FBluetoothRadioHandle
{
    HANDLE Handle;
    FBluetoothRadioHandle(HANDLE InHandle) : Handle(InHandle) {}
    ~FBluetoothRadioHandle() { if (Handle) CloseHandle(Handle); }
    operator HANDLE() const { return Handle; }
    bool IsValid() const { return Handle != nullptr; }
};

struct FBluetoothDeviceFindHandle
{
    HBLUETOOTH_DEVICE_FIND Handle;
    FBluetoothDeviceFindHandle(HBLUETOOTH_DEVICE_FIND InHandle) : Handle(InHandle) {}
    ~FBluetoothDeviceFindHandle() { if (Handle) BluetoothFindDeviceClose(Handle); }
    operator HBLUETOOTH_DEVICE_FIND() const { return Handle; }
    bool IsValid() const { return Handle != nullptr; }
};
#endif

bool UBluetoothManager::HasBluetoothAdapter()
{
    bool bHasAdapter = false;
#if PLATFORM_WINDOWS
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_BLUETOOTH, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA DeviceInfoData;
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (SetupDiEnumDeviceInfo(hDevInfo, 0, &DeviceInfoData))
        {
            bHasAdapter = true;
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
#endif
    return bHasAdapter;
}

bool UBluetoothManager::IsBluetoothEnabled()
{
#if PLATFORM_WINDOWS
    BLUETOOTH_FIND_RADIO_PARAMS radioFindParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
    HANDLE hRadio = nullptr;

    FBluetoothRadioFindHandle hRadioFind(BluetoothFindFirstRadio(&radioFindParams, &hRadio));
    FBluetoothRadioHandle RadioHandle(hRadio);

    return hRadioFind.IsValid();
#else
    return false;
#endif
}

TArray<FBluetoothDeviceInfo> UBluetoothManager::GetPairedDevices()
{
    TArray<FBluetoothDeviceInfo> ResultList;

#if PLATFORM_WINDOWS
    BLUETOOTH_FIND_RADIO_PARAMS radioFindParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
    HANDLE hRadioRaw = nullptr;

    FBluetoothRadioFindHandle hRadioFind(BluetoothFindFirstRadio(&radioFindParams, &hRadioRaw));
    FBluetoothRadioHandle hRadio(hRadioRaw);

    if (!hRadioFind.IsValid() || !hRadio.IsValid())
    {
        return ResultList;
    }

    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams{};
    searchParams.dwSize = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS);
    searchParams.fReturnAuthenticated = 1;
    searchParams.fReturnRemembered = 1;
    searchParams.fReturnUnknown = 0;
    searchParams.fReturnConnected = 1;
    searchParams.fIssueInquiry = 0;
    searchParams.cTimeoutMultiplier = 0;
    searchParams.hRadio = hRadio;

    BLUETOOTH_DEVICE_INFO deviceInfo = { 0 };
    deviceInfo.dwSize = sizeof(BLUETOOTH_DEVICE_INFO);

    FBluetoothDeviceFindHandle hDeviceFind(BluetoothFindFirstDevice(&searchParams, &deviceInfo));

    if (hDeviceFind.IsValid())
    {
        do
        {
            if (deviceInfo.fAuthenticated || deviceInfo.fRemembered)
            {
                FBluetoothDeviceInfo Info;
                Info.Name = FString(deviceInfo.szName);
                Info.Address = FString::Printf(TEXT("%02X:%02X:%02X:%02X:%02X:%02X"),
                    deviceInfo.Address.rgBytes[5], deviceInfo.Address.rgBytes[4],
                    deviceInfo.Address.rgBytes[3], deviceInfo.Address.rgBytes[2],
                    deviceInfo.Address.rgBytes[1], deviceInfo.Address.rgBytes[0]);
                Info.bIsConnected = deviceInfo.fConnected;
                Info.bIsAuthenticated = deviceInfo.fAuthenticated;

                ResultList.Add(Info);
            }
        } while (BluetoothFindNextDevice(hDeviceFind, &deviceInfo));
    }

#endif
    return ResultList;
}

bool UBluetoothManager::IsBluetoothDeviceConnected(FString DeviceAddress)
{
    TArray<FBluetoothDeviceInfo> Devices = GetPairedDevices();
    for (const FBluetoothDeviceInfo& Device : Devices)
    {
        if (Device.Address.Equals(DeviceAddress, ESearchCase::IgnoreCase))
        {
            return Device.bIsConnected;
        }
    }
    return false;
}


