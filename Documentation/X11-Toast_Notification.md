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

<img src="Images\Notification\ShowToastNotification.png" width="400">




## 🧹 Cleanup Tray Icon

**The `Show toast notification` creat an tray process to insialize the notification, this node `cleaup tray icon` should be used everytime player quit the game to close the tray.**

<img src="Images\Notification\CleanupTray.png" width="400">


