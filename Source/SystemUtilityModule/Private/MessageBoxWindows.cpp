// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "MessageBoxWindows.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h" 
// -----------------

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include <CommCtrl.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

void UNativeMessageBox::ShowNativeMessageBox(
    const FString& Title,
    const FString& Message,
    EMessageBoxButtons Buttons,
    EWNTMessageBoxIcon Icon,
    EMessageBoxResult& Result)
{

    Result = EMessageBoxResult::Canceled;

#if PLATFORM_WINDOWS
    HWND ParentWindow = NULL;
    if (GEngine && GEngine->GameViewport)
    {
        ParentWindow = (HWND)GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
    }

    UINT uType = 0;
    switch (Buttons)
    {
    case EMessageBoxButtons::Ok:          uType |= MB_OK; break;
    case EMessageBoxButtons::OkCancel:    uType |= MB_OKCANCEL; break;
    case EMessageBoxButtons::YesNo:       uType |= MB_YESNO; break;
    case EMessageBoxButtons::YesNoCancel: uType |= MB_YESNOCANCEL; break;
    }


    switch (Icon)
    {
    case EWNTMessageBoxIcon::Error:       uType |= MB_ICONSTOP; break;
    case EWNTMessageBoxIcon::Warning:     uType |= MB_ICONWARNING; break;
    case EWNTMessageBoxIcon::Information: uType |= MB_ICONINFORMATION; break;
    case EWNTMessageBoxIcon::Question:    uType |= MB_ICONQUESTION; break;
    case EWNTMessageBoxIcon::None:        break;
    }

    int32 WinResult = MessageBoxW(ParentWindow, *Message, *Title, uType);

    switch (WinResult)
    {
    case IDOK:
    case IDYES:
        Result = EMessageBoxResult::Confirmed;
        break;

    case IDNO:
        Result = EMessageBoxResult::Declined;
        break;

    case IDCANCEL:
    default:
        Result = EMessageBoxResult::Canceled;
        break;
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("ShowNativeMessageBox: Not supported on this platform"));
    Result = EMessageBoxResult::Canceled;
#endif
}

void UNativeMessageBox::ShowMessageBox(
    const FString& Title,
    const FString& Message,
    EWNTMessageBoxIcon Icon,
    const FString& FirstButtonText,
    const FString& SecondButtonText,
    bool bShowSecondButton,
    ECustomDialogResult& Result)
{

    Result = ECustomDialogResult::SecondButton;

#if PLATFORM_WINDOWS
    HWND ParentWindow = NULL;
    if (GEngine && GEngine->GameViewport)
    {
        ParentWindow = (HWND)GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
    }

    TASKDIALOGCONFIG Config = { 0 };
    Config.cbSize = sizeof(Config);
    Config.hwndParent = ParentWindow;
    Config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_ALLOW_DIALOG_CANCELLATION;

    Config.pszWindowTitle = *Title;   
    Config.pszMainInstruction = *Message;

    switch (Icon)
    {
    case EWNTMessageBoxIcon::Information: Config.pszMainIcon = TD_INFORMATION_ICON; break;
    case EWNTMessageBoxIcon::Warning:     Config.pszMainIcon = TD_WARNING_ICON; break;
    case EWNTMessageBoxIcon::Error:       Config.pszMainIcon = TD_ERROR_ICON; break;
    case EWNTMessageBoxIcon::Question:    Config.pszMainIcon = TD_INFORMATION_ICON; break;
    default:                           Config.pszMainIcon = nullptr; break;
    }

    TASKDIALOG_BUTTON Buttons[2];
    int ButtonCount = 0;

    Buttons[ButtonCount].nButtonID = 101;
    Buttons[ButtonCount].pszButtonText = *FirstButtonText;
    ButtonCount++;


    if (bShowSecondButton)
    {
        Buttons[ButtonCount].nButtonID = 102;
        Buttons[ButtonCount].pszButtonText = *SecondButtonText;
        ButtonCount++;
    }

    Config.pButtons = Buttons;
    Config.cButtons = ButtonCount;

    int nPressedButtonID = 0;
    HRESULT hr = TaskDialogIndirect(&Config, &nPressedButtonID, nullptr, nullptr);


    if (SUCCEEDED(hr))
    {

        if (nPressedButtonID == 101)
        {
            Result = ECustomDialogResult::FirstButton;
        }
        else
        {

            Result = ECustomDialogResult::SecondButton;
        }
    }
#else
    // Non-Windows Fallback
    UE_LOG(LogTemp, Warning, TEXT("Native Custom MessageBox is Windows Only."));
#endif
}