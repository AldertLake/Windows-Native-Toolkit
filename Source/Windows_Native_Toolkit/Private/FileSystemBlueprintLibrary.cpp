/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#include "FileSystemBlueprintLibrary.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

// these are allow and hide windows platform types
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h" 

bool UFileSystemBlueprintLibrary::MoveFileToFolder(
    const FString& SourceFilePath,
    const FString& DestinationFolderPath,
    bool& bSuccess,
    FString& OutErrorMessage)
{
    bSuccess = false;
    OutErrorMessage.Empty();

    if (!FPaths::FileExists(SourceFilePath))
    {
        OutErrorMessage = FString::Printf(TEXT("Source file does not exist: %s"), *SourceFilePath);
        return false;
    }

    if (!FPaths::DirectoryExists(DestinationFolderPath))
    {
        IFileManager::Get().MakeDirectory(*DestinationFolderPath, true);
    }

    FString FileName = FPaths::GetCleanFilename(SourceFilePath);
    FString DestinationPath = FPaths::Combine(DestinationFolderPath, FileName);

    bSuccess = IFileManager::Get().Move(*DestinationPath, *SourceFilePath, true, true);
    if (!bSuccess)
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to move file from %s to %s"), *SourceFilePath, *DestinationPath);
    }

    return bSuccess;
}

bool UFileSystemBlueprintLibrary::DeleteFileW(
    const FString& FilePath,
    bool& bSuccess,
    FString& OutErrorMessage)
{
    bSuccess = false;
    OutErrorMessage.Empty();

    if (FilePath.IsEmpty())
    {
        OutErrorMessage = TEXT("Invalid file path.");
        return false;
    }

    if (!FPaths::FileExists(FilePath))
    {
        OutErrorMessage = TEXT("File does not exist.");
        return false;
    }

    if (::DeleteFileW(*FilePath))
    {
        bSuccess = true;
        return true;
    }
    else
    {
        OutErrorMessage = TEXT("Failed to delete the file.");
        return false;
    }
}

bool UFileSystemBlueprintLibrary::GetFileInfo(
    const FString& FilePath,
    FFileInfo& OutFileInfo,
    bool& bSuccess,
    FString& OutErrorMessage)
{
    bSuccess = false;
    OutErrorMessage.Empty();

    if (!FPaths::FileExists(FilePath))
    {
        OutErrorMessage = FString::Printf(TEXT("File does not exist: %s"), *FilePath);
        return false;
    }

    FFileStatData FileStat = IFileManager::Get().GetStatData(*FilePath);
    if (FileStat.bIsValid)
    {
        OutFileInfo.FileSize = FileStat.FileSize;
        OutFileInfo.CreationTime = FileStat.CreationTime; // the get file info in generale is unstable sometimes it gives random errors lol.
        bSuccess = true;
    }
    else
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to get file info: %s"), *FilePath);
    }

    return bSuccess;
}

bool UFileSystemBlueprintLibrary::MoveFolderToFolder(
    const FString& SourceFolderPath,
    const FString& DestinationFolderPath,
    bool& bSuccess,
    FString& OutErrorMessage)
{
    bSuccess = false;
    OutErrorMessage.Empty();

    if (!FPaths::DirectoryExists(SourceFolderPath))
    {
        OutErrorMessage = FString::Printf(TEXT("Source folder does not exist: %s"), *SourceFolderPath);
        return false;
    }

    if (!FPaths::DirectoryExists(DestinationFolderPath))
    {
        IFileManager::Get().MakeDirectory(*DestinationFolderPath, true);
    }

    FString FolderName = FPaths::GetBaseFilename(SourceFolderPath);
    FString DestinationPath = FPaths::Combine(DestinationFolderPath, FolderName);

    bSuccess = IFileManager::Get().Move(*DestinationPath, *SourceFolderPath, true, true);
    if (!bSuccess)
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to move folder from %s to %s"), *SourceFolderPath, *DestinationPath);
    }

    return bSuccess;
}

bool UFileSystemBlueprintLibrary::DeleteFolder(
    const FString& FolderPath,
    bool& bSuccess,
    FString& OutErrorMessage)
{
    bSuccess = false;
    OutErrorMessage.Empty();

    if (!FPaths::DirectoryExists(FolderPath))
    {
        OutErrorMessage = FString::Printf(TEXT("Folder does not exist: %s"), *FolderPath);
        return false;
    }

    bSuccess = IFileManager::Get().DeleteDirectory(*FolderPath, false, true);
    if (!bSuccess)
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to delete folder: %s"), *FolderPath);
    }

    return bSuccess;
}

bool UFileSystemBlueprintLibrary::GetFolderInfo(
    const FString& FolderPath,
    FFileInfo& OutFolderInfo,
    bool& bSuccess,
    FString& OutErrorMessage)
{
    bSuccess = false;
    OutErrorMessage.Empty();

    if (!FPaths::DirectoryExists(FolderPath))
    {
        OutErrorMessage = FString::Printf(TEXT("Folder does not exist: %s"), *FolderPath);
        return false;
    }

    FFileStatData FolderStat = IFileManager::Get().GetStatData(*FolderPath);
    if (FolderStat.bIsValid)
    {
        TArray<FString> FileNames;
        IFileManager::Get().FindFilesRecursive(FileNames, *FolderPath, TEXT("*.*"), true, false);

        int64 TotalSize = 0;
        for (const FString& File : FileNames)
        {
            FFileStatData FileStat = IFileManager::Get().GetStatData(*File);
            if (FileStat.bIsValid)
            {
                TotalSize += FileStat.FileSize;
            }
        }

        OutFolderInfo.FileSize = TotalSize;
        OutFolderInfo.CreationTime = FolderStat.CreationTime;
        bSuccess = true;
    }
    else
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to get folder info: %s"), *FolderPath);
    }

    return bSuccess;
}