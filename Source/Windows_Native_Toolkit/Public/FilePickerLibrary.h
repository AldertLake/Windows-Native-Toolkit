/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FilePickerLibrary.generated.h"

UENUM(BlueprintType)
enum class EFilePickerType : uint8
{
    File,
    Folder
};

UCLASS()
class UFilePickerLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "File Operations System")
    static bool OpenFileFolderPicker(
        EFilePickerType PickerType,
        bool bAllFilesTypeSupported,
        const FString& AllowedFileExtensions,
        FString& OutSelectedPath
    );

    UFUNCTION(BlueprintPure, Category = "System Info")
    static FString GetCurrentKeyboardLayout();

    UFUNCTION(BlueprintPure, Category = "System Info")
    static FString GetSystemLanguage();
};

