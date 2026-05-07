// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileSystemBlueprintLibrary.generated.h"

/** Windows drive and partition types reported by the platform file system. */
UENUM(BlueprintType)
enum class EPartitionType : uint8
{
    Unknown     UMETA(DisplayName = "Unknown"),
    NoRoot      UMETA(DisplayName = "No Root Directory"),
    Removable   UMETA(DisplayName = "Removable (USB/Flash)"),
    Fixed       UMETA(DisplayName = "Fixed (HDD/SSD)"),
    Network     UMETA(DisplayName = "Network Drive"),
    CDROM       UMETA(DisplayName = "CD-ROM / DVD"),
    RamDisk     UMETA(DisplayName = "RAM Disk")
};

/** Capacity and identity details for one available drive. */
USTRUCT(BlueprintType)
struct FPartitionInfo
{
    GENERATED_BODY()

    /** Drive root path, for example C:/ or D:/. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    FString DriveLetter;

    /** User-visible volume label reported by Windows. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    FString VolumeLabel;

    /** File system name such as NTFS, exFAT, or FAT32. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    FString FileSystem;

    /** Windows drive type. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    EPartitionType DriveType = EPartitionType::Unknown;

    /** True when this partition contains the active Windows installation. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    bool bIsSystemPartition = false;

    /** Total drive capacity in bytes. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    int64 TotalSizeBytes = 0;

    /** Free drive capacity in bytes. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    int64 FreeSizeBytes = 0;

    /** Used drive capacity in bytes. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    int64 UsedSizeBytes = 0;
};

/** Basic metadata for one file or folder path. */
USTRUCT(BlueprintType)
struct FfsFileInfo
{
    GENERATED_BODY()

    /** True when the file or folder exists. */
    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    bool bExists = false;

    /** True when the path points to a folder. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    bool bIsDirectory = false;

    /** True when the file attributes mark the path as read-only. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    bool bIsReadOnly = false;

    /** File size in bytes. Folder sizes are returned by the async folder-size node. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    int64 FileSizeBytes = 0;

    /** File or folder creation time. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    FDateTime CreationTime;

    /** Last access time reported by the platform file system. */
    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    FDateTime AccessTime;
};

/** Native Windows file-system helper nodes for Blueprints. */
UCLASS()
class FILEIOUTILITYMODULE_API UFileSystemBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Moves a file into a destination folder. Returns false and OutError when the path is unsafe or the move fails. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Move File To Path"))
    static bool MoveFileToFolder(const FString& Source, const FString& Destination, bool bOverwrite, FString& OutError);

    /** Moves a folder into a destination folder. Root and Windows system folders are rejected for safety. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Move Folder To Path"))
    static bool MoveFolderToFolder(const FString& Source, const FString& Destination, bool bOverwrite, FString& OutError);

    /** Permanently deletes a file after safety validation. Prefer Recycle File when user data is involved. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Delete File"))
    static bool DeleteFileW(const FString& Path, FString& OutError);

    /** Permanently deletes a folder after safety validation. Prefer Recycle Folder when user data is involved. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Delete Folder"))
    static bool DeleteFolder(const FString& Path, FString& OutError);

    /** Returns file or folder metadata. Use Get Folder Size Async for folder sizes to avoid blocking the game thread. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Get File/Folder Info"))
    static FfsFileInfo GetFileInfo(const FString& Path);

    /** Moves a file to the Windows Recycle Bin instead of deleting it permanently. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Recycle File"))
    static bool RecycleFile(const FString& FilePath, FString& OutError);

    /** Moves a folder to the Windows Recycle Bin instead of deleting it permanently. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Recycle Folder"))
    static bool RecycleFolder(const FString& FolderPath, FString& OutError);

    /** Opens Windows Explorer and selects the specified file or folder. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Show File In Explorer"))
    static void ShowFileInExplorer(const FString& FilePath);

    /** Returns available drives and basic capacity information. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Get Drives"))
    static TArray<FPartitionInfo> GetAllAvailablePartitions();

    /** Renames a file in place. Include the extension in NewFileName when the file should keep one. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Rename File"))
    static bool RenameFile(const FString& FilePath, const FString& NewFileName, bool bOverwrite, FString& OutError);
};

