# Audio System Functions 
This section covers the **Audio System** functions, allowing you to get and set system volume and retrieve the active audio device name.


## Features


### 📌 GetSystemVolume
Retrieves the current system volume level (range: `0.0` to `1.0`).

### 🔹 **Usage:**
```cpp
#include "AudioSystemLibrary.h"

float Volume = UAudioSystemLibrary::GetSystemVolume();
UE_LOG(LogTemp, Log, TEXT("Current System Volume: %f"), Volume);
```
---

### 🎚️ SetSystemVolume
Sets the system volume to a specified level (range: `0.0` to `1.0`).

### 🔹 **Usage:**
```cpp
#include "AudioSystemLibrary.h"

UAudioSystemLibrary::SetSystemVolume(0.5f); // Sets volume to 50%
UE_LOG(LogTemp, Log, TEXT("System Volume Set to 50%%"));
```
---

### 🎧 GetCurrentAudioDeviceName
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

<img src="Images\Audio\GetSystemVolume.png" width="400">


## 🎚️ SetSystemVolume-Blueprint

**Sets the system volume to a specified level Float (range: `0.0` to `1.0`).**

<img src="Images\Audio\SetSystemVolume.png" width="400">


## 🎧 GetCurrentAudioDeviceName-Blueprint

**Retrieves the name of the currently active audio playback device As A String.**

<img src="Images\Audio\GetAudioDeviceName.png" width="400">



