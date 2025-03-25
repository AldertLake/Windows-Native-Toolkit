# 🖥️ Network Utilities

The **Network Utilities** module provides essential network functionality for **Unreal Engine**, allowing developers to:

✔ Retrieve the **local IP address**

✔ Check the **internet connection**

✔ Detect the **connection type** (Wi-Fi, Ethernet, etc.)

✔ Check the **Netwrok Adapter Name** (Realtek RTL8852BE WiFi 6 802.11ax PCIe Adapter)

✔ Check the **Wi-Fi Newtork Name SSID** (TP-Link-87839_5G)



## 📜 Features

### 🛠️ `GetLocalIpAddress()`
**Description:**
🔹 Retrieves the **local IP address** of the computer.
🔹 Returns an **IPv4 address** in `FString` format.
🔹 If no address is available, returns an empty string.

**Example usage:**
```cpp
FString LocalIP = UNetworkUtilities::GetLocalIpAddress();
UE_LOG(LogTemp, Log, TEXT("Local IP Address: %s"), *LocalIP);
```

---

### 🛠️ `IsConnectedToInternet()`
**Description:**
🔹 Checks if the computer is **connected to the Internet**.
🔹 Returns a `bool`:
- `true` → Active connection
- `false` → No Internet access

**Example usage:**
```cpp
if (UNetworkUtilities::IsConnectedToInternet())
{
UE_LOG(LogTemp, Log, TEXT("The device is connected to the Internet."));
}
else
{
UE_LOG(LogTemp, Warning, TEXT("No Internet connection."));
}
```

---

### 🛠️ `GetConnectionType()`
**Description:**
🔹 Identifies the **active network connection type**:
- **"Wi-Fi"** → Wireless connection
- **"Ethernet"** → Network cable
- **"Both"** → Simultaneous Wi-Fi + Ethernet connection
- **"None"** → No network connection detected

**Usage example:**
```cpp
FString ConnectionType = UNetworkUtilities::GetConnectionType();
UE_LOG(LogTemp, Log, TEXT("Connection type: %s"), *ConnectionType);
```

---

## 🛠️ Integration into Unreal Engine

1️⃣ **Add the header to your C++ file**
```cpp
#include "NetworkUtilities.h"
```

---


---

# Network Utilities In Blueprint


## 🌐 Get Local Ip Adress

**Retrieves the player local ip adress in forme of string, can be used when building an Co-op mutliplayer local system.**

<img src="https://github.com/user-attachments/assets/d18984a4-b4b5-4f90-bef1-27d5c52fc595" width="400">



## 🔗 Is Connected to internet

**Retrieves If player has an active internet conection on his PC.**

<img src="https://github.com/user-attachments/assets/1e513503-1b5c-4ccb-a835-2f51f7463c96" width="400">



## 🛜 Get Wifi Network Name

**Output the player `wifi` Network Name AKA SSID.**

<img src="https://github.com/user-attachments/assets/f3570bee-588c-4d0f-8d64-f7ab0261aa2b" width="400">


## 🌀 Get Connection Type

**Output the player connection type in forme of string `wifi`, `Ethernet` or `Both`.**

<img src="https://github.com/user-attachments/assets/939af872-2b69-43c9-a382-33d21d95c6a7" width="400">



## 🆔 Get Active Network Card

**Output the player Network Card `Wirless`, `Ethernet` Name as a  `string`.**

<img src="https://github.com/user-attachments/assets/5e8551ef-3beb-4794-ba39-2c68362e2512" width="400">







