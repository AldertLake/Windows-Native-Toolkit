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


FString UFilePickerLibrary::MakeFileFilter(const TArray<FFileDialogFilter>& Filters)
{
    FString Result;
    Result.Reserve(Filters.Num() * 32);

    for (const FFileDialogFilter& Filter : Filters)
    {
        const FString Name = Filter.Name.TrimStartAndEnd();
        const FString Pattern = Filter.Pattern.TrimStartAndEnd();
        
        if (!Name.IsEmpty() && !Pattern.IsEmpty())
        {
            if (!Result.IsEmpty())
            {
                Result.AppendChar('|');
            }
            Result.Append(Name);
            Result.AppendChar('|');
            Result.Append(Pattern);
        }
    }
    
    return Result;
}

bool UFilePickerLibrary::OpenPathPicker(
    const FString& DialogTitle,
    const FString& DefaultPath,
    const FString& FileTypes,
    bool bAllowMultiple,
    EFilePickerType PickerType,
    TArray<FString>& OutPaths)
{
    OutPaths.Empty();

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

    if (PickerType == EFilePickerType::File && !FileTypes.IsEmpty())
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
            
            OutPaths.Reserve(Count);

            for (DWORD i = 0; i < Count; i++)
            {
                Microsoft::WRL::ComPtr<IShellItem> Item;
                if (SUCCEEDED(Results->GetItemAt(i, &Item)))
                {
                    PWSTR FilePath = nullptr;
                    if (SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &FilePath)))
                    {
                        FString NormalizedPath(FilePath);
                        FPaths::NormalizeFilename(NormalizedPath);
                        OutPaths.Add(NormalizedPath);
                        CoTaskMemFree(FilePath);
                    }
                }
            }
            bSuccess = (OutPaths.Num() > 0);
        }
    }

    return bSuccess;
#else
    return false;
#endif
}

bool UFilePickerLibrary::ShowSaveFilePicker(
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
                FPaths::NormalizeFilename(OutFilename);
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
