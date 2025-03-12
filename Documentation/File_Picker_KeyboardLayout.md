# Windows Native Toolkit

**Windows Native Toolkit** is an Unreal Engine plugin that provides access to native Windows features, including file selection, keyboard layout detection, and system language retrieval.

## Feature Functions In C++

### 📂 File & Folder Picker
- `OpenFileFolderPicker()`: Allows users to open a file or folder selection dialog.

### ⌨️ Keyboard Layout
- `GetCurrentKeyboardLayout()`: Retrieves the current keyboard layout (e.g., 00000409, 00000809).

### 🌍 System Language
- `GetSystemLanguage()`: Fetches the system's default main language (windows only) .

## Example Usage in C++

```cpp
#include "FilePickerLibrary.h"

void ExampleUsage()
{
    FString SelectedPath;
    bool bSuccess = UFilePickerLibrary::OpenFileFolderPicker(EFilePickerType::File, true, TEXT(""), SelectedPath);
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Selected Path: %s"), *SelectedPath);
    }
}
```

## Feature Functions In Blueprint

### 📂 File & Folder Picker
- Use `Open File Folder Picker` To Open The Picker, choose between files or folders. If `All Files Type supported` is True, Allowed File Extantions will be ignored,
 The `Outpout Return` Value Mean If The User Selected A File/Folder or not,the `Out Selected Path` Is The path of the file the user selected.

![image](https://github.com/user-attachments/assets/582668ed-9ec2-43dc-b11d-0cb68d310ac5)

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

### ⌨️ Keyboard Layout

- Use `Get Current Keyboard Layout` Get The Keyboard Layout Code Structure eg: `00000409`

![image](https://github.com/user-attachments/assets/1ef09fdf-f405-4f77-89c5-929f2ddf45de)

### ⌨️ Simple List Of Layouts Structures Meaning :

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

















### 🌍 System Language
- `Get System Langue` Simply output the user syste (windows only) language used as main language.

![image](https://github.com/user-attachments/assets/4ac2a17c-b2df-43c9-a17b-494bd1d69cce)
