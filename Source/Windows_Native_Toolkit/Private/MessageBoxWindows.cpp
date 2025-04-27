/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#include "MessageBoxWindows.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <CommCtrl.h>

FMessageBoxResult UMessageBoxWindows::ShowMessageBox(
    const FString& Title,
    const FString& Content,
    EMessageBoxIcon IconType,
    bool bShowSecondButton,
    const FString& FirstButtonText,
    const FString& SecondButtonText)
{
    FMessageBoxResult Result;

    // convertir FString en wide string
    const FString WideTitle = Title;
    const FString WideContent = Content;
    const FString WideFirstButton = FirstButtonText;
    const FString WideSecondButton = SecondButtonText;

    // Set up du task dialog configuration
    TASKDIALOGCONFIG config = { 0 };
    config.cbSize = sizeof(config);
    config.hwndParent = nullptr;
    config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;

    // Set du title et du content
    config.pszWindowTitle = *WideTitle;
    config.pszMainInstruction = *WideContent;

    // Set de L'icon
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

    TASKDIALOG_BUTTON buttons[2];
    int buttonCount = bShowSecondButton ? 2 : 1;

    buttons[0].nButtonID = 100;  // Custom ID for first button
    buttons[0].pszButtonText = *WideFirstButton;

    if (bShowSecondButton)
    {
        buttons[1].nButtonID = 101;  // Custom ID for second button
        buttons[1].pszButtonText = *WideSecondButton;
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
        case 100:  // First button ID
            Result.bFirstButtonPressed = true;
            Result.bWasClosedWithoutSelection = false;
            break;
        case 101:  // Second button ID
            Result.bFirstButtonPressed = false;
            Result.bWasClosedWithoutSelection = false;
            break;
        case 0:    // Dialog closed
            Result.bFirstButtonPressed = false;
            Result.bWasClosedWithoutSelection = true;
            break;
        }
    }
    else
    {
        Result.bFirstButtonPressed = false;
        Result.bWasClosedWithoutSelection = true;
    }

    return Result;
}

#include "Windows/HideWindowsPlatformTypes.h"