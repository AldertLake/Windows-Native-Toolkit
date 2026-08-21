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

    /** Opens a native Windows picker for files or folders and always returns an array of selected paths. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Open Path Picker"))
    static bool OpenPathPicker(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, bool bAllowMultiple, EFilePickerType PickerType, TArray<FString>& OutPaths);

    /** Builds a dialog filter string from friendly Blueprint structs. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Make File Filter"))
    static FString MakeFileFilter(const TArray<FFileDialogFilter>& Filters);

    /** Opens a native Save File dialog and returns the chosen path. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management", meta = (DisplayName = "Open Save File Picker"))
    static bool ShowSaveFilePicker(
        const FString& DialogTitle,
        const FString& DefaultPath,
        const FString& DefaultFileName,
        const FString& FileTypes,
        FString& OutFilename
    );

private:
};

