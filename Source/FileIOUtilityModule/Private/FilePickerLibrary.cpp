// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "FilePickerLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Widgets/SWindow.h"
#include <shobjidl.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

bool UFilePickerLibrary::OpenFileFolderPicker(
    const FString& DialogTitle,
    const FString& DefaultPath,
    const FString& FileTypes,
    bool bAllowMultiple,
    EFilePickerType PickerType,
    TArray<FString>& OutFilenames)
{
    OutFilenames.Empty();

#if PLATFORM_WINDOWS

    void* ParentWindowHandle = nullptr;
    if (GEngine && GEngine->GameViewport)
    {
        ParentWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
    }

    IFileOpenDialog* FileDialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&FileDialog));

    if (FAILED(hr))
    {
        return false;
    }

    DWORD dwFlags;
    if (SUCCEEDED(FileDialog->GetOptions(&dwFlags)))
    {
        dwFlags |= FOS_FORCEFILESYSTEM; 

        // --- NEW: Folder Picking Logic ---
        if (PickerType == EFilePickerType::Folder)
        {
            dwFlags |= FOS_PICKFOLDERS;
        }

        if (bAllowMultiple)
        {
            dwFlags |= FOS_ALLOWMULTISELECT;
        }

        FileDialog->SetOptions(dwFlags);
    }

    // 4. Set Title
    FileDialog->SetTitle(*DialogTitle);

    // 5. Set Default Folder
    if (!DefaultPath.IsEmpty())
    {
        IShellItem* DefaultItem = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(*DefaultPath, nullptr, IID_PPV_ARGS(&DefaultItem))))
        {
            FileDialog->SetFolder(DefaultItem);
            DefaultItem->Release();
        }
    }

    if (PickerType == EFilePickerType::File)
    {
        TArray<FString> RawFilters;
        FileTypes.ParseIntoArray(RawFilters, TEXT("|"), true);

        TArray<COMDLG_FILTERSPEC> FileTypesCOM;
        TArray<FString> TempNames;
        TArray<FString> TempSpecs;

        for (int32 i = 0; i < RawFilters.Num(); i += 2)
        {
            if (RawFilters.IsValidIndex(i + 1))
            {
                COMDLG_FILTERSPEC Spec;
                TempNames.Add(RawFilters[i]);
                TempSpecs.Add(RawFilters[i + 1]);

                Spec.pszName = *TempNames.Last();
                Spec.pszSpec = *TempSpecs.Last();

                FileTypesCOM.Add(Spec);
            }
        }

        if (FileTypesCOM.Num() > 0)
        {
            FileDialog->SetFileTypes(FileTypesCOM.Num(), FileTypesCOM.GetData());
        }
    }

    hr = FileDialog->Show((HWND)ParentWindowHandle);

    bool bSuccess = false;

    if (SUCCEEDED(hr))
    {
        IShellItemArray* Results = nullptr;
        if (SUCCEEDED(FileDialog->GetResults(&Results)))
        {
            DWORD Count = 0;
            Results->GetCount(&Count);

            for (DWORD i = 0; i < Count; i++)
            {
                IShellItem* Item = nullptr;
                if (SUCCEEDED(Results->GetItemAt(i, &Item)))
                {
                    PWSTR FilePath = nullptr;
                    if (SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &FilePath)))
                    {
                        OutFilenames.Add(FString(FilePath));
                        CoTaskMemFree(FilePath);
                    }
                    Item->Release();
                }
            }
            Results->Release();
            bSuccess = (OutFilenames.Num() > 0);
        }
    }

    FileDialog->Release();
    return bSuccess;
#else
    return false;
#endif
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