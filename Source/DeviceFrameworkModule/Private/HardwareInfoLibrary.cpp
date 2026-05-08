// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "HardwareInfoLibrary.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"
#include "RHI.h"
#include "HAL/PlatformMisc.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#include <atomic>

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsSystemIncludes.h"
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <Xinput.h>
#include <shellapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <setupapi.h>
#include <initguid.h>
#include <devpkey.h>
#include <wbemidl.h>
#include "Windows/HideWindowsPlatformTypes.h"

using Microsoft::WRL::ComPtr;





namespace WNT_Private
{
    static void LogSystemInfoError(const TCHAR* Context, const FString& Message)
    {
        UE_LOG(LogTemp, Error, TEXT("Error: %s failed. %s"), Context, *Message);
    }

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

    struct FWmiQueryRow
    {
        TMap<FString, FString> Values;
    };

    struct FWmiConnection
    {
        TUniquePtr<FScopedComInit> ComInit;
        ComPtr<IWbemServices> Services;
    };

    struct FAdapterEntry
    {
        ComPtr<IDXGIAdapter3> Adapter;
        DXGI_ADAPTER_DESC1 Desc;
        int32 Index;
        bool bIsActiveRHI;
    };

    static FCriticalSection CacheMutex;
    static TArray<FAdapterEntry> CachedAdapters;
    static int32 ActiveRHIIndex = 0;
    static bool bEnumerated = false;

    static EGPUVendor VendorIdToEnum(uint32 Id)
    {
        switch (Id)
        {
        case 0x10DE: return EGPUVendor::Nvidia;
        case 0x1002: return EGPUVendor::AMD;
        case 0x8086: return EGPUVendor::Intel;
        case 0x5143: return EGPUVendor::Qualcomm;
        default:     return EGPUVendor::Unknown;
        }
    }

    static FString NormalizeWmiDate(const FString& RawDate)
    {
        if (RawDate.Len() < 8)
        {
            return RawDate;
        }

        return FString::Printf(TEXT("%s-%s-%s"), *RawDate.Mid(0, 4), *RawDate.Mid(4, 2), *RawDate.Mid(6, 2));
    }

    static FString VariantToString(const VARIANT& VariantValue)
    {
        switch (VariantValue.vt)
        {
        case VT_BSTR:
            return VariantValue.bstrVal ? FString(VariantValue.bstrVal) : FString();
        case VT_BOOL:
            return VariantValue.boolVal == VARIANT_TRUE ? TEXT("True") : TEXT("False");
        case VT_UI1:
            return FString::FromInt(VariantValue.bVal);
        case VT_I2:
            return FString::FromInt(VariantValue.iVal);
        case VT_UI2:
            return FString::FromInt(VariantValue.uiVal);
        case VT_I4:
        case VT_INT:
            return FString::FromInt(VariantValue.intVal);
        case VT_UI4:
        case VT_UINT:
            return FString::Printf(TEXT("%u"), VariantValue.uintVal);
        case VT_I8:
            return FString::Printf(TEXT("%lld"), static_cast<long long>(VariantValue.llVal));
        case VT_UI8:
            return FString::Printf(TEXT("%llu"), static_cast<unsigned long long>(VariantValue.ullVal));
        default:
            return FString();
        }
    }

    static bool EnsureWmiConnection(FWmiConnection& OutConnection, FString& OutError)
    {
        static bool bSecurityInitialized = false;
        static bool bSecurityAttempted = false;

        OutConnection.ComInit = MakeUnique<FScopedComInit>();
        if (!OutConnection.ComInit->IsUsable())
        {
            OutError = TEXT("Failed to initialize COM for WMI.");
            return false;
        }

        if (!bSecurityAttempted)
        {
            bSecurityAttempted = true;
            const HRESULT SecurityHr = CoInitializeSecurity(
                nullptr,
                -1,
                nullptr,
                nullptr,
                RPC_C_AUTHN_LEVEL_DEFAULT,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                nullptr,
                EOAC_NONE,
                nullptr);
            bSecurityInitialized = SUCCEEDED(SecurityHr) || SecurityHr == RPC_E_TOO_LATE;
        }

        if (!bSecurityInitialized)
        {
            OutError = TEXT("Failed to initialize COM security for WMI.");
            return false;
        }

        ComPtr<IWbemLocator> Locator;
        HRESULT Hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&Locator));
        if (FAILED(Hr) || !Locator)
        {
            OutError = FString::Printf(TEXT("Failed to create a WMI locator. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
            return false;
        }

        BSTR NamespacePath = SysAllocString(L"ROOT\\CIMV2");
        Hr = Locator->ConnectServer(
            NamespacePath,
            nullptr,
            nullptr,
            nullptr,
            0,
            nullptr,
            nullptr,
            &OutConnection.Services);
        SysFreeString(NamespacePath);
        if (FAILED(Hr) || !OutConnection.Services)
        {
            OutError = FString::Printf(TEXT("Failed to connect to the WMI CIMV2 namespace. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
            return false;
        }

        Hr = CoSetProxyBlanket(
            OutConnection.Services.Get(),
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            nullptr,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            nullptr,
            EOAC_NONE);
        if (FAILED(Hr))
        {
            OutError = FString::Printf(TEXT("Failed to configure WMI proxy security. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
            return false;
        }

        return true;
    }

    static bool ExecWmiQuery(const FString& Query, const TArray<FString>& PropertyNames, TArray<FWmiQueryRow>& OutRows, FString& OutError)
    {
        OutRows.Reset();

        FWmiConnection Connection;
        if (!EnsureWmiConnection(Connection, OutError))
        {
            return false;
        }

        ComPtr<IEnumWbemClassObject> Enumerator;
        BSTR QueryLanguage = SysAllocString(L"WQL");
        BSTR QueryString = SysAllocString(*Query);
        const HRESULT Hr = Connection.Services->ExecQuery(
            QueryLanguage,
            QueryString,
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            nullptr,
            &Enumerator);
        SysFreeString(QueryLanguage);
        SysFreeString(QueryString);
        if (FAILED(Hr) || !Enumerator)
        {
            OutError = FString::Printf(TEXT("WMI query failed. HRESULT: 0x%08X"), static_cast<uint32>(Hr));
            return false;
        }

        while (true)
        {
            ULONG ReturnedCount = 0;
            ComPtr<IWbemClassObject> Object;
            const HRESULT NextHr = Enumerator->Next(WBEM_INFINITE, 1, Object.GetAddressOf(), &ReturnedCount);
            if (FAILED(NextHr) || ReturnedCount == 0 || !Object)
            {
                break;
            }

            FWmiQueryRow Row;
            for (const FString& PropertyName : PropertyNames)
            {
                VARIANT Value;
                VariantInit(&Value);
                if (SUCCEEDED(Object->Get(*PropertyName, 0, &Value, nullptr, nullptr)))
                {
                    Row.Values.Add(PropertyName, VariantToString(Value));
                }
                VariantClear(&Value);
            }

            OutRows.Add(MoveTemp(Row));
        }

        return true;
    }

    static FString FindWmiValue(const TArray<FWmiQueryRow>& Rows, const FString& MatchProperty, const FString& MatchValue, const FString& ReturnProperty)
    {
        const FString NormalizedMatch = MatchValue.TrimStartAndEnd();
        for (const FWmiQueryRow& Row : Rows)
        {
            const FString* CandidateValue = Row.Values.Find(MatchProperty);
            if (CandidateValue && CandidateValue->TrimStartAndEnd().Equals(NormalizedMatch, ESearchCase::IgnoreCase))
            {
                if (const FString* ReturnValue = Row.Values.Find(ReturnProperty))
                {
                    return *ReturnValue;
                }
            }
        }

        return FString();
    }

    static FString BuildGpuIdToken(uint32 VendorId, uint32 DeviceId)
    {
        return FString::Printf(TEXT("VEN_%04X&DEV_%04X"), VendorId, DeviceId).ToUpper();
    }

    static bool DoesPnpDeviceIdMatchAdapter(const FString& PnpDeviceId, const FAdapterEntry& Entry)
    {
        const FString DeviceIdUpper = PnpDeviceId.TrimStartAndEnd().ToUpper();
        if (DeviceIdUpper.IsEmpty())
        {
            return false;
        }

        return DeviceIdUpper.Contains(BuildGpuIdToken(Entry.Desc.VendorId, Entry.Desc.DeviceId));
    }

    static FString ParseNullSeparatedWideStringList(const uint8* Buffer, DWORD BufferSizeBytes)
    {
        if (!Buffer || BufferSizeBytes < sizeof(WCHAR))
        {
            return FString();
        }

        const WCHAR* Cursor = reinterpret_cast<const WCHAR*>(Buffer);
        const int32 CharCount = static_cast<int32>(BufferSizeBytes / sizeof(WCHAR));
        TArray<FString> Parts;

        int32 Index = 0;
        while (Index < CharCount && Cursor[Index] != L'\0')
        {
            const WCHAR* Current = Cursor + Index;
            const FString Value(Current);
            if (!Value.IsEmpty())
            {
                Parts.Add(Value);
            }

            Index += Value.Len() + 1;
        }

        return FString::Join(Parts, TEXT(" | "));
    }

    static FString QueryDevicePropertyString(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA& DeviceInfoData, const DEVPROPKEY& PropertyKey)
    {
        DEVPROPTYPE PropertyType = 0;
        DWORD RequiredBytes = 0;
        SetupDiGetDevicePropertyW(DeviceInfoSet, &DeviceInfoData, &PropertyKey, &PropertyType, nullptr, 0, &RequiredBytes, 0);
        if (RequiredBytes == 0)
        {
            return FString();
        }

        TArray<uint8> Buffer;
        Buffer.SetNumZeroed(RequiredBytes);
        if (!SetupDiGetDevicePropertyW(DeviceInfoSet, &DeviceInfoData, &PropertyKey, &PropertyType, Buffer.GetData(), RequiredBytes, &RequiredBytes, 0))
        {
            return FString();
        }

        if (PropertyType == DEVPROP_TYPE_STRING)
        {
            return FString(reinterpret_cast<const WCHAR*>(Buffer.GetData())).TrimStartAndEnd();
        }

        if (PropertyType == DEVPROP_TYPE_STRING_LIST)
        {
            return ParseNullSeparatedWideStringList(Buffer.GetData(), RequiredBytes).TrimStartAndEnd();
        }

        return FString();
    }

    static FString QueryDeviceRegistryPropertyString(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA& DeviceInfoData, DWORD Property)
    {
        DWORD PropertyType = 0;
        DWORD RequiredBytes = 0;
        SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet, &DeviceInfoData, Property, &PropertyType, nullptr, 0, &RequiredBytes);
        if (RequiredBytes == 0)
        {
            return FString();
        }

        TArray<uint8> Buffer;
        Buffer.SetNumZeroed(RequiredBytes);
        if (!SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet, &DeviceInfoData, Property, &PropertyType, Buffer.GetData(), RequiredBytes, &RequiredBytes))
        {
            return FString();
        }

        if (PropertyType == REG_SZ || PropertyType == REG_EXPAND_SZ)
        {
            return FString(reinterpret_cast<const WCHAR*>(Buffer.GetData())).TrimStartAndEnd();
        }

        if (PropertyType == REG_MULTI_SZ)
        {
            return ParseNullSeparatedWideStringList(Buffer.GetData(), RequiredBytes).TrimStartAndEnd();
        }

        return FString();
    }

    static FString GetPhysicalLocationFromSetupApi(const FString& PnpDeviceId)
    {
        if (PnpDeviceId.TrimStartAndEnd().IsEmpty())
        {
            return FString();
        }

        const HDEVINFO DeviceInfoSet = SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
        if (DeviceInfoSet == INVALID_HANDLE_VALUE)
        {
            return FString();
        }

        FString PhysicalLocation;
        SP_DEVINFO_DATA DeviceInfoData = {};
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        constexpr DWORD DeviceInstanceIdCapacity = 512;

        for (DWORD DeviceIndex = 0; SetupDiEnumDeviceInfo(DeviceInfoSet, DeviceIndex, &DeviceInfoData); ++DeviceIndex)
        {
            WCHAR DeviceInstanceId[DeviceInstanceIdCapacity] = {};
            if (!SetupDiGetDeviceInstanceIdW(DeviceInfoSet, &DeviceInfoData, DeviceInstanceId, DeviceInstanceIdCapacity, nullptr))
            {
                continue;
            }

            const FString CandidateId(DeviceInstanceId);
            if (!CandidateId.Equals(PnpDeviceId, ESearchCase::IgnoreCase))
            {
                continue;
            }

            FString LocationInfo = QueryDevicePropertyString(DeviceInfoSet, DeviceInfoData, DEVPKEY_Device_LocationInfo);
            if (LocationInfo.IsEmpty())
            {
                LocationInfo = QueryDeviceRegistryPropertyString(DeviceInfoSet, DeviceInfoData, SPDRP_LOCATION_INFORMATION);
            }

            const FString LocationPath = QueryDevicePropertyString(DeviceInfoSet, DeviceInfoData, DEVPKEY_Device_LocationPaths);
            if (!LocationInfo.IsEmpty() && !LocationPath.IsEmpty())
            {
                PhysicalLocation = FString::Printf(TEXT("%s [%s]"), *LocationInfo, *LocationPath);
            }
            else if (!LocationInfo.IsEmpty())
            {
                PhysicalLocation = LocationInfo;
            }
            else
            {
                PhysicalLocation = LocationPath;
            }

            break;
        }

        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return PhysicalLocation.TrimStartAndEnd();
    }

    static void EnumerateAdapters()
    {
        if (bEnumerated) return;
        bEnumerated = true;

        ComPtr<IDXGIFactory4> Factory;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&Factory)))) return;

        ComPtr<IDXGIAdapter1> Adapter1;
        for (UINT i = 0; Factory->EnumAdapters1(i, &Adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 Desc;
            if (FAILED(Adapter1->GetDesc1(&Desc))) { Adapter1.Reset(); continue; }
            if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { Adapter1.Reset(); continue; }

            FAdapterEntry Entry;
            Entry.Desc = Desc;
            Entry.Index = CachedAdapters.Num();
            Entry.bIsActiveRHI = (GRHIVendorId != 0 && Desc.VendorId == GRHIVendorId);

            if (Entry.bIsActiveRHI) ActiveRHIIndex = Entry.Index;
            Adapter1.As(&Entry.Adapter);
            CachedAdapters.Add(MoveTemp(Entry));
            Adapter1.Reset();
        }


        if (CachedAdapters.Num() > 0 && ActiveRHIIndex >= CachedAdapters.Num())
            ActiveRHIIndex = 0;
    }

    static FAdapterEntry* GetAdapter(int32 Index)
    {
        FScopeLock Lock(&CacheMutex);
        EnumerateAdapters();
        return CachedAdapters.IsValidIndex(Index) ? &CachedAdapters[Index] : nullptr;
    }

    static FAdapterEntry* GetRHIAdapter()
    {
        FScopeLock Lock(&CacheMutex);
        EnumerateAdapters();
        return CachedAdapters.IsValidIndex(ActiveRHIIndex) ? &CachedAdapters[ActiveRHIIndex] : nullptr;
    }


    static FCriticalSection PerfMutex;
    static PDH_HQUERY PerfQuery = nullptr;
    static PDH_HCOUNTER DedicatedCounter = nullptr;
    static PDH_HCOUNTER SharedCounter = nullptr;
    static PDH_HCOUNTER GPUEngineCounter = nullptr;
    static PDH_HCOUNTER CPUCounter = nullptr;
    static TMap<int32, PDH_HCOUNTER> CpuThreadCounters;
    static bool bPerfInitialized = false;

    static bool InitPerfCounters()
    {
        if (bPerfInitialized) return PerfQuery != nullptr;
        bPerfInitialized = true;

        if (::PdhOpenQuery(nullptr, 0, &PerfQuery) != ERROR_SUCCESS)
        {
            PerfQuery = nullptr;
            return false;
        }


        ::PdhAddEnglishCounter(PerfQuery, TEXT("\\GPU Adapter Memory(*)\\Dedicated Usage"), 0, &DedicatedCounter);
        ::PdhAddEnglishCounter(PerfQuery, TEXT("\\GPU Adapter Memory(*)\\Shared Usage"), 0, &SharedCounter);
        ::PdhAddEnglishCounter(PerfQuery, TEXT("\\GPU Engine(*)\\Utilization Percentage"), 0, &GPUEngineCounter);
        ::PdhAddEnglishCounter(PerfQuery, TEXT("\\Processor(_Total)\\% Processor Time"), 0, &CPUCounter);


        ::PdhCollectQueryData(PerfQuery);
        return true;
    }

    static void CollectPerfData()
    {
        if (PerfQuery) ::PdhCollectQueryData(PerfQuery);
    }

    static FString FormatLUID(const LUID& Luid)
    {
        return FString::Printf(TEXT("luid_0x%08x_0x%08x"),
            static_cast<uint32>(Luid.HighPart), Luid.LowPart);
    }


    static int64 QueryCounterForLUID(PDH_HCOUNTER Counter, const FString& LuidStr)
    {
        if (!Counter) return 0;

        DWORD BufSize = 0, Count = 0;
        ::PdhGetFormattedCounterArray(Counter, PDH_FMT_LARGE, &BufSize, &Count, nullptr);
        if (BufSize == 0) return 0;

        TArray<uint8> Buf;
        Buf.SetNumUninitialized(BufSize);
        auto* Items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(Buf.GetData());

        if (::PdhGetFormattedCounterArray(Counter, PDH_FMT_LARGE, &BufSize, &Count, Items) != ERROR_SUCCESS)
            return 0;

        int64 Total = 0;
        for (DWORD i = 0; i < Count; ++i)
        {
            FString Name(Items[i].szName);
            if (Name.Contains(LuidStr))
                Total += Items[i].FmtValue.largeValue;
        }
        return Total;
    }

    static bool MatchesGpuUsageMode(const FString& EngineType, EGPUUsageMode UsageMode)
    {
        const FString LowerEngineType = EngineType.ToLower();

        switch (UsageMode)
        {
        case EGPUUsageMode::Overall:
            return true;
        case EGPUUsageMode::ThreeDimensional:
            return LowerEngineType.Contains(TEXT("3d"));
        case EGPUUsageMode::Copy:
            return LowerEngineType.Contains(TEXT("copy"));
        case EGPUUsageMode::Compute:
            return LowerEngineType.Contains(TEXT("compute"));
        case EGPUUsageMode::VideoDecode:
            return LowerEngineType.Contains(TEXT("videodecode"));
        case EGPUUsageMode::VideoEncode:
            return LowerEngineType.Contains(TEXT("videoencode"));
        case EGPUUsageMode::VideoProcessing:
            return LowerEngineType.Contains(TEXT("videoprocessing"));
        case EGPUUsageMode::Graphics:
            return LowerEngineType.Contains(TEXT("graphics"));
        case EGPUUsageMode::Overlay:
            return LowerEngineType.Contains(TEXT("overlay"));
        case EGPUUsageMode::Cryptographic:
            return LowerEngineType.Contains(TEXT("cryptographic"));
        case EGPUUsageMode::Other:
            return !(LowerEngineType.Contains(TEXT("3d"))
                || LowerEngineType.Contains(TEXT("copy"))
                || LowerEngineType.Contains(TEXT("compute"))
                || LowerEngineType.Contains(TEXT("videodecode"))
                || LowerEngineType.Contains(TEXT("videoencode"))
                || LowerEngineType.Contains(TEXT("videoprocessing"))
                || LowerEngineType.Contains(TEXT("graphics"))
                || LowerEngineType.Contains(TEXT("overlay"))
                || LowerEngineType.Contains(TEXT("cryptographic")));
        default:
            return true;
        }
    }

    static float QueryGPUUtil(const FString& LuidStr, EGPUUsageMode UsageMode)
    {
        if (!GPUEngineCounter) return 0.0f;

        DWORD BufSize = 0, Count = 0;
        ::PdhGetFormattedCounterArray(GPUEngineCounter, PDH_FMT_DOUBLE, &BufSize, &Count, nullptr);
        if (BufSize == 0) return 0.0f;

        TArray<uint8> Buf;
        Buf.SetNumUninitialized(BufSize);
        auto* Items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(Buf.GetData());

        if (::PdhGetFormattedCounterArray(GPUEngineCounter, PDH_FMT_DOUBLE, &BufSize, &Count, Items) != ERROR_SUCCESS)
            return 0.0f;


        TMap<FString, double> EngineUtils;
        for (DWORD i = 0; i < Count; ++i)
        {
            FString Name(Items[i].szName);
            if (!Name.Contains(LuidStr)) continue;

            const int32 EngineTypeIndex = Name.Find(TEXT("engtype_"));
            FString EngType = EngineTypeIndex != INDEX_NONE ? Name.Mid(EngineTypeIndex + 8) : TEXT("unknown");
            if (!MatchesGpuUsageMode(EngType, UsageMode))
            {
                continue;
            }

            double& Sum = EngineUtils.FindOrAdd(EngType);
            Sum += Items[i].FmtValue.doubleValue;
        }

        double MaxUtil = 0.0;
        for (auto& Pair : EngineUtils)
        {
            MaxUtil = FMath::Max(MaxUtil, Pair.Value);
        }
        return static_cast<float>(FMath::Clamp(MaxUtil, 0.0, 100.0));
    }

    static bool EnsureCpuThreadCounter(int32 ThreadIndex, FString& OutError)
    {
        if (ThreadIndex < 0)
        {
            OutError = TEXT("CPU thread index must be zero or greater.");
            return false;
        }

        if (!InitPerfCounters() || !PerfQuery)
        {
            OutError = TEXT("CPU performance counters are not available.");
            return false;
        }

        if (CpuThreadCounters.Contains(ThreadIndex))
        {
            return true;
        }

        PDH_HCOUNTER Counter = nullptr;
        const FString CounterPath = FString::Printf(TEXT("\\Processor(%d)\\%% Processor Time"), ThreadIndex);
        const PDH_STATUS Status = ::PdhAddEnglishCounter(PerfQuery, *CounterPath, 0, &Counter);
        if (Status != ERROR_SUCCESS || !Counter)
        {
            OutError = FString::Printf(TEXT("Failed to create a CPU thread counter for index %d."), ThreadIndex);
            return false;
        }

        CpuThreadCounters.Add(ThreadIndex, Counter);
        ::PdhCollectQueryData(PerfQuery);
        return true;
    }

    static FString GetMemoryFormFactorDisplayName(int32 FormFactor)
    {
        switch (FormFactor)
        {
        case 8:  return TEXT("DIMM");
        case 12: return TEXT("SODIMM");
        case 13: return TEXT("SRIMM");
        case 9:  return TEXT("TSOP");
        case 10: return TEXT("PGA");
        case 26: return TEXT("DDR4");
        case 27: return TEXT("DDR5");
        default: return TEXT("Unknown");
        }
    }

    static bool PopulateGpuAdvancedInfo(const FAdapterEntry& Entry, FGPUAdapterAdvancedInfo& OutAdvanced)
    {
        OutAdvanced = FGPUAdapterAdvancedInfo();
        OutAdvanced.HardwareReservedMemoryMB = static_cast<int64>(Entry.Desc.DedicatedSystemMemory >> 20);

        FString WmiError;
        TArray<FWmiQueryRow> VideoControllers;
        if (!ExecWmiQuery(TEXT("SELECT Name, DriverVersion, DriverDate, PNPDeviceID, AdapterRAM FROM Win32_VideoController"), { TEXT("Name"), TEXT("DriverVersion"), TEXT("DriverDate"), TEXT("PNPDeviceID"), TEXT("AdapterRAM") }, VideoControllers, WmiError))
        {
            return false;
        }

        FString PnpDeviceId;
        for (const FWmiQueryRow& Row : VideoControllers)
        {
            const FString* Name = Row.Values.Find(TEXT("Name"));
            const FString* DeviceId = Row.Values.Find(TEXT("PNPDeviceID"));
            const bool bPnpMatches = DeviceId && DoesPnpDeviceIdMatchAdapter(*DeviceId, Entry);
            if (!Name)
            {
                if (!bPnpMatches)
                {
                    continue;
                }
            }

            const FString AdapterName = FString(Entry.Desc.Description).TrimStartAndEnd();
            const bool bNameMatches = Name && (Name->TrimStartAndEnd().Equals(AdapterName, ESearchCase::IgnoreCase) || Name->Contains(AdapterName) || AdapterName.Contains(*Name));
            if (bNameMatches || bPnpMatches)
            {
                if (const FString* DriverVersion = Row.Values.Find(TEXT("DriverVersion")))
                {
                    OutAdvanced.DriverVersion = *DriverVersion;
                }
                if (const FString* DriverDate = Row.Values.Find(TEXT("DriverDate")))
                {
                    OutAdvanced.DriverDate = NormalizeWmiDate(*DriverDate);
                }
                if (DeviceId)
                {
                    PnpDeviceId = *DeviceId;
                }
                if (OutAdvanced.HardwareReservedMemoryMB <= 0 && Entry.Desc.DedicatedVideoMemory == 0)
                {
                    if (const FString* AdapterRam = Row.Values.Find(TEXT("AdapterRAM")))
                    {
                        const uint64 AdapterRamBytes = FCString::Strtoui64(**AdapterRam, nullptr, 10);
                        if (AdapterRamBytes > 0)
                        {
                            OutAdvanced.HardwareReservedMemoryMB = static_cast<int64>(AdapterRamBytes >> 20);
                        }
                    }
                }
                break;
            }
        }

        if (!PnpDeviceId.IsEmpty())
        {
            OutAdvanced.PhysicalLocation = GetPhysicalLocationFromSetupApi(PnpDeviceId);

            TArray<FWmiQueryRow> PnpEntities;
            if (OutAdvanced.PhysicalLocation.IsEmpty()
                && ExecWmiQuery(TEXT("SELECT PNPDeviceID, LocationInformation FROM Win32_PnPEntity"), { TEXT("PNPDeviceID"), TEXT("LocationInformation") }, PnpEntities, WmiError))
            {
                OutAdvanced.PhysicalLocation = FindWmiValue(PnpEntities, TEXT("PNPDeviceID"), PnpDeviceId, TEXT("LocationInformation"));
            }
        }

        return true;
    }

    static FGPUAdapterInfo BuildGpuAdapterInfo(const FAdapterEntry& Entry)
    {
        FGPUAdapterInfo Info;
        Info.AdapterIndex = Entry.Index;
        Info.AdapterName = FString(Entry.Desc.Description);
        Info.Vendor = VendorIdToEnum(Entry.Desc.VendorId);
        Info.bIsActiveRHI = Entry.bIsActiveRHI;
        PopulateGpuAdvancedInfo(Entry, Info.Advanced);
        return Info;
    }
}

#endif

namespace WNT_Command
{
    static FString GetShellExecutable(EWNTCommandShell Shell)
    {
#if PLATFORM_WINDOWS
        return Shell == EWNTCommandShell::PowerShell ? TEXT("powershell.exe") : TEXT("cmd.exe");
#else
        return FString();
#endif
    }

    static FString EscapeForDoubleQuotes(const FString& Value)
    {
        FString Escaped = Value;
        Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
        return Escaped;
    }

    static FString BuildShellParameters(EWNTCommandShell Shell, const FString& Command)
    {
        if (Shell == EWNTCommandShell::PowerShell)
        {
            return FString::Printf(TEXT("-NoProfile -ExecutionPolicy Bypass -Command \"%s\""), *EscapeForDoubleQuotes(Command));
        }
        return FString::Printf(TEXT("/S /C \"%s\""), *Command);
    }

    static bool StartElevated(const FString& Executable, const FString& Parameters, bool bHidden, const FString& WorkingDirectory, FWNTCommandResult* OutResult)
    {
#if PLATFORM_WINDOWS
        SHELLEXECUTEINFO ShellInfo = {};
        ShellInfo.cbSize = sizeof(SHELLEXECUTEINFO);
        ShellInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        ShellInfo.lpVerb = TEXT("runas");
        ShellInfo.lpFile = *Executable;
        ShellInfo.lpParameters = *Parameters;
        ShellInfo.lpDirectory = WorkingDirectory.IsEmpty() ? nullptr : *WorkingDirectory;
        ShellInfo.nShow = bHidden ? SW_HIDE : SW_SHOW;

        const BOOL bStarted = ShellExecuteEx(&ShellInfo);
        if (!bStarted)
        {
            if (OutResult)
            {
                OutResult->StdErr = FString::Printf(TEXT("Windows error: %lu"), GetLastError());
            }
            return false;
        }

        if (OutResult)
        {
            OutResult->bStarted = true;
            if (ShellInfo.hProcess)
            {
                WaitForSingleObject(ShellInfo.hProcess, INFINITE);
                DWORD ExitCode = 0;
                if (GetExitCodeProcess(ShellInfo.hProcess, &ExitCode))
                {
                    OutResult->ExitCode = static_cast<int32>(ExitCode);
                }
                OutResult->bCompleted = true;
                CloseHandle(ShellInfo.hProcess);
            }
        }
        else if (ShellInfo.hProcess)
        {
            CloseHandle(ShellInfo.hProcess);
        }

        return true;
#else
        if (OutResult)
        {
            OutResult->StdErr = TEXT("Elevated commands are only available on Windows.");
        }
        return false;
#endif
    }

    static FWNTCommandResult RunCaptured(const FString& Executable, const FString& Parameters, const FWNTCommandOptions& Options)
    {
        FWNTCommandResult Result;

        if (Executable.IsEmpty() || Parameters.IsEmpty())
        {
            Result.StdErr = TEXT("Command is empty or unsupported on this platform.");
            return Result;
        }

        void* ReadPipe = nullptr;
        void* WritePipe = nullptr;
        void* StdErrReadPipe = nullptr;
        void* StdErrWritePipe = nullptr;

        const bool bStdOutPipe = FPlatformProcess::CreatePipe(ReadPipe, WritePipe);
        const bool bStdErrPipe = FPlatformProcess::CreatePipe(StdErrReadPipe, StdErrWritePipe);
        if (!bStdOutPipe || !bStdErrPipe)
        {
            if (ReadPipe || WritePipe)
            {
                FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
            }
            if (StdErrReadPipe || StdErrWritePipe)
            {
                FPlatformProcess::ClosePipe(StdErrReadPipe, StdErrWritePipe);
            }
            Result.StdErr = TEXT("Failed to create output pipes.");
            return Result;
        }

        uint32 ProcessId = 0;
        FProcHandle ProcHandle = FPlatformProcess::CreateProc(
            *Executable,
            *Parameters,
            false,
            Options.bHidden,
            Options.bHidden,
            &ProcessId,
            0,
            Options.WorkingDirectory.IsEmpty() ? nullptr : *Options.WorkingDirectory,
            WritePipe,
            nullptr,
            StdErrWritePipe);

        if (!ProcHandle.IsValid())
        {
            FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
            FPlatformProcess::ClosePipe(StdErrReadPipe, StdErrWritePipe);
            Result.StdErr = TEXT("Failed to start command process.");
            return Result;
        }

        Result.bStarted = true;
        const double StartTime = FPlatformTime::Seconds();
        while (FPlatformProcess::IsProcRunning(ProcHandle))
        {
            Result.StdOut += FPlatformProcess::ReadPipe(ReadPipe);
            Result.StdErr += FPlatformProcess::ReadPipe(StdErrReadPipe);

            if (Options.TimeoutSeconds > 0.0f && (FPlatformTime::Seconds() - StartTime) >= Options.TimeoutSeconds)
            {
                Result.bTimedOut = true;
                FPlatformProcess::TerminateProc(ProcHandle, true);
                break;
            }

            FPlatformProcess::Sleep(0.02f);
        }

        Result.StdOut += FPlatformProcess::ReadPipe(ReadPipe);
        Result.StdErr += FPlatformProcess::ReadPipe(StdErrReadPipe);

        if (!Result.bTimedOut)
        {
            int32 ReturnCode = -1;
            if (FPlatformProcess::GetProcReturnCode(ProcHandle, &ReturnCode))
            {
                Result.ExitCode = ReturnCode;
            }
            Result.bCompleted = true;
        }
        else
        {
            Result.StdErr = TEXT("Command timed out and was terminated.");
        }

        FPlatformProcess::CloseProc(ProcHandle);
        FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
        FPlatformProcess::ClosePipe(StdErrReadPipe, StdErrWritePipe);
        return Result;
    }

    static FWNTCommandResult RunCommand(const FString& Command, FWNTCommandOptions Options)
    {
        FWNTCommandResult Result;
        const FString TrimmedCommand = Command.TrimStartAndEnd();
        if (TrimmedCommand.IsEmpty())
        {
            Result.StdErr = TEXT("Command is empty.");
            return Result;
        }

        const FString Executable = GetShellExecutable(Options.Shell);
        const FString Parameters = BuildShellParameters(Options.Shell, TrimmedCommand);
        if (Executable.IsEmpty())
        {
            Result.StdErr = TEXT("Command execution is only available on Windows.");
            return Result;
        }

        if (Options.bRunAsAdmin)
        {
            if (Options.bCaptureOutput)
            {
                Result.StdErr = TEXT("Output capture is not available for elevated commands launched through UAC.");
            }
            StartElevated(Executable, Parameters, Options.bHidden, Options.WorkingDirectory, &Result);
            return Result;
        }

        if (Options.bCaptureOutput)
        {
            return RunCaptured(Executable, Parameters, Options);
        }

        uint32 ProcessId = 0;
        FProcHandle Handle = FPlatformProcess::CreateProc(
            *Executable,
            *Parameters,
            true,
            Options.bHidden,
            Options.bHidden,
            &ProcessId,
            0,
            Options.WorkingDirectory.IsEmpty() ? nullptr : *Options.WorkingDirectory,
            nullptr);

        Result.bStarted = Handle.IsValid();
        Result.bCompleted = Result.bStarted;
        Result.ExitCode = Result.bStarted ? 0 : -1;
        if (Handle.IsValid())
        {
            FPlatformProcess::CloseProc(Handle);
        }
        else
        {
            Result.StdErr = TEXT("Failed to start command process.");
        }

        return Result;
    }
}

UAsyncRunCommandAction* UAsyncRunCommandAction::RunCommandAsync(const UObject* WorldContextObject, const FString& Command, FWNTCommandOptions Options)
{
    UAsyncRunCommandAction* Node = NewObject<UAsyncRunCommandAction>();
    Node->CommandText = Command;
    Node->CommandOptions = Options;

    if (WorldContextObject)
    {
        Node->RegisterWithGameInstance(WorldContextObject);
    }
    else
    {
        Node->AddToRoot();
        Node->bAddedToRootForCompatibility = true;
    }

    return Node;
}

void UAsyncRunCommandAction::Activate()
{
    TWeakObjectPtr<UAsyncRunCommandAction> WeakThis(this);
    const FString CommandCopy = CommandText;
    const FWNTCommandOptions OptionsCopy = CommandOptions;

    Async(EAsyncExecution::Thread, [WeakThis, CommandCopy, OptionsCopy]()
    {
        const FWNTCommandResult Result = WNT_Command::RunCommand(CommandCopy, OptionsCopy);
        const bool bSucceeded = Result.bStarted && !Result.bTimedOut && (Result.ExitCode == 0 || !Result.bCompleted);

        if (!bSucceeded && !Result.StdErr.IsEmpty())
        {
            WNT_Private::LogSystemInfoError(TEXT("Run Command Async"), Result.StdErr);
        }

        AsyncTask(ENamedThreads::GameThread, [WeakThis, Result, bSucceeded]()
        {
            if (UAsyncRunCommandAction* Node = WeakThis.Get())
            {
                Node->Finalize(Result, bSucceeded);
            }
        });
    });
}

void UAsyncRunCommandAction::Finalize(const FWNTCommandResult& InResult, bool bSuccess)
{
    if (bSuccess)
    {
        OnSuccess.Broadcast(InResult);
    }
    else
    {
        OnFail.Broadcast(InResult);
    }

    if (bAddedToRootForCompatibility)
    {
        RemoveFromRoot();
        bAddedToRootForCompatibility = false;
    }

    SetReadyToDestroy();
}

void USystemInfoBPLibrary::GetPhysicalMemoryInfo(int64& TotalPhysicalMB, int64& UsedPhysicalMB, int64& FreePhysicalMB)
{
#if PLATFORM_WINDOWS
    MEMORYSTATUSEX MemInfo;
    MemInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (::GlobalMemoryStatusEx(&MemInfo))
    {
        TotalPhysicalMB = static_cast<int64>(MemInfo.ullTotalPhys >> 20);
        FreePhysicalMB = static_cast<int64>(MemInfo.ullAvailPhys >> 20);
        UsedPhysicalMB = TotalPhysicalMB - FreePhysicalMB;
        return;
    }
#endif
    TotalPhysicalMB = UsedPhysicalMB = FreePhysicalMB = 0;
}

void USystemInfoBPLibrary::GetVirtualMemoryInfo(int64& TotalVirtualMB, int64& UsedVirtualMB, int64& FreeVirtualMB)
{
#if PLATFORM_WINDOWS
    MEMORYSTATUSEX MemInfo;
    MemInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (::GlobalMemoryStatusEx(&MemInfo))
    {
        TotalVirtualMB = static_cast<int64>(MemInfo.ullTotalPageFile >> 20);
        FreeVirtualMB = static_cast<int64>(MemInfo.ullAvailPageFile >> 20);
        UsedVirtualMB = TotalVirtualMB - FreeVirtualMB;
        return;
    }
#endif
    TotalVirtualMB = UsedVirtualMB = FreeVirtualMB = 0;
}

void USystemInfoBPLibrary::GetMemoryInfo(int32& MemorySpeedMHz, FMemorySlotInfo& SlotInfo, int64& HardwareReservedMemoryMB)
{
    MemorySpeedMHz = 0;
    SlotInfo = FMemorySlotInfo();
    HardwareReservedMemoryMB = 0;

#if PLATFORM_WINDOWS
    FString WmiError;
    TArray<WNT_Private::FWmiQueryRow> Modules;
    if (WNT_Private::ExecWmiQuery(TEXT("SELECT Capacity, ConfiguredClockSpeed, Speed, FormFactor FROM Win32_PhysicalMemory"), { TEXT("Capacity"), TEXT("ConfiguredClockSpeed"), TEXT("Speed"), TEXT("FormFactor") }, Modules, WmiError))
    {
        uint64 InstalledCapacityBytes = 0;
        for (const WNT_Private::FWmiQueryRow& Row : Modules)
        {
            ++SlotInfo.UsedSlots;

            if (MemorySpeedMHz <= 0)
            {
                const FString* ConfiguredSpeed = Row.Values.Find(TEXT("ConfiguredClockSpeed"));
                const FString* Speed = Row.Values.Find(TEXT("Speed"));
                const FString SpeedString = (ConfiguredSpeed && !ConfiguredSpeed->IsEmpty()) ? *ConfiguredSpeed : (Speed ? *Speed : FString());
                MemorySpeedMHz = FCString::Atoi(*SpeedString);
            }

            if (SlotInfo.FormFactor.IsEmpty())
            {
                if (const FString* FormFactor = Row.Values.Find(TEXT("FormFactor")))
                {
                    SlotInfo.FormFactor = WNT_Private::GetMemoryFormFactorDisplayName(FCString::Atoi(**FormFactor));
                }
            }

            if (const FString* Capacity = Row.Values.Find(TEXT("Capacity")))
            {
                InstalledCapacityBytes += FCString::Strtoui64(**Capacity, nullptr, 10);
            }
        }

        MEMORYSTATUSEX MemInfo = {};
        MemInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (::GlobalMemoryStatusEx(&MemInfo) && InstalledCapacityBytes > MemInfo.ullTotalPhys)
        {
            HardwareReservedMemoryMB = static_cast<int64>((InstalledCapacityBytes - MemInfo.ullTotalPhys) >> 20);
        }
    }

    TArray<WNT_Private::FWmiQueryRow> Arrays;
    if (WNT_Private::ExecWmiQuery(TEXT("SELECT MemoryDevices FROM Win32_PhysicalMemoryArray"), { TEXT("MemoryDevices") }, Arrays, WmiError) && Arrays.Num() > 0)
    {
        if (const FString* MemoryDevices = Arrays[0].Values.Find(TEXT("MemoryDevices")))
        {
            SlotInfo.TotalSlots = FCString::Atoi(**MemoryDevices);
        }
    }
#endif
}

void USystemInfoBPLibrary::GetCPUInfo(FString& DeviceName, ECPUVendor& Vendor, int32& PhysicalCores, int32& LogicalThreads, FCPUCacheInfo& CacheInfo)
{
    DeviceName = FPlatformMisc::GetCPUBrand().TrimStartAndEnd();
    if (DeviceName.IsEmpty())
    {
        DeviceName = TEXT("Unknown Processor");
    }

    CacheInfo = FCPUCacheInfo();
    PhysicalCores = FPlatformMisc::NumberOfCores();
    LogicalThreads = FPlatformMisc::NumberOfCoresIncludingHyperthreads();

    const FString VendorId = FPlatformMisc::GetCPUVendor();
    if (VendorId.Equals(TEXT("GenuineIntel"), ESearchCase::IgnoreCase) || DeviceName.Contains(TEXT("Intel")))
        Vendor = ECPUVendor::Intel;
    else if (VendorId.Equals(TEXT("AuthenticAMD"), ESearchCase::IgnoreCase) || DeviceName.Contains(TEXT("AMD")))
        Vendor = ECPUVendor::AMD;
    else if (DeviceName.Contains(TEXT("Apple")))
        Vendor = ECPUVendor::Apple;
    else if (DeviceName.Contains(TEXT("Snapdragon")) || DeviceName.Contains(TEXT("Qualcomm")))
        Vendor = ECPUVendor::Qualcomm;
    else
        Vendor = ECPUVendor::Generic;

#if PLATFORM_WINDOWS
    DWORD BufferSize = 0;
    ::GetLogicalProcessorInformationEx(RelationCache, nullptr, &BufferSize);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && BufferSize > 0)
    {
        TArray<uint8> Buffer;
        Buffer.SetNumZeroed(BufferSize);

        if (::GetLogicalProcessorInformationEx(RelationCache, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(Buffer.GetData()), &BufferSize))
        {
            uint8* Cursor = Buffer.GetData();
            const uint8* End = Cursor + BufferSize;
            while (Cursor < End)
            {
                const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* Entry = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(Cursor);
                if (Entry->Relationship == RelationCache)
                {
                    const int32 CacheSizeKB = static_cast<int32>(Entry->Cache.CacheSize / 1024);
                    switch (Entry->Cache.Level)
                    {
                    case 1: CacheInfo.L1CacheKB += CacheSizeKB; break;
                    case 2: CacheInfo.L2CacheKB += CacheSizeKB; break;
                    case 3: CacheInfo.L3CacheKB += CacheSizeKB; break;
                    default: break;
                    }
                }

                Cursor += Entry->Size;
            }
        }
    }
#endif
}

float USystemInfoBPLibrary::GetOverallCPUUsage()
{
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::PerfMutex);
    if (WNT_Private::InitPerfCounters() && WNT_Private::CPUCounter)
    {
        WNT_Private::CollectPerfData();

        PDH_FMT_COUNTERVALUE Val;
        if (::PdhGetFormattedCounterValue(WNT_Private::CPUCounter, PDH_FMT_DOUBLE, nullptr, &Val) == ERROR_SUCCESS)
        {
            return static_cast<float>(FMath::Clamp(Val.doubleValue, 0.0, 100.0));
        }
    }
#endif
    return 0.0f;
}

float USystemInfoBPLibrary::GetCPUThreadUsage(int32 Thread)
{
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::PerfMutex);
    FString ErrorMessage;
    if (!WNT_Private::EnsureCpuThreadCounter(Thread, ErrorMessage))
    {
        WNT_Private::LogSystemInfoError(TEXT("Get CPU Thread Usage"), ErrorMessage);
        return 0.0f;
    }

    WNT_Private::CollectPerfData();
    if (PDH_HCOUNTER* Counter = WNT_Private::CpuThreadCounters.Find(Thread))
    {
        PDH_FMT_COUNTERVALUE Value;
        if (::PdhGetFormattedCounterValue(*Counter, PDH_FMT_DOUBLE, nullptr, &Value) == ERROR_SUCCESS)
        {
            return static_cast<float>(FMath::Clamp(Value.doubleValue, 0.0, 100.0));
        }
    }
#endif

    return 0.0f;
}

int32 USystemInfoBPLibrary::GetCPUCurrentSpeedMHz()
{
#if PLATFORM_WINDOWS
    FString WmiError;
    TArray<WNT_Private::FWmiQueryRow> Rows;
    if (WNT_Private::ExecWmiQuery(TEXT("SELECT CurrentClockSpeed FROM Win32_Processor"), { TEXT("CurrentClockSpeed") }, Rows, WmiError) && Rows.Num() > 0)
    {
        if (const FString* Value = Rows[0].Values.Find(TEXT("CurrentClockSpeed")))
        {
            return FCString::Atoi(**Value);
        }
    }
#endif

    return 0;
}

bool USystemInfoBPLibrary::IsCPUVirtualizationEnabled()
{
#if PLATFORM_WINDOWS
    FString WmiError;
    TArray<WNT_Private::FWmiQueryRow> Rows;
    if (WNT_Private::ExecWmiQuery(TEXT("SELECT VirtualizationFirmwareEnabled FROM Win32_Processor"), { TEXT("VirtualizationFirmwareEnabled") }, Rows, WmiError) && Rows.Num() > 0)
    {
        if (const FString* Value = Rows[0].Values.Find(TEXT("VirtualizationFirmwareEnabled")))
        {
            return Value->Equals(TEXT("True"), ESearchCase::IgnoreCase);
        }
    }
#endif

    return false;
}
TArray<FGPUAdapterInfo> USystemInfoBPLibrary::GetAllGPUAdapters()
{
    TArray<FGPUAdapterInfo> Result;
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::CacheMutex);
    WNT_Private::EnumerateAdapters();

    Result.Reserve(WNT_Private::CachedAdapters.Num());
    for (const auto& Entry : WNT_Private::CachedAdapters)
    {
        Result.Add(WNT_Private::BuildGpuAdapterInfo(Entry));
    }
#endif
    return Result;
}

FGPUAdapterInfo USystemInfoBPLibrary::GetGPUInformations(int32 Adapter)
{
    FGPUAdapterInfo Result;
#if PLATFORM_WINDOWS
    if (auto* Entry = WNT_Private::GetAdapter(Adapter))
    {
        Result = WNT_Private::BuildGpuAdapterInfo(*Entry);
    }
    else
    {
        WNT_Private::LogSystemInfoError(TEXT("Get GPU Information"), TEXT("GPU adapter index is invalid."));
    }
#else
    WNT_Private::LogSystemInfoError(TEXT("Get GPU Information"), TEXT("GPU runtime information is only available on Windows."));
#endif
    return Result;
}

FString USystemInfoBPLibrary::GetGPUName(int32 Adapter)
{
#if PLATFORM_WINDOWS
    if (auto* Entry = WNT_Private::GetAdapter(Adapter))
    {
        return FString(Entry->Desc.Description);
    }
#endif
    return TEXT("Unknown Graphics Adapter");
}

EGPUVendor USystemInfoBPLibrary::GetGPUManufacturer(int32 Adapter)
{
#if PLATFORM_WINDOWS
    if (auto* Entry = WNT_Private::GetAdapter(Adapter))
    {
        return WNT_Private::VendorIdToEnum(Entry->Desc.VendorId);
    }
#endif
    return EGPUVendor::Unknown;
}





int64 USystemInfoBPLibrary::GetTotalDedicatedVRAM(int32 Adapter)
{
#if PLATFORM_WINDOWS
    if (auto* Entry = WNT_Private::GetAdapter(Adapter))
    {
        return static_cast<int64>(Entry->Desc.DedicatedVideoMemory >> 20);
    }
#endif
    return 0;
}

int64 USystemInfoBPLibrary::GetUsedDedicatedVRAM(int32 Adapter)
{
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::PerfMutex);
    if (!WNT_Private::InitPerfCounters()) return 0;

    auto* Entry = WNT_Private::GetAdapter(Adapter);
    if (!Entry) return 0;

    WNT_Private::CollectPerfData();
    const FString LuidStr = WNT_Private::FormatLUID(Entry->Desc.AdapterLuid);
    return WNT_Private::QueryCounterForLUID(WNT_Private::DedicatedCounter, LuidStr) >> 20;
#else
    return 0;
#endif
}





int64 USystemInfoBPLibrary::GetTotalVirtualVRAM(int32 Adapter)
{
#if PLATFORM_WINDOWS
    if (auto* Entry = WNT_Private::GetAdapter(Adapter))
    {
        return static_cast<int64>(Entry->Desc.SharedSystemMemory >> 20);
    }
#endif
    return 0;
}

int64 USystemInfoBPLibrary::GetUsedVirtualVRAM(int32 Adapter)
{
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::PerfMutex);
    if (!WNT_Private::InitPerfCounters()) return 0;

    auto* Entry = WNT_Private::GetAdapter(Adapter);
    if (!Entry) return 0;

    WNT_Private::CollectPerfData();
    const FString LuidStr = WNT_Private::FormatLUID(Entry->Desc.AdapterLuid);
    return WNT_Private::QueryCounterForLUID(WNT_Private::SharedCounter, LuidStr) >> 20;
#else
    return 0;
#endif
}





int64 USystemInfoBPLibrary::GetGameVRAMUsage()
{
#if PLATFORM_WINDOWS
    auto* Entry = WNT_Private::GetRHIAdapter();
    if (Entry && Entry->Adapter)
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO MemInfo;
        if (SUCCEEDED(Entry->Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &MemInfo)))
        {
            return static_cast<int64>(MemInfo.CurrentUsage >> 20);
        }
    }
#endif
    return 0;
}





float USystemInfoBPLibrary::GetGPUUsagePercent(int32 Adapter, EGPUUsageMode UsageMode)
{
#if PLATFORM_WINDOWS
    FScopeLock Lock(&WNT_Private::PerfMutex);
    if (!WNT_Private::InitPerfCounters()) return 0.0f;

    auto* Entry = WNT_Private::GetAdapter(Adapter);
    if (!Entry) return 0.0f;

    WNT_Private::CollectPerfData();
    const FString LuidStr = WNT_Private::FormatLUID(Entry->Desc.AdapterLuid);
    return WNT_Private::QueryGPUUtil(LuidStr, UsageMode);
#else
    return 0.0f;
#endif
}





void USystemInfoBPLibrary::GetInputDevices(bool& HasGamepad, bool& HasMouse)
{
    HasGamepad = false;
    HasMouse = false;
#if PLATFORM_WINDOWS
    UINT DeviceCount = 0;
    if (::GetRawInputDeviceList(nullptr, &DeviceCount, sizeof(RAWINPUTDEVICELIST)) != static_cast<UINT>(-1) && DeviceCount > 0)
    {
        TArray<RAWINPUTDEVICELIST> Devices;
        Devices.SetNumUninitialized(DeviceCount);
        const UINT Result = ::GetRawInputDeviceList(Devices.GetData(), &DeviceCount, sizeof(RAWINPUTDEVICELIST));
        if (Result != static_cast<UINT>(-1))
        {
            for (UINT i = 0; i < Result; ++i)
            {
                if (Devices[i].dwType == RIM_TYPEMOUSE) HasMouse = true;
            }
        }
    }
    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
    {
        XINPUT_STATE State = {};
        if (::XInputGetState(i, &State) == ERROR_SUCCESS) { HasGamepad = true; break; }
    }
#endif
}





EGraphicsRHI USystemInfoBPLibrary::GetRHIName()
{
    if (!GDynamicRHI) return EGraphicsRHI::Unknown;
    const FString RHIName = FString(GDynamicRHI->GetName());
    if (RHIName == TEXT("D3D12"))             return EGraphicsRHI::DirectX12;
    if (RHIName == TEXT("D3D11"))             return EGraphicsRHI::DirectX11;
    if (RHIName.StartsWith(TEXT("Vulkan")))   return EGraphicsRHI::Vulkan;
    if (RHIName == TEXT("Metal"))             return EGraphicsRHI::Metal;
    if (RHIName.Contains(TEXT("OpenGL")))     return EGraphicsRHI::OpenGL;
    return EGraphicsRHI::Unknown;
}

bool USystemInfoBPLibrary::CanRestartGame()
{
    if (IsRunningCommandlet())
    {
        return false;
    }
#if WITH_EDITOR
    if (GIsEditor)
    {
        return false;
    }
#endif
    return FPlatformProcess::ExecutablePath() && FCString::Strlen(FPlatformProcess::ExecutablePath()) > 0;
}

void USystemInfoBPLibrary::RestartGameWithCommandLine(const FString& ExtraCommandLine)
{
#if PLATFORM_WINDOWS
    if (!CanRestartGame())
    {
        return;
    }

    static bool bIsRestarting = false;
    if (bIsRestarting) return;

    const FString RestartSentinel = TEXT("--restarted");
    if (ExtraCommandLine.Contains(RestartSentinel, ESearchCase::IgnoreCase)) return;

    FString Sanitized;
    Sanitized.Reserve(ExtraCommandLine.Len());
    for (const TCHAR C : ExtraCommandLine)
    {
        if (FChar::IsAlpha(C) || FChar::IsDigit(C) ||
            C == TEXT(' ') || C == TEXT('-') || C == TEXT('_') ||
            C == TEXT('=') || C == TEXT(':') || C == TEXT('/') ||
            C == TEXT('\\') || C == TEXT('.') || C == TEXT(','))
            Sanitized.AppendChar(C);
        else
            Sanitized.AppendChar(TEXT(' '));
    }
    Sanitized.TrimEndInline();

    FString CmdLine;
    if (!Sanitized.IsEmpty()) CmdLine = Sanitized + TEXT(" ");
    CmdLine += RestartSentinel;
    bIsRestarting = true;

    FCoreDelegates::OnPreExit.Broadcast();
    if (GConfig) GConfig->Flush(false, GEngineIni);

    const FString ExePath = FPlatformProcess::ExecutablePath();
    if (ExePath.IsEmpty()) { bIsRestarting = false; return; }

    FProcHandle ProcHandle = FPlatformProcess::CreateProc(
        *ExePath, *CmdLine, true, false, false, nullptr, 0, nullptr, nullptr);
    if (!ProcHandle.IsValid()) { bIsRestarting = false; return; }

    constexpr double TimeoutSeconds = 5.0;
    const double StartTime = FPlatformTime::Seconds();
    bool bChildRunning = false;
    while ((FPlatformTime::Seconds() - StartTime) < TimeoutSeconds)
    {
        if (FPlatformProcess::IsProcRunning(ProcHandle)) { bChildRunning = true; break; }
        FPlatformProcess::Sleep(0.05f);
    }
    FPlatformProcess::CloseProc(ProcHandle);
    if (bChildRunning) FPlatformMisc::RequestExit(true);
    else bIsRestarting = false;
#endif
}

bool USystemInfoBPLibrary::ExecuteWindowsCMD(const FString& Command, bool bRunAsAdmin, bool bHidden)
{
    FWNTCommandOptions Options;
    Options.Shell = EWNTCommandShell::Cmd;
    Options.bRunAsAdmin = bRunAsAdmin;
    Options.bHidden = bHidden;
    Options.bCaptureOutput = false;
    const FWNTCommandResult Result = WNT_Command::RunCommand(Command, Options);
    if (!Result.bStarted && !Result.StdErr.IsEmpty())
    {
        WNT_Private::LogSystemInfoError(TEXT("Run Command Prompt Command"), Result.StdErr);
    }
    return Result.bStarted;
}

bool USystemInfoBPLibrary::ExecutePowerShell(const FString& Command, bool bRunAsAdmin, bool bHidden)
{
    FWNTCommandOptions Options;
    Options.Shell = EWNTCommandShell::PowerShell;
    Options.bRunAsAdmin = bRunAsAdmin;
    Options.bHidden = bHidden;
    Options.bCaptureOutput = false;
    const FWNTCommandResult Result = WNT_Command::RunCommand(Command, Options);
    if (!Result.bStarted && !Result.StdErr.IsEmpty())
    {
        WNT_Private::LogSystemInfoError(TEXT("Run PowerShell Command"), Result.StdErr);
    }
    return Result.bStarted;
}

bool USystemInfoBPLibrary::CanForceKillGame()
{
    if (IsRunningCommandlet())
    {
        return false;
    }
#if WITH_EDITOR
    if (GIsEditor)
    {
        return false;
    }
#endif
    return true;
}

void USystemInfoBPLibrary::ForceKillGame()
{
    if (!CanForceKillGame())
    {
        return;
    }

#if PLATFORM_WINDOWS
    ::TerminateProcess(::GetCurrentProcess(), 0);
#else
    FPlatformMisc::RequestExit(true);
#endif
}


