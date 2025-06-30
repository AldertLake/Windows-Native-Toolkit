// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "FilePickerLibrary.h"
#include "DesktopPlatformModule.h"
#include "Misc/Paths.h"
#include "Framework/Application/SlateApplication.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

bool UFilePickerLibrary::OpenFileFolderPicker(
    EFilePickerType PickerType,
    bool bAllFilesTypeSupported,
    const FString& AllowedFileExtensions,
    FString& OutSelectedPath)
{
    OutSelectedPath.Empty();

    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        return false;
    }

    const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
    FString ResultPath;

    if (PickerType == EFilePickerType::File)
    {
        FString FileTypeFilter = bAllFilesTypeSupported ? TEXT("All Files (*.*)|*.*") : AllowedFileExtensions;
        TArray<FString> OutFiles;

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
#if PLATFORM_WINDOWS
    TCHAR KeyboardLayoutName[KL_NAMELENGTH] = { 0 };
    if (GetKeyboardLayoutName(KeyboardLayoutName) && KeyboardLayoutName[0])
    {
        return FString(KeyboardLayoutName);
    }
    return FString(TEXT("Unknown"));
#else
    return FString(TEXT("Unknown"));
#endif
}

FString UFilePickerLibrary::GetSystemLanguage()
{
#if PLATFORM_WINDOWS
    LANGID LangID = GetSystemDefaultLangID();
    if (LangID != 0)
    {
        WCHAR LocaleName[LOCALE_NAME_MAX_LENGTH] = { 0 };
        if (LCIDToLocaleName(MAKELCID(LangID, SORT_DEFAULT), LocaleName, LOCALE_NAME_MAX_LENGTH, 0) && LocaleName[0])
        {
            return FString(LocaleName);
        }
    }
    return FString(TEXT("Unknown"));
#else
    return FString(TEXT("Unknown"));
#endif
}