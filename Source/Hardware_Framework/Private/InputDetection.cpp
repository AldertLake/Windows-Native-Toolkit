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


#include "InputDetection.h"
#include "Framework/Application/SlateApplication.h"

void UInputDetectionLibrary::GetInputDeviceStatus(bool& GamepadConnected, bool& MouseConnected)
{
    GamepadConnected = false;
    MouseConnected = false;

    if (FSlateApplication::IsInitialized())
    {
        // Detecter le gamepad
        GamepadConnected = FSlateApplication::Get().IsGamepadAttached();

        // Detect la sourie(mouse)
        MouseConnected = FSlateApplication::Get().IsMouseAttached();
    }
}
