#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FilePickerLibrary.generated.h"

/**
 * Enum to specify the type of file picker dialog (file or folder).
 */
UENUM(BlueprintType)
enum class EFilePickerType : uint8
{
    File,
    Folder
};

/**
 * Blueprint function library for file/folder picker dialogs and system info on Windows.
 * Provides functions to open file/folder pickers and query keyboard layout and system language.
 * Note: Some functions are Windows-only; unsupported platforms return default values.
 */
UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UFilePickerLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Opens a file or folder picker dialog.
     * @param PickerType Type of picker (File or Folder).
     * @param bAllFilesTypeSupported If true, allows all file types; otherwise, uses AllowedFileExtensions.
     * @param AllowedFileExtensions File extension filter (e.g., "Text Files (*.txt)|*.txt").
     * @param OutSelectedPath The selected file or folder path.
     * @return True if a path was selected, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "File Operations System")
    static bool OpenFileFolderPicker(
        EFilePickerType PickerType,
        bool bAllFilesTypeSupported,
        const FString& AllowedFileExtensions,
        FString& OutSelectedPath
    );

    /**
     * Gets the current keyboard layout name.
     * @return Keyboard layout name or "Unknown" if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "System Info")
    static FString GetCurrentKeyboardLayout();

    /**
     * Gets the system language (locale name).
     * @return Locale name or "Unknown" if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "System Info")
    static FString GetSystemLanguage();
};