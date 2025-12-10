// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileSystemBlueprintLibrary.generated.h"

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

USTRUCT(BlueprintType)
struct FPartitionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    FString DriveLetter; // e.g., "C:/"

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    FString VolumeLabel; // Work only if the target partion was renamed

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    FString FileSystem; // e.g., "NTFS", "FAT32"

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    EPartitionType DriveType = EPartitionType::Unknown;

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    bool bIsSystemPartition = false; // True if Windows is installed here

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    int64 TotalSizeBytes = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    int64 FreeSizeBytes = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    int64 UsedSizeBytes = 0;
};

USTRUCT(BlueprintType)
struct FfsFileInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "File Info")
    bool bExists = false;

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    bool bIsDirectory = false;

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    bool bIsReadOnly = false;

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    int64 FileSizeBytes = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    FDateTime CreationTime;

    UPROPERTY(BlueprintReadOnly, Category = "Disk Info")
    FDateTime AccessTime;
};

UCLASS()
class FILEIOUTILITYMODULE_API UFileSystemBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //Move files to a new path (folder).
    //Support the WNT File & folder picker.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Move File To Path"))
    static bool MoveFileToFolder(const FString& Source, const FString& Destination, bool bOverwrite, FString& OutError);

    //Moves a folder to another folder (path).
    //Support the WNT File & folder picker.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Move Folder To Path"))
    static bool MoveFolderToFolder(const FString& Source, const FString& Destination, bool bOverwrite, FString& OutError);

    //Delete a selected file (has limmitations)
    //Support the WNT File & folder picker.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Delete File"))
    static bool DeleteFileW(const FString& Path, FString& OutError);

    //Delete a whole folder & all it's content (has limmitations).
    //Support the WNT File & folder picker & may take time if file large.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Delete Folder"))
    static bool DeleteFolder(const FString& Path, FString& OutError);

    //Get a file or folder informations, offer a big variaty of informations.
    //Support the WNT File & folder picker & may take time if file large.
    //For retarded people, to convert to GB divide by 1073741824; to convert to mb divide by 1048576.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Get File/Folder Info"))
    static FfsFileInfo GetFileInfo(const FString& Path);

    //Show a list of all partitions avaialable in the system & give you huge variaty of information for each partition.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Get All Available Paritions"))
    static TArray<FPartitionInfo> GetAllAvailablePartitions();

    //Will rename your file - note that you should also enter the file extantion to not corupt it !
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Rename File"))
    static bool RenameFile(const FString& FilePath, const FString& NewFileName, bool bOverwrite, FString& OutError);
};