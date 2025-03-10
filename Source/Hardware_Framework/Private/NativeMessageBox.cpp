/*

 * Copyright (C) 2025 AldertLake. All rights reserved.
 *
 * This file is part of the Windows Native ToolKit, an Unreal Engine Plugin.
 *
 * Unauthorized copying, distribution, or modification of this file is strictly prohibited.
 *
 * Anyone who bought this project has the full right to modify it like he want.
 *
 *
 * Author: AldertLake
 * Date: 2025/1/9
 ______________________________________________________________________________________________________________

  AAAAAAA     L          DDDDDDD     EEEEEEE     RRRRRRR    TTTTTTT    L          AAAAAAA     KKKKKK    EEEEEEE
 A       A    L          D       D   E           R     R       T       L         A       A    K     K   E
 AAAAAAAAA    L          D       D   EEEEE       RRRRRRR       T       L         AAAAAAAAA    KKKKKK    EEEE
 A       A    L          D       D   E           R   R         T       L         A       A    K   K     E
 A       A    LLLLLLL    DDDDDDD     EEEEEEE     R    R        T       LLLLLLL   A       A    K    K    EEEEEEE
 ______________________________________________________________________________________________________________
*/

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