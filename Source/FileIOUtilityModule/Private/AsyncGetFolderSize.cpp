// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "AsyncGetFolderSize.h"
#include "Async/Async.h"
#include "HAL/PlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/ThreadSafeBool.h"
#include "Misc/Paths.h"
#include "UObject/WeakObjectPtr.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace
{
FString NormalizeFolderSizePath(const FString& Path)
{
    FString Fixed = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(Fixed);
    if (Fixed.Len() > 3 && Fixed.EndsWith(TEXT("/")))
    {
        Fixed.LeftChopInline(1);
    }
    return Fixed;
}

bool IsRootPath(const FString& Path)
{
    if (Path.Equals(TEXT("/")) || Path.Equals(TEXT("\\")))
    {
        return true;
    }

    if (Path.Len() <= 3 && Path.Len() >= 2 && Path[1] == TEXT(':'))
    {
        return true;
    }

    return FPaths::IsDrive(Path);
}

bool IsWindowsSystemPath(const FString& Path)
{
#if PLATFORM_WINDOWS
    TCHAR WindowsDirectory[MAX_PATH] = { 0 };
    if (GetWindowsDirectory(WindowsDirectory, MAX_PATH) <= 0)
    {
        return false;
    }

    FString SystemPath = NormalizeFolderSizePath(FString(WindowsDirectory));
    FString SystemPathWithSlash = SystemPath;
    if (!SystemPathWithSlash.EndsWith(TEXT("/")))
    {
        SystemPathWithSlash += TEXT("/");
    }

    FString TargetPath = NormalizeFolderSizePath(Path);
    FString TargetPathWithSlash = TargetPath;
    if (!TargetPathWithSlash.EndsWith(TEXT("/")))
    {
        TargetPathWithSlash += TEXT("/");
    }

    return TargetPathWithSlash.Equals(SystemPathWithSlash, ESearchCase::IgnoreCase)
        || TargetPathWithSlash.StartsWith(SystemPathWithSlash, ESearchCase::IgnoreCase);
#else
    return false;
#endif
}

bool ValidateFolderSizePath(const FString& FolderPath, FString& OutPath, FString& OutError)
{
    if (FolderPath.TrimStartAndEnd().IsEmpty())
    {
        OutError = TEXT("Folder path is empty.");
        return false;
    }

    OutPath = NormalizeFolderSizePath(FolderPath);

    FText Reason;
    if (!FPaths::ValidatePath(OutPath, &Reason))
    {
        OutError = FString::Printf(TEXT("Folder path is invalid: %s"), *Reason.ToString());
        return false;
    }

    if (IsRootPath(OutPath))
    {
        OutError = TEXT("Refusing to scan a drive root.");
        return false;
    }

    if (IsWindowsSystemPath(OutPath))
    {
        OutError = TEXT("Refusing to scan the Windows system directory.");
        return false;
    }

    if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*OutPath))
    {
        OutError = FString::Printf(TEXT("Folder does not exist: %s"), *OutPath);
        return false;
    }

    return true;
}

}

UAsyncGetFolderSize* UAsyncGetFolderSize::GetFolderSizeAsync(const FString& FolderPath)
{
    UAsyncGetFolderSize* Node = NewObject<UAsyncGetFolderSize>();
    Node->TargetFolderPath = FolderPath;
    Node->bCancelRequested = MakeShared<FThreadSafeBool, ESPMode::ThreadSafe>(false);
    Node->AddToRoot();
    Node->bAddedToRootForCompatibility = true;
    return Node;
}

UAsyncGetFolderSize* UAsyncGetFolderSize::GetFolderSizeAsyncWithWorldContext(const UObject* WorldContextObject, const FString& FolderPath)
{
    UAsyncGetFolderSize* Node = NewObject<UAsyncGetFolderSize>();
    Node->TargetFolderPath = FolderPath;
    Node->bCancelRequested = MakeShared<FThreadSafeBool, ESPMode::ThreadSafe>(false);
    if (WorldContextObject)
    {
        Node->RegisterWithGameInstance(WorldContextObject);
    }
    else
    {
        Node->AddToRoot();
        Node->bAddedToRootForCompatibility = true;
    }
    return Node;
}

void UAsyncGetFolderSize::Cancel()
{
    if (bCancelRequested.IsValid())
    {
        *bCancelRequested = true;
    }
}

void UAsyncGetFolderSize::Activate()
{
    if (!bCancelRequested.IsValid())
    {
        bCancelRequested = MakeShared<FThreadSafeBool, ESPMode::ThreadSafe>(false);
    }

    FString PathCopy;
    FString ErrorMessage;
    if (!ValidateFolderSizePath(TargetFolderPath, PathCopy, ErrorMessage))
    {
        OnError.Broadcast(ErrorMessage);
        OnFail.Broadcast(0);
        if (bAddedToRootForCompatibility)
        {
            RemoveFromRoot();
            bAddedToRootForCompatibility = false;
        }
        SetReadyToDestroy();
        return;
    }

    TWeakObjectPtr<UAsyncGetFolderSize> WeakThis(this);
    TSharedPtr<FThreadSafeBool, ESPMode::ThreadSafe> CancelFlag = bCancelRequested;

    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, PathCopy, CancelFlag]()
    {
        struct FFolderSizeVisitor : public IPlatformFile::FDirectoryVisitor
        {
            int64 TotalSize = 0;
            int32 VisitedFileCount = 0;
            TWeakObjectPtr<UAsyncGetFolderSize> WeakNode;
            TSharedPtr<FThreadSafeBool, ESPMode::ThreadSafe> CancelFlag;

            virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
            {
                if (CancelFlag.IsValid() && *CancelFlag)
                {
                    return false;
                }

                if (!bIsDirectory)
                {
                    const int64 FileSize = FPlatformFileManager::Get().GetPlatformFile().FileSize(FilenameOrDirectory);
                    if (FileSize >= 0)
                    {
                        TotalSize += FileSize;
                    }

                    ++VisitedFileCount;
                    if ((VisitedFileCount % 128) == 0)
                    {
                        const int64 CurrentSize = TotalSize;
                        TWeakObjectPtr<UAsyncGetFolderSize> ProgressNode = WeakNode;
                        AsyncTask(ENamedThreads::GameThread, [ProgressNode, CurrentSize]()
                        {
                            if (UAsyncGetFolderSize* Node = ProgressNode.Get())
                            {
                                Node->OnProgress.Broadcast(CurrentSize);
                            }
                        });
                    }
                }
                return true;
            }
        };

        FFolderSizeVisitor Visitor;
        Visitor.WeakNode = WeakThis;
        Visitor.CancelFlag = CancelFlag;

        const bool bCompleted = FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*PathCopy, Visitor);
        const bool bCanceled = CancelFlag.IsValid() && *CancelFlag;
        const int64 FinalSize = Visitor.TotalSize;

        AsyncTask(ENamedThreads::GameThread, [WeakThis, FinalSize, bCompleted, bCanceled]()
        {
            if (UAsyncGetFolderSize* Node = WeakThis.Get())
            {
                if (bCanceled)
                {
                    Node->OnCanceled.Broadcast();
                }
                else if (bCompleted)
                {
                    Node->OnProgress.Broadcast(FinalSize);
                    Node->OnSuccess.Broadcast(FinalSize);
                }
                else
                {
                    Node->OnError.Broadcast(TEXT("Failed to finish scanning the folder."));
                    Node->OnFail.Broadcast(0);
                }

                if (Node->bAddedToRootForCompatibility)
                {
                    Node->RemoveFromRoot();
                    Node->bAddedToRootForCompatibility = false;
                }
                Node->SetReadyToDestroy();
            }
        });
    });
}


