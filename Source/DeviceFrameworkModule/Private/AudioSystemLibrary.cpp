// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "AudioSystemLibrary.h"
#include "WNTPrivateUtils.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <audioclient.h>
#include <endpointvolume.h>
#include <ksmedia.h>
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
struct DEVICE_SHARE_MODE;

struct __declspec(uuid("f8679f50-850a-41cf-9c72-430f290290c8")) IPolicyConfig : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR DeviceId, WAVEFORMATEX** Format) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR DeviceId, BOOL bDefault, WAVEFORMATEX** Format) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR DeviceId) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR DeviceId, WAVEFORMATEX* EndpointFormat, WAVEFORMATEX* MixFormat) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR DeviceId, BOOL bDefault, INT64* DefaultPeriod, INT64* MinimumPeriod) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR DeviceId, INT64* Period) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR DeviceId, DEVICE_SHARE_MODE* Mode) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR DeviceId, DEVICE_SHARE_MODE* Mode) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR DeviceId, BOOL bFxStore, const PROPERTYKEY& Key, PROPVARIANT* Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR DeviceId, BOOL bFxStore, const PROPERTYKEY& Key, PROPVARIANT* Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR DeviceId, ERole Role) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR DeviceId, BOOL bVisible) = 0;
};

class __declspec(uuid("870af99c-171d-4f9e-af0d-e63df40c2bc9")) CPolicyConfigClient;

struct FAudioFormatCandidate
{
    TArray<uint8> Bytes;
    FWNTAudioDeviceFormat Info;
};

static EDataFlow ToDataFlow(EWNTAudioDeviceFlow Flow)
{
    return Flow == EWNTAudioDeviceFlow::Input ? eCapture : eRender;
}

static ERole ToRole(EWNTAudioDeviceRole Role)
{
    return Role == EWNTAudioDeviceRole::Communication ? eCommunications : eConsole;
}

static bool CreateEnumerator(ComPtr<IMMDeviceEnumerator>& OutEnumerator, FString& OutError)
{
    const HRESULT Hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&OutEnumerator));
    if (FAILED(Hr) || !OutEnumerator)
    {
        OutError = FString::Printf(TEXT("Failed to create the Windows audio-device enumerator. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return true;
}

static bool CreatePolicyConfig(ComPtr<IPolicyConfig>& OutPolicyConfig, FString& OutError)
{
    const HRESULT Hr = CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&OutPolicyConfig));
    if (FAILED(Hr) || !OutPolicyConfig)
    {
        OutError = FString::Printf(TEXT("Failed to create the Windows audio policy configuration client. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return true;
}

static bool GetDeviceById(const FString& DeviceID, ComPtr<IMMDevice>& OutDevice, FString& OutError)
{
    if (DeviceID.TrimStartAndEnd().IsEmpty())
    {
        OutError = TEXT("Audio device ID is empty.");
        return false;
    }

    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!CreateEnumerator(Enumerator, OutError))
    {
        return false;
    }

    const HRESULT Hr = Enumerator->GetDevice(*DeviceID, &OutDevice);
    if (FAILED(Hr) || !OutDevice)
    {
        OutError = FString::Printf(TEXT("Audio device was not found. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return true;
}

static bool ActivateEndpointVolumeById(const FString& DeviceID, ComPtr<IAudioEndpointVolume>& OutEndpoint, FString& OutError)
{
    ComPtr<IMMDevice> Device;
    if (!GetDeviceById(DeviceID, Device, OutError))
    {
        return false;
    }

    const HRESULT Hr = Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &OutEndpoint);
    if (FAILED(Hr) || !OutEndpoint)
    {
        OutError = FString::Printf(TEXT("Audio endpoint volume is not available for this device. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return true;
}

static bool ActivateAudioClientById(const FString& DeviceID, ComPtr<IAudioClient>& OutAudioClient, FString& OutError)
{
    ComPtr<IMMDevice> Device;
    if (!GetDeviceById(DeviceID, Device, OutError))
    {
        return false;
    }

    const HRESULT Hr = Device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &OutAudioClient);
    if (FAILED(Hr) || !OutAudioClient)
    {
        OutError = FString::Printf(TEXT("Audio client is not available for this device. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return false;
    }

    return true;
}

static FString GetDefaultDeviceId(IMMDeviceEnumerator* Enumerator, EDataFlow Flow, ERole Role)
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

static bool ReadDeviceInfo(IMMDevice* Device, const FString& DefaultId, const FString& CommunicationId, FAudioDeviceInfo& OutInfo, FString& OutError)
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
        OutError = FString::Printf(TEXT("Failed to read the audio device ID. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
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

static FString DescribeChannelCount(int32 ChannelCount)
{
    switch (ChannelCount)
    {
    case 1: return TEXT("Mono");
    case 2: return TEXT("Stereo");
    default: return FString::Printf(TEXT("%d Channels"), ChannelCount);
    }
}

static bool IsFloatFormat(const WAVEFORMATEX* Format)
{
    if (!Format)
    {
        return false;
    }

    if (Format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        return true;
    }

    if (Format->wFormatTag == WAVE_FORMAT_EXTENSIBLE && Format->cbSize >= (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
    {
        const WAVEFORMATEXTENSIBLE* Extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(Format);
        return IsEqualGUID(Extensible->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) != 0;
    }

    return false;
}

static FString DescribeAudioFormat(const WAVEFORMATEX* Format)
{
    if (!Format)
    {
        return TEXT("Unknown Format");
    }

    const FString ChannelText = DescribeChannelCount(Format->nChannels);
    const FString TypeText = IsFloatFormat(Format) ? TEXT("Float") : TEXT("PCM");
    return FString::Printf(TEXT("%s, %d-bit, %d Hz (%s)"), *ChannelText, Format->wBitsPerSample, static_cast<int32>(Format->nSamplesPerSec), *TypeText);
}

static bool CopyWaveFormat(const WAVEFORMATEX* Format, TArray<uint8>& OutBytes)
{
    if (!Format)
    {
        return false;
    }

    const uint32 ByteCount = sizeof(WAVEFORMATEX) + Format->cbSize;
    OutBytes.SetNumUninitialized(ByteCount);
    FMemory::Memcpy(OutBytes.GetData(), Format, ByteCount);
    return true;
}

static uint32 GetChannelMask(const WAVEFORMATEX* Format)
{
    if (Format && Format->wFormatTag == WAVE_FORMAT_EXTENSIBLE && Format->cbSize >= (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
    {
        const WAVEFORMATEXTENSIBLE* Extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(Format);
        return Extensible->dwChannelMask;
    }

    if (!Format)
    {
        return 0;
    }

    if (Format->nChannels == 1)
    {
        return SPEAKER_FRONT_CENTER;
    }

    if (Format->nChannels == 2)
    {
        return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    }

    return 0;
}

static void AddCandidateFormat(int32 Channels, int32 SampleRate, int32 BitsPerSample, bool bFloat, uint32 ChannelMask, TArray<WAVEFORMATEXTENSIBLE>& OutCandidates)
{
    if (Channels <= 0 || SampleRate <= 0 || BitsPerSample <= 0)
    {
        return;
    }

    WAVEFORMATEXTENSIBLE Candidate = {};
    Candidate.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    Candidate.Format.nChannels = static_cast<WORD>(Channels);
    Candidate.Format.nSamplesPerSec = static_cast<DWORD>(SampleRate);
    Candidate.Format.wBitsPerSample = static_cast<WORD>(BitsPerSample);
    Candidate.Format.nBlockAlign = static_cast<WORD>((Channels * BitsPerSample) / 8);
    Candidate.Format.nAvgBytesPerSec = Candidate.Format.nSamplesPerSec * Candidate.Format.nBlockAlign;
    Candidate.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    Candidate.Samples.wValidBitsPerSample = static_cast<WORD>(BitsPerSample);
    Candidate.dwChannelMask = ChannelMask;
    Candidate.SubFormat = bFloat ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
    OutCandidates.Add(Candidate);
}

static void BuildCandidateFormats(const WAVEFORMATEX* BaseFormat, TArray<WAVEFORMATEXTENSIBLE>& OutCandidates)
{
    const int32 Channels = FMath::Max<int32>(BaseFormat ? BaseFormat->nChannels : 2, 1);
    const uint32 ChannelMask = GetChannelMask(BaseFormat);
    const bool bBaseFloat = IsFloatFormat(BaseFormat);
    const int32 BaseBits = BaseFormat ? BaseFormat->wBitsPerSample : 16;
    const int32 BaseRate = BaseFormat ? static_cast<int32>(BaseFormat->nSamplesPerSec) : 48000;

    AddCandidateFormat(Channels, BaseRate, BaseBits, bBaseFloat, ChannelMask, OutCandidates);

    static const int32 SampleRates[] = { 8000, 11025, 16000, 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000 };
    static const int32 PcmBits[] = { 16, 24, 32 };

    for (const int32 SampleRate : SampleRates)
    {
        for (const int32 Bits : PcmBits)
        {
            AddCandidateFormat(Channels, SampleRate, Bits, false, ChannelMask, OutCandidates);
        }

        AddCandidateFormat(Channels, SampleRate, 32, true, ChannelMask, OutCandidates);
    }
}

static TArray<FAudioFormatCandidate> QuerySupportedFormatsInternal(const FString& DeviceID, FString& OutError)
{
    TArray<FAudioFormatCandidate> Result;

    ComPtr<IAudioClient> AudioClient;
    if (!ActivateAudioClientById(DeviceID, AudioClient, OutError))
    {
        return Result;
    }

    ComPtr<IPolicyConfig> PolicyConfig;
    CreatePolicyConfig(PolicyConfig, OutError);

    WAVEFORMATEX* BaseFormat = nullptr;
    HRESULT Hr = PolicyConfig ? PolicyConfig->GetDeviceFormat(*DeviceID, 0, &BaseFormat) : E_FAIL;
    if (FAILED(Hr) || !BaseFormat)
    {
        Hr = AudioClient->GetMixFormat(&BaseFormat);
    }

    if (FAILED(Hr) || !BaseFormat)
    {
        OutError = FString::Printf(TEXT("Failed to read the device base format. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
        return Result;
    }

    TArray<WAVEFORMATEXTENSIBLE> Candidates;
    BuildCandidateFormats(BaseFormat, Candidates);

    TSet<FString> UniqueKeys;
    for (WAVEFORMATEXTENSIBLE Candidate : Candidates)
    {
        WAVEFORMATEX* ClosestMatch = nullptr;
        const HRESULT SupportedHr = AudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &Candidate.Format, &ClosestMatch);

        const WAVEFORMATEX* AcceptedFormat = nullptr;
        if (SupportedHr == S_OK)
        {
            AcceptedFormat = &Candidate.Format;
        }
        else if (SupportedHr == S_FALSE && ClosestMatch)
        {
            AcceptedFormat = ClosestMatch;
        }

        if (AcceptedFormat)
        {
            FAudioFormatCandidate Entry;
            Entry.Info.ChannelCount = AcceptedFormat->nChannels;
            Entry.Info.SampleRate = static_cast<int32>(AcceptedFormat->nSamplesPerSec);
            Entry.Info.BitsPerSample = AcceptedFormat->wBitsPerSample;
            Entry.Info.Description = DescribeAudioFormat(AcceptedFormat);

            const FString Key = FString::Printf(TEXT("%d|%d|%d|%d"), Entry.Info.ChannelCount, Entry.Info.SampleRate, Entry.Info.BitsPerSample, IsFloatFormat(AcceptedFormat) ? 1 : 0);
            if (!UniqueKeys.Contains(Key) && CopyWaveFormat(AcceptedFormat, Entry.Bytes))
            {
                UniqueKeys.Add(Key);
                Result.Add(MoveTemp(Entry));
            }
        }

        if (ClosestMatch)
        {
            CoTaskMemFree(ClosestMatch);
        }
    }

    if (BaseFormat)
    {
        CoTaskMemFree(BaseFormat);
    }

    Result.Sort([](const FAudioFormatCandidate& A, const FAudioFormatCandidate& B)
    {
        if (A.Info.SampleRate != B.Info.SampleRate)
        {
            return A.Info.SampleRate < B.Info.SampleRate;
        }

        if (A.Info.BitsPerSample != B.Info.BitsPerSample)
        {
            return A.Info.BitsPerSample < B.Info.BitsPerSample;
        }

        return A.Info.ChannelCount < B.Info.ChannelCount;
    });

    for (int32 Index = 0; Index < Result.Num(); ++Index)
    {
        Result[Index].Info.FormatIndex = Index;
    }

    return Result;
}

static bool SetDefaultEndpointInternal(const FString& DeviceID, ERole Role, const TCHAR* Context)
{
    if (DeviceID.TrimStartAndEnd().IsEmpty())
    {
        WNT_Private::LogWntError(Context, TEXT("Audio device ID is empty."));
        return false;
    }

    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        WNT_Private::LogWntError(Context, TEXT("Failed to initialize COM for audio policy configuration."));
        return false;
    }

    FString ErrorMessage;
    ComPtr<IPolicyConfig> PolicyConfig;
    if (!CreatePolicyConfig(PolicyConfig, ErrorMessage))
    {
        WNT_Private::LogWntError(Context, ErrorMessage);
        return false;
    }

    const HRESULT Hr = PolicyConfig->SetDefaultEndpoint(*DeviceID, Role);
    if (FAILED(Hr))
    {
        WNT_Private::LogWntError(Context, FString::Printf(TEXT("Windows rejected the default endpoint request. HRESULT: 0x%08X"), static_cast<uint32>(Hr)));
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
    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        return Result;
    }

    FString ErrorMessage;
    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!CreateEnumerator(Enumerator, ErrorMessage))
    {
        WNT_Private::LogWntError(TEXT("Get Audio Devices"), ErrorMessage);
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
        if (ReadDeviceInfo(Device.Get(), DefaultId, CommunicationId, Info, ErrorMessage))
        {
            Result.Add(MoveTemp(Info));
        }
    }
#endif

    return Result;
}

bool UAudioSystemLibrary::GetDefaultAudioDevice(EWNTAudioDeviceFlow Flow, EWNTAudioDeviceRole Role, FAudioDeviceInfo& OutDevice)
{
    OutDevice = FAudioDeviceInfo();

#if PLATFORM_WINDOWS
    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        WNT_Private::LogWntError(TEXT("Get Default Audio Device"), TEXT("Failed to initialize COM for audio device access."));
        return false;
    }

    FString ErrorMessage;
    ComPtr<IMMDeviceEnumerator> Enumerator;
    if (!CreateEnumerator(Enumerator, ErrorMessage))
    {
        WNT_Private::LogWntError(TEXT("Get Default Audio Device"), ErrorMessage);
        return false;
    }

    const EDataFlow DataFlow = ToDataFlow(Flow);
    ComPtr<IMMDevice> Device;
    const HRESULT Hr = Enumerator->GetDefaultAudioEndpoint(DataFlow, ToRole(Role), &Device);
    if (FAILED(Hr) || !Device)
    {
        WNT_Private::LogWntError(TEXT("Get Default Audio Device"), FString::Printf(TEXT("Default audio device was not found. HRESULT: 0x%08X"), static_cast<uint32>(Hr)));
        return false;
    }

    const FString DefaultId = GetDefaultDeviceId(Enumerator.Get(), DataFlow, eConsole);
    const FString CommunicationId = GetDefaultDeviceId(Enumerator.Get(), DataFlow, eCommunications);

    if (!ReadDeviceInfo(Device.Get(), DefaultId, CommunicationId, OutDevice, ErrorMessage))
    {
        WNT_Private::LogWntError(TEXT("Get Default Audio Device"), ErrorMessage);
        return false;
    }

    return true;
#else
    WNT_Private::LogWntError(TEXT("Get Default Audio Device"), TEXT("Audio devices are only available on Windows."));
    return false;
#endif
}

bool UAudioSystemLibrary::SetDefaultAudioDevice(const FString& DeviceID)
{
#if PLATFORM_WINDOWS
    const bool bConsoleSet = SetDefaultEndpointInternal(DeviceID, eConsole, TEXT("Set Default Audio Device"));
    const bool bMultimediaSet = SetDefaultEndpointInternal(DeviceID, eMultimedia, TEXT("Set Default Audio Device"));
    return bConsoleSet && bMultimediaSet;
#else
    WNT_Private::LogWntError(TEXT("Set Default Audio Device"), TEXT("Audio devices are only available on Windows."));
    return false;
#endif
}

bool UAudioSystemLibrary::SetCommunicationAudioDevice(const FString& DeviceID)
{
#if PLATFORM_WINDOWS
    return SetDefaultEndpointInternal(DeviceID, eCommunications, TEXT("Set Communication Audio Device"));
#else
    WNT_Private::LogWntError(TEXT("Set Communication Audio Device"), TEXT("Audio devices are only available on Windows."));
    return false;
#endif
}

bool UAudioSystemLibrary::SetAudioDeviceVolume(const FString& DeviceID, float Volume)
{
#if PLATFORM_WINDOWS
    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        WNT_Private::LogWntError(TEXT("Set Audio Volume"), TEXT("Failed to initialize COM for audio volume control."));
        return false;
    }

    FString ErrorMessage;
    ComPtr<IAudioEndpointVolume> Endpoint;
    if (!ActivateEndpointVolumeById(DeviceID, Endpoint, ErrorMessage))
    {
        WNT_Private::LogWntError(TEXT("Set Audio Volume"), ErrorMessage);
        return false;
    }

    const HRESULT Hr = Endpoint->SetMasterVolumeLevelScalar(FMath::Clamp(Volume, 0.0f, 1.0f), nullptr);
    if (FAILED(Hr))
    {
        WNT_Private::LogWntError(TEXT("Set Audio Volume"), FString::Printf(TEXT("Failed to set audio volume. HRESULT: 0x%08X"), static_cast<uint32>(Hr)));
        return false;
    }

    return true;
#else
    WNT_Private::LogWntError(TEXT("Set Audio Volume"), TEXT("Audio volume is only available on Windows."));
    return false;
#endif
}

bool UAudioSystemLibrary::GetAudioDeviceVolume(const FString& DeviceID, float& OutVolume)
{
    OutVolume = 0.0f;

#if PLATFORM_WINDOWS
    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        WNT_Private::LogWntError(TEXT("Get Audio Volume"), TEXT("Failed to initialize COM for audio volume access."));
        return false;
    }

    FString ErrorMessage;
    ComPtr<IAudioEndpointVolume> Endpoint;
    if (!ActivateEndpointVolumeById(DeviceID, Endpoint, ErrorMessage))
    {
        WNT_Private::LogWntError(TEXT("Get Audio Volume"), ErrorMessage);
        return false;
    }

    const HRESULT Hr = Endpoint->GetMasterVolumeLevelScalar(&OutVolume);
    if (FAILED(Hr))
    {
        WNT_Private::LogWntError(TEXT("Get Audio Volume"), FString::Printf(TEXT("Failed to read audio volume. HRESULT: 0x%08X"), static_cast<uint32>(Hr)));
        return false;
    }

    return true;
#else
    WNT_Private::LogWntError(TEXT("Get Audio Volume"), TEXT("Audio volume is only available on Windows."));
    return false;
#endif
}

bool UAudioSystemLibrary::SetAudioDeviceMuted(const FString& DeviceID, bool bMuted)
{
#if PLATFORM_WINDOWS
    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        WNT_Private::LogWntError(TEXT("Set Audio Muted"), TEXT("Failed to initialize COM for audio mute control."));
        return false;
    }

    FString ErrorMessage;
    ComPtr<IAudioEndpointVolume> Endpoint;
    if (!ActivateEndpointVolumeById(DeviceID, Endpoint, ErrorMessage))
    {
        WNT_Private::LogWntError(TEXT("Set Audio Muted"), ErrorMessage);
        return false;
    }

    const HRESULT Hr = Endpoint->SetMute(bMuted ? 1 : 0, nullptr);
    if (FAILED(Hr))
    {
        WNT_Private::LogWntError(TEXT("Set Audio Muted"), FString::Printf(TEXT("Failed to set the audio mute state. HRESULT: 0x%08X"), static_cast<uint32>(Hr)));
        return false;
    }

    return true;
#else
    WNT_Private::LogWntError(TEXT("Set Audio Muted"), TEXT("Audio mute is only available on Windows."));
    return false;
#endif
}

bool UAudioSystemLibrary::GetAudioDeviceMuted(const FString& DeviceID, bool& bMuted)
{
    bMuted = false;

#if PLATFORM_WINDOWS
    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        WNT_Private::LogWntError(TEXT("Is Audio Muted"), TEXT("Failed to initialize COM for audio mute access."));
        return false;
    }

    FString ErrorMessage;
    ComPtr<IAudioEndpointVolume> Endpoint;
    if (!ActivateEndpointVolumeById(DeviceID, Endpoint, ErrorMessage))
    {
        WNT_Private::LogWntError(TEXT("Is Audio Muted"), ErrorMessage);
        return false;
    }

    BOOL bWindowsMuted = 0;
    const HRESULT Hr = Endpoint->GetMute(&bWindowsMuted);
    if (FAILED(Hr))
    {
        WNT_Private::LogWntError(TEXT("Is Audio Muted"), FString::Printf(TEXT("Failed to read the audio mute state. HRESULT: 0x%08X"), static_cast<uint32>(Hr)));
        return false;
    }

    bMuted = bWindowsMuted != 0;
    return true;
#else
    WNT_Private::LogWntError(TEXT("Is Audio Muted"), TEXT("Audio mute is only available on Windows."));
    return false;
#endif
}

bool UAudioSystemLibrary::GetAudioDevicePeak(const FString& DeviceID, float& OutPeakValue)
{
    OutPeakValue = 0.0f;

#if PLATFORM_WINDOWS
    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        WNT_Private::LogWntError(TEXT("Get Audio Peak"), TEXT("Failed to initialize COM for audio metering."));
        return false;
    }

    FString ErrorMessage;
    ComPtr<IMMDevice> Device;
    if (!GetDeviceById(DeviceID, Device, ErrorMessage))
    {
        WNT_Private::LogWntError(TEXT("Get Audio Peak"), ErrorMessage);
        return false;
    }

    ComPtr<IAudioMeterInformation> Meter;
    const HRESULT ActivateHr = Device->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, nullptr, &Meter);
    if (FAILED(ActivateHr) || !Meter)
    {
        WNT_Private::LogWntError(TEXT("Get Audio Peak"), FString::Printf(TEXT("Audio metering is not available for this device. HRESULT: 0x%08X"), static_cast<uint32>(ActivateHr)));
        return false;
    }

    const HRESULT Hr = Meter->GetPeakValue(&OutPeakValue);
    if (FAILED(Hr))
    {
        WNT_Private::LogWntError(TEXT("Get Audio Peak"), FString::Printf(TEXT("Failed to read the audio peak value. HRESULT: 0x%08X"), static_cast<uint32>(Hr)));
        return false;
    }

    return true;
#else
    WNT_Private::LogWntError(TEXT("Get Audio Peak"), TEXT("Audio metering is only available on Windows."));
    return false;
#endif
}

TArray<FWNTAudioDeviceFormat> UAudioSystemLibrary::GetSupportedAudioDeviceFormats(const FString& DeviceID)
{
    TArray<FWNTAudioDeviceFormat> Result;

#if PLATFORM_WINDOWS
    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        WNT_Private::LogWntError(TEXT("Get Supported Audio Device Default Formats"), TEXT("Failed to initialize COM for audio format enumeration."));
        return Result;
    }

    FString ErrorMessage;
    const TArray<FAudioFormatCandidate> Formats = QuerySupportedFormatsInternal(DeviceID, ErrorMessage);
    if (Formats.IsEmpty() && !ErrorMessage.IsEmpty())
    {
        WNT_Private::LogWntError(TEXT("Get Supported Audio Device Default Formats"), ErrorMessage);
    }

    Result.Reserve(Formats.Num());
    for (const FAudioFormatCandidate& Format : Formats)
    {
        Result.Add(Format.Info);
    }
#else
    WNT_Private::LogWntError(TEXT("Get Supported Audio Device Default Formats"), TEXT("Audio format enumeration is only available on Windows."));
#endif

    return Result;
}

bool UAudioSystemLibrary::SetAudioDeviceDefaultFormat(const FString& DeviceID, int32 FormatIndex)
{
#if PLATFORM_WINDOWS
    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        WNT_Private::LogWntError(TEXT("Set Audio Device Default Format"), TEXT("Failed to initialize COM for audio format configuration."));
        return false;
    }

    FString ErrorMessage;
    TArray<FAudioFormatCandidate> Formats = QuerySupportedFormatsInternal(DeviceID, ErrorMessage);
    if (!Formats.IsValidIndex(FormatIndex))
    {
        const FString Reason = ErrorMessage.IsEmpty()
            ? TEXT("The requested format index is invalid.")
            : ErrorMessage;
        WNT_Private::LogWntError(TEXT("Set Audio Device Default Format"), Reason);
        return false;
    }

    ComPtr<IPolicyConfig> PolicyConfig;
    if (!CreatePolicyConfig(PolicyConfig, ErrorMessage))
    {
        WNT_Private::LogWntError(TEXT("Set Audio Device Default Format"), ErrorMessage);
        return false;
    }

    WAVEFORMATEX* Format = reinterpret_cast<WAVEFORMATEX*>(Formats[FormatIndex].Bytes.GetData());
    const HRESULT Hr = PolicyConfig->SetDeviceFormat(*DeviceID, Format, Format);
    if (FAILED(Hr))
    {
        WNT_Private::LogWntError(TEXT("Set Audio Device Default Format"), FString::Printf(TEXT("Windows rejected the audio format change. HRESULT: 0x%08X"), static_cast<uint32>(Hr)));
        return false;
    }

    return true;
#else
    WNT_Private::LogWntError(TEXT("Set Audio Device Default Format"), TEXT("Audio format configuration is only available on Windows."));
    return false;
#endif
}

bool UAudioSystemLibrary::SetAudioDeviceEnabled(const FString& DeviceID, bool bEnabled)
{
#if PLATFORM_WINDOWS
    if (DeviceID.TrimStartAndEnd().IsEmpty())
    {
        WNT_Private::LogWntError(TEXT("Set Audio Device Visibility"), TEXT("Audio device ID is empty."));
        return false;
    }

    WNT_Private::FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        WNT_Private::LogWntError(TEXT("Set Audio Device Visibility"), TEXT("Failed to initialize COM for audio endpoint configuration."));
        return false;
    }

    FString ErrorMessage;
    ComPtr<IPolicyConfig> PolicyConfig;
    if (!CreatePolicyConfig(PolicyConfig, ErrorMessage))
    {
        WNT_Private::LogWntError(TEXT("Set Audio Device Visibility"), ErrorMessage);
        return false;
    }

    const HRESULT Hr = PolicyConfig->SetEndpointVisibility(*DeviceID, bEnabled ? 1 : 0);
    if (FAILED(Hr))
    {
        WNT_Private::LogWntError(TEXT("Set Audio Device Visibility"), FString::Printf(TEXT("Windows rejected the endpoint visibility change. HRESULT: 0x%08X"), static_cast<uint32>(Hr)));
        return false;
    }

    return true;
#else
    WNT_Private::LogWntError(TEXT("Set Audio Device Visibility"), TEXT("Audio endpoint configuration is only available on Windows."));
    return false;
#endif
}
