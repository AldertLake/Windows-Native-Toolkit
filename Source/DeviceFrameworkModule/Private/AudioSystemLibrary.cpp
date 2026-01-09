// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "AudioSystemLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <wrl/client.h> 
#include "Windows/HideWindowsPlatformTypes.h"

#pragma comment(lib, "Mmdevapi.lib") 

using namespace Microsoft::WRL;

static ComPtr<IMMDeviceEnumerator> GlobalEnumerator = nullptr;

static bool GetEnumerator(ComPtr<IMMDeviceEnumerator>& OutEnumerator)
{
    if (GlobalEnumerator)
    {
        OutEnumerator = GlobalEnumerator;
        return true;
    }

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&GlobalEnumerator));
    if (SUCCEEDED(hr))
    {
        OutEnumerator = GlobalEnumerator;
        return true;
    }
    return false;
}

static bool GetDefaultAudioEndpoint(EDataFlow DataFlow, ComPtr<IAudioEndpointVolume>& OutEndpoint, ComPtr<IMMDevice>& OutDevice)
{
    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!GetEnumerator(Enumerator)) return false;

    HRESULT hr = Enumerator->GetDefaultAudioEndpoint(DataFlow, eConsole, &OutDevice);
    if (FAILED(hr)) return false;

    hr = OutDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &OutEndpoint);
    return SUCCEEDED(hr);
}
#endif

float UAudioSystemLibrary::GetSystemVolume()
{
#if PLATFORM_WINDOWS
    ComPtr<IAudioEndpointVolume> Endpoint;
    ComPtr<IMMDevice> Device;

    if (GetDefaultAudioEndpoint(eRender, Endpoint, Device))
    {
        float CurrentVolume = 0.0f;
        if (SUCCEEDED(Endpoint->GetMasterVolumeLevelScalar(&CurrentVolume)))
        {
            return CurrentVolume;
        }
    }
#endif
    return -1.0f;
}

void UAudioSystemLibrary::SetSystemVolume(float Volume)
{
#if PLATFORM_WINDOWS
    ComPtr<IAudioEndpointVolume> Endpoint;
    ComPtr<IMMDevice> Device;

    if (GetDefaultAudioEndpoint(eRender, Endpoint, Device))
    {
        Endpoint->SetMasterVolumeLevelScalar(FMath::Clamp(Volume, 0.0f, 1.0f), nullptr);
    }
#endif
}

FString UAudioSystemLibrary::GetCurrentAudioDeviceName()
{
#if PLATFORM_WINDOWS
    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!GetEnumerator(Enumerator)) return TEXT("Unknown");

    ComPtr<IMMDevice> Device;
    if (FAILED(Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Device)))
        return TEXT("No Device");

    ComPtr<IPropertyStore> Props;
    if (SUCCEEDED(Device->OpenPropertyStore(STGM_READ, &Props)))
    {
        PROPVARIANT VarName;
        PropVariantInit(&VarName);
        if (SUCCEEDED(Props->GetValue(PKEY_Device_FriendlyName, &VarName)) && VarName.vt == VT_LPWSTR)
        {
            FString Name(VarName.pwszVal);
            PropVariantClear(&VarName);
            return Name;
        }
        PropVariantClear(&VarName);
    }
#endif
    return TEXT("Unknown");
}

TArray<FAudioDeviceInfo> UAudioSystemLibrary::GetAllAudioOutputDevices()
{
    TArray<FAudioDeviceInfo> Result;
#if PLATFORM_WINDOWS
    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!GetEnumerator(Enumerator)) return Result;

    FString DefaultConsoleID;
    FString DefaultCommID;
    LPWSTR wstrDefID = nullptr;
    ComPtr<IMMDevice> DefDevice;

    if (SUCCEEDED(Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &DefDevice)))
    {
        if (SUCCEEDED(DefDevice->GetId(&wstrDefID)))
        {
            DefaultConsoleID = FString(wstrDefID);
            CoTaskMemFree(wstrDefID);
        }
    }

    DefDevice.Reset();
    if (SUCCEEDED(Enumerator->GetDefaultAudioEndpoint(eRender, eCommunications, &DefDevice)))
    {
        if (SUCCEEDED(DefDevice->GetId(&wstrDefID)))
        {
            DefaultCommID = FString(wstrDefID);
            CoTaskMemFree(wstrDefID);
        }
    }

    ComPtr<IMMDeviceCollection> Collection;
    if (FAILED(Enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &Collection))) return Result;

    UINT Count = 0;
    Collection->GetCount(&Count);

    for (UINT i = 0; i < Count; i++)
    {
        ComPtr<IMMDevice> Device;
        if (SUCCEEDED(Collection->Item(i, &Device)))
        {
            FAudioDeviceInfo Info;
            LPWSTR wstrID = nullptr;

            if (SUCCEEDED(Device->GetId(&wstrID)))
            {
                Info.DeviceID = FString(wstrID);
                CoTaskMemFree(wstrID);
            }

            if (!Info.DeviceID.IsEmpty())
            {
                if (Info.DeviceID == DefaultConsoleID) Info.bIsDefaultDevice = true;
                if (Info.DeviceID == DefaultCommID)    Info.bIsCommunicationDevice = true;
            }

            ComPtr<IPropertyStore> Props;
            if (SUCCEEDED(Device->OpenPropertyStore(STGM_READ, &Props)))
            {
                PROPVARIANT VarName;
                PropVariantInit(&VarName);
                if (SUCCEEDED(Props->GetValue(PKEY_Device_FriendlyName, &VarName)) && VarName.vt == VT_LPWSTR)
                {
                    Info.DeviceName = FString(VarName.pwszVal);
                }
                PropVariantClear(&VarName);
            }
            Result.Add(Info);
        }
    }
#endif
    return Result;
}

void UAudioSystemLibrary::SetVolumeForDevice(const FString& DeviceID, float Volume)
{
#if PLATFORM_WINDOWS
    if (DeviceID.IsEmpty()) return;

    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!GetEnumerator(Enumerator)) return;

    ComPtr<IMMDevice> Device;
    if (FAILED(Enumerator->GetDevice(*DeviceID, &Device))) return;

    ComPtr<IAudioEndpointVolume> Endpoint;
    if (SUCCEEDED(Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &Endpoint)))
    {
        Endpoint->SetMasterVolumeLevelScalar(FMath::Clamp(Volume, 0.0f, 1.0f), nullptr);
    }
#endif
}

TArray<FAudioDeviceInfo> UAudioSystemLibrary::GetAllAudioInputDevices()
{
    TArray<FAudioDeviceInfo> Result;
#if PLATFORM_WINDOWS
    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!GetEnumerator(Enumerator)) return Result;

    FString DefaultConsoleID;
    FString DefaultCommID;
    LPWSTR wstrDefID = nullptr;
    ComPtr<IMMDevice> DefDevice;

    if (SUCCEEDED(Enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &DefDevice)))
    {
        if (SUCCEEDED(DefDevice->GetId(&wstrDefID)))
        {
            DefaultConsoleID = FString(wstrDefID);
            CoTaskMemFree(wstrDefID);
        }
    }

    DefDevice.Reset();
    if (SUCCEEDED(Enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &DefDevice)))
    {
        if (SUCCEEDED(DefDevice->GetId(&wstrDefID)))
        {
            DefaultCommID = FString(wstrDefID);
            CoTaskMemFree(wstrDefID);
        }
    }

    ComPtr<IMMDeviceCollection> Collection;
    if (FAILED(Enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &Collection))) return Result;

    UINT Count = 0;
    Collection->GetCount(&Count);

    for (UINT i = 0; i < Count; i++)
    {
        ComPtr<IMMDevice> Device;
        if (SUCCEEDED(Collection->Item(i, &Device)))
        {
            FAudioDeviceInfo Info;
            LPWSTR wstrID = nullptr;

            if (SUCCEEDED(Device->GetId(&wstrID)))
            {
                Info.DeviceID = FString(wstrID);
                CoTaskMemFree(wstrID);
            }

            if (!Info.DeviceID.IsEmpty())
            {
                if (Info.DeviceID == DefaultConsoleID) Info.bIsDefaultDevice = true;
                if (Info.DeviceID == DefaultCommID)    Info.bIsCommunicationDevice = true;
            }

            ComPtr<IPropertyStore> Props;
            if (SUCCEEDED(Device->OpenPropertyStore(STGM_READ, &Props)))
            {
                PROPVARIANT VarName;
                PropVariantInit(&VarName);
                if (SUCCEEDED(Props->GetValue(PKEY_Device_FriendlyName, &VarName)) && VarName.vt == VT_LPWSTR)
                {
                    Info.DeviceName = FString(VarName.pwszVal);
                }
                PropVariantClear(&VarName);
            }
            Result.Add(Info);
        }
    }
#endif
    return Result;
}

void UAudioSystemLibrary::SetInputVolumeForDevice(const FString& DeviceID, float Volume)
{
#if PLATFORM_WINDOWS
    if (DeviceID.IsEmpty()) return;

    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!GetEnumerator(Enumerator)) return;

    ComPtr<IMMDevice> Device;
    if (FAILED(Enumerator->GetDevice(*DeviceID, &Device))) return;

    ComPtr<IAudioEndpointVolume> Endpoint;
    if (SUCCEEDED(Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &Endpoint)))
    {
        Endpoint->SetMasterVolumeLevelScalar(FMath::Clamp(Volume, 0.0f, 1.0f), nullptr);
    }
#endif
}

float UAudioSystemLibrary::GetOutputDeviceVolume(const FString& DeviceID)
{
#if PLATFORM_WINDOWS
    if (DeviceID.IsEmpty()) return -1.0f;

    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!GetEnumerator(Enumerator)) return -1.0f;

    ComPtr<IMMDevice> Device;
    if (FAILED(Enumerator->GetDevice(*DeviceID, &Device))) return -1.0f;

    ComPtr<IAudioEndpointVolume> Endpoint;
    if (SUCCEEDED(Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &Endpoint)))
    {
        float CurrentVolume = 0.0f;
        if (SUCCEEDED(Endpoint->GetMasterVolumeLevelScalar(&CurrentVolume)))
        {
            return CurrentVolume;
        }
    }
#endif
    return -1.0f;
}

float UAudioSystemLibrary::GetInputDeviceVolume(const FString& DeviceID)
{
    return GetOutputDeviceVolume(DeviceID);
}

float UAudioSystemLibrary::GetAudioDevicePeakValue(const FString& DeviceID)
{
#if PLATFORM_WINDOWS
    if (DeviceID.IsEmpty()) return 0.0f;

    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!GetEnumerator(Enumerator)) return 0.0f;

    ComPtr<IMMDevice> Device;
    if (FAILED(Enumerator->GetDevice(*DeviceID, &Device))) return 0.0f;

    ComPtr<IAudioMeterInformation> MeterInfo;
    if (SUCCEEDED(Device->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, nullptr, &MeterInfo)))
    {
        float PeakValue = 0.0f;
        if (SUCCEEDED(MeterInfo->GetPeakValue(&PeakValue)))
        {
            return PeakValue;
        }
    }
#endif
    return 0.0f;
}