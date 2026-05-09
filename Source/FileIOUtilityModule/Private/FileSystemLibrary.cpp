// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "FileSystemLibrary.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/Paths.h"
#include "UObject/WeakObjectPtr.h"
#include <atomic>

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <fileapi.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <shlobj.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif


static IPlatformFile& GetPlatformFile()
{
    return FPlatformFileManager::Get().GetPlatformFile();
}

static FString NormalizeFileSystemPath(const FString& Path)
{
    FString Fixed = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(Fixed);
    if (Fixed.Len() > 3 && Fixed.EndsWith(TEXT("/")))
    {
        Fixed.LeftChopInline(1);
    }
    return Fixed;
}

static bool IsDriveRootPath(const FString& Path)
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

static bool IsWindowsProtectedPath(const FString& Path)
{
#if PLATFORM_WINDOWS
    TCHAR WindowsDirectory[MAX_PATH] = { 0 };
    if (GetWindowsDirectory(WindowsDirectory, MAX_PATH) <= 0)
    {
        return false;
    }

    FString SystemPath = NormalizeFileSystemPath(FString(WindowsDirectory));
    FString TargetPath = NormalizeFileSystemPath(Path);

    if (!SystemPath.EndsWith(TEXT("/")))
    {
        SystemPath += TEXT("/");
    }

    if (!TargetPath.EndsWith(TEXT("/")))
    {
        TargetPath += TEXT("/");
    }

    return TargetPath.Equals(SystemPath, ESearchCase::IgnoreCase) || TargetPath.StartsWith(SystemPath, ESearchCase::IgnoreCase);
#else
    return false;
#endif
}

static bool ValidatePathString(const FString& RawPath, FString& OutPath, FString& OutError)
{
    if (RawPath.TrimStartAndEnd().IsEmpty())
    {
        OutError = TEXT("Path is empty.");
        return false;
    }

    OutPath = NormalizeFileSystemPath(RawPath);
    FText Reason;
    if (!FPaths::ValidatePath(OutPath, &Reason))
    {
        OutError = FString::Printf(TEXT("Path is invalid: %s"), *Reason.ToString());
        return false;
    }

    return true;
}

static bool ValidateDestructivePath(const FString& RawPath, FString& OutPath, FString& OutError)
{
    if (!ValidatePathString(RawPath, OutPath, OutError))
    {
        return false;
    }

    if (IsDriveRootPath(OutPath))
    {
        OutError = TEXT("Refusing to modify a drive root.");
        return false;
    }

    if (IsWindowsProtectedPath(OutPath))
    {
        OutError = TEXT("Refusing to modify the Windows system directory.");
        return false;
    }

    return true;
}

static bool ValidateManagedDirectoryPath(const FString& RawPath, FString& OutPath, FString& OutError)
{
    if (!ValidatePathString(RawPath, OutPath, OutError))
    {
        return false;
    }

    if (IsDriveRootPath(OutPath))
    {
        OutError = TEXT("Refusing to use a drive root for this operation.");
        return false;
    }

    if (IsWindowsProtectedPath(OutPath))
    {
        OutError = TEXT("Refusing to use the Windows system directory for this operation.");
        return false;
    }

    return true;
}

static bool AreSamePath(const FString& A, const FString& B)
{
    return NormalizeFileSystemPath(A).Equals(NormalizeFileSystemPath(B), ESearchCase::IgnoreCase);
}

static bool IsValidCleanFilename(const FString& Name)
{
    return !Name.TrimStartAndEnd().IsEmpty()
        && Name == FPaths::GetCleanFilename(Name)
        && !Name.Contains(TEXT("/"))
        && !Name.Contains(TEXT("\\"))
        && FPaths::MakeValidFileName(Name).Equals(Name);
}

static void LogFileOperationError(const TCHAR* Context, const FString& Message)
{
    UE_LOG(LogTemp, Error, TEXT("Error: %s failed. %s"), Context, *Message);
}

static bool MoveFileToFolderInternal(const FString& Source, const FString& Destination, bool bOverwrite, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();

    FString CleanSource;
    FString CleanDestFolder;
    if (!ValidateDestructivePath(Source, CleanSource, OutError) || !ValidateManagedDirectoryPath(Destination, CleanDestFolder, OutError))
    {
        return false;
    }

    if (!PlatformFile.FileExists(*CleanSource))
    {
        OutError = FString::Printf(TEXT("Source file does not exist: %s"), *CleanSource);
        return false;
    }

    FString FileName = FPaths::GetCleanFilename(CleanSource);
    FString FullDestPath = FPaths::Combine(CleanDestFolder, FileName);
    FPaths::NormalizeFilename(FullDestPath);

    if (AreSamePath(CleanSource, FullDestPath))
    {
        OutError = TEXT("Source and destination are the same file.");
        return false;
    }

    if (PlatformFile.DirectoryExists(*FullDestPath))
    {
        OutError = TEXT("Destination is a directory, not a file.");
        return false;
    }

    if (PlatformFile.FileExists(*FullDestPath))
    {
        if (bOverwrite)
        {

            if (!PlatformFile.DeleteFile(*FullDestPath))
            {
                PlatformFile.SetReadOnly(*FullDestPath, false);
                if (!PlatformFile.DeleteFile(*FullDestPath))
                {
                    OutError = TEXT("Failed to overwrite existing file (Locked or Permission Denied).");
                    return false;
                }
            }
        }
        else
        {
            OutError = TEXT("Destination file already exists.");
            return false;
        }
    }


    if (!PlatformFile.DirectoryExists(*CleanDestFolder))
    {
        if (!PlatformFile.CreateDirectoryTree(*CleanDestFolder))
        {
            OutError = TEXT("Failed to create destination directory.");
            return false;
        }
    }

    if (!PlatformFile.MoveFile(*FullDestPath, *CleanSource))
    {
        if (PlatformFile.CopyFile(*FullDestPath, *CleanSource))
        {
            PlatformFile.DeleteFile(*CleanSource);
            return true;
        }

        OutError = FString::Printf(TEXT("OS Error moving file to: %s"), *FullDestPath);
        return false;
    }

    return true;
}

static bool MoveFolderToFolderInternal(const FString& Source, const FString& Destination, bool bOverwrite, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();

    FString CleanSource;
    FString CleanDestParent;
    if (!ValidateDestructivePath(Source, CleanSource, OutError) || !ValidateManagedDirectoryPath(Destination, CleanDestParent, OutError))
    {
        return false;
    }

    if (!PlatformFile.DirectoryExists(*CleanSource))
    {
        OutError = TEXT("Source directory does not exist.");
        return false;
    }


    FString FolderName = FPaths::GetCleanFilename(CleanSource);
    FString FullDestPath = FPaths::Combine(CleanDestParent, FolderName);
    FPaths::NormalizeFilename(FullDestPath);

    if (AreSamePath(CleanSource, FullDestPath))
    {
        OutError = TEXT("Source and destination are the same folder.");
        return false;
    }

    FString SourceCheck = CleanSource;
    if (!SourceCheck.EndsWith(TEXT("/"))) SourceCheck += TEXT("/");
    FString DestCheck = FullDestPath;
    if (!DestCheck.EndsWith(TEXT("/"))) DestCheck += TEXT("/");

    if (DestCheck.StartsWith(SourceCheck))
    {
        OutError = TEXT("Cannot move a folder into itself.");
        return false;
    }


    if (PlatformFile.DirectoryExists(*FullDestPath))
    {
        if (!bOverwrite)
        {
            OutError = FString::Printf(TEXT("Target folder already exists: %s"), *FullDestPath);
            return false;
        }
    }

    if (!PlatformFile.DirectoryExists(*CleanDestParent))
    {
        if (!PlatformFile.CreateDirectoryTree(*CleanDestParent))
        {
            OutError = TEXT("Failed to create destination directory.");
            return false;
        }
    }

    if (PlatformFile.MoveFile(*FullDestPath, *CleanSource))
    {
        return true;
    }

    if (PlatformFile.CopyDirectoryTree(*FullDestPath, *CleanSource, true))
    {
        if (PlatformFile.DeleteDirectoryRecursively(*CleanSource))
        {
            return true;
        }
        OutError = TEXT("Moved data successfully, but failed to delete source folder (Permissions?).");
        return true;
    }

    OutError = TEXT("Failed to move folder. (Check permissions or open files).");
    return false;
}

static bool CopyFileToFolderInternal(const FString& Source, const FString& Destination, bool bOverwrite, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();

    FString CleanSource;
    FString CleanDestFolder;
    if (!ValidateDestructivePath(Source, CleanSource, OutError) || !ValidateManagedDirectoryPath(Destination, CleanDestFolder, OutError))
    {
        return false;
    }

    if (!PlatformFile.FileExists(*CleanSource))
    {
        OutError = FString::Printf(TEXT("Source file does not exist: %s"), *CleanSource);
        return false;
    }

    FString FileName = FPaths::GetCleanFilename(CleanSource);
    FString FullDestPath = FPaths::Combine(CleanDestFolder, FileName);
    FPaths::NormalizeFilename(FullDestPath);

    if (AreSamePath(CleanSource, FullDestPath))
    {
        OutError = TEXT("Source and destination are the same file.");
        return false;
    }

    if (!PlatformFile.DirectoryExists(*CleanDestFolder) && !PlatformFile.CreateDirectoryTree(*CleanDestFolder))
    {
        OutError = TEXT("Failed to create destination directory.");
        return false;
    }

    if (PlatformFile.FileExists(*FullDestPath))
    {
        if (!bOverwrite)
        {
            OutError = TEXT("Destination file already exists.");
            return false;
        }

        PlatformFile.SetReadOnly(*FullDestPath, false);
        if (!PlatformFile.DeleteFile(*FullDestPath))
        {
            OutError = TEXT("Failed to overwrite the destination file.");
            return false;
        }
    }

    if (!PlatformFile.CopyFile(*FullDestPath, *CleanSource))
    {
        OutError = FString::Printf(TEXT("Failed to copy file to: %s"), *FullDestPath);
        return false;
    }

    return true;
}

static bool DeleteFileInternal(const FString& Path, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();
    FString Target;
    if (!ValidateDestructivePath(Path, Target, OutError))
    {
        return false;
    }

    if (!PlatformFile.FileExists(*Target))
    {
        OutError = TEXT("File does not exist.");
        return false;
    }


    if (PlatformFile.DeleteFile(*Target)) return true;

    PlatformFile.SetReadOnly(*Target, false);

    if (PlatformFile.DeleteFile(*Target)) return true;

    OutError = TEXT("Failed to delete file (Access Denied).");
    return false;
}

static bool CopyFolderToFolderInternal(const FString& Source, const FString& Destination, bool bOverwrite, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();

    FString CleanSource;
    FString CleanDestParent;
    if (!ValidateDestructivePath(Source, CleanSource, OutError) || !ValidateManagedDirectoryPath(Destination, CleanDestParent, OutError))
    {
        return false;
    }

    if (!PlatformFile.DirectoryExists(*CleanSource))
    {
        OutError = TEXT("Source directory does not exist.");
        return false;
    }

    const FString FolderName = FPaths::GetCleanFilename(CleanSource);
    FString FullDestPath = FPaths::Combine(CleanDestParent, FolderName);
    FPaths::NormalizeFilename(FullDestPath);

    FString SourceCheck = CleanSource;
    if (!SourceCheck.EndsWith(TEXT("/")))
    {
        SourceCheck += TEXT("/");
    }

    FString DestCheck = FullDestPath;
    if (!DestCheck.EndsWith(TEXT("/")))
    {
        DestCheck += TEXT("/");
    }

    if (DestCheck.StartsWith(SourceCheck))
    {
        OutError = TEXT("Cannot copy a folder into itself.");
        return false;
    }

    if (PlatformFile.DirectoryExists(*FullDestPath))
    {
        if (!bOverwrite)
        {
            OutError = FString::Printf(TEXT("Target folder already exists: %s"), *FullDestPath);
            return false;
        }

        if (!PlatformFile.DeleteDirectoryRecursively(*FullDestPath))
        {
            OutError = TEXT("Failed to overwrite the destination folder.");
            return false;
        }
    }

    if (!PlatformFile.DirectoryExists(*CleanDestParent) && !PlatformFile.CreateDirectoryTree(*CleanDestParent))
    {
        OutError = TEXT("Failed to create destination directory.");
        return false;
    }

    if (!PlatformFile.CopyDirectoryTree(*FullDestPath, *CleanSource, true))
    {
        OutError = TEXT("Failed to copy the folder.");
        return false;
    }

    return true;
}

static bool DeleteFolderInternal(const FString& Path, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();
    FString Target;
    if (!ValidateDestructivePath(Path, Target, OutError))
    {
        return false;
    }

    if (!PlatformFile.DirectoryExists(*Target))
    {
        OutError = TEXT("Directory does not exist.");
        return false;
    }

    if (PlatformFile.DeleteDirectoryRecursively(*Target))
    {
        return true;
    }

    OutError = TEXT("Failed to delete directory (Files might be in use).");
    return false;
}

static bool RenameFileInternal(const FString& FilePath, const FString& NewFileName, bool bOverwrite, FString& OutError);
static bool RenameFolderInternal(const FString& FolderPath, const FString& NewFolderName, bool bOverwrite, FString& OutError);
static bool RecycleFileInternal(const FString& FilePath, FString& OutError);
static bool RecycleFolderInternal(const FString& FolderPath, FString& OutError);

static const TCHAR* GetFileOperationContextName(UAsyncFileSystemOperation::EFileOperationKind OperationKind)
{
    switch (OperationKind)
    {
    case UAsyncFileSystemOperation::EFileOperationKind::MoveFile:
        return TEXT("Move File To Path");
    case UAsyncFileSystemOperation::EFileOperationKind::MoveFolder:
        return TEXT("Move Folder To Path");
    case UAsyncFileSystemOperation::EFileOperationKind::CopyFile:
        return TEXT("Copy File To Path");
    case UAsyncFileSystemOperation::EFileOperationKind::CopyFolder:
        return TEXT("Copy Folder To Path");
    case UAsyncFileSystemOperation::EFileOperationKind::DeleteFile:
        return TEXT("Delete File");
    case UAsyncFileSystemOperation::EFileOperationKind::DeleteFolder:
        return TEXT("Delete Folder");
    case UAsyncFileSystemOperation::EFileOperationKind::RecycleFile:
        return TEXT("Recycle File");
    case UAsyncFileSystemOperation::EFileOperationKind::RecycleFolder:
        return TEXT("Recycle Folder");
    case UAsyncFileSystemOperation::EFileOperationKind::RenameFile:
        return TEXT("Rename File");
    case UAsyncFileSystemOperation::EFileOperationKind::RenameFolder:
        return TEXT("Rename Folder");
    default:
        return TEXT("File Operation");
    }
}

UAsyncFileSystemOperation* UAsyncFileSystemOperation::CreateOperation(const UObject* WorldContextObject, EFileOperationKind InOperationKind)
{
    UAsyncFileSystemOperation* Node = NewObject<UAsyncFileSystemOperation>();
    Node->OperationKind = InOperationKind;
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

UAsyncFileSystemOperation* UAsyncFileSystemOperation::MoveFileToFolder(const UObject* WorldContextObject, const FString& Source, const FString& Destination, bool bOverwrite)
{
    UAsyncFileSystemOperation* Node = CreateOperation(WorldContextObject, EFileOperationKind::MoveFile);
    Node->SourcePath = Source;
    Node->DestinationPath = Destination;
    Node->bOverwriteExisting = bOverwrite;
    return Node;
}

UAsyncFileSystemOperation* UAsyncFileSystemOperation::MoveFolderToFolder(const UObject* WorldContextObject, const FString& Source, const FString& Destination, bool bOverwrite)
{
    UAsyncFileSystemOperation* Node = CreateOperation(WorldContextObject, EFileOperationKind::MoveFolder);
    Node->SourcePath = Source;
    Node->DestinationPath = Destination;
    Node->bOverwriteExisting = bOverwrite;
    return Node;
}

UAsyncFileSystemOperation* UAsyncFileSystemOperation::CopyFileToFolder(const UObject* WorldContextObject, const FString& Source, const FString& Destination, bool bOverwrite)
{
    UAsyncFileSystemOperation* Node = CreateOperation(WorldContextObject, EFileOperationKind::CopyFile);
    Node->SourcePath = Source;
    Node->DestinationPath = Destination;
    Node->bOverwriteExisting = bOverwrite;
    return Node;
}

UAsyncFileSystemOperation* UAsyncFileSystemOperation::CopyFolderToFolder(const UObject* WorldContextObject, const FString& Source, const FString& Destination, bool bOverwrite)
{
    UAsyncFileSystemOperation* Node = CreateOperation(WorldContextObject, EFileOperationKind::CopyFolder);
    Node->SourcePath = Source;
    Node->DestinationPath = Destination;
    Node->bOverwriteExisting = bOverwrite;
    return Node;
}

UAsyncFileSystemOperation* UAsyncFileSystemOperation::DeleteFileW(const UObject* WorldContextObject, const FString& Path)
{
    UAsyncFileSystemOperation* Node = CreateOperation(WorldContextObject, EFileOperationKind::DeleteFile);
    Node->TargetPath = Path;
    return Node;
}

UAsyncFileSystemOperation* UAsyncFileSystemOperation::DeleteFolder(const UObject* WorldContextObject, const FString& Path)
{
    UAsyncFileSystemOperation* Node = CreateOperation(WorldContextObject, EFileOperationKind::DeleteFolder);
    Node->TargetPath = Path;
    return Node;
}

UAsyncFileSystemOperation* UAsyncFileSystemOperation::RecycleFile(const UObject* WorldContextObject, const FString& FilePath)
{
    UAsyncFileSystemOperation* Node = CreateOperation(WorldContextObject, EFileOperationKind::RecycleFile);
    Node->TargetPath = FilePath;
    return Node;
}

UAsyncFileSystemOperation* UAsyncFileSystemOperation::RecycleFolder(const UObject* WorldContextObject, const FString& FolderPath)
{
    UAsyncFileSystemOperation* Node = CreateOperation(WorldContextObject, EFileOperationKind::RecycleFolder);
    Node->TargetPath = FolderPath;
    return Node;
}

UAsyncFileSystemOperation* UAsyncFileSystemOperation::RenameFile(const UObject* WorldContextObject, const FString& FilePath, const FString& NewFileName, bool bOverwrite)
{
    UAsyncFileSystemOperation* Node = CreateOperation(WorldContextObject, EFileOperationKind::RenameFile);
    Node->TargetPath = FilePath;
    Node->TargetName = NewFileName;
    Node->bOverwriteExisting = bOverwrite;
    return Node;
}

UAsyncFileSystemOperation* UAsyncFileSystemOperation::RenameFolder(const UObject* WorldContextObject, const FString& FolderPath, const FString& NewFolderName, bool bOverwrite)
{
    UAsyncFileSystemOperation* Node = CreateOperation(WorldContextObject, EFileOperationKind::RenameFolder);
    Node->TargetPath = FolderPath;
    Node->TargetName = NewFolderName;
    Node->bOverwriteExisting = bOverwrite;
    return Node;
}

void UAsyncFileSystemOperation::Activate()
{
    TWeakObjectPtr<UAsyncFileSystemOperation> WeakThis(this);
    const EFileOperationKind OperationKindCopy = OperationKind;
    const FString SourceCopy = SourcePath;
    const FString DestinationCopy = DestinationPath;
    const FString PathCopy = TargetPath;
    const FString NameCopy = TargetName;
    const bool bOverwriteCopy = bOverwriteExisting;

    Async(EAsyncExecution::Thread, [WeakThis, OperationKindCopy, SourceCopy, DestinationCopy, PathCopy, NameCopy, bOverwriteCopy]()
    {
        bool bSuccess = false;
        FString ErrorMessage;

        switch (OperationKindCopy)
        {
        case EFileOperationKind::MoveFile:
            bSuccess = MoveFileToFolderInternal(SourceCopy, DestinationCopy, bOverwriteCopy, ErrorMessage);
            break;
        case EFileOperationKind::MoveFolder:
            bSuccess = MoveFolderToFolderInternal(SourceCopy, DestinationCopy, bOverwriteCopy, ErrorMessage);
            break;
        case EFileOperationKind::CopyFile:
            bSuccess = CopyFileToFolderInternal(SourceCopy, DestinationCopy, bOverwriteCopy, ErrorMessage);
            break;
        case EFileOperationKind::CopyFolder:
            bSuccess = CopyFolderToFolderInternal(SourceCopy, DestinationCopy, bOverwriteCopy, ErrorMessage);
            break;
        case EFileOperationKind::DeleteFile:
            bSuccess = DeleteFileInternal(PathCopy, ErrorMessage);
            break;
        case EFileOperationKind::DeleteFolder:
            bSuccess = DeleteFolderInternal(PathCopy, ErrorMessage);
            break;
        case EFileOperationKind::RecycleFile:
            bSuccess = RecycleFileInternal(PathCopy, ErrorMessage);
            break;
        case EFileOperationKind::RecycleFolder:
            bSuccess = RecycleFolderInternal(PathCopy, ErrorMessage);
            break;
        case EFileOperationKind::RenameFile:
            bSuccess = RenameFileInternal(PathCopy, NameCopy, bOverwriteCopy, ErrorMessage);
            break;
        case EFileOperationKind::RenameFolder:
            bSuccess = RenameFolderInternal(PathCopy, NameCopy, bOverwriteCopy, ErrorMessage);
            break;
        default:
            ErrorMessage = TEXT("Unknown file operation.");
            break;
        }

        if (!bSuccess)
        {
            LogFileOperationError(GetFileOperationContextName(OperationKindCopy), ErrorMessage);
        }

        AsyncTask(ENamedThreads::GameThread, [WeakThis, bSuccess]()
        {
            if (UAsyncFileSystemOperation* Node = WeakThis.Get())
            {
                Node->Finalize(bSuccess);
            }
        });
    });
}

void UAsyncFileSystemOperation::Finalize(bool bSuccess)
{
    if (bSuccess)
    {
        OnSuccess.Broadcast();
    }
    else
    {
        OnFail.Broadcast();
    }

    if (bAddedToRootForCompatibility)
    {
        RemoveFromRoot();
        bAddedToRootForCompatibility = false;
    }

    SetReadyToDestroy();
}

FWNTFileInfo UFileSystemLibrary::GetFileInfo(const FString& FilePath)
{
    IPlatformFile& PlatformFile = GetPlatformFile();
    FString Target = NormalizeFileSystemPath(FilePath);
    FWNTFileInfo Info;
    Info.AbsolutePath = Target;
    Info.FileName = FPaths::GetCleanFilename(Target);
    Info.Extension = FPaths::GetExtension(Target, false);

    FFileStatData StatData = PlatformFile.GetStatData(*Target);

    if (StatData.bIsValid && !StatData.bIsDirectory)
    {
        Info.bExists = true;
        Info.bIsReadOnly = StatData.bIsReadOnly;
        Info.FileSizeBytes = StatData.FileSize;
        Info.CreationTime = StatData.CreationTime;
        Info.AccessTime = StatData.AccessTime;
        Info.ModificationTime = StatData.ModificationTime;
    }

    return Info;
}

FWNTFolderInfo UFileSystemLibrary::GetFolderInfo(const FString& FolderPath)
{
    IPlatformFile& PlatformFile = GetPlatformFile();
    FString Target = NormalizeFileSystemPath(FolderPath);
    FWNTFolderInfo Info;
    Info.AbsolutePath = Target;
    Info.FolderName = FPaths::GetCleanFilename(Target);
    Info.ParentPath = FPaths::GetPath(Target);

    FFileStatData StatData = PlatformFile.GetStatData(*Target);
    if (StatData.bIsValid && StatData.bIsDirectory)
    {
        Info.bExists = true;
        Info.bIsReadOnly = StatData.bIsReadOnly;
        Info.CreationTime = StatData.CreationTime;
        Info.AccessTime = StatData.AccessTime;
        Info.ModificationTime = StatData.ModificationTime;
    }

    return Info;
}

TArray<FPartitionInfo> UFileSystemLibrary::GetAllAvailablePartitions()
{
    TArray<FPartitionInfo> Partitions;

#if PLATFORM_WINDOWS
    TCHAR SysDir[MAX_PATH];
    FString SystemDriveLetter;
    if (GetWindowsDirectory(SysDir, MAX_PATH) > 0)
    {

        SystemDriveLetter = FString(SysDir).Left(3);
    }

    const int32 BufferSize = 512;
    TCHAR Buffer[BufferSize];
    DWORD Result = GetLogicalDriveStrings(BufferSize, Buffer);

    if (Result > 0 && Result <= BufferSize)
    {
        TCHAR* CurrentDrive = Buffer;
        while (*CurrentDrive)
        {
            FPartitionInfo Info;
            Info.DriveLetter = FString(CurrentDrive);

            UINT Type = GetDriveType(CurrentDrive);
            switch (Type)
            {
            case DRIVE_REMOVABLE: Info.DriveType = EPartitionType::Removable; break;
            case DRIVE_FIXED:     Info.DriveType = EPartitionType::Fixed;     break;
            case DRIVE_REMOTE:    Info.DriveType = EPartitionType::Network;   break;
            case DRIVE_CDROM:     Info.DriveType = EPartitionType::CDROM;     break;
            case DRIVE_RAMDISK:   Info.DriveType = EPartitionType::RamDisk;   break;
            case DRIVE_NO_ROOT_DIR: Info.DriveType = EPartitionType::NoRoot;  break;
            default:              Info.DriveType = EPartitionType::Unknown;   break;
            }

            if (!SystemDriveLetter.IsEmpty() && Info.DriveLetter.StartsWith(SystemDriveLetter.Left(1)))
            {
                Info.bIsSystemPartition = true;
            }

            if (Type != DRIVE_NO_ROOT_DIR)
            {
                ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;

                if (GetDiskFreeSpaceEx(CurrentDrive, &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
                {
                    Info.TotalSizeBytes = (int64)TotalNumberOfBytes.QuadPart;
                    Info.FreeSizeBytes = (int64)TotalNumberOfFreeBytes.QuadPart;
                    Info.UsedSizeBytes = Info.TotalSizeBytes - Info.FreeSizeBytes;
                }

                TCHAR VolumeName[MAX_PATH + 1] = { 0 };
                TCHAR FileSystemName[MAX_PATH + 1] = { 0 };
                DWORD SerialNumber, MaxComponentLen, FileSystemFlags;

                UINT OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

                if (GetVolumeInformation(
                    CurrentDrive,
                    VolumeName, ARRAYSIZE(VolumeName),
                    &SerialNumber,
                    &MaxComponentLen,
                    &FileSystemFlags,
                    FileSystemName, ARRAYSIZE(FileSystemName)))
                {
                    Info.VolumeLabel = FString(VolumeName);
                    Info.FileSystem = FString(FileSystemName);
                }
                else
                {
                    Info.VolumeLabel = TEXT("Removable Disk");
                    Info.FileSystem = TEXT("Unknown");
                }

                SetErrorMode(OldMode);
            }

            Partitions.Add(Info);

            CurrentDrive += FCString::Strlen(CurrentDrive) + 1;
        }
    }
#endif

    return Partitions;
}

static bool RenameFileInternal(const FString& FilePath, const FString& NewFileName, bool bOverwrite, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();

    FString CleanSource;
    if (!ValidateDestructivePath(FilePath, CleanSource, OutError))
    {
        return false;
    }

    if (!PlatformFile.FileExists(*CleanSource))
    {
        OutError = FString::Printf(TEXT("Source file does not exist: %s"), *CleanSource);
        return false;
    }

    FString ParentDir = FPaths::GetPath(CleanSource);
    FString CleanNewName = FPaths::GetCleanFilename(NewFileName);
    if (!IsValidCleanFilename(CleanNewName))
    {
        OutError = TEXT("New file name is invalid.");
        return false;
    }

    FString FullDestPath = FPaths::Combine(ParentDir, CleanNewName);

    FPaths::NormalizeFilename(FullDestPath);
    if (AreSamePath(CleanSource, FullDestPath))
    {
        OutError = TEXT("Source and target file names are the same.");
        return false;
    }

    if (PlatformFile.FileExists(*FullDestPath))
    {
        if (bOverwrite)
        {
            if (!PlatformFile.DeleteFile(*FullDestPath))
            {
                PlatformFile.SetReadOnly(*FullDestPath, false);
                if (!PlatformFile.DeleteFile(*FullDestPath))
                {
                    OutError = TEXT("Target filename exists and cannot be overwritten (Locked/ReadOnly).");
                    return false;
                }
            }
        }
        else
        {
            OutError = FString::Printf(TEXT("A file with the name '%s' already exists in this directory."), *CleanNewName);
            return false;
        }
    }

    if (!PlatformFile.MoveFile(*FullDestPath, *CleanSource))
    {
        if (PlatformFile.CopyFile(*FullDestPath, *CleanSource))
        {
            PlatformFile.DeleteFile(*CleanSource);
            return true;
        }

        OutError = TEXT("Failed to rename file (Unknown OS Error).");
        return false;
    }

    return true;
}

static bool RenameFolderInternal(const FString& FolderPath, const FString& NewFolderName, bool bOverwrite, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();

    FString CleanSource;
    if (!ValidateDestructivePath(FolderPath, CleanSource, OutError))
    {
        return false;
    }

    if (!PlatformFile.DirectoryExists(*CleanSource))
    {
        OutError = FString::Printf(TEXT("Source folder does not exist: %s"), *CleanSource);
        return false;
    }

    const FString ParentDir = FPaths::GetPath(CleanSource);
    const FString CleanNewName = FPaths::GetCleanFilename(NewFolderName);
    if (!IsValidCleanFilename(CleanNewName))
    {
        OutError = TEXT("New folder name is invalid.");
        return false;
    }

    FString FullDestPath = FPaths::Combine(ParentDir, CleanNewName);
    FPaths::NormalizeFilename(FullDestPath);

    if (AreSamePath(CleanSource, FullDestPath))
    {
        OutError = TEXT("Source and target folder names are the same.");
        return false;
    }

    if (PlatformFile.FileExists(*FullDestPath))
    {
        OutError = TEXT("A file already exists with the requested folder name.");
        return false;
    }

    if (PlatformFile.DirectoryExists(*FullDestPath))
    {
        if (!bOverwrite)
        {
            OutError = FString::Printf(TEXT("A folder with the name '%s' already exists in this directory."), *CleanNewName);
            return false;
        }

        if (!PlatformFile.DeleteDirectoryRecursively(*FullDestPath))
        {
            OutError = TEXT("Target folder exists and cannot be overwritten.");
            return false;
        }
    }

    if (PlatformFile.MoveFile(*FullDestPath, *CleanSource))
    {
        return true;
    }

    if (PlatformFile.CopyDirectoryTree(*FullDestPath, *CleanSource, true))
    {
        if (PlatformFile.DeleteDirectoryRecursively(*CleanSource))
        {
            return true;
        }

        OutError = TEXT("Renamed folder contents but failed to remove the old folder.");
        return true;
    }

    OutError = TEXT("Failed to rename folder.");
    return false;
}

static bool RecycleFileInternal(const FString& FilePath, FString& OutError)
{
    FString Target;
    if (!ValidateDestructivePath(FilePath, Target, OutError))
    {
        return false;
    }
    if (!GetPlatformFile().FileExists(*Target))
    {
        OutError = TEXT("File does not exist.");
        return false;
    }

#if PLATFORM_WINDOWS
    FString CleanPath = Target;
    CleanPath.ReplaceInline(TEXT("/"), TEXT("\\"));

    CleanPath += TEXT('\0');

    SHFILEOPSTRUCTW FileOp = { 0 };
    FileOp.wFunc = FO_DELETE;
    FileOp.pFrom = *CleanPath;
    FileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int Result = SHFileOperationW(&FileOp);
    if (Result == 0) return true;

    OutError = FString::Printf(TEXT("Failed to recycle file. Windows Error Code: %d"), Result);
    return false;
#else
    OutError = TEXT("Not supported on this platform.");
    return false;
#endif
}

static bool RecycleFolderInternal(const FString& FolderPath, FString& OutError)
{
    FString Target;
    if (!ValidateDestructivePath(FolderPath, Target, OutError))
    {
        return false;
    }
    if (!GetPlatformFile().DirectoryExists(*Target))
    {
        OutError = TEXT("Folder does not exist.");
        return false;
    }

#if PLATFORM_WINDOWS
    FString CleanPath = Target;
    CleanPath.ReplaceInline(TEXT("/"), TEXT("\\"));

    CleanPath += TEXT('\0');

    SHFILEOPSTRUCTW FileOp = { 0 };
    FileOp.wFunc = FO_DELETE;
    FileOp.pFrom = *CleanPath;
    FileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int Result = SHFileOperationW(&FileOp);
    if (Result == 0) return true;

    OutError = FString::Printf(TEXT("Failed to recycle folder. Windows Error Code: %d"), Result);
    return false;
#else
    OutError = TEXT("Not supported on this platform.");
    return false;
#endif
}

void UFileSystemLibrary::ShowFileInExplorer(const FString& FilePath)
{
#if PLATFORM_WINDOWS
    FString CleanPath = NormalizeFileSystemPath(FilePath);
    CleanPath.ReplaceInline(TEXT("/"), TEXT("\\"));

    FString Args = FString::Printf(TEXT("/select,\"%s\""), *CleanPath);
    FPlatformProcess::CreateProc(TEXT("explorer.exe"), *Args, true, false, false, nullptr, 0, nullptr, nullptr);
#endif
}

UAsyncGetFolderSize* UAsyncGetFolderSize::GetFolderSize(const UObject* WorldContextObject, const FString& FolderPath)
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
    if (!ValidateManagedDirectoryPath(TargetFolderPath, PathCopy, ErrorMessage))
    {
        UE_LOG(LogTemp, Error, TEXT("Error: Get Folder Size failed. %s"), *ErrorMessage);
        OnFail.Broadcast(0);
        if (bAddedToRootForCompatibility)
        {
            RemoveFromRoot();
            bAddedToRootForCompatibility = false;
        }
        SetReadyToDestroy();
        return;
    }

    if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*PathCopy))
    {
        UE_LOG(LogTemp, Error, TEXT("Error: Get Folder Size failed. Folder does not exist: %s"), *PathCopy);
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
                    UE_LOG(LogTemp, Error, TEXT("Error: Get Folder Size failed. The operation was canceled."));
                    Node->OnFail.Broadcast(0);
                }
                else if (bCompleted)
                {
                    Node->OnProgress.Broadcast(FinalSize);
                    Node->OnSuccess.Broadcast(FinalSize);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Error: Get Folder Size failed to finish scanning the folder."));
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


