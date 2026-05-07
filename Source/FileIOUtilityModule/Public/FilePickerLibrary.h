// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FilePickerLibrary.generated.h"

/** Selects whether the shared picker helper opens files or folders. */
UENUM(BlueprintType)
enum class EFilePickerType : uint8
{
    File UMETA(DisplayName = "File"),
    Folder UMETA(DisplayName = "Folder")
};

/** One Windows file-dialog filter entry. */
USTRUCT(BlueprintType)
struct FFileDialogFilter
{
    GENERATED_BODY()

    /** Label shown in the Windows dialog, such as Images or Text Files. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "File Dialog")
    FString Name;

    /** Wildcard pattern such as *.png;*.jpg or *.txt. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "File Dialog")
    FString Pattern;
};

/** Native Windows file, folder, and locale helper nodes for Blueprints. */
UCLASS()
class FILEIOUTILITYMODULE_API UFilePickerLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Opens a native Windows file picker and returns one selected file. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Open File"))
    static bool OpenFile(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, FString& OutFilename);

    /** Opens a native Windows file picker and returns all selected files. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Open Files"))
    static bool OpenFiles(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, TArray<FString>& OutFilenames);

    /** Opens a native Windows folder picker and returns the selected folder path. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Open Folder"))
    static bool OpenFolder(const FString& DialogTitle, const FString& DefaultPath, FString& OutFolder);

    /** Builds a dialog filter string from friendly Blueprint structs. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Make File Filter"))
    static FString MakeFileFilter(const TArray<FFileDialogFilter>& Filters);

    /** Opens a native Save File dialog and returns the chosen path. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Save File"))
    static bool OpenSaveFileDialog(
        const FString& DialogTitle,
        const FString& DefaultPath,
        const FString& DefaultFileName,
        const FString& FileTypes,
        FString& OutFilename
    );

    /** Returns the current Windows keyboard layout identifier. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Windows Details", meta = (DisplayName = "Get Keyboard Layout"))
    static FString GetCurrentKeyboardLayout();

    /** Returns the current Windows system language as a locale name. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Windows Details", meta = (DisplayName = "Get System Language"))
    static FString GetSystemLanguage();

private:
    static bool OpenFileFolderPicker(
        const FString& DialogTitle,
        const FString& DefaultPath,
        const FString& FileTypes,
        bool bAllowMultiple,
        EFilePickerType PickerType,
        TArray<FString>& OutFilenames
    );
};

