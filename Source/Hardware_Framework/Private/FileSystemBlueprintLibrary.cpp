/*

 * Copyright (C) 2025 AldertLake. All rights reserved.
 *
 * This file is part of the Windows Native ToolKit, an Unreal Engine Plugin.
 *
 * Unauthorized copying, distribution, or modification of this file is strictly prohibited.
 *
 * Anyone who bought this project has the full right to modify it like he want.
 *
 *
 * Author: AldertLake
 * Date: 2025/1/9
 ______________________________________________________________________________________________________________

  AAAAAAA     L          DDDDDDD     EEEEEEE     RRRRRRR    TTTTTTT    L          AAAAAAA     KKKKKK    EEEEEEE
 A       A    L          D       D   E           R     R       T       L         A       A    K     K   E
 AAAAAAAAA    L          D       D   EEEEE       RRRRRRR       T       L         AAAAAAAAA    KKKKKK    EEEE
 A       A    L          D       D   E           R   R         T       L         A       A    K   K     E
 A       A    LLLLLLL    DDDDDDD     EEEEEEE     R    R        T       LLLLLLL   A       A    K    K    EEEEEEE
 ______________________________________________________________________________________________________________
*/


#include "FileSystemBlueprintLibrary.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Cest pour Windows 7
#endif

// les operation sur fichier se trouve ici 
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

bool UFileSystemBlueprintLibrary::DeleteFile(
    const FString& FilePath,
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

    bSuccess = IFileManager::Get().Delete(*FilePath, false, true);
    if (!bSuccess)
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to delete file: %s"), *FilePath);
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

    if (!FPaths::FileExists(FilePath))
    {
        OutErrorMessage = FString::Printf(TEXT("File does not exist: %s"), *FilePath);
        return false;
    }

    FFileStatData FileStat = IFileManager::Get().GetStatData(*FilePath);
    if (FileStat.bIsValid)
    {
        OutFileInfo.FileSize = FileStat.FileSize;
        OutFileInfo.CreationTime = FileStat.CreationTime;
        bSuccess = true;
    }
    else
    {
        OutErrorMessage = FString::Printf(TEXT("Failed to get file info: %s"), *FilePath);
    }

    return bSuccess;
}

// les operation sur les dossier se trouve ici
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
        // pour la taille dun dossier faut calculer la taille de tout les elemenets cest du pain in the ass
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






