# Battery Utility (c++ & Blueprint)

The **Battery Utility** module in Windows Native Toolkit allows Unreal Engine developers to retrieve battery-related information from the system.

## Features

### 🔋 Battery Status
- `HasBattery()`: Checks if the system has a battery.

### ⚡ Battery Level
- `GetBatteryLevel()`: Retrieves the current battery percentage.  
  - Returns `100%` if plugged in.  
  - Returns `-1` if no battery is detected.

### 🔌 Charging State
- `IsCharging()`: Determines whether the device is currently charging.


### 💹 Charging State
- `IsCharging()`: Determines whether the device is currently charging.


## Example Usage in C++

```cpp
#include "BatteryUtility.h"

void ExampleUsage()
{
    if (UBatteryUtility::HasBattery())
    {
        int32 BatteryLevel = UBatteryUtility::GetBatteryLevel();
        bool bIsCharging = UBatteryUtility::IsCharging();

        UE_LOG(LogTemp, Log, TEXT("Battery Level: %d%%"), BatteryLevel);
        UE_LOG(LogTemp, Log, TEXT("Charging: %s"), bIsCharging ? TEXT("Yes") : TEXT("No"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("No battery detected."));
    }
}

```
---

# Battery Utility In BluePrint


## 🔋 Has Battery-Blueprint

**Retrieves if user has a battery in his laptop or is using a desktop with no battery.**

<img src="https://github.com/user-attachments/assets/b2ac3cef-8dd3-4dbe-a2ea-54a9cd61afe4" width="400">



## ⚡ Get Battery Level-Blueprint

**Get the user battery charging level in forme of Float (range: `0.0` to `1.0`).**

<img src="https://github.com/user-attachments/assets/3377a5d5-ce8f-4741-b77d-e2c4c3f9cbfc" width="400">




## 🔌 Is Chargine-Blueprint

**Retrieves if user has charger plugged in or not.**

<img src="https://github.com/user-attachments/assets/acf377ce-22e0-4a11-8ad3-29483445088e" width="400">


## 💹 Is Fully Charged-Blueprint

**Retrieves if Battery is charged 100% or not.**

<img src="https://github.com/user-attachments/assets/1dc9e640-e5b1-4f39-857c-3bd9512681a0" width="400">


