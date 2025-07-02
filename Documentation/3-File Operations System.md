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

<img src="https://github.com/user-attachments/assets/bbfed09e-54b3-4c57-ad9a-03a64a2fda86" width="400">

### 📂 How To Specify Allowed Files Extantions ?

**If you want to restrict the Picker to show only one type of file:**

Text Files Only Exemple
```txt
Text Files (*.txt)|*.txt
```

PNG Images Only Exemple
```txt
PNG Files (*.png)|*.png
```

**If you want to allow more than one file type under a single category:**

Text and CSV Files Exemple 
```txt
Text and CSV Files (*.txt;*.csv)|*.txt;*.csv
```

JPEG and PNG Images Exemple
```txt
Image Files (*.jpg;*.jpeg;*.png)|*.jpg;*.jpeg;*.png
```
**If you want to provide multiple categories in the file dialog’s dropdown:**

Text Files and Images Separately  
```txt
Text Files (*.txt)|*.txt|Image Files (*.png;*.jpg)|*.png;*.jpg
```

JSON, XML, and All Files 
```txt
JSON Files (*.json)|*.json|XML Files (*.xml)|*.xml|All Files (*.*)|*.*
```

**IIf you want to use a custom label for your file types:**

Custom File Type  
```txt
My Custom Files (*.custom)|*.custom
```

Custom Type with All Files  
```txt
My Custom Files (*.custom)|*.custom|All Files (*.*)|*.*
```



## 📂 Move File To Folder - Move Folder To Folder

**Used to move a folder/file to a specific destination folder, output if operation succes or not and the error code if not.**

**Note: There are some restrictions on file operation. For example, the C:/ drive is severely restricted in Windows, 
and no operations can be performed on it. Also, specifying a path before the local user folder is not possible due to the lack of administrator privileges.**

<img src="https://github.com/user-attachments/assets/386de9e3-cd40-41d7-badd-ad8fa419db0e" width="400">





## 📂 Get File/folder Info

**Get info related to a file/folder by selecting the path of folder/file**

<img src="https://github.com/user-attachments/assets/4a75033f-f680-42c2-aad3-44bc68e5aea0" width="800">





## 📂 Delete Folder/file

**Delete a folder or file by its own path, success boolean and error message if impossible.**

<img src="https://github.com/user-attachments/assets/e1f86e67-ed13-480e-890e-3e201e33500f" width="800">





## ⌨️ Get System Languge

**Used to move a folder/file to a specific destination folder, output if operation succes or not and the error code if not.**

<img src="https://github.com/user-attachments/assets/a7ef1a06-23f2-425d-8560-a075142e2769" width="400">






## ⌨️ Get Current Keyboard Layout

**Retrieves if user has charger plugged in or not.**

<img src="https://github.com/user-attachments/assets/1115d003-6b1b-4a7d-ad84-f49377617dcd" width="400">

Below is a list of common keyboard layout codes that the `GetCurrentKeyboardLayout` function can return on Windows, along with their meanings. These codes represent the active input locale identifier (KLID) for the current thread.

| Layout Code | Language/Region          |
|-------------|--------------------------|
| 00000409    | English (United States)  |
| 00000809    | English (United Kingdom) |
| 0000040C    | French (France)          |
| 00000407    | German (Germany)         |
| 00000410    | Italian (Italy)          |
| 0000040A    | Spanish (Spain)          |
| 00000411    | Japanese                 |
| 00000412    | Korean                   |
| 00000404    | Chinese (Taiwan)         |
| 00000804    | Chinese (PRC)            |
| 00000405    | Czech                    |
| 00000406    | Danish                   |
| 00000413    | Dutch (Netherlands)      |
| 0000040B    | Finnish                  |
| 00000408    | Greek                    |
| 0000040E    | Hungarian                |
| 00000414    | Norwegian (Bokmål)       |
| 00000415    | Polish                   |
| 00000416    | Portuguese (Brazil)      |
| 00000816    | Portuguese (Portugal)    |
| 00000419    | Russian                  |
| 0000041D    | Swedish                  |
| 0000041F    | Turkish                  |

## Variant Keyboard Layouts

Some languages have variant layouts with non-zero variant identifiers. Here are a few examples:

| Layout Code | Language/Region          |
|-------------|--------------------------|
| 00010409    | United States-International |
| 00020409    | United States-Dvorak     |
| 0001040C    | French (Belgium)         |
| 00020408    | Greek Latin              |
| 00010407    | German (IBM)             |
| 0001040A    | Spanish (Latin America)  |

## Notes
- **Standard Layouts**: Codes like "00000409" represent the default layout for a language.
- **Variant Layouts**: Codes like "00010409" indicate alternative layouts for the same language.
- **Dynamic Output**: The function returns the active layout code, which depends on the user’s current settings and can change (e.g., via Alt+Shift).
- For a complete list, refer to [Microsoft’s Keyboard Layout Documentation](https://learn.microsoft.com/en-us/windows/win32/intl/keyboard-identifiers-and-input-method-identifiers).



## ⚠️ Limitation
There is some **limmitation** while using files operations especialy when trying to move or delete an file/folder present in **`C:/`** drive, bcs of **security** reasons, windows sometimes block such operations.

