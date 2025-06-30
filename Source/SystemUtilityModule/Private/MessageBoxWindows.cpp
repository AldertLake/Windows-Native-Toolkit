// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "MessageBoxWindows.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h"  
#include <CommCtrl.h>
#include "Windows/HideWindowsPlatformTypes.h"


FMessageBoxResult UMessageBoxWindows::ShowMessageBox(
    const FString& Title,
    const FString& Content,
    EMessageBoxIcon IconType,
    bool bShowSecondButton,
    const FString& FirstButtonText,
    const FString& SecondButtonText)
{
    FMessageBoxResult Result;

    // Convert FString to wide strings directly
    const TCHAR* WideTitle = *Title;
    const TCHAR* WideContent = *Content;
    const TCHAR* WideFirstButton = *FirstButtonText;
    const TCHAR* WideSecondButton = *SecondButtonText;

    // Set up task dialog configuration
    TASKDIALOGCONFIG config = { 0 };
    config.cbSize = sizeof(config);
    config.hwndParent = nullptr;
    config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;

    // Set title and content
    config.pszWindowTitle = WideTitle;
    config.pszMainInstruction = WideContent;

    // Set icon based on IconType
    switch (IconType)
    {
    case EMessageBoxIcon::Information:
        config.pszMainIcon = TD_INFORMATION_ICON;
        break;
    case EMessageBoxIcon::Warning:
        config.pszMainIcon = TD_WARNING_ICON;
        break;
    case EMessageBoxIcon::Error:
        config.pszMainIcon = TD_ERROR_ICON;
        break;
    case EMessageBoxIcon::Question:
        config.pszMainIcon = nullptr;  // No specific question icon, using null
        break;
    case EMessageBoxIcon::None:
    default:
        config.pszMainIcon = nullptr;
        break;
    }

    // Configure buttons
    TASKDIALOG_BUTTON buttons[2];
    int buttonCount = bShowSecondButton ? 2 : 1;

    buttons[0].nButtonID = 100;  // Custom ID for first button
    buttons[0].pszButtonText = WideFirstButton;

    if (bShowSecondButton)
    {
        buttons[1].nButtonID = 101;  // Custom ID for second button
        buttons[1].pszButtonText = WideSecondButton;
    }

    config.pButtons = buttons;
    config.cButtons = buttonCount;

    // Show the dialog
    int nButtonPressed = 0;
    HRESULT hr = TaskDialogIndirect(&config, &nButtonPressed, nullptr, nullptr);

    // Process result
    if (SUCCEEDED(hr))
    {
        switch (nButtonPressed)
        {
        case 100:  // First button pressed
            Result.bFirstButtonPressed = true;
            Result.bWasClosedWithoutSelection = false;
            break;
        case 101:  // Second button pressed
            Result.bFirstButtonPressed = false;
            Result.bWasClosedWithoutSelection = false;
            break;
        case 0:    // Dialog closed without selection (e.g., via close button)
            Result.bFirstButtonPressed = false;
            Result.bWasClosedWithoutSelection = true;
            break;
        }
    }
    else
    {
        // Handle failure to create dialog (e.g., invalid parameters or system error)
        Result.bFirstButtonPressed = false;
        Result.bWasClosedWithoutSelection = true;
    }

    return Result;
}