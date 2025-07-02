# Hardware Information Functions (c++ & Blueprint)

The **Hardware Section** of the Windows Native Toolkit provides essential functions for retrieving detailed system hardware information. These functions allow Unreal Engine developers to access real-time data about the user's machine, including **memory**, **CPU**, **GPU**, and **input devices**.

## Features

### 💾 GetMemoryInfo 
**Retrieves system memory details, including total, used, and free memory for both physical and virtual memory.**

**Blueprint Function:**
```cpp
void GetMemoryInfo(int32& TotalPhysicalMB, int32& UsedPhysicalMB, int32& FreePhysicalMB,
    int32& TotalVirtualMB, int32& UsedVirtualMB, int32& FreeVirtualMB);
```

**Example Usage:**
```cpp
int32 TotalPhysical, UsedPhysical, FreePhysical;
int32 TotalVirtual, UsedVirtual, FreeVirtual;
USystemInfoBPLibrary::GetMemoryInfo(TotalPhysical, UsedPhysical, FreePhysical, TotalVirtual, UsedVirtual, FreeVirtual);
UE_LOG(LogTemp, Log, TEXT("Total RAM: %d MB"), TotalPhysical);
```
---

### 🖥️ GetCPUInfo 
**Retrieves CPU details, including name, manufacturer, core count, and thread count.**

**Blueprint Function:**
```cpp
void GetCPUInfo(FString& Name, FString& Manufacturer, int32& Cores, int32& Threads);
```

**Example Usage:**
```cpp
FString CPUName, CPUManufacturer;
int32 Cores, Threads;
USystemInfoBPLibrary::GetCPUInfo(CPUName, CPUManufacturer, Cores, Threads);
UE_LOG(LogTemp, Log, TEXT("CPU: %s, Manufacturer: %s, Cores: %d, Threads: %d"), *CPUName, *CPUManufacturer, Cores, Threads);
```
---

### 📊 GetGPUInfo 
**Retrieves GPU details, including name, manufacturer, and VRAM usage.**

**Blueprint Function:**
```cpp
void GetGPUInfo(FString& Name, FString& Manufacturer, int32& TotalVRAMMB,
    int32& UsedVRAMMB, int32& FreeVRAMMB);
```

**Example Usage:**
```cpp
FString GPUName, GPUManufacturer;
int32 TotalVRAM, UsedVRAM, FreeVRAM;
USystemInfoBPLibrary::GetGPUInfo(GPUName, GPUManufacturer, TotalVRAM, UsedVRAM, FreeVRAM);
UE_LOG(LogTemp, Log, TEXT("GPU: %s, Manufacturer: %s, Total VRAM: %d MB"), *GPUName, *GPUManufacturer, TotalVRAM);
```
---

### 🎮 GetInputDevices 
**Detects available input devices such as gamepads, mice, and keyboards.**

**Blueprint Function:**
```cpp
void GetInputDevices(bool& HasGamepad, bool& HasMouse, bool& HasKeyboard);
```

**Example Usage:**
```cpp
bool bGamepad, bMouse, bKeyboard;
USystemInfoBPLibrary::GetInputDevices(bGamepad, bMouse, bKeyboard);
UE_LOG(LogTemp, Log, TEXT("Gamepad: %s, Mouse: %s, Keyboard: %s"),
    bGamepad ? TEXT("Yes") : TEXT("No"),
    bMouse ? TEXT("Yes") : TEXT("No"),
    bKeyboard ? TEXT("Yes") : TEXT("No"));
```
---

# Hardware Information In BluePrint


## 📌 Get CPU Info-Blueprint

**Retrieves User CPU Name, Manufacturer, Cores and threads number.**

<img src="Images\HardwareInfo\CPUInfo.png" width="400">




## 🎚️ Get GPU Info-Blueprint

**Get the user Graphics card related info (Manufacturer, Name, Vram in MB...).**

<img src="Images\HardwareInfo\GPUInfo.png" width="400">





## 🎧 Get Memory Info-Blueprint

**Retrieves user memory related info in MB.**

<img src="Images\HardwareInfo\MemoryInfo.png" width="400">




## 🎚️ Get Input Devices-Blueprint

**Get if user has controller, keyboard, mouse connected to the PC or not.**

<img src="Images\HardwareInfo\InputDevices.png" width="400">




