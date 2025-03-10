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

#include "FilePickerLibrary.h"
#include "DesktopPlatformModule.h"
#include "Misc/Paths.h"
#include "Framework\Application\SlateApplication.h"
#include <Windows.h> // Pour Windows API calls 

// La Fonction OpenFileFolderPicker je sais le nom est drole et tu peux y modifier si tu veux :D
bool UFilePickerLibrary::OpenFileFolderPicker(
    EFilePickerType PickerType,
    bool bAllFilesTypeSupported,
    const FString& AllowedFileExtensions,
    FString& OutSelectedPath)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        UE_LOG(LogTemp, Warning, TEXT("DesktopPlatform module not available."));
        return false;
    }

    const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
    FString ResultPath;

    if (PickerType == EFilePickerType::File)
    {
        TArray<FString> OutFiles;
        FString FileTypeFilter = bAllFilesTypeSupported ? TEXT("All Files (*.*)|*.*") : AllowedFileExtensions;

        bool bSuccess = DesktopPlatform->OpenFileDialog(
            ParentWindowHandle,
            TEXT("Select a File"),
            FPaths::ProjectDir(),
            TEXT(""),
            FileTypeFilter,
            EFileDialogFlags::None,
            OutFiles
        );

        if (bSuccess && OutFiles.Num() > 0)
        {
            ResultPath = OutFiles[0];
        }
        else
        {
            return false;
        }
    }
    else // Le picker des folder 
    {
        bool bSuccess = DesktopPlatform->OpenDirectoryDialog(
            ParentWindowHandle,
            TEXT("Select a Folder"),
            FPaths::ProjectDir(),
            ResultPath
        );

        if (!bSuccess)
        {
            return false;
        }
    }

    OutSelectedPath = ResultPath;
    return true;
}

// Ca cest pour le keyboard layout tu peux utuliser ca pour faire difference entre azerty et qwerty
FString UFilePickerLibrary::GetCurrentKeyboardLayout()
{
    // Buffer to store the keyboard layout name (KLID)
    TCHAR KeyboardLayoutName[KL_NAMELENGTH];
    if (GetKeyboardLayoutName(KeyboardLayoutName))
    {
        // Convert TCHAR to FString
        return FString(KeyboardLayoutName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to retrieve keyboard layout."));
        return FString(TEXT("Unknown"));
    }
}

// la langue du system
FString UFilePickerLibrary::GetSystemLanguage()
{
    // la langue default du system
    LANGID LangID = GetSystemDefaultLangID();
    if (LangID != 0)
    {
        // Buffer pour guarder le nom locale
        WCHAR LocaleName[LOCALE_NAME_MAX_LENGTH];
        if (LCIDToLocaleName(MAKELCID(LangID, SORT_DEFAULT), LocaleName, LOCALE_NAME_MAX_LENGTH, 0))
        {
            // Convert en FS String
            return FString(LocaleName);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Failed to retrieve system language."));
    return FString(TEXT("Unknown"));
}





