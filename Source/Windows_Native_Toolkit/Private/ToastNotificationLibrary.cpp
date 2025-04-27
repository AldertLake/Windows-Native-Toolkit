/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#include "ToastNotificationLibrary.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include <ShellAPI.h>
#include "Misc/ConfigCacheIni.h" 
#include "Misc/CoreDelegates.h"

#include "Windows/HideWindowsPlatformTypes.h"

static NOTIFYICONDATAW TrayIconData = { 0 };
static bool bIsTrayIconInitialized = false;
bool UToastNotificationLibrary::bIsShutdownRegistered = false;

// Static initialization
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

// Static shutdown (just in case we need to manually clean up)
void UToastNotificationLibrary::StaticShutdown()
{
    CleanupTrayIcon();
    bIsShutdownRegistered = false;
}

FString UToastNotificationLibrary::GetGameTitle()
{
    FString GameTitle;
    GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectName"), GameTitle, GGameIni);
    if (GameTitle.IsEmpty())
    {
        GameTitle = TEXT("My Unreal Game"); // Fallback title
    }
    return GameTitle;
}

void UToastNotificationLibrary::ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType)
{
    if (!IsRunningOnWindows())
    {
        UE_LOG(LogTemp, Warning, TEXT("Toast notifications are only supported on Windows."));
        return;
    }

    // Register shutdown callback if not already done
    StaticInitialize();

    DisplayTrayNotification(Title, Message, IconType);
}

void UToastNotificationLibrary::CleanupTrayIcon()
{
    if (bIsTrayIconInitialized)
    {
        Shell_NotifyIconW(NIM_DELETE, &TrayIconData);
        bIsTrayIconInitialized = false;
        ZeroMemory(&TrayIconData, sizeof(NOTIFYICONDATAW)); // Clear the structure to prevent stale data
        UE_LOG(LogTemp, Log, TEXT("Tray icon cleaned up."));
    }
}

void UToastNotificationLibrary::DisplayTrayNotification(const FString& Title, const FString& Message, EToastIconType IconType)
{
    // Always attempt to clean up any existing tray icon to avoid conflicts
    if (bIsTrayIconInitialized)
    {
        Shell_NotifyIconW(NIM_DELETE, &TrayIconData);
        bIsTrayIconInitialized = false;
        ZeroMemory(&TrayIconData, sizeof(NOTIFYICONDATAW));
    }

    // Initialize the tray icon
    HWND hWnd = GetActiveWindow(); // Use the current active window as the owner
    if (!hWnd)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get active window for tray notification."));
        return;
    }

    // Get the game title for the tooltip
    FString GameTitle = GetGameTitle();
    std::wstring GameTitleW = std::wstring(GameTitle.GetCharArray().GetData());

    TrayIconData.cbSize = sizeof(NOTIFYICONDATAW);
    TrayIconData.hWnd = hWnd;
    TrayIconData.uID = 1; // Unique ID for the tray icon
    TrayIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    TrayIconData.uCallbackMessage = WM_USER + 1; // Custom message ID
    TrayIconData.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Default app icon
    TrayIconData.uTimeout = 5000; // 5 seconds timeout

    // Set tooltip to game title (limited to 128 chars)
    wcsncpy_s(TrayIconData.szTip, GameTitleW.c_str(), 128);

    if (!Shell_NotifyIconW(NIM_ADD, &TrayIconData))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add tray icon."));
        return;
    }

    bIsTrayIconInitialized = true;

    // Set the balloon icon based on the user's choice
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

    // Update the notification with title and message
    wcsncpy_s(TrayIconData.szInfoTitle, *Title, 64); // Title limited to 64 chars
    wcsncpy_s(TrayIconData.szInfo, *Message, 256);   // Message limited to 256 chars

    if (!Shell_NotifyIconW(NIM_MODIFY, &TrayIconData))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to show tray notification."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Tray notification shown: %s - %s"), *Title, *Message);
    }
}

bool UToastNotificationLibrary::IsRunningOnWindows()
{
#if PLATFORM_WINDOWS
    return true;
#else
    return false;
#endif
}