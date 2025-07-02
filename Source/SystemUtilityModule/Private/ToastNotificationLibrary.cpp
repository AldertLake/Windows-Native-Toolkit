// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "ToastNotificationLibrary.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h" 
#include <ShellAPI.h>
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#include "Windows/HideWindowsPlatformTypes.h"
#include <string> // Added for std::wstring

static NOTIFYICONDATAW TrayIconData = { 0 };
static bool bIsTrayIconInitialized = false;
bool UToastNotificationLibrary::bIsShutdownRegistered = false;

// Registers cleanup callback on application exit
void UToastNotificationLibrary::StaticInitialize()
{
    if (!bIsShutdownRegistered)
    {
        FCoreDelegates::OnPreExit.AddLambda([]()
            {
                CleanupTrayIcon();
            });
        bIsShutdownRegistered = true;
    }
}

// Manually cleans up resources (optional)
void UToastNotificationLibrary::StaticShutdown()
{
    CleanupTrayIcon();
    bIsShutdownRegistered = false;
}

// Retrieves the game title from the configuration or uses a fallback
FString UToastNotificationLibrary::GetGameTitle()
{
    FString GameTitle;
    GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectName"), GameTitle, GGameIni);
    return GameTitle.IsEmpty() ? TEXT("My Unreal Game") : GameTitle;
}

// Displays a tray notification with the specified title, message, and icon type
void UToastNotificationLibrary::ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType)
{
    StaticInitialize(); // Ensure cleanup is registered
    DisplayTrayNotification(Title, Message, IconType);
}

// Removes the tray icon from the system tray
void UToastNotificationLibrary::CleanupTrayIcon()
{
    if (bIsTrayIconInitialized)
    {
        Shell_NotifyIconW(NIM_DELETE, &TrayIconData);
        bIsTrayIconInitialized = false;
        ZeroMemory(&TrayIconData, sizeof(NOTIFYICONDATAW)); // Clear structure to prevent stale data
    }
}

// Internal implementation of tray notification display
void UToastNotificationLibrary::DisplayTrayNotification(const FString& Title, const FString& Message, EToastIconType IconType)
{
    // Clean up any existing tray icon to avoid conflicts
    if (bIsTrayIconInitialized)
    {
        Shell_NotifyIconW(NIM_DELETE, &TrayIconData);
        bIsTrayIconInitialized = false;
        ZeroMemory(&TrayIconData, sizeof(NOTIFYICONDATAW));
    }

    // Get the active window handle
    HWND hWnd = GetActiveWindow();
    if (!hWnd)
    {
        return; // Silently fail if no window is available
    }

    // Get the game title for the tooltip
    FString GameTitle = GetGameTitle();
    std::wstring GameTitleW(GameTitle.GetCharArray().GetData());

    // Initialize tray icon data
    TrayIconData.cbSize = sizeof(NOTIFYICONDATAW);
    TrayIconData.hWnd = hWnd;
    TrayIconData.uID = 1; // Unique ID for the tray icon
    TrayIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    TrayIconData.uCallbackMessage = WM_USER + 1; // Custom message ID
    TrayIconData.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Default application icon
    TrayIconData.uTimeout = 5000; // 5-second timeout

    // Set tooltip (limited to 128 characters)
    wcsncpy_s(TrayIconData.szTip, 128, GameTitleW.c_str(), _TRUNCATE);

    // Add the tray icon
    if (!Shell_NotifyIconW(NIM_ADD, &TrayIconData))
    {
        return; // Silently fail if tray icon creation fails
    }

    bIsTrayIconInitialized = true;

    // Set the balloon icon based on IconType
    switch (IconType)
    {
    case EToastIconType::Warning:
        TrayIconData.dwInfoFlags = NIIF_WARNING; // Exclamation mark
        break;
    case EToastIconType::Error:
        TrayIconData.dwInfoFlags = NIIF_ERROR;   // Error (red X)
        break;
    case EToastIconType::Info:
    default:
        TrayIconData.dwInfoFlags = NIIF_INFO;    // Info (blue "i")
        break;
    }

    // Set title and message (with length limits)
    wcsncpy_s(TrayIconData.szInfoTitle, 64, *Title, _TRUNCATE); // Title limited to 64 chars
    wcsncpy_s(TrayIconData.szInfo, 256, *Message, _TRUNCATE);   // Message limited to 256 chars

    // Show the notification
    Shell_NotifyIconW(NIM_MODIFY, &TrayIconData);
}