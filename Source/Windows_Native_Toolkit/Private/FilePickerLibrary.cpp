/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#include "FilePickerLibrary.h"
#include "DesktopPlatformModule.h"
#include "Misc/Paths.h"
#include "Framework\Application\SlateApplication.h"
#include <Windows.h> 


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
    else 
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


FString UFilePickerLibrary::GetCurrentKeyboardLayout()
{

    TCHAR KeyboardLayoutName[KL_NAMELENGTH];
    if (GetKeyboardLayoutName(KeyboardLayoutName))
    {
     
        return FString(KeyboardLayoutName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to retrieve keyboard layout."));
        return FString(TEXT("Unknown"));
    }
}


FString UFilePickerLibrary::GetSystemLanguage()
{
 
    LANGID LangID = GetSystemDefaultLangID();
    if (LangID != 0)
    {
       
        WCHAR LocaleName[LOCALE_NAME_MAX_LENGTH];
        if (LCIDToLocaleName(MAKELCID(LangID, SORT_DEFAULT), LocaleName, LOCALE_NAME_MAX_LENGTH, 0))
        {
           
            return FString(LocaleName);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Failed to retrieve system language."));
    return FString(TEXT("Unknown"));
}





