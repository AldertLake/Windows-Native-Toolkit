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

    //This function will open a file picker on windows, you can custom every option & spect of the file picker.
    //Note that you may need to reffear to the docs to understand how to use the FileTypes specifiar (unsupported in folders).
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management")
    static bool OpenFileFolderPicker(
        const FString& DialogTitle,
        const FString& DefaultPath,
        const FString& FileTypes,
        bool bAllowMultiple,
        EFilePickerType PickerType,
        TArray<FString>& OutFilenames
    );

    //Output Keyboard Layout Used By Use System (Check Documentation For More Info)
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Windows Details")
    static FString GetCurrentKeyboardLayout();

    //Ouput The system Language 
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Windows Details")
    static FString GetSystemLanguage();
};