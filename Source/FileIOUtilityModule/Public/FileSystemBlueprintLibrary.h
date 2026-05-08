// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
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

/** Metadata returned for one file path. */
USTRUCT(BlueprintType)
struct FWNTFileInfo
{
    GENERATED_BODY()

    /** True when the file exists. */
    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    bool bExists = false;

    /** Absolute file path after normalization. */
    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    FString AbsolutePath;

    /** Clean file name including the extension. */
    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    FString FileName;

    /** File extension without the leading dot. */
    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    FString Extension;

    /** True when the file attributes mark the path as read-only. */
    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    bool bIsReadOnly = false;

    /** File size in bytes. */
    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    int64 FileSizeBytes = 0;

    /** File creation time. */
    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    FDateTime CreationTime;

    /** Last access time reported by the platform file system. */
    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    FDateTime AccessTime;

    /** Last modification time reported by the platform file system. */
    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    FDateTime ModificationTime;
};

/** Metadata returned for one folder path. */
USTRUCT(BlueprintType)
struct FWNTFolderInfo
{
    GENERATED_BODY()

    /** True when the folder exists. */
    UPROPERTY(BlueprintReadOnly, Category = "Folder Info")
    bool bExists = false;

    /** Absolute folder path after normalization. */
    UPROPERTY(BlueprintReadOnly, Category = "Folder Info")
    FString AbsolutePath;

    /** Clean folder name. */
    UPROPERTY(BlueprintReadOnly, Category = "Folder Info")
    FString FolderName;

    /** Parent folder path after normalization. */
    UPROPERTY(BlueprintReadOnly, Category = "Folder Info")
    FString ParentPath;

    /** True when the folder attributes mark the path as read-only. */
    UPROPERTY(BlueprintReadOnly, Category = "Folder Info")
    bool bIsReadOnly = false;

    /** Folder creation time. */
    UPROPERTY(BlueprintReadOnly, Category = "Folder Info")
    FDateTime CreationTime;

    /** Last access time reported by the platform file system. */
    UPROPERTY(BlueprintReadOnly, Category = "Folder Info")
    FDateTime AccessTime;

    /** Last modification time reported by the platform file system. */
    UPROPERTY(BlueprintReadOnly, Category = "Folder Info")
    FDateTime ModificationTime;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWNTFileOperationCompleted);

/** Async file-system operation node with direct success and fail execution pins. */
UCLASS()
class FILEIOUTILITYMODULE_API UAsyncFileSystemOperation : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    /** Called when the operation finishes successfully. */
    UPROPERTY(BlueprintAssignable)
    FWNTFileOperationCompleted OnSuccess;

    /** Called when the operation fails. The detailed reason is written to the Unreal log. */
    UPROPERTY(BlueprintAssignable)
    FWNTFileOperationCompleted OnFail;

    /** Moves a file into a destination folder without blocking the game thread. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Move File To Path"))
    static UAsyncFileSystemOperation* MoveFileToFolder(const UObject* WorldContextObject, const FString& Source, const FString& Destination, bool bOverwrite);

    /** Moves a folder into a destination folder without blocking the game thread. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Move Folder To Path"))
    static UAsyncFileSystemOperation* MoveFolderToFolder(const UObject* WorldContextObject, const FString& Source, const FString& Destination, bool bOverwrite);

    /** Copies a file into a destination folder without blocking the game thread. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Copy File To Path"))
    static UAsyncFileSystemOperation* CopyFileToFolder(const UObject* WorldContextObject, const FString& Source, const FString& Destination, bool bOverwrite);

    /** Copies a folder into a destination folder without blocking the game thread. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Copy Folder To Path"))
    static UAsyncFileSystemOperation* CopyFolderToFolder(const UObject* WorldContextObject, const FString& Source, const FString& Destination, bool bOverwrite);

    /** Permanently deletes a file after safety validation without blocking the game thread. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Delete File"))
    static UAsyncFileSystemOperation* DeleteFileW(const UObject* WorldContextObject, const FString& Path);

    /** Permanently deletes a folder after safety validation without blocking the game thread. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Delete Folder"))
    static UAsyncFileSystemOperation* DeleteFolder(const UObject* WorldContextObject, const FString& Path);

    /** Moves a file to the Windows Recycle Bin instead of deleting it permanently. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Recycle File"))
    static UAsyncFileSystemOperation* RecycleFile(const UObject* WorldContextObject, const FString& FilePath);

    /** Moves a folder to the Windows Recycle Bin instead of deleting it permanently. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Recycle Folder"))
    static UAsyncFileSystemOperation* RecycleFolder(const UObject* WorldContextObject, const FString& FolderPath);

    /** Renames a file in place without blocking the game thread. Include the extension in NewFileName when the file should keep one. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Rename File"))
    static UAsyncFileSystemOperation* RenameFile(const UObject* WorldContextObject, const FString& FilePath, const FString& NewFileName, bool bOverwrite);

    /** Renames a folder in place without blocking the game thread. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Rename Folder"))
    static UAsyncFileSystemOperation* RenameFolder(const UObject* WorldContextObject, const FString& FolderPath, const FString& NewFolderName, bool bOverwrite);

    virtual void Activate() override;

public:
    enum class EFileOperationKind : uint8
    {
        MoveFile,
        MoveFolder,
        CopyFile,
        CopyFolder,
        DeleteFile,
        DeleteFolder,
        RecycleFile,
        RecycleFolder,
        RenameFile,
        RenameFolder
    };

private:
    static UAsyncFileSystemOperation* CreateOperation(const UObject* WorldContextObject, EFileOperationKind InOperationKind);

    void Finalize(bool bSuccess);

    EFileOperationKind OperationKind = EFileOperationKind::MoveFile;
    FString SourcePath;
    FString DestinationPath;
    FString TargetPath;
    FString TargetName;
    bool bOverwriteExisting = false;
    bool bAddedToRootForCompatibility = false;
};

/** Native Windows file-system helper nodes for Blueprints. */
UCLASS()
class FILEIOUTILITYMODULE_API UFileSystemBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Returns metadata for one file path. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Get File Info"))
    static FWNTFileInfo GetFileInfo(const FString& FilePath);

    /** Returns metadata for one folder path. Folder size is intentionally handled by Get Folder Size to avoid blocking the game thread. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Get Folder Info"))
    static FWNTFolderInfo GetFolderInfo(const FString& FolderPath);

    /** Opens Windows Explorer and selects the specified file or folder. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Show File In Explorer"))
    static void ShowFileInExplorer(const FString& FilePath);

    /** Returns available drives and basic capacity information. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Get Drives"))
    static TArray<FPartitionInfo> GetAllAvailablePartitions();

};

