// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "FilePickerLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Widgets/SWindow.h"
#include <objbase.h>
#include <shobjidl.h>
#include <wrl/client.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if PLATFORM_WINDOWS
namespace
{
struct FScopedComInit
{
    HRESULT Result = E_FAIL;
    bool bNeedsUninitialize = false;

    FScopedComInit()
    {
        Result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        bNeedsUninitialize = SUCCEEDED(Result);
    }

    ~FScopedComInit()
    {
        if (bNeedsUninitialize)
        {
            CoUninitialize();
        }
    }

    bool IsUsable() const
    {
        return SUCCEEDED(Result) || Result == RPC_E_CHANGED_MODE;
    }
};

void* GetDialogParentWindow()
{
    if (FSlateApplication::IsInitialized())
    {
        return const_cast<void*>(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr));
    }

    if (GEngine && GEngine->GameViewport && GEngine->GameViewport->GetWindow().IsValid())
    {
        TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
        if (Window.IsValid() && Window->GetNativeWindow().IsValid())
        {
            return Window->GetNativeWindow()->GetOSWindowHandle();
        }
    }

    return nullptr;
}

void BuildFilterSpecs(const FString& FileTypes, TArray<FString>& OutNames, TArray<FString>& OutSpecs, TArray<COMDLG_FILTERSPEC>& OutComSpecs)
{
    TArray<FString> RawFilters;
    FileTypes.ParseIntoArray(RawFilters, TEXT("|"), true);

    for (int32 i = 0; i + 1 < RawFilters.Num(); i += 2)
    {
        FString Name = RawFilters[i].TrimStartAndEnd();
        FString Pattern = RawFilters[i + 1].TrimStartAndEnd();
        if (!Name.IsEmpty() && !Pattern.IsEmpty())
        {
            OutNames.Add(Name);
            OutSpecs.Add(Pattern);
        }
    }

    for (int32 i = 0; i < OutNames.Num(); ++i)
    {
        COMDLG_FILTERSPEC Spec;
        Spec.pszName = *OutNames[i];
        Spec.pszSpec = *OutSpecs[i];
        OutComSpecs.Add(Spec);
    }
}
}
#endif

bool UFilePickerLibrary::OpenFile(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, FString& OutFilename)
{
    TArray<FString> Files;
    const bool bResult = OpenFileFolderPicker(DialogTitle, DefaultPath, FileTypes, false, EFilePickerType::File, Files);
    OutFilename = Files.Num() > 0 ? Files[0] : FString();
    return bResult && !OutFilename.IsEmpty();
}

bool UFilePickerLibrary::OpenFiles(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, TArray<FString>& OutFilenames)
{
    return OpenFileFolderPicker(DialogTitle, DefaultPath, FileTypes, true, EFilePickerType::File, OutFilenames);
}

bool UFilePickerLibrary::OpenFolder(const FString& DialogTitle, const FString& DefaultPath, FString& OutFolder)
{
    TArray<FString> Folders;
    const bool bResult = OpenFileFolderPicker(DialogTitle, DefaultPath, FString(), false, EFilePickerType::Folder, Folders);
    OutFolder = Folders.Num() > 0 ? Folders[0] : FString();
    return bResult && !OutFolder.IsEmpty();
}

FString UFilePickerLibrary::MakeFileFilter(const TArray<FFileDialogFilter>& Filters)
{
    TArray<FString> Parts;
    for (const FFileDialogFilter& Filter : Filters)
    {
        const FString Name = Filter.Name.TrimStartAndEnd();
        const FString Pattern = Filter.Pattern.TrimStartAndEnd();
        if (!Name.IsEmpty() && !Pattern.IsEmpty())
        {
            Parts.Add(Name);
            Parts.Add(Pattern);
        }
    }
    return FString::Join(Parts, TEXT("|"));
}

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
    FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        return false;
    }

    void* ParentWindowHandle = GetDialogParentWindow();

    Microsoft::WRL::ComPtr<IFileOpenDialog> FileDialog;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&FileDialog));

    if (FAILED(hr))
    {
        return false;
    }

    DWORD dwFlags;
    if (SUCCEEDED(FileDialog->GetOptions(&dwFlags)))
    {
        dwFlags |= FOS_FORCEFILESYSTEM;

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

    FileDialog->SetTitle(*DialogTitle);

    if (!DefaultPath.IsEmpty())
    {
        Microsoft::WRL::ComPtr<IShellItem> DefaultItem;
        if (SUCCEEDED(SHCreateItemFromParsingName(*DefaultPath, nullptr, IID_PPV_ARGS(&DefaultItem))))
        {
            FileDialog->SetFolder(DefaultItem.Get());
        }
    }

    if (PickerType == EFilePickerType::File)
    {
        TArray<COMDLG_FILTERSPEC> FileTypesCOM;
        TArray<FString> TempNames;
        TArray<FString> TempSpecs;
        BuildFilterSpecs(FileTypes, TempNames, TempSpecs, FileTypesCOM);

        if (FileTypesCOM.Num() > 0)
        {
            FileDialog->SetFileTypes(FileTypesCOM.Num(), FileTypesCOM.GetData());
        }
    }

    hr = FileDialog->Show((HWND)ParentWindowHandle);

    bool bSuccess = false;

    if (SUCCEEDED(hr))
    {
        Microsoft::WRL::ComPtr<IShellItemArray> Results;
        if (SUCCEEDED(FileDialog->GetResults(&Results)))
        {
            DWORD Count = 0;
            Results->GetCount(&Count);

            for (DWORD i = 0; i < Count; i++)
            {
                Microsoft::WRL::ComPtr<IShellItem> Item;
                if (SUCCEEDED(Results->GetItemAt(i, &Item)))
                {
                    PWSTR FilePath = nullptr;
                    if (SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &FilePath)))
                    {
                        OutFilenames.Add(FString(FilePath));
                        CoTaskMemFree(FilePath);
                    }
                }
            }
            bSuccess = (OutFilenames.Num() > 0);
        }
    }

    return bSuccess;
#else
    return false;
#endif
}

bool UFilePickerLibrary::OpenSaveFileDialog(
    const FString& DialogTitle,
    const FString& DefaultPath,
    const FString& DefaultFileName,
    const FString& FileTypes,
    FString& OutFilename)
{
    OutFilename.Empty();

#if PLATFORM_WINDOWS
    FScopedComInit ComInit;
    if (!ComInit.IsUsable())
    {
        return false;
    }

    void* ParentWindowHandle = GetDialogParentWindow();

    Microsoft::WRL::ComPtr<IFileSaveDialog> FileDialog;
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&FileDialog));

    if (FAILED(hr)) return false;

    DWORD dwFlags;
    if (SUCCEEDED(FileDialog->GetOptions(&dwFlags)))
    {
        FileDialog->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT);
    }

    FileDialog->SetTitle(*DialogTitle);

    if (!DefaultPath.IsEmpty())
    {
        Microsoft::WRL::ComPtr<IShellItem> DefaultItem;
        if (SUCCEEDED(SHCreateItemFromParsingName(*DefaultPath, nullptr, IID_PPV_ARGS(&DefaultItem))))
        {
            FileDialog->SetFolder(DefaultItem.Get());
        }
    }

    if (!DefaultFileName.IsEmpty())
    {
        FileDialog->SetFileName(*DefaultFileName);
    }

    TArray<COMDLG_FILTERSPEC> FileTypesCOM;
    TArray<FString> TempNames;
    TArray<FString> TempSpecs;
    BuildFilterSpecs(FileTypes, TempNames, TempSpecs, FileTypesCOM);

    if (FileTypesCOM.Num() > 0)
    {
        FileDialog->SetFileTypes(FileTypesCOM.Num(), FileTypesCOM.GetData());
        FileDialog->SetFileTypeIndex(1);

        if (TempSpecs.Num() > 0)
        {
            FString Ext = TempSpecs[0].Replace(TEXT("*."), TEXT(""));
            if (!Ext.Contains(TEXT("*")))
            {
                FileDialog->SetDefaultExtension(*Ext);
            }
        }
    }

    hr = FileDialog->Show((HWND)ParentWindowHandle);

    if (SUCCEEDED(hr))
    {
        Microsoft::WRL::ComPtr<IShellItem> ResultItem;
        if (SUCCEEDED(FileDialog->GetResult(&ResultItem)))
        {
            PWSTR FilePath = nullptr;
            if (SUCCEEDED(ResultItem->GetDisplayName(SIGDN_FILESYSPATH, &FilePath)))
            {
                OutFilename = FString(FilePath);
                CoTaskMemFree(FilePath);
                return true;
            }
        }
    }

    return false;
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


