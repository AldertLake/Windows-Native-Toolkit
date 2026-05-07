// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "AudioSystemLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <objbase.h>
#include <propkeydef.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <wrl/client.h>
#include "Windows/HideWindowsPlatformTypes.h"

using Microsoft::WRL::ComPtr;

namespace
{
struct FScopedComInit
{
    HRESULT Result = E_FAIL;
    bool bNeedsUninitialize = false;

    FScopedComInit()
    {
        Result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        bNeedsUninitialize = SUCCEEDED(Result);
    }

    ~FScopedComInit()
    {
        if (bNeedsUninitialize)
        {
            CoUninitialize();
        }
    }

    bool IsUsable() const
    {
        return SUCCEEDED(Result) || Result == RPC_E_CHANGED_MODE;
    }
};

EDataFlow ToDataFlow(EWNTAudioDeviceFlow Flow)
{
    return Flow == EWNTAudioDeviceFlow::Input ? eCapture : eRender;
}

ERole ToRole(EWNTAudioDeviceRole Role)
{
    return Role == EWNTAudioDeviceRole::Communication ? eCommunications : eConsole;
}

bool CreateEnumerator(ComPtr<IMMDeviceEnumerator>& OutEnumerator, FString& OutError)
{
    const HRESULT Hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&OutEnumerator));
    if (FAILED(Hr) || !OutEnumerator)
    {
        OutError = FString::Printf(TEXT("Failed to create Windows audio device enumerator. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }
    return true;
}

bool ReadDeviceInfo(IMMDevice* Device, const FString& DefaultId, const FString& CommunicationId, FAudioDeviceInfo& OutInfo, FString& OutError)
{
    if (!Device)
    {
        OutError = TEXT("Audio device is invalid.");
        return false;
    }

    LPWSTR RawId = nullptr;
    HRESULT Hr = Device->GetId(&RawId);
    if (FAILED(Hr) || !RawId)
    {
        OutError = FString::Printf(TEXT("Failed to read audio device ID. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    OutInfo.DeviceID = FString(RawId);
    CoTaskMemFree(RawId);
    OutInfo.bIsDefaultDevice = OutInfo.DeviceID.Equals(DefaultId, ESearchCase::IgnoreCase);
    OutInfo.bIsCommunicationDevice = OutInfo.DeviceID.Equals(CommunicationId, ESearchCase::IgnoreCase);

    ComPtr<IPropertyStore> Properties;
    Hr = Device->OpenPropertyStore(STGM_READ, &Properties);
    if (SUCCEEDED(Hr) && Properties)
    {
        PROPVARIANT NameValue;
        PropVariantInit(&NameValue);
        if (SUCCEEDED(Properties->GetValue(PKEY_Device_FriendlyName, &NameValue)) && NameValue.vt == VT_LPWSTR && NameValue.pwszVal)
        {
            OutInfo.DeviceName = FString(NameValue.pwszVal);
        }
        PropVariantClear(&NameValue);
    }

    if (OutInfo.DeviceName.IsEmpty())
    {
        OutInfo.DeviceName = TEXT("Unknown Audio Device");
    }

    return true;
}

FString GetDefaultDeviceId(IMMDeviceEnumerator* Enumerator, EDataFlow Flow, ERole Role)
{
    if (!Enumerator)
    {
        return FString();
    }

    ComPtr<IMMDevice> Device;
    if (FAILED(Enumerator->GetDefaultAudioEndpoint(Flow, Role, &Device)) || !Device)
    {
        return FString();
    }

    LPWSTR RawId = nullptr;
    if (FAILED(Device->GetId(&RawId)) || !RawId)
    {
        return FString();
    }

    FString Result(RawId);
    CoTaskMemFree(RawId);
    return Result;
}

bool ActivateEndpointVolumeById(const FString& DeviceID, ComPtr<IAudioEndpointVolume>& OutEndpoint, FString& OutError)
{
    if (DeviceID.TrimStartAndEnd().IsEmpty())
    {
        OutError = TEXT("Audio device ID is empty.");
        return false;
    }

    FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        OutError = TEXT("Failed to initialize COM for audio endpoint access.");
        return false;
    }

    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!CreateEnumerator(Enumerator, OutError))
    {
        return false;
    }

    ComPtr<IMMDevice> Device;
    HRESULT Hr = Enumerator->GetDevice(*DeviceID, &Device);
    if (FAILED(Hr) || !Device)
    {
        OutError = FString::Printf(TEXT("Audio device was not found. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    Hr = Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &OutEndpoint);
    if (FAILED(Hr) || !OutEndpoint)
    {
        OutError = FString::Printf(TEXT("Audio endpoint volume is not available for this device. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return true;
}
}
#endif

TArray<FAudioDeviceInfo> UAudioSystemLibrary::GetAudioDevices(EWNTAudioDeviceFlow Flow)
{
    TArray<FAudioDeviceInfo> Result;
#if PLATFORM_WINDOWS
    FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        return Result;
    }

    FString Error;
    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!CreateEnumerator(Enumerator, Error))
    {
        return Result;
    }

    const EDataFlow DataFlow = ToDataFlow(Flow);
    const FString DefaultId = GetDefaultDeviceId(Enumerator.Get(), DataFlow, eConsole);
    const FString CommunicationId = GetDefaultDeviceId(Enumerator.Get(), DataFlow, eCommunications);

    ComPtr<IMMDeviceCollection> Collection;
    if (FAILED(Enumerator->EnumAudioEndpoints(DataFlow, DEVICE_STATE_ACTIVE, &Collection)) || !Collection)
    {
        return Result;
    }

    UINT Count = 0;
    Collection->GetCount(&Count);
    Result.Reserve(static_cast<int32>(Count));

    for (UINT Index = 0; Index < Count; ++Index)
    {
        ComPtr<IMMDevice> Device;
        if (FAILED(Collection->Item(Index, &Device)) || !Device)
        {
            continue;
        }

        FAudioDeviceInfo Info;
        if (ReadDeviceInfo(Device.Get(), DefaultId, CommunicationId, Info, Error))
        {
            Result.Add(MoveTemp(Info));
        }
    }
#endif
    return Result;
}

bool UAudioSystemLibrary::GetDefaultAudioDevice(EWNTAudioDeviceFlow Flow, EWNTAudioDeviceRole Role, FAudioDeviceInfo& OutDevice, FString& OutError)
{
    OutDevice = FAudioDeviceInfo();
    OutError.Reset();
#if PLATFORM_WINDOWS
    FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        OutError = TEXT("Failed to initialize COM for audio device access.");
        return false;
    }

    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!CreateEnumerator(Enumerator, OutError))
    {
        return false;
    }

    const EDataFlow DataFlow = ToDataFlow(Flow);
    ComPtr<IMMDevice> Device;
    const HRESULT Hr = Enumerator->GetDefaultAudioEndpoint(DataFlow, ToRole(Role), &Device);
    if (FAILED(Hr) || !Device)
    {
        OutError = FString::Printf(TEXT("Default audio device was not found. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return ReadDeviceInfo(Device.Get(), GetDefaultDeviceId(Enumerator.Get(), DataFlow, eConsole), GetDefaultDeviceId(Enumerator.Get(), DataFlow, eCommunications), OutDevice, OutError);
#else
    OutError = TEXT("Audio devices are only available on Windows.");
    return false;
#endif
}

bool UAudioSystemLibrary::SetAudioDeviceVolume(const FString& DeviceID, float Volume, FString& OutError)
{
    OutError.Reset();
#if PLATFORM_WINDOWS
    ComPtr<IAudioEndpointVolume> Endpoint;
    if (!ActivateEndpointVolumeById(DeviceID, Endpoint, OutError))
    {
        return false;
    }

    const HRESULT Hr = Endpoint->SetMasterVolumeLevelScalar(FMath::Clamp(Volume, 0.0f, 1.0f), nullptr);
    if (FAILED(Hr))
    {
        OutError = FString::Printf(TEXT("Failed to set audio volume. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return true;
#else
    OutError = TEXT("Audio volume is only available on Windows.");
    return false;
#endif
}

bool UAudioSystemLibrary::GetAudioDeviceVolume(const FString& DeviceID, float& OutVolume, FString& OutError)
{
    OutVolume = 0.0f;
    OutError.Reset();
#if PLATFORM_WINDOWS
    ComPtr<IAudioEndpointVolume> Endpoint;
    if (!ActivateEndpointVolumeById(DeviceID, Endpoint, OutError))
    {
        return false;
    }

    const HRESULT Hr = Endpoint->GetMasterVolumeLevelScalar(&OutVolume);
    if (FAILED(Hr))
    {
        OutError = FString::Printf(TEXT("Failed to read audio volume. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return true;
#else
    OutError = TEXT("Audio volume is only available on Windows.");
    return false;
#endif
}

bool UAudioSystemLibrary::SetAudioDeviceMuted(const FString& DeviceID, bool bMuted, FString& OutError)
{
    OutError.Reset();
#if PLATFORM_WINDOWS
    ComPtr<IAudioEndpointVolume> Endpoint;
    if (!ActivateEndpointVolumeById(DeviceID, Endpoint, OutError))
    {
        return false;
    }

    const HRESULT Hr = Endpoint->SetMute(bMuted ? 1 : 0, nullptr);
    if (FAILED(Hr))
    {
        OutError = FString::Printf(TEXT("Failed to set audio mute state. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return true;
#else
    OutError = TEXT("Audio mute is only available on Windows.");
    return false;
#endif
}

bool UAudioSystemLibrary::GetAudioDeviceMuted(const FString& DeviceID, bool& bMuted, FString& OutError)
{
    bMuted = false;
    OutError.Reset();
#if PLATFORM_WINDOWS
    ComPtr<IAudioEndpointVolume> Endpoint;
    if (!ActivateEndpointVolumeById(DeviceID, Endpoint, OutError))
    {
        return false;
    }

    BOOL bWindowsMuted = 0;
    const HRESULT Hr = Endpoint->GetMute(&bWindowsMuted);
    if (FAILED(Hr))
    {
        OutError = FString::Printf(TEXT("Failed to read audio mute state. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    bMuted = bWindowsMuted != 0;
    return true;
#else
    OutError = TEXT("Audio mute is only available on Windows.");
    return false;
#endif
}

bool UAudioSystemLibrary::GetAudioDevicePeak(const FString& DeviceID, float& OutPeakValue, FString& OutError)
{
    OutPeakValue = 0.0f;
    OutError.Reset();
#if PLATFORM_WINDOWS
    if (DeviceID.TrimStartAndEnd().IsEmpty())
    {
        OutError = TEXT("Audio device ID is empty.");
        return false;
    }

    FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        OutError = TEXT("Failed to initialize COM for audio metering.");
        return false;
    }

    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!CreateEnumerator(Enumerator, OutError))
    {
        return false;
    }

    ComPtr<IMMDevice> Device;
    HRESULT Hr = Enumerator->GetDevice(*DeviceID, &Device);
    if (FAILED(Hr) || !Device)
    {
        OutError = FString::Printf(TEXT("Audio device was not found. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    ComPtr<IAudioMeterInformation> Meter;
    Hr = Device->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, nullptr, &Meter);
    if (FAILED(Hr) || !Meter)
    {
        OutError = FString::Printf(TEXT("Audio meter is not available for this device. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    Hr = Meter->GetPeakValue(&OutPeakValue);
    if (FAILED(Hr))
    {
        OutError = FString::Printf(TEXT("Failed to read audio peak value. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return true;
#else
    OutError = TEXT("Audio metering is only available on Windows.");
    return false;
#endif
}


