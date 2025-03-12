# Windows Native Toolkit - Audio System Functions

## Overview
The **Windows Native Toolkit** provides Unreal Engine developers with native access to Windows features.  
This section covers the **Audio System** functions, allowing you to get and set system volume and retrieve the active audio device name.


# Audio System Functions C++ (Scrool Down For Blueprint)


## 📌 GetSystemVolume
Retrieves the current system volume level (range: `0.0` to `1.0`).

### 🔹 **Usage:**
```cpp
#include "AudioSystemLibrary.h"

float Volume = UAudioSystemLibrary::GetSystemVolume();
UE_LOG(LogTemp, Log, TEXT("Current System Volume: %f"), Volume);
```
---

## 🎚️ SetSystemVolume
Sets the system volume to a specified level (range: `0.0` to `1.0`).

### 🔹 **Usage:**
```cpp
#include "AudioSystemLibrary.h"

UAudioSystemLibrary::SetSystemVolume(0.5f); // Sets volume to 50%
UE_LOG(LogTemp, Log, TEXT("System Volume Set to 50%%"));
```
---

## 🎧 GetCurrentAudioDeviceName
Retrieves the name of the currently active audio playback device.

### 🔹 **Usage:**
```cpp
#include "AudioSystemLibrary.h"

FString DeviceName = UAudioSystemLibrary::GetCurrentAudioDeviceName();
UE_LOG(LogTemp, Log, TEXT("Current Audio Device: %s"), *DeviceName);

```
---

# Audio System Functions In Blueprint


## 📌 GetSystemVolume-Blueprint

**Retrieves the current system volume level Float (range: `0.0` to `1.0`).**

![image](https://github.com/user-attachments/assets/6a469517-0884-4eaf-b760-14ef202abc7a)


## 🎚️ SetSystemVolume-Blueprint

**Sets the system volume to a specified level Float (range: `0.0` to `1.0`).**

![image](https://github.com/user-attachments/assets/b731a5e7-a5f4-498e-b2c4-b16f8faa85f1)


## 🎧 GetCurrentAudioDeviceName-Blueprint

**Retrieves the name of the currently active audio playback device As A String.**

![image](https://github.com/user-attachments/assets/1de45d40-a4e7-4e8a-80b1-c0643468ec47)



