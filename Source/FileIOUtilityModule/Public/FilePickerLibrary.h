// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

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
class FILEIOUTILITYMODULE_API UFilePickerLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //Open File Folder Picker 
    UFUNCTION(BlueprintCallable, Category = "File Operations System")
    static bool OpenFileFolderPicker(
        EFilePickerType PickerType,
        bool bAllFilesTypeSupported,
        const FString& AllowedFileExtensions,
        FString& OutSelectedPath
    );

    //Output Keyboard Layout Used By Use System (Check Documentation For More Info)
    UFUNCTION(BlueprintPure, Category = "System Info")
    static FString GetCurrentKeyboardLayout();

    //Ouput The system Language 
    UFUNCTION(BlueprintPure, Category = "System Info")
    static FString GetSystemLanguage();
};