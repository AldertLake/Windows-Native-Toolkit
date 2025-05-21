/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

#include "AudioSystemLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// Log category for audio system
DEFINE_LOG_CATEGORY_STATIC(LogAudioSystem, Log, All);

// RAII wrapper for COM objects
#if PLATFORM_WINDOWS
template<typename T>
class FComPtr
{
public:
    T* Ptr = nullptr;

    FComPtr() = default;
    explicit FComPtr(T* InPtr) : Ptr(InPtr) {}
    ~FComPtr() { if (Ptr) Ptr->Release(); }
    FComPtr(const FComPtr&) = delete;
    FComPtr& operator=(const FComPtr&) = delete;

    T* operator->() const { return Ptr; }
    T** operator&() { return &Ptr; }
    bool IsValid() const { return Ptr != nullptr; }

    void Reset()
    {
        if (Ptr)
        {
            Ptr->Release();
            Ptr = nullptr;
        }
    }
};

// Helper to initialize COM and get default audio endpoint
static bool GetDefaultAudioEndpoint(FComPtr<IMMDevice>& OutDevice, FComPtr<IAudioEndpointVolume>& OutEndpoint)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to initialize COM: 0x%08X"), hr);
        return false;
    }

    FComPtr<IMMDeviceEnumerator> Enumerator;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&Enumerator);
    if (FAILED(hr))
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to create device enumerator: 0x%08X"), hr);
        CoUninitialize();
        return false;
    }

    hr = Enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &OutDevice);
    if (FAILED(hr))
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to get default audio endpoint: 0x%08X"), hr);
        CoUninitialize();
        return false;
    }

    hr = OutDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&OutEndpoint);
    if (FAILED(hr))
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to activate audio endpoint: 0x%08X"), hr);
        CoUninitialize();
        return false;
    }

    return true;
}
#endif

float UAudioSystemLibrary::GetSystemVolume()
{
#if PLATFORM_WINDOWS
    FComPtr<IMMDevice> Device;
    FComPtr<IAudioEndpointVolume> Endpoint;

    if (!GetDefaultAudioEndpoint(Device, Endpoint))
    {
        CoUninitialize();
        return -1.0f;
    }

    float Volume = -1.0f;
    HRESULT hr = Endpoint->GetMasterVolumeLevelScalar(&Volume);
    if (FAILED(hr))
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to get volume: 0x%08X"), hr);
    }

    CoUninitialize();
    return Volume;
#else
    UE_LOG(LogAudioSystem, Warning, TEXT("GetSystemVolume not supported on this platform"));
    return -1.0f;
#endif
}

void UAudioSystemLibrary::SetSystemVolume(float Volume)
{
#if PLATFORM_WINDOWS
    Volume = FMath::Clamp(Volume, 0.0f, 1.0f);

    FComPtr<IMMDevice> Device;
    FComPtr<IAudioEndpointVolume> Endpoint;

    if (!GetDefaultAudioEndpoint(Device, Endpoint))
    {
        CoUninitialize();
        return;
    }

    HRESULT hr = Endpoint->SetMasterVolumeLevelScalar(Volume, nullptr);
    if (FAILED(hr))
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to set volume: 0x%08X"), hr);
    }

    CoUninitialize();
#else
    UE_LOG(LogAudioSystem, Warning, TEXT("SetSystemVolume not supported on this platform"));
#endif
}

FString UAudioSystemLibrary::GetCurrentAudioDeviceName()
{
#if PLATFORM_WINDOWS
    FComPtr<IMMDeviceEnumerator> Enumerator;
    FComPtr<IMMDevice> Device;
    FComPtr<IPropertyStore> Props;
    PROPVARIANT VarName;
    PropVariantInit(&VarName);
    FString DeviceName = TEXT("Unknown");

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to initialize COM: 0x%08X"), hr);
        return DeviceName;
    }

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&Enumerator);
    if (FAILED(hr))
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to create device enumerator: 0x%08X"), hr);
        CoUninitialize();
        return DeviceName;
    }

    hr = Enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &Device);
    if (FAILED(hr))
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to get default audio endpoint: 0x%08X"), hr);
        CoUninitialize();
        return DeviceName;
    }

    hr = Device->OpenPropertyStore(STGM_READ, &Props);
    if (FAILED(hr))
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to open property store: 0x%08X"), hr);
        CoUninitialize();
        return DeviceName;
    }

    hr = Props->GetValue(PKEY_Device_FriendlyName, &VarName);
    if (SUCCEEDED(hr) && VarName.pwszVal)
    {
        DeviceName = FString(VarName.pwszVal);
        PropVariantClear(&VarName);
    }
    else
    {
        UE_LOG(LogAudioSystem, Warning, TEXT("Failed to get device name: 0x%08X"), hr);
    }

    CoUninitialize();
    return DeviceName;
#else
    UE_LOG(LogAudioSystem, Warning, TEXT("GetCurrentAudioDeviceName not supported on this platform"));
    return FString(TEXT("Unknown"));
#endif
}