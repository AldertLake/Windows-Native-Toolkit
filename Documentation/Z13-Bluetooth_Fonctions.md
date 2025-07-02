# Bluetooth Fonctions (c++ & Blueprint)

The **Bluetooth Fonctions** module in Windows Native Toolkit allows Unreal Engine developers to retrieve Bluetooth-related information from the system.

## Features

### 🔢 Get Paired Device Count
- `GetPairedDeviceCount()`: Get number of paired devices using bluetooth.

### 🔛 Is Bluetooth Enabled
- `IsBluetoothEnabled()`: Retrieves if bluetooth is enabled or disabled.  

### 🔗 Get Paired Device Name
- `GetPairedDeviceName()`: Determines the name of paired devices by index.


## Example Usage in C++

```cpp
#include "BluetoothManager.h"
#include "Engine/Engine.h"

void TestBluetoothFunctions()
{
    // Check if Bluetooth is enabled
    bool bIsBluetoothEnabled = UBluetoothManager::IsBluetoothEnabled();
    UE_LOG(LogTemp, Log, TEXT("Bluetooth Enabled: %s"), bIsBluetoothEnabled ? TEXT("Yes") : TEXT("No"));

    // Get the count of paired Bluetooth devices
    int32 PairedDeviceCount = UBluetoothManager::GetPairedDeviceCount();
    UE_LOG(LogTemp, Log, TEXT("Paired Bluetooth Devices: %d"), PairedDeviceCount);

    // Iterate through paired devices and log their names
    for (int32 i = 0; i < PairedDeviceCount; i++)
    {
        FString DeviceName = UBluetoothManager::GetPairedDeviceName(i);
        UE_LOG(LogTemp, Log, TEXT("Device %d: %s"), i, *DeviceName);
    }
}

// Call this function in your game mode, actor, or any suitable UE5 location (e.g., BeginPlay)
void AMyActor::BeginPlay()
{
    Super::BeginPlay();
    TestBluetoothFunctions();
}


```
---

## 🔢 Get Paired Device Count

**Retrieves number of total devices paired to player system using bluetooth.**

<img src="Images\Bluetooth\GetpairedBTdevicecount.png" width="400">



## 🔛 Is Bluetooth Enabled

**Retrieves if user has bluetooth on or off in his system.**

<img src="Images\Bluetooth\IsBTEnabled.png" width="400">


## 🔗 Get Paired Device Name

**Retrieves Paired device name by desired index, each device has an index asigned to it by default when the function is called
for exemple if you connect firstly an BT mouse then an BT Headset, the BT mouse index is `0` and the Headset index is `1`**

<img src="Images\Bluetooth\GetPairedBTDeviceName.png" width="400">

## ⚠️ Limitation
There is some **limmitation** while using this on system with more than One Bluetooth Adapter, to prevent errors and chaos i configured this to only work on **First Bluetooth Adapter**, and cases where player use Two ore more
bluetooth adapters is very very rare.


