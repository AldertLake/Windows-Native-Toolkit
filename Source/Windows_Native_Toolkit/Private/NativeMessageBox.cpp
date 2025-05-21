/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

#include "NativeMessageBox.h"
#include "Windows/WindowsHWrapper.h"

FNativeMessageBoxResult UNativeMessageBox::ShowNativeMessageBox(
    const FString& Title,
    const FString& Content,
    ENativeMessageBoxIcon IconType,
    bool bShowSecondButton)
{
    FNativeMessageBoxResult Result;

    // Convert FString to wide char for Windows API
    const TCHAR* TitleText = *Title;
    const TCHAR* ContentText = *Content;

    // Set message box icon
    UINT IconFlag = 0;
    switch (IconType)
    {
    case ENativeMessageBoxIcon::Information:
        IconFlag = MB_ICONINFORMATION;
        break;
    case ENativeMessageBoxIcon::Warning:
        IconFlag = MB_ICONWARNING;
        break;
    case ENativeMessageBoxIcon::Error:
        IconFlag = MB_ICONERROR;
        break;
    case ENativeMessageBoxIcon::Question:
        IconFlag = MB_ICONQUESTION;
        break;
    case ENativeMessageBoxIcon::None:
    default:
        IconFlag = 0;
        break;
    }

    // Set button configuration
    UINT ButtonFlag = MB_OK;
    if (bShowSecondButton)
    {
        ButtonFlag = MB_OKCANCEL;
    }

    // Combine flags
    UINT Flags = IconFlag | ButtonFlag;

    // Show the message box
    int32 WindowsResult = MessageBox(
        nullptr,           // No parent window
        ContentText,       // Message content
        TitleText,         // Window title
        Flags              // Combined flags
    );

    // Process the result
    switch (WindowsResult)
    {
    case IDOK:
        Result.bFirstButtonPressed = true;
        Result.bWasClosedWithoutSelection = false;
        break;
    case IDCANCEL:
        Result.bFirstButtonPressed = false;
        Result.bWasClosedWithoutSelection = bShowSecondButton ? false : true;
        break;
    case 0:  // Window was closed (X button or Alt+F4)
        Result.bFirstButtonPressed = false;
        Result.bWasClosedWithoutSelection = true;
        break;
    default:
        Result.bFirstButtonPressed = false;
        Result.bWasClosedWithoutSelection = true;
        break;
    }

    return Result;
}