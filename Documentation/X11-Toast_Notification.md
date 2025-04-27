# Toast Notifications (c++ & Blueprint)

The **Toast Notifications** module in Windows Native Toolkit allows Unreal Engine developers to toast notification with custom title, descreption and preset icons.

## Features

### 🔔 Toast Notification
- `TestToastNotification()`: Get number of paired devices using bluetooth.

## Example Usage in C++

```cpp
#include "ToastNotificationLibrary.h"
#include "Engine/Engine.h"

void TestToastNotification()
{
    FString Title = TEXT("Notification Title");
    FString Message = TEXT("This is a test notification from Unreal Engine.");
    
    // Show a toast notification with an info icon
    UToastNotificationLibrary::ShowToastNotification(Title, Message, EToastIconType::Info);
}

// Call this function in your game mode, actor, or any suitable UE5 location (e.g., BeginPlay)
void AMyActor::BeginPlay()
{
    Super::BeginPlay();
    TestToastNotification();
}

```


---

# Toast Notifications In Blueprint


## 🔔 Show Toast Notification

**Used to toast a notification, you can select title, descreption (content) of the notification. **

<img src="https://github.com/user-attachments/assets/c879181a-dc8f-4168-b838-ea88e4d941f6" width="400">




## 🧹 Cleanup Tray Icon

**The `Show toast notification` creat an tray process to insialize the notification, this node `cleaup tray icon` should be used everytime player quit the game to close the tray.**

<img src="https://github.com/user-attachments/assets/6a7bc32e-37ac-49ee-b9fe-105ee341e89a" width="400">


