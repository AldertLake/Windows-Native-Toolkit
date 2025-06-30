// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileSystemBlueprintLibrary.generated.h"

USTRUCT(BlueprintType)
struct FFileInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "File Operations System")
    int64 FileSize;

    UPROPERTY(BlueprintReadOnly, Category = "File Operations System")
    FDateTime CreationTime;

    FFileInfo()
        : FileSize(0)
        , CreationTime(0)
    {
    }
};

UCLASS()
class UFileSystemBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "File Operations System")
    static bool MoveFileToFolder(
        const FString& SourceFilePath,
        const FString& DestinationFolderPath,
        bool& bSuccess,
        FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, Category = "File Operations System")
    static bool DeleteFileW(
        const FString& FilePath,
        bool& bSuccess,
        FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, Category = "File Operations System")
    static bool GetFileInfo(
        const FString& FilePath,
        FFileInfo& OutFileInfo,
        bool& bSuccess,
        FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, Category = "File Operations System")
    static bool MoveFolderToFolder(
        const FString& SourceFolderPath,
        const FString& DestinationFolderPath,
        bool& bSuccess,
        FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, Category = "File Operations System")
    static bool DeleteFolder(
        const FString& FolderPath,
        bool& bSuccess,
        FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, Category = "File Operations System")
    static bool GetFolderInfo(
        const FString& FolderPath,
        FFileInfo& OutFolderInfo,
        bool& bSuccess,
        FString& OutErrorMessage
    );
};