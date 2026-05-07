// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "RefreshRateFunctionLibrary.h"

#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "HAL/PlatformProcess.h"
#include "Widgets/SWindow.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if PLATFORM_WINDOWS
namespace WNTDisplay
{
	static bool IsActiveDisplayDevice(const DISPLAY_DEVICE& Device)
	{
		const bool bActive = (Device.StateFlags & DISPLAY_DEVICE_ACTIVE) != 0;
		const bool bAttachedToDesktop = (Device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0;
		const bool bMirroringDriver = (Device.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0;
		return (bActive || bAttachedToDesktop) && !bMirroringDriver;
	}

	static int32 RationalToRefreshRate(const DISPLAYCONFIG_RATIONAL& Rational)
	{
		if (Rational.Denominator == 0)
		{
			return 0;
		}

		return FMath::Max(0, FMath::RoundToInt(static_cast<double>(Rational.Numerator) / static_cast<double>(Rational.Denominator)));
	}

	static bool TryGetPrimaryMonitorId(FString& OutMonitorId)
	{
		OutMonitorId.Reset();

		for (DWORD Index = 0; ; ++Index)
		{
			DISPLAY_DEVICE Device = {};
			Device.cb = sizeof(DISPLAY_DEVICE);
			if (!EnumDisplayDevices(nullptr, Index, &Device, 0))
			{
				break;
			}

			if (!IsActiveDisplayDevice(Device))
			{
				continue;
			}

			if ((Device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0)
			{
				OutMonitorId = FString(Device.DeviceName);
				return true;
			}
		}

		return false;
	}

	static bool TryGetGameWindowMonitorId(FString& OutMonitorId)
	{
		OutMonitorId.Reset();

		if (!GEngine || !GEngine->GameViewport)
		{
			return false;
		}

		const TSharedPtr<SWindow> WindowPtr = GEngine->GameViewport->GetWindow();
		if (!WindowPtr.IsValid())
		{
			return false;
		}

		const TSharedPtr<FGenericWindow> NativeWindow = WindowPtr->GetNativeWindow();
		if (!NativeWindow.IsValid())
		{
			return false;
		}

		const HWND WindowHandle = static_cast<HWND>(NativeWindow->GetOSWindowHandle());
		if (!WindowHandle)
		{
			return false;
		}

		const HMONITOR MonitorHandle = MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTONEAREST);
		if (!MonitorHandle)
		{
			return false;
		}

		MONITORINFOEX MonitorInfo = {};
		MonitorInfo.cbSize = sizeof(MONITORINFOEX);
		if (!GetMonitorInfo(MonitorHandle, &MonitorInfo))
		{
			return false;
		}

		OutMonitorId = FString(MonitorInfo.szDevice);
		return !OutMonitorId.IsEmpty();
	}

	static bool ResolveMonitorId(const FString& MonitorId, FString& OutMonitorId, FString* OutError = nullptr)
	{
		OutMonitorId = MonitorId.TrimStartAndEnd();
		if (!OutMonitorId.IsEmpty())
		{
			return true;
		}

		if (TryGetGameWindowMonitorId(OutMonitorId))
		{
			return true;
		}

		if (TryGetPrimaryMonitorId(OutMonitorId))
		{
			return true;
		}

		if (OutError)
		{
			*OutError = TEXT("Monitor was not found.");
		}
		return false;
	}

	static bool TryGetCurrentMode(const FString& MonitorId, FWNTDisplayMode& OutMode)
	{
		OutMode = FWNTDisplayMode();
		OutMode.MonitorId = MonitorId;

		DEVMODE DevMode = {};
		DevMode.dmSize = sizeof(DEVMODE);
		if (!EnumDisplaySettings(*MonitorId, ENUM_CURRENT_SETTINGS, &DevMode))
		{
			return false;
		}

		OutMode.Resolution = FIntPoint(static_cast<int32>(DevMode.dmPelsWidth), static_cast<int32>(DevMode.dmPelsHeight));
		OutMode.RefreshRate = static_cast<int32>(DevMode.dmDisplayFrequency);
		OutMode.bIsValid = OutMode.Resolution.X > 0 && OutMode.Resolution.Y > 0 && OutMode.RefreshRate > 0;
		return OutMode.bIsValid;
	}

	static bool TryQueryDisplayConfig(TArray<DISPLAYCONFIG_PATH_INFO>& OutPaths, TArray<DISPLAYCONFIG_MODE_INFO>& OutModes)
	{
		UINT32 PathCount = 0;
		UINT32 ModeCount = 0;
		if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &PathCount, &ModeCount) != ERROR_SUCCESS)
		{
			return false;
		}

		for (int32 Attempt = 0; Attempt < 3; ++Attempt)
		{
			OutPaths.SetNumZeroed(PathCount);
			OutModes.SetNumZeroed(ModeCount);

			const LONG QueryResult = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &PathCount, OutPaths.GetData(), &ModeCount, OutModes.GetData(), nullptr);
			if (QueryResult == ERROR_SUCCESS)
			{
				OutPaths.SetNum(PathCount);
				OutModes.SetNum(ModeCount);
				return true;
			}

			if (QueryResult != ERROR_INSUFFICIENT_BUFFER)
			{
				return false;
			}

			if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &PathCount, &ModeCount) != ERROR_SUCCESS)
			{
				return false;
			}
		}

		return false;
	}

	static bool TryGetDisplayPathForMonitor(const FString& MonitorId, DISPLAYCONFIG_PATH_INFO& OutPath)
	{
		TArray<DISPLAYCONFIG_PATH_INFO> Paths;
		TArray<DISPLAYCONFIG_MODE_INFO> Modes;
		if (!TryQueryDisplayConfig(Paths, Modes))
		{
			return false;
		}

		for (const DISPLAYCONFIG_PATH_INFO& Path : Paths)
		{
			DISPLAYCONFIG_SOURCE_DEVICE_NAME SourceName = {};
			SourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
			SourceName.header.size = sizeof(SourceName);
			SourceName.header.adapterId = Path.sourceInfo.adapterId;
			SourceName.header.id = Path.sourceInfo.id;

			if (DisplayConfigGetDeviceInfo(&SourceName.header) != ERROR_SUCCESS)
			{
				continue;
			}

			if (FString(SourceName.viewGdiDeviceName).Equals(MonitorId, ESearchCase::IgnoreCase))
			{
				OutPath = Path;
				return true;
			}
		}

		return false;
	}

	static bool TryGetRecommendedMode(const FString& MonitorId, FWNTDisplayMode& OutMode)
	{
		DISPLAYCONFIG_PATH_INFO Path = {};
		if (!TryGetDisplayPathForMonitor(MonitorId, Path))
		{
			return false;
		}

		DISPLAYCONFIG_TARGET_PREFERRED_MODE PreferredMode = {};
		PreferredMode.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
		PreferredMode.header.size = sizeof(PreferredMode);
		PreferredMode.header.adapterId = Path.targetInfo.adapterId;
		PreferredMode.header.id = Path.targetInfo.id;

		if (DisplayConfigGetDeviceInfo(&PreferredMode.header) != ERROR_SUCCESS)
		{
			return false;
		}

		OutMode = FWNTDisplayMode();
		OutMode.MonitorId = MonitorId;
		OutMode.Resolution = FIntPoint(static_cast<int32>(PreferredMode.width), static_cast<int32>(PreferredMode.height));
		OutMode.RefreshRate = RationalToRefreshRate(PreferredMode.targetMode.targetVideoSignalInfo.vSyncFreq);
		OutMode.bIsValid = OutMode.Resolution.X > 0 && OutMode.Resolution.Y > 0;
		return OutMode.bIsValid;
	}

	static bool TryGetHighestSupportedMode(const FString& MonitorId, FWNTDisplayMode& OutMode)
	{
		OutMode = FWNTDisplayMode();
		OutMode.MonitorId = MonitorId;

		bool bFound = false;
		DEVMODE DevMode = {};
		DevMode.dmSize = sizeof(DEVMODE);

		for (DWORD ModeIndex = 0; EnumDisplaySettings(*MonitorId, ModeIndex, &DevMode) && ModeIndex < 2048; ++ModeIndex)
		{
			if (DevMode.dmBitsPerPel != 32)
			{
				continue;
			}

			const FIntPoint CandidateResolution(static_cast<int32>(DevMode.dmPelsWidth), static_cast<int32>(DevMode.dmPelsHeight));
			const int32 CandidateRefreshRate = static_cast<int32>(DevMode.dmDisplayFrequency);
			const int64 CandidatePixels = static_cast<int64>(CandidateResolution.X) * static_cast<int64>(CandidateResolution.Y);
			const int64 BestPixels = static_cast<int64>(OutMode.Resolution.X) * static_cast<int64>(OutMode.Resolution.Y);

			if (!bFound || CandidatePixels > BestPixels || (CandidatePixels == BestPixels && CandidateRefreshRate > OutMode.RefreshRate))
			{
				OutMode.Resolution = CandidateResolution;
				OutMode.RefreshRate = CandidateRefreshRate;
				OutMode.bIsValid = CandidateResolution.X > 0 && CandidateResolution.Y > 0;
				bFound = OutMode.bIsValid;
			}
		}

		return bFound;
	}

	static TArray<FWNTDisplayMode> GetSupportedModes(const FString& MonitorId)
	{
		TArray<FWNTDisplayMode> Modes;
		TSet<FString> UniqueKeys;

		DEVMODE DevMode = {};
		DevMode.dmSize = sizeof(DEVMODE);

		for (DWORD ModeIndex = 0; EnumDisplaySettings(*MonitorId, ModeIndex, &DevMode) && ModeIndex < 2048; ++ModeIndex)
		{
			if (DevMode.dmBitsPerPel != 32)
			{
				continue;
			}

			const int32 Width = static_cast<int32>(DevMode.dmPelsWidth);
			const int32 Height = static_cast<int32>(DevMode.dmPelsHeight);
			const int32 RefreshRate = static_cast<int32>(DevMode.dmDisplayFrequency);
			if (Width <= 0 || Height <= 0 || RefreshRate <= 0)
			{
				continue;
			}

			const FString Key = FString::Printf(TEXT("%d|%d|%d"), Width, Height, RefreshRate);
			if (UniqueKeys.Contains(Key))
			{
				continue;
			}

			UniqueKeys.Add(Key);

			FWNTDisplayMode Mode;
			Mode.MonitorId = MonitorId;
			Mode.Resolution = FIntPoint(Width, Height);
			Mode.RefreshRate = RefreshRate;
			Mode.bIsValid = true;
			Modes.Add(MoveTemp(Mode));
		}

		Modes.Sort([](const FWNTDisplayMode& A, const FWNTDisplayMode& B)
		{
			const int64 APixels = static_cast<int64>(A.Resolution.X) * static_cast<int64>(A.Resolution.Y);
			const int64 BPixels = static_cast<int64>(B.Resolution.X) * static_cast<int64>(B.Resolution.Y);
			if (APixels != BPixels)
			{
				return APixels < BPixels;
			}
			return A.RefreshRate < B.RefreshRate;
		});

		return Modes;
	}

	static FString GetMonitorFriendlyName(const DISPLAY_DEVICE& Device)
	{
		DISPLAY_DEVICE MonitorDevice = {};
		MonitorDevice.cb = sizeof(DISPLAY_DEVICE);
		if (EnumDisplayDevices(Device.DeviceName, 0, &MonitorDevice, 0))
		{
			const FString MonitorName = FString(MonitorDevice.DeviceString).TrimStartAndEnd();
			if (!MonitorName.IsEmpty())
			{
				return MonitorName;
			}
		}

		return FString(Device.DeviceString).TrimStartAndEnd();
	}

	static bool TryBuildMonitorInfo(const DISPLAY_DEVICE& Device, const FString& GameWindowMonitorId, FWNTMonitorInfo& OutInfo)
	{
		OutInfo = FWNTMonitorInfo();
		OutInfo.MonitorId = FString(Device.DeviceName);
		OutInfo.Name = GetMonitorFriendlyName(Device);
		OutInfo.bIsPrimary = (Device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;
		OutInfo.bIsGameWindowMonitor = OutInfo.MonitorId.Equals(GameWindowMonitorId, ESearchCase::IgnoreCase);

		FWNTDisplayMode CurrentMode;
		if (TryGetCurrentMode(OutInfo.MonitorId, CurrentMode))
		{
			OutInfo.CurrentResolution = CurrentMode.Resolution;
			OutInfo.CurrentRefreshRate = CurrentMode.RefreshRate;
		}

		FWNTDisplayMode RecommendedMode;
		if (TryGetRecommendedMode(OutInfo.MonitorId, RecommendedMode))
		{
			OutInfo.NativeResolution = RecommendedMode.Resolution;
		}
		else
		{
			FWNTDisplayMode HighestMode;
			if (TryGetHighestSupportedMode(OutInfo.MonitorId, HighestMode))
			{
				OutInfo.NativeResolution = HighestMode.Resolution;
			}
		}

		return !OutInfo.MonitorId.IsEmpty();
	}

	static bool TryGetMonitorInfoById(const FString& MonitorId, FWNTMonitorInfo& OutInfo)
	{
		const FString RequestedMonitorId = MonitorId.TrimStartAndEnd();
		if (RequestedMonitorId.IsEmpty())
		{
			return false;
		}

		FString GameWindowMonitorId;
		TryGetGameWindowMonitorId(GameWindowMonitorId);

		for (DWORD Index = 0; ; ++Index)
		{
			DISPLAY_DEVICE Device = {};
			Device.cb = sizeof(DISPLAY_DEVICE);
			if (!EnumDisplayDevices(nullptr, Index, &Device, 0))
			{
				break;
			}

			if (!IsActiveDisplayDevice(Device))
			{
				continue;
			}

			if (RequestedMonitorId.Equals(FString(Device.DeviceName), ESearchCase::IgnoreCase))
			{
				return TryBuildMonitorInfo(Device, GameWindowMonitorId, OutInfo);
			}
		}

		return false;
	}
}
#endif

TArray<FWNTMonitorInfo> URefreshRateFunctionLibrary::GetMonitors()
{
	TArray<FWNTMonitorInfo> Result;

#if PLATFORM_WINDOWS
	FString GameWindowMonitorId;
	WNTDisplay::TryGetGameWindowMonitorId(GameWindowMonitorId);

	for (DWORD Index = 0; ; ++Index)
	{
		DISPLAY_DEVICE Device = {};
		Device.cb = sizeof(DISPLAY_DEVICE);
		if (!EnumDisplayDevices(nullptr, Index, &Device, 0))
		{
			break;
		}

		if (!WNTDisplay::IsActiveDisplayDevice(Device))
		{
			continue;
		}

		FWNTMonitorInfo Info;
		if (WNTDisplay::TryBuildMonitorInfo(Device, GameWindowMonitorId, Info))
		{
			Result.Add(MoveTemp(Info));
		}
	}
#endif

	return Result;
}

FString URefreshRateFunctionLibrary::GetGameWindowMonitorId()
{
#if PLATFORM_WINDOWS
	FString MonitorId;
	if (WNTDisplay::TryGetGameWindowMonitorId(MonitorId))
	{
		return MonitorId;
	}
#endif

	return FString();
}

FWNTMonitorInfo URefreshRateFunctionLibrary::GetGameWindowMonitor()
{
	FWNTMonitorInfo Result;

#if PLATFORM_WINDOWS
	const FString MonitorId = GetGameWindowMonitorId();
	if (!MonitorId.IsEmpty())
	{
		WNTDisplay::TryGetMonitorInfoById(MonitorId, Result);
	}
#endif

	return Result;
}

FWNTDisplayMode URefreshRateFunctionLibrary::GetCurrentDisplayMode(const FString& MonitorId)
{
	FWNTDisplayMode Result;

#if PLATFORM_WINDOWS
	FString ResolvedMonitorId;
	if (WNTDisplay::ResolveMonitorId(MonitorId, ResolvedMonitorId))
	{
		WNTDisplay::TryGetCurrentMode(ResolvedMonitorId, Result);
	}
#endif

	return Result;
}

bool URefreshRateFunctionLibrary::TestDisplayMode(const FString& MonitorId, FIntPoint Resolution, int32 RefreshRate, FString& OutError)
{
	OutError.Reset();

#if PLATFORM_WINDOWS
	FString ResolvedMonitorId;
	if (!WNTDisplay::ResolveMonitorId(MonitorId, ResolvedMonitorId, &OutError))
	{
		return false;
	}

	if (Resolution.X <= 0 || Resolution.Y <= 0 || RefreshRate <= 0)
	{
		OutError = TEXT("Resolution and refresh rate must be greater than zero.");
		return false;
	}

	DEVMODE DevMode = {};
	DevMode.dmSize = sizeof(DEVMODE);
	if (!EnumDisplaySettings(*ResolvedMonitorId, ENUM_CURRENT_SETTINGS, &DevMode))
	{
		OutError = TEXT("Failed to read current display settings.");
		return false;
	}

	DevMode.dmPelsWidth = static_cast<DWORD>(Resolution.X);
	DevMode.dmPelsHeight = static_cast<DWORD>(Resolution.Y);
	DevMode.dmDisplayFrequency = static_cast<DWORD>(RefreshRate);
	DevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

	const LONG TestResult = ChangeDisplaySettingsEx(*ResolvedMonitorId, &DevMode, nullptr, CDS_TEST, nullptr);
	if (TestResult != DISP_CHANGE_SUCCESSFUL)
	{
		OutError = FString::Printf(TEXT("Windows rejected the display mode. Code: %ld"), TestResult);
		return false;
	}

	return true;
#else
	OutError = TEXT("Display mode control is only available on Windows.");
	return false;
#endif
}

bool URefreshRateFunctionLibrary::ApplyDisplayMode(const FString& MonitorId, FIntPoint Resolution, int32 RefreshRate, float RevertAfterSeconds, FString& OutError)
{
	OutError.Reset();

#if PLATFORM_WINDOWS
	FString ResolvedMonitorId;
	if (!WNTDisplay::ResolveMonitorId(MonitorId, ResolvedMonitorId, &OutError))
	{
		return false;
	}

	if (!TestDisplayMode(ResolvedMonitorId, Resolution, RefreshRate, OutError))
	{
		return false;
	}

	DEVMODE PreviousMode = {};
	PreviousMode.dmSize = sizeof(DEVMODE);
	if (!EnumDisplaySettings(*ResolvedMonitorId, ENUM_CURRENT_SETTINGS, &PreviousMode))
	{
		OutError = TEXT("Failed to read current display settings before applying the new mode.");
		return false;
	}

	DEVMODE NewMode = PreviousMode;
	NewMode.dmPelsWidth = static_cast<DWORD>(Resolution.X);
	NewMode.dmPelsHeight = static_cast<DWORD>(Resolution.Y);
	NewMode.dmDisplayFrequency = static_cast<DWORD>(RefreshRate);
	NewMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

	const LONG ApplyResult = ChangeDisplaySettingsEx(*ResolvedMonitorId, &NewMode, nullptr, CDS_FULLSCREEN, nullptr);
	if (ApplyResult != DISP_CHANGE_SUCCESSFUL)
	{
		OutError = FString::Printf(TEXT("Failed to apply display mode. Code: %ld"), ApplyResult);
		return false;
	}

	if (RevertAfterSeconds > 0.0f)
	{
		Async(EAsyncExecution::Thread, [ResolvedMonitorId, PreviousMode, RevertAfterSeconds]()
		{
			FPlatformProcess::Sleep(RevertAfterSeconds);
			DEVMODE RevertMode = PreviousMode;
			ChangeDisplaySettingsEx(*ResolvedMonitorId, &RevertMode, nullptr, CDS_FULLSCREEN, nullptr);
		});
	}

	return true;
#else
	OutError = TEXT("Display mode control is only available on Windows.");
	return false;
#endif
}

FIntPoint URefreshRateFunctionLibrary::GetNativeResolution(const FString& MonitorId)
{
#if PLATFORM_WINDOWS
	FString ResolvedMonitorId;
	if (WNTDisplay::ResolveMonitorId(MonitorId, ResolvedMonitorId))
	{
		FWNTDisplayMode RecommendedMode;
		if (WNTDisplay::TryGetRecommendedMode(ResolvedMonitorId, RecommendedMode))
		{
			return RecommendedMode.Resolution;
		}

		FWNTDisplayMode HighestMode;
		if (WNTDisplay::TryGetHighestSupportedMode(ResolvedMonitorId, HighestMode))
		{
			return HighestMode.Resolution;
		}
	}
#endif

	return FIntPoint(1920, 1080);
}

TArray<FIntPoint> URefreshRateFunctionLibrary::GetSupportedDisplayResolutions(const FString& MonitorId)
{
	TArray<FIntPoint> Resolutions;

#if PLATFORM_WINDOWS
	FString ResolvedMonitorId;
	if (!WNTDisplay::ResolveMonitorId(MonitorId, ResolvedMonitorId))
	{
		return Resolutions;
	}

	TSet<FIntPoint> UniqueResolutions;
	for (const FWNTDisplayMode& Mode : WNTDisplay::GetSupportedModes(ResolvedMonitorId))
	{
		UniqueResolutions.Add(Mode.Resolution);
	}

	Resolutions = UniqueResolutions.Array();
	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		const int64 APixels = static_cast<int64>(A.X) * static_cast<int64>(A.Y);
		const int64 BPixels = static_cast<int64>(B.X) * static_cast<int64>(B.Y);
		if (APixels != BPixels)
		{
			return APixels < BPixels;
		}
		if (A.X != B.X)
		{
			return A.X < B.X;
		}
		return A.Y < B.Y;
	});
#else
	Resolutions.Add(FIntPoint(1920, 1080));
#endif

	return Resolutions;
}

int32 URefreshRateFunctionLibrary::GetCurrentRefreshRate(const FString& MonitorId)
{
#if PLATFORM_WINDOWS
	FWNTDisplayMode Mode = GetCurrentDisplayMode(MonitorId);
	if (Mode.bIsValid)
	{
		return Mode.RefreshRate;
	}
#endif

	return 60;
}

bool URefreshRateFunctionLibrary::SetRefreshRate(int32 NewRefreshRate, const FString& MonitorId, FString& OutError)
{
	OutError.Reset();

#if PLATFORM_WINDOWS
	if (NewRefreshRate <= 0)
	{
		OutError = TEXT("Refresh rate must be greater than zero.");
		return false;
	}

	const FWNTDisplayMode CurrentMode = GetCurrentDisplayMode(MonitorId);
	if (!CurrentMode.bIsValid)
	{
		OutError = TEXT("Failed to read the current display mode.");
		return false;
	}

	return ApplyDisplayMode(CurrentMode.MonitorId, CurrentMode.Resolution, NewRefreshRate, 0.0f, OutError);
#else
	OutError = TEXT("Refresh rate control is only available on Windows.");
	return false;
#endif
}

TArray<int32> URefreshRateFunctionLibrary::GetSupportedRefreshRates(const FString& MonitorId, FIntPoint Resolution)
{
	TArray<int32> Rates;

#if PLATFORM_WINDOWS
	FString ResolvedMonitorId;
	if (!WNTDisplay::ResolveMonitorId(MonitorId, ResolvedMonitorId))
	{
		return Rates;
	}

	if (Resolution.X <= 0 || Resolution.Y <= 0)
	{
		const FWNTDisplayMode CurrentMode = GetCurrentDisplayMode(ResolvedMonitorId);
		if (!CurrentMode.bIsValid)
		{
			return Rates;
		}
		Resolution = CurrentMode.Resolution;
	}

	TSet<int32> UniqueRates;
	for (const FWNTDisplayMode& Mode : WNTDisplay::GetSupportedModes(ResolvedMonitorId))
	{
		if (Mode.Resolution == Resolution && Mode.RefreshRate > 0)
		{
			UniqueRates.Add(Mode.RefreshRate);
		}
	}

	Rates = UniqueRates.Array();
	Rates.Sort();
#endif

	return Rates;
}

