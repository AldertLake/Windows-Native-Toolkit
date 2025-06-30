// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "FileSystemBlueprintLibrary.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

// Helper to validate and normalize paths
static bool ValidatePath(const FString& Path, bool bIsDirectory, FString& OutNormalizedPath, FString& OutErrorMessage)
{
    if (Path.IsEmpty())
    {
        OutErrorMessage = TEXT("Path is empty.");
        return false;
    }

    OutNormalizedPath = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(OutNormalizedPath);

    if (bIsDirectory && !FPaths::DirectoryExists(OutNormalizedPath))
    {
        OutErrorMessage = FString::Printf(TEXT("Directory does not exist: %s"), *OutNormalizedPath);
        return false;
    }
    else if (!bIsDirectory && !FPaths::FileExists(OutNormalizedPath))
    {
        OutErrorMessage = FString::Printf(TEXT("File does not exist: %s"), *OutNormalizedPath);
        return false;
    }

    return true;
}

bool UFileSystemBlueprintLibrary::MoveFileToFolder(
    const FString& SourceFilePath,
    const FString& DestinationFolderPath,
    bool& bSuccess,
    FString& OutErrorMessage)
{
    bSuccess = false;
    OutErrorMessage.Empty();

    FString NormalizedSource, NormalizedDestFolder;
    if (!ValidatePath(SourceFilePath, false, NormalizedSource, OutErrorMessage) ||
        !ValidatePath(DestinationFolderPath, true, NormalizedDestFolder, OutErrorMessage))
    {
        return false;
    }

    // Create destination folder if it doesn't exist
    if (!FPaths::DirectoryExists(NormalizedDestFolder))
    {
        if (!IFileManager::Get().MakeDirectory(*NormalizedDestFolder, true))
        {
            OutErrorMessage = FString::Printf(TEXT("Failed to create directory: %s"), *NormalizedDestFolder);
            return false;
        }
    }

    FString FileName = FPaths::GetCleanFilename(NormalizedSource);
    FString DestinationPath = FPaths::Combine(NormalizedDestFolder, FileName);

    bSuccess = IFileManager::Get().Move(*DestinationPath, *NormalizedSource, true, true);
    if (!bSuccess)
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to move file from %s to %s"), *NormalizedSource, *DestinationPath);
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

    FString NormalizedPath;
    if (!ValidatePath(FilePath, false, NormalizedPath, OutErrorMessage))
    {
        return false;
    }

    bSuccess = IFileManager::Get().Delete(*NormalizedPath, false, true, true);
    if (!bSuccess)
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to delete file: %s"), *NormalizedPath);
    }

    return bSuccess;
}

bool UFileSystemBlueprintLibrary::GetFileInfo(
    const FString& FilePath,
    FFileInfo& OutFileInfo,
    bool& bSuccess,
    FString& OutErrorMessage)
{
    bSuccess = false;
    OutErrorMessage.Empty();
    OutFileInfo = FFileInfo();

    FString NormalizedPath;
    if (!ValidatePath(FilePath, false, NormalizedPath, OutErrorMessage))
    {
        return false;
    }

    FFileStatData FileStat = IFileManager::Get().GetStatData(*NormalizedPath);
    if (FileStat.bIsValid)
    {
        OutFileInfo.FileSize = FileStat.FileSize;
        OutFileInfo.CreationTime = FileStat.CreationTime;
        bSuccess = true;
    }
    else
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to get file info: %s"), *NormalizedPath);
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

    FString NormalizedSource, NormalizedDestFolder;
    if (!ValidatePath(SourceFolderPath, true, NormalizedSource, OutErrorMessage) ||
        !ValidatePath(DestinationFolderPath, true, NormalizedDestFolder, OutErrorMessage))
    {
        return false;
    }

    // Create destination folder if it doesn't exist
    if (!FPaths::DirectoryExists(NormalizedDestFolder))
    {
        if (!IFileManager::Get().MakeDirectory(*NormalizedDestFolder, true))
        {
            OutErrorMessage = FString::Printf(TEXT("Failed to create directory: %s"), *NormalizedDestFolder);
            return false;
        }
    }

    FString FolderName = FPaths::GetBaseFilename(NormalizedSource);
    FString DestinationPath = FPaths::Combine(NormalizedDestFolder, FolderName);

    bSuccess = IFileManager::Get().Move(*DestinationPath, *NormalizedSource, true, true);
    if (!bSuccess)
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to move folder from %s to %s"), *NormalizedSource, *DestinationPath);
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

    FString NormalizedPath;
    if (!ValidatePath(FolderPath, true, NormalizedPath, OutErrorMessage))
    {
        return false;
    }

    bSuccess = IFileManager::Get().DeleteDirectory(*NormalizedPath, false, true);
    if (!bSuccess)
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to delete folder: %s"), *NormalizedPath);
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
    OutFolderInfo = FFileInfo();

    FString NormalizedPath;
    if (!ValidatePath(FolderPath, true, NormalizedPath, OutErrorMessage))
    {
        return false;
    }

    FFileStatData FolderStat = IFileManager::Get().GetStatData(*NormalizedPath);
    if (FolderStat.bIsValid)
    {
        TArray<FString> FileNames;
        IFileManager::Get().FindFilesRecursive(FileNames, *NormalizedPath, TEXT("*.*"), true, false);

        int64 TotalSize = 0;
        for (const FString& File : FileNames)
        {
            FString FullFilePath = FPaths::Combine(NormalizedPath, File);
            FFileStatData FileStat = IFileManager::Get().GetStatData(*FullFilePath);
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
        OutErrorMessage = FString::Printf(TEXT("Failed to get folder info: %s"), *NormalizedPath);
    }

    return bSuccess;
}