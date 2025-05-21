/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

#include "ToastNotificationLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <Shellapi.h>
#include <string>

#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#include "Logging/LogMacros.h"

// Log category for toast notifications
DEFINE_LOG_CATEGORY_STATIC(LogToastNotification, Log, All);

// Static member definitions
#if PLATFORM_WINDOWS
static NOTIFYICONDATAW TrayIconData = { 0 };
static bool bIsTrayIconInitialized = false;
static bool bIsShutdownRegistered = false;
#endif

void UToastNotificationLibrary::StaticInitialize()
{
#if PLATFORM_WINDOWS
    if (!bIsShutdownRegistered)
    {
        FCoreDelegates::OnPreExit.AddLambda([]()
        {
            CleanupTrayIcon();
        });
        bIsShutdownRegistered = true;
    }
#else
    UE_LOG(LogToastNotification, Warning, TEXT("StaticInitialize is only supported on Windows platforms."));
#endif
}

void UToastNotificationLibrary::StaticShutdown()
{
#if PLATFORM_WINDOWS
    CleanupTrayIcon();
    bIsShutdownRegistered = false;
#else
    UE_LOG(LogToastNotification, Warning, TEXT("StaticShutdown is only supported on Windows platforms."));
#endif
}

FString UToastNotificationLibrary::GetGameTitle()
{
    FString GameTitle;
    if (GConfig && GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectName"), GameTitle, GGameIni))
    {
        return GameTitle.IsEmpty() ? TEXT("My Unreal Game") : GameTitle;
    }
    return TEXT("My Unreal Game");
}

void UToastNotificationLibrary::ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType)
{
#if PLATFORM_WINDOWS
    StaticInitialize();
    DisplayTrayNotification(Title, Message, IconType);
#else
    UE_LOG(LogToastNotification, Warning, TEXT("ShowToastNotification is only supported on Windows platforms."));
#endif
}

void UToastNotificationLibrary::CleanupTrayIcon()
{
#if PLATFORM_WINDOWS
    if (bIsTrayIconInitialized)
    {
        if (!Shell_NotifyIconW(NIM_DELETE, &TrayIconData))
        {
            UE_LOG(LogToastNotification, Warning, TEXT("Failed to remove tray icon: %d"), GetLastError());
        }
        bIsTrayIconInitialized = false;
        ZeroMemory(&TrayIconData, sizeof(NOTIFYICONDATAW));
    }
#else
    UE_LOG(LogToastNotification, Warning, TEXT("CleanupTrayIcon is only supported on Windows platforms."));
#endif
}

void UToastNotificationLibrary::DisplayTrayNotification(const FString& Title, const FString& Message, EToastIconType IconType)
{
#if PLATFORM_WINDOWS
    // Clean up any existing tray icon to avoid conflicts
    CleanupTrayIcon();

    // Get the active window handle
    HWND hWnd = GetActiveWindow();
    if (!hWnd)
    {
        UE_LOG(LogToastNotification, Warning, TEXT("No active window handle available for tray notification."));
        return;
    }

    // Get the game title for the tooltip
    FString GameTitle = GetGameTitle();
    std::wstring GameTitleW(*GameTitle);

    // Initialize tray icon data
    TrayIconData.cbSize = sizeof(NOTIFYICONDATAW);
    TrayIconData.hWnd = hWnd;
    TrayIconData.uID = 1;
    TrayIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    TrayIconData.uCallbackMessage = WM_USER + 1;
    TrayIconData.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    TrayIconData.uTimeout = 5000; // 5-second timeout

    // Set tooltip (limited to 128 characters)
    wcsncpy_s(TrayIconData.szTip, _countof(TrayIconData.szTip), GameTitleW.c_str(), _TRUNCATE);

    // Add the tray icon
    if (!Shell_NotifyIconW(NIM_ADD, &TrayIconData))
    {
        UE_LOG(LogToastNotification, Warning, TEXT("Failed to add tray icon: %d"), GetLastError());
        return;
    }

    bIsTrayIconInitialized = true;

    // Set the balloon icon based on IconType
    switch (IconType)
    {
    case EToastIconType::Warning:
        TrayIconData.dwInfoFlags = NIIF_WARNING;
        break;
    case EToastIconType::Error:
        TrayIconData.dwInfoFlags = NIIF_ERROR;
        break;
    case EToastIconType::Info:
    default:
        TrayIconData.dwInfoFlags = NIIF_INFO;
        break;
    }

    // Set title and message (with length limits)
    wcsncpy_s(TrayIconData.szInfoTitle, _countof(TrayIconData.szInfoTitle), *Title, _TRUNCATE);
    wcsncpy_s(TrayIconData.szInfo, _countof(TrayIconData.szInfo), *Message, _TRUNCATE);

    // Show the notification
    if (!Shell_NotifyIconW(NIM_MODIFY, &TrayIconData))
    {
        UE_LOG(LogToastNotification, Warning, TEXT("Failed to show tray notification: %d"), GetLastError());
    }
#else
    UE_LOG(LogToastNotification, Warning, TEXT("DisplayTrayNotification is only supported on Windows platforms."));
#endif
}