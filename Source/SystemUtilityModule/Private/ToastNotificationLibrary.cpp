// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "ToastNotificationLibrary.h" 
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"


#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include "Windows/HideWindowsPlatformTypes.h"

static const UINT TRAY_ICON_ID = 5500;
static bool bIsCleanupRegistered = false;
#endif

void UToastNotificationLibrary::ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType)
{
#if PLATFORM_WINDOWS

    if (!GEngine || !GEngine->GameViewport) return;

    TSharedPtr<SWindow> WindowPtr = GEngine->GameViewport->GetWindow();
    if (!WindowPtr.IsValid()) return;

    TSharedPtr<FGenericWindow> NativeWindow = WindowPtr->GetNativeWindow();
    if (!NativeWindow.IsValid()) return;

    HWND ParentWindow = (HWND)NativeWindow->GetOSWindowHandle();
    if (!ParentWindow) return;

    NOTIFYICONDATAW Nid = { 0 };
    Nid.cbSize = sizeof(NOTIFYICONDATAW);
    Nid.hWnd = ParentWindow;
    Nid.uID = TRAY_ICON_ID;

    Nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_INFO;
    Nid.uCallbackMessage = WM_USER + 300; // Unique callback ID

    // Load Game Icon
    HICON GameIcon = (HICON)GetClassLongPtr(ParentWindow, GCLP_HICON);
    Nid.hIcon = GameIcon ? GameIcon : LoadIcon(NULL, IDI_APPLICATION);

    const TCHAR* ProjName = FApp::GetProjectName();
    if (ProjName)
    {
        StringCchCopyW(Nid.szTip, ARRAYSIZE(Nid.szTip), (LPCWSTR)ProjName);
    }
    else
    {
        StringCchCopyW(Nid.szTip, ARRAYSIZE(Nid.szTip), L"Unreal Game");
    }

    StringCchCopyW(Nid.szInfoTitle, ARRAYSIZE(Nid.szInfoTitle), (LPCWSTR)*Title);
    StringCchCopyW(Nid.szInfo, ARRAYSIZE(Nid.szInfo), (LPCWSTR)*Message);

    // Set Icon Type
    switch (IconType)
    {
    case EToastIconType::Info:    Nid.dwInfoFlags = NIIF_INFO; break;
    case EToastIconType::Warning: Nid.dwInfoFlags = NIIF_WARNING; break;
    case EToastIconType::Error:   Nid.dwInfoFlags = NIIF_ERROR; break;
    default:                      Nid.dwInfoFlags = NIIF_NONE; break;
    }

    BOOL bSuccess = Shell_NotifyIconW(NIM_MODIFY, &Nid);

    if (!bSuccess)
    {
        // Icon missing, add it fresh
        bSuccess = Shell_NotifyIconW(NIM_ADD, &Nid);

        if (bSuccess)
        {
            // Enable modern behavior (Version 4)
            Nid.uVersion = NOTIFYICON_VERSION_4;
            Shell_NotifyIconW(NIM_SETVERSION, &Nid);

            // Register cleanup
            if (!bIsCleanupRegistered)
            {
                FCoreDelegates::OnPreExit.AddStatic(&UToastNotificationLibrary::CleanupTrayIcon);
                bIsCleanupRegistered = true;
            }
        }
    }
#endif
}

void UToastNotificationLibrary::CleanupTrayIcon()
{
#if PLATFORM_WINDOWS
    NOTIFYICONDATAW Nid = { 0 };
    Nid.cbSize = sizeof(NOTIFYICONDATAW);

    // Try to get window handle safely
    if (GEngine && GEngine->GameViewport)
    {
        TSharedPtr<SWindow> Win = GEngine->GameViewport->GetWindow();
        if (Win.IsValid() && Win->GetNativeWindow().IsValid())
        {
            Nid.hWnd = (HWND)Win->GetNativeWindow()->GetOSWindowHandle();
        }
    }

    Nid.uID = TRAY_ICON_ID;
    Shell_NotifyIconW(NIM_DELETE, &Nid);
#endif
}