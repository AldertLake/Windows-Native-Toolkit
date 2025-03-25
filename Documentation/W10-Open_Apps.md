# Open And Check Apps Feature (c++ & Blueprint)

The **Open And Check Apps Feature** module in Windows Native Toolkit allows Unreal Engine developers to open apps with `.exe` extantions and check if they are running or not.

## Features

### 🔢 Open An Apps By Path
- `OpenApps()`: Run an app with `.exe` extantion.

### 🔛 Check If App Is Running
- `IsAppRunning()`: Retrieves if the app process is running or not.  

## Example Usage in C++

```cpp
#include "OpenApps.h"
#include "Engine/Engine.h"

void TestOpenApps()
{
    FString AppPath = TEXT("C:\\Windows\\System32\\notepad.exe");
    
    // Attempt to open Notepad
    bool bOpened = UOpenApps::OpenApps(AppPath);
    UE_LOG(LogTemp, Log, TEXT("Open Notepad: %s"), bOpened ? TEXT("Success") : TEXT("Failed"));
    
    // Check if Notepad is running
    bool bIsRunning = UOpenApps::IsAppRunning(AppPath);
    UE_LOG(LogTemp, Log, TEXT("Is Notepad Running: %s"), bIsRunning ? TEXT("Yes") : TEXT("No"));
}

// Call this function in your game mode, actor, or any suitable UE5 location (e.g., BeginPlay)
void AMyActor::BeginPlay()
{
    Super::BeginPlay();
    TestOpenApps();
}


```


---

# Open Apps Functions


## 📌 Open Apps

**Launch an windows programe by selecting the `.exe` path, exemple:`C:\Users\UserName\AppData\Roaming\Spotify\Spotify.exe, note that you should never use the path with "" always remove them, the return value boolean show if the app is opened or not.**

<img src="https://github.com/user-attachments/assets/0d824b7f-c392-4773-a830-220b0443b048" width="400">




## 🎚️ Is App running

**Verify if an app process is running using the app path `C:\Users\UserName\AppData\Roaming\Spotify\Spotify.exe`, never used the path with the "" always remove them, the return value show if the app is running `Boolean=True` or not `Boolean=False`**

<img src="https://github.com/user-attachments/assets/a7327bd4-1d73-46ad-81c3-d26a3f0cee3e" width="400">

## ⚠️ Limitation
There is some **limmitation** while using this especialy when trying to open and programe installed in **`C:/`** drive, bcs of **security** reasons, windows sometimes block such operations.

