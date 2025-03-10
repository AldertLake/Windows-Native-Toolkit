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


#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileSystemBlueprintLibrary.generated.h"

USTRUCT(BlueprintType)
struct FFileInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "File System")
    int64 FileSize;

    UPROPERTY(BlueprintReadOnly, Category = "File System")
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
    // operation sur fichier (les node bien sure)
    UFUNCTION(BlueprintCallable, Category = "File System")
    static bool MoveFileToFolder(
        const FString& SourceFilePath,
        const FString& DestinationFolderPath,
        bool& bSuccess,
        FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, Category = "File System")
    static bool DeleteFile(
        const FString& FilePath,
        bool& bSuccess,
        FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, Category = "File System")
    static bool GetFileInfo(
        const FString& FilePath,
        FFileInfo& OutFileInfo,
        bool& bSuccess,
        FString& OutErrorMessage
    );

    // operation sur dossier (les node ici )
    UFUNCTION(BlueprintCallable, Category = "File System")
    static bool MoveFolderToFolder(
        const FString& SourceFolderPath,
        const FString& DestinationFolderPath,
        bool& bSuccess,
        FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, Category = "File System")
    static bool DeleteFolder(
        const FString& FolderPath,
        bool& bSuccess,
        FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, Category = "File System")
    static bool GetFolderInfo(
        const FString& FolderPath,
        FFileInfo& OutFolderInfo,
        bool& bSuccess,
        FString& OutErrorMessage
    );
};























