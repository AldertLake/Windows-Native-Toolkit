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

<img src="Images\Battery\HasBattery.png" width="400">



## ⚡ Get Battery Level-Blueprint

**Get the user battery charging level in forme of Float (range: `0.0` to `1.0`).**

<img src="Images\Battery\GetBatteryLevel.png" width="400">




## 🔌 Is Chargine-Blueprint

**Retrieves if user has charger plugged in or not.**

<img src="Images\Battery\IsBatteryCharging.png" width="400">


## 💹 Is Fully Charged-Blueprint

**Retrieves if Battery is charged 100% or not.**

<img src="Images\Battery\IsBatteryCharget.png" width="400">


