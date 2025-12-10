// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "FileSystemBlueprintLibrary.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <fileapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif


static IPlatformFile& GetPlatformFile()
{
    return FPlatformFileManager::Get().GetPlatformFile();
}

static FString FixPath(const FString& Path)
{
    FString Fixed = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(Fixed);
    if (Fixed.Len() > 3 && Fixed.EndsWith(TEXT("/")))
    {
        Fixed.LeftChopInline(1);
    }
    return Fixed;
}

bool UFileSystemBlueprintLibrary::MoveFileToFolder(const FString& Source, const FString& Destination, bool bOverwrite, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();

    FString CleanSource = FixPath(Source);
    FString CleanDestFolder = FixPath(Destination);

    if (!PlatformFile.FileExists(*CleanSource))
    {
        OutError = FString::Printf(TEXT("Source file does not exist: %s"), *CleanSource);
        return false;
    }

    FString FileName = FPaths::GetCleanFilename(CleanSource);
    FString FullDestPath = FPaths::Combine(CleanDestFolder, FileName);
    FPaths::NormalizeFilename(FullDestPath);


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

bool UFileSystemBlueprintLibrary::MoveFolderToFolder(const FString& Source, const FString& Destination, bool bOverwrite, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();

    FString CleanSource = FixPath(Source);
    FString CleanDestParent = FixPath(Destination); 

    if (!PlatformFile.DirectoryExists(*CleanSource))
    {
        OutError = TEXT("Source directory does not exist.");
        return false;
    }


    FString FolderName = FPaths::GetCleanFilename(CleanSource);
    FString FullDestPath = FPaths::Combine(CleanDestParent, FolderName);
    FPaths::NormalizeFilename(FullDestPath);


    if (FullDestPath.StartsWith(CleanSource))
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
        PlatformFile.CreateDirectoryTree(*CleanDestParent);
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

bool UFileSystemBlueprintLibrary::DeleteFileW(const FString& Path, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();
    FString Target = FixPath(Path);

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

bool UFileSystemBlueprintLibrary::DeleteFolder(const FString& Path, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();
    FString Target = FixPath(Path);

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

FfsFileInfo UFileSystemBlueprintLibrary::GetFileInfo(const FString& Path)
{
    IPlatformFile& PlatformFile = GetPlatformFile();
    FString Target = FixPath(Path);
    FfsFileInfo Info;

    FFileStatData StatData = PlatformFile.GetStatData(*Target);

    if (StatData.bIsValid)
    {
        Info.bExists = true;
        Info.bIsDirectory = StatData.bIsDirectory;
        Info.bIsReadOnly = StatData.bIsReadOnly;
        Info.FileSizeBytes = StatData.FileSize;
        Info.CreationTime = StatData.CreationTime;
        Info.AccessTime = StatData.AccessTime;

        if (Info.bIsDirectory)
        {
            struct FFolderSizeVisitor : public IPlatformFile::FDirectoryVisitor
            {
                int64 Size = 0;
                virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
                {
                    if (!bIsDirectory)
                    {
                        Size += IPlatformFile::GetPlatformPhysical().FileSize(FilenameOrDirectory);
                    }
                    return true;
                }
            };

            FFolderSizeVisitor Visitor;
            PlatformFile.IterateDirectoryRecursively(*Target, Visitor);
            Info.FileSizeBytes = Visitor.Size;
        }
    }

    return Info;
}

TArray<FPartitionInfo> UFileSystemBlueprintLibrary::GetAllAvailablePartitions()
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

                SetErrorMode(OldMode); // Restore error mode
            }

            Partitions.Add(Info);

            CurrentDrive += FCString::Strlen(CurrentDrive) + 1;
        }
    }
#endif

    return Partitions;
}

bool UFileSystemBlueprintLibrary::RenameFile(const FString& FilePath, const FString& NewFileName, bool bOverwrite, FString& OutError)
{
    IPlatformFile& PlatformFile = GetPlatformFile();

    FString CleanSource = FixPath(FilePath);

    if (!PlatformFile.FileExists(*CleanSource))
    {
        OutError = FString::Printf(TEXT("Source file does not exist: %s"), *CleanSource);
        return false;
    }

    FString ParentDir = FPaths::GetPath(CleanSource);
    FString CleanNewName = FPaths::GetCleanFilename(NewFileName); 
    FString FullDestPath = FPaths::Combine(ParentDir, CleanNewName);

    FPaths::NormalizeFilename(FullDestPath);

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