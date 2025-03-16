# File Picker & File System Utility

The **File Picker & File System Utility** module in Windows Native Toolkit provides Unreal Engine developers with powerful file management features, including file/folder selection, file operations, and system language detection.

## Features

### 📂 File & Folder Selection
- `OpenFileFolderPicker()`: Opens a dialog to select a file or folder.

### ⌨️ System Info
- `GetCurrentKeyboardLayout()`: Retrieves the system's current keyboard layout.
- `GetSystemLanguage()`: Retrieves the system's default language.

### 📁 File Operations
- `MoveFileToFolder()`: Moves a file to a specified folder.
- `DeleteFile()`: Deletes a file.
- `GetFileInfo()`: Retrieves information about a file (size, creation time).

### 📂 Folder Operations
- `MoveFolderToFolder()`: Moves a folder to another folder.
- `DeleteFolder()`: Deletes a folder.
- `GetFolderInfo()`: Retrieves folder details, including total size.

## Example Usage in C++

```cpp
#include "FilePickerLibrary.h"
#include "FileSystemBlueprintLibrary.h"

void ExampleUsage()
{
    FString SelectedPath;
    if (UFilePickerLibrary::OpenFileFolderPicker(EFilePickerType::File, true, TEXT("*.txt"), SelectedPath))
    {
        UE_LOG(LogTemp, Log, TEXT("Selected File: %s"), *SelectedPath);
    }

    FString KeyboardLayout = UFilePickerLibrary::GetCurrentKeyboardLayout();
    FString SystemLanguage = UFilePickerLibrary::GetSystemLanguage();
    UE_LOG(LogTemp, Log, TEXT("Keyboard Layout: %s"), *KeyboardLayout);
    UE_LOG(LogTemp, Log, TEXT("System Language: %s"), *SystemLanguage);

    FString SourceFile = TEXT("C:/Example/Source.txt");
    FString DestinationFolder = TEXT("C:/Example/Destination/");
    bool bSuccess;
    FString ErrorMessage;

    UFileSystemBlueprintLibrary::MoveFileToFolder(SourceFile, DestinationFolder, bSuccess, ErrorMessage);
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("File moved successfully."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Error: %s"), *ErrorMessage);
    }
}
```
---

# File Picker & File System Utility In BluePrint


## 📂 Open File Folder Picker

**Retrieves if user has a battery in his laptop or is using a desktop with no battery.**

<img src="https://github.com/user-attachments/assets/85526236-4541-42ce-933d-9489a5303467" width="400">




## 📂 Move File To Folder - Move Folder To Folder

**Used to move a folder/file to a specific destination folder, output if operation succes or not and the error code if not.**

**Note: There are some restrictions on file operation. For example, the C:/ drive is severely restricted in Windows, 
and no operations can be performed on it. Also, specifying a path before the local user folder is not possible due to the lack of administrator privileges.**

<img src="https://github.com/user-attachments/assets/fddf7bdc-e361-4762-b6fe-59dfc9cf512e" width="1050">





## 📂 Get File/folder Info

**Get info related to a file/folder by selecting the path of folder/file**

<img src="https://github.com/user-attachments/assets/7547b2fd-62ab-45a6-9b18-0c40267dd436" width="1050">





## 📂 Delete Folder/file

**Delete a folder or file by its own path, success boolean and error message if impossible.**

<img src="https://github.com/user-attachments/assets/8729950c-84db-4cf5-8e6a-18b4b5147dca" width="1050">





## 📂 Move File To Folder - Move Folder To Folder

**Used to move a folder/file to a specific destination folder, output if operation succes or not and the error code if not.**

**Note: There are some restrictions on file operation. For example, the C:/ drive is severely restricted in Windows, 
and no operations can be performed on it. Also, specifying a path before the local user folder is not possible due to the lack of administrator privileges.**

<img src="https://github.com/user-attachments/assets/fddf7bdc-e361-4762-b6fe-59dfc9cf512e" width="1050">





## 📂 Get File/folder Info

**Retrieves if user has charger plugged in or not.**

<img src="https://github.com/user-attachments/assets/7547b2fd-62ab-45a6-9b18-0c40267dd436" width="400">

