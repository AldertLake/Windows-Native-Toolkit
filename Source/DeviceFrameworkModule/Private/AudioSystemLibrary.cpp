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

struct FLocalComInit
{
    FLocalComInit() { CoInitialize(nullptr); }
    ~FLocalComInit() { CoUninitialize(); }
};
#endif

#if PLATFORM_WINDOWS
static bool GetDefaultAudioEndpoint(ComPtr<IAudioEndpointVolume>& OutEndpoint, ComPtr<IMMDevice>& OutDevice)
{
    ComPtr<IMMDeviceEnumerator> Enumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&Enumerator));
    if (FAILED(hr)) return false;

    hr = Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &OutDevice);
    if (FAILED(hr)) return false;

    hr = OutDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &OutEndpoint);
    return SUCCEEDED(hr);
}
#endif

float UAudioSystemLibrary::GetSystemVolume()
{
#if PLATFORM_WINDOWS
    FLocalComInit ComInit;
    ComPtr<IAudioEndpointVolume> Endpoint;
    ComPtr<IMMDevice> Device;

    if (GetDefaultAudioEndpoint(Endpoint, Device))
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
    FLocalComInit ComInit;
    ComPtr<IAudioEndpointVolume> Endpoint;
    ComPtr<IMMDevice> Device;

    if (GetDefaultAudioEndpoint(Endpoint, Device))
    {
        Endpoint->SetMasterVolumeLevelScalar(FMath::Clamp(Volume, 0.0f, 1.0f), nullptr);
    }
#endif
}

FString UAudioSystemLibrary::GetCurrentAudioDeviceName()
{
#if PLATFORM_WINDOWS
    FLocalComInit ComInit;
    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&Enumerator))))
        return TEXT("Unknown");

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

// --- OUTPUT DEVICES ---

TArray<FAudioDeviceInfo> UAudioSystemLibrary::GetAllAudioOutputDevices()
{
    TArray<FAudioDeviceInfo> Result;
#if PLATFORM_WINDOWS
    FLocalComInit ComInit;

    ComPtr<IMMDeviceEnumerator> Enumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&Enumerator));
    if (FAILED(hr)) return Result;

    ComPtr<IMMDeviceCollection> Collection;
    hr = Enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &Collection);
    if (FAILED(hr)) return Result;

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
    FLocalComInit ComInit;

    ComPtr<IMMDeviceEnumerator> Enumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&Enumerator));
    if (FAILED(hr)) return;

    ComPtr<IMMDevice> Device;
    hr = Enumerator->GetDevice(*DeviceID, &Device);
    if (FAILED(hr)) return;

    ComPtr<IAudioEndpointVolume> Endpoint;
    hr = Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &Endpoint);

    if (SUCCEEDED(hr))
    {
        Endpoint->SetMasterVolumeLevelScalar(FMath::Clamp(Volume, 0.0f, 1.0f), nullptr);
    }
#endif
}

// --- INPUT DEVICES ---

TArray<FAudioDeviceInfo> UAudioSystemLibrary::GetAllAudioInputDevices()
{
    TArray<FAudioDeviceInfo> Result;
#if PLATFORM_WINDOWS
    FLocalComInit ComInit;

    ComPtr<IMMDeviceEnumerator> Enumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&Enumerator));
    if (FAILED(hr)) return Result;

    ComPtr<IMMDeviceCollection> Collection;

    hr = Enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &Collection);
    if (FAILED(hr)) return Result;

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
    FLocalComInit ComInit;

    ComPtr<IMMDeviceEnumerator> Enumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&Enumerator));
    if (FAILED(hr)) return;

    ComPtr<IMMDevice> Device;

    hr = Enumerator->GetDevice(*DeviceID, &Device);
    if (FAILED(hr)) return;

    ComPtr<IAudioEndpointVolume> Endpoint;
    hr = Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &Endpoint);

    if (SUCCEEDED(hr))
    {
        Endpoint->SetMasterVolumeLevelScalar(FMath::Clamp(Volume, 0.0f, 1.0f), nullptr);
    }
#endif
}