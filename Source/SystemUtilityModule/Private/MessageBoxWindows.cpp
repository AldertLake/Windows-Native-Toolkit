// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "MessageBoxWindows.h"
#include "Async/Async.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#include <atomic>

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include <CommCtrl.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if PLATFORM_WINDOWS
static HWND GetBestParentWindowHandle()
{
    if (FSlateApplication::IsInitialized())
    {
        return static_cast<HWND>(const_cast<void*>(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr)));
    }

    if (GEngine && GEngine->GameViewport && GEngine->GameViewport->GetWindow().IsValid())
    {
        TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
        if (Window.IsValid() && Window->GetNativeWindow().IsValid())
        {
            return static_cast<HWND>(Window->GetNativeWindow()->GetOSWindowHandle());
        }
    }

    return nullptr;
}
#endif

namespace WNTMessageBox
{
    static EMessageBoxResult RunNativeDialog(HWND ParentWindow, const FString& Title, const FString& Message, EMessageBoxButtons Buttons, EWNTMessageBoxIcon Icon)
    {
        EMessageBoxResult Result = EMessageBoxResult::Canceled;

#if PLATFORM_WINDOWS
        UINT Type = 0;
        switch (Buttons)
        {
        case EMessageBoxButtons::Ok:          Type |= MB_OK; break;
        case EMessageBoxButtons::OkCancel:    Type |= MB_OKCANCEL; break;
        case EMessageBoxButtons::YesNo:       Type |= MB_YESNO; break;
        case EMessageBoxButtons::YesNoCancel: Type |= MB_YESNOCANCEL; break;
        }

        switch (Icon)
        {
        case EWNTMessageBoxIcon::Error:       Type |= MB_ICONSTOP; break;
        case EWNTMessageBoxIcon::Warning:     Type |= MB_ICONWARNING; break;
        case EWNTMessageBoxIcon::Information: Type |= MB_ICONINFORMATION; break;
        case EWNTMessageBoxIcon::Question:    Type |= MB_ICONQUESTION; break;
        case EWNTMessageBoxIcon::None:        break;
        }

        switch (MessageBoxW(ParentWindow, *Message, *Title, Type))
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
        UE_LOG(LogTemp, Error, TEXT("Error: Show Native Message Box is only available on Windows."));
#endif

        return Result;
    }

    static ECustomDialogResult RunCustomDialog(HWND ParentWindow, const FString& Title, const FString& Message, EWNTMessageBoxIcon Icon, const FString& FirstButtonText, const FString& SecondButtonText, bool bShowSecondButton)
    {
        ECustomDialogResult Result = ECustomDialogResult::SecondButton;

#if PLATFORM_WINDOWS
        TASKDIALOGCONFIG Config = { 0 };
        Config.cbSize = sizeof(Config);
        Config.hwndParent = ParentWindow;
        Config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_ALLOW_DIALOG_CANCELLATION | TDF_SIZE_TO_CONTENT;
        Config.pszWindowTitle = *Title;
        Config.pszMainInstruction = *Message;

        switch (Icon)
        {
        case EWNTMessageBoxIcon::Information: Config.pszMainIcon = TD_INFORMATION_ICON; break;
        case EWNTMessageBoxIcon::Warning:     Config.pszMainIcon = TD_WARNING_ICON; break;
        case EWNTMessageBoxIcon::Error:       Config.pszMainIcon = TD_ERROR_ICON; break;
        case EWNTMessageBoxIcon::Question:    Config.pszMainIcon = TD_INFORMATION_ICON; break;
        default:                              Config.pszMainIcon = nullptr; break;
        }

        TASKDIALOG_BUTTON Buttons[2];
        int32 ButtonCount = 0;

        Buttons[ButtonCount].nButtonID = 101;
        Buttons[ButtonCount].pszButtonText = *FirstButtonText;
        ++ButtonCount;

        if (bShowSecondButton)
        {
            Buttons[ButtonCount].nButtonID = 102;
            Buttons[ButtonCount].pszButtonText = *SecondButtonText;
            ++ButtonCount;
        }

        Config.pButtons = Buttons;
        Config.cButtons = ButtonCount;

        int PressedButtonId = 0;
        if (SUCCEEDED(TaskDialogIndirect(&Config, &PressedButtonId, nullptr, nullptr)))
        {
            Result = (PressedButtonId == 101) ? ECustomDialogResult::FirstButton : ECustomDialogResult::SecondButton;
        }
#else
        UE_LOG(LogTemp, Error, TEXT("Error: Show Regular Message Box is only available on Windows."));
#endif

        return Result;
    }

}

UAsyncNativeMessageBoxAction* UAsyncNativeMessageBoxAction::ShowNativeMessageBoxAsync(const UObject* WorldContextObject, const FString& Title, const FString& Message, EMessageBoxButtons Buttons, EWNTMessageBoxIcon Icon)
{
    UAsyncNativeMessageBoxAction* Node = NewObject<UAsyncNativeMessageBoxAction>();
    Node->DialogTitle = Title;
    Node->DialogMessage = Message;
    Node->DialogButtons = Buttons;
    Node->DialogIcon = Icon;

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

void UAsyncNativeMessageBoxAction::Activate()
{
    TWeakObjectPtr<UAsyncNativeMessageBoxAction> WeakThis(this);
    const FString TitleCopy = DialogTitle;
    const FString MessageCopy = DialogMessage;
    const EMessageBoxButtons ButtonsCopy = DialogButtons;
    const EWNTMessageBoxIcon IconCopy = DialogIcon;
    const HWND ParentWindow = GetBestParentWindowHandle();

    Async(EAsyncExecution::Thread, [WeakThis, ParentWindow, TitleCopy, MessageCopy, ButtonsCopy, IconCopy]()
    {
        const EMessageBoxResult Result = WNTMessageBox::RunNativeDialog(ParentWindow, TitleCopy, MessageCopy, ButtonsCopy, IconCopy);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Result]()
        {
            if (UAsyncNativeMessageBoxAction* Node = WeakThis.Get())
            {
                Node->Finalize(Result);
            }
        });
    });
}

void UAsyncNativeMessageBoxAction::Finalize(EMessageBoxResult Result)
{
    switch (Result)
    {
    case EMessageBoxResult::Confirmed:
        OnConfirmed.Broadcast();
        break;
    case EMessageBoxResult::Declined:
        OnDeclined.Broadcast();
        break;
    case EMessageBoxResult::Canceled:
    default:
        OnCanceled.Broadcast();
        break;
    }

    if (bAddedToRootForCompatibility)
    {
        RemoveFromRoot();
        bAddedToRootForCompatibility = false;
    }

    SetReadyToDestroy();
}

UAsyncRegularMessageBoxAction* UAsyncRegularMessageBoxAction::ShowMessageBoxAsync(const UObject* WorldContextObject, const FString& Title, const FString& Message, EWNTMessageBoxIcon Icon, const FString& FirstButtonText, const FString& SecondButtonText, bool bShowSecondButton)
{
    UAsyncRegularMessageBoxAction* Node = NewObject<UAsyncRegularMessageBoxAction>();
    Node->DialogTitle = Title;
    Node->DialogMessage = Message;
    Node->DialogIcon = Icon;
    Node->PrimaryButtonText = FirstButtonText;
    Node->SecondaryButtonText = SecondButtonText;
    Node->bDisplaySecondButton = bShowSecondButton;

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

void UAsyncRegularMessageBoxAction::Activate()
{
    TWeakObjectPtr<UAsyncRegularMessageBoxAction> WeakThis(this);
    const FString TitleCopy = DialogTitle;
    const FString MessageCopy = DialogMessage;
    const EWNTMessageBoxIcon IconCopy = DialogIcon;
    const FString FirstButtonTextCopy = PrimaryButtonText;
    const FString SecondButtonTextCopy = SecondaryButtonText;
    const bool bShowSecondButtonCopy = bDisplaySecondButton;
    const HWND ParentWindow = GetBestParentWindowHandle();

    Async(EAsyncExecution::Thread, [WeakThis, ParentWindow, TitleCopy, MessageCopy, IconCopy, FirstButtonTextCopy, SecondButtonTextCopy, bShowSecondButtonCopy]()
    {
        const ECustomDialogResult Result = WNTMessageBox::RunCustomDialog(ParentWindow, TitleCopy, MessageCopy, IconCopy, FirstButtonTextCopy, SecondButtonTextCopy, bShowSecondButtonCopy);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Result]()
        {
            if (UAsyncRegularMessageBoxAction* Node = WeakThis.Get())
            {
                Node->Finalize(Result);
            }
        });
    });
}

void UAsyncRegularMessageBoxAction::Finalize(ECustomDialogResult Result)
{
    switch (Result)
    {
    case ECustomDialogResult::FirstButton:
        OnFirstButton.Broadcast();
        break;
    case ECustomDialogResult::SecondButton:
    default:
        OnSecondButton.Broadcast();
        break;
    }

    if (bAddedToRootForCompatibility)
    {
        RemoveFromRoot();
        bAddedToRootForCompatibility = false;
    }

    SetReadyToDestroy();
}

void UNativeMessageBox::ShowNativeMessageBox(
    const FString& Title,
    const FString& Message,
    EMessageBoxButtons Buttons,
    EWNTMessageBoxIcon Icon,
    EMessageBoxResult& Result)
{
    Result = WNTMessageBox::RunNativeDialog(GetBestParentWindowHandle(), Title, Message, Buttons, Icon);
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
    Result = WNTMessageBox::RunCustomDialog(GetBestParentWindowHandle(), Title, Message, Icon, FirstButtonText, SecondButtonText, bShowSecondButton);
}


