# 🖥️ Network Utilities
 
Le module **Network Utilities** fournit des fonctionnalités réseau essentielles pour **Unreal Engine**, permettant aux développeurs de :  
✔ Récupérer l'**adresse IP locale**  
✔ Vérifier la **connexion Internet**  
✔ Détecter le **type de connexion** (Wi-Fi, Ethernet, etc.)  



## 📜 Features

### 🛠️ `GetLocalIpAddress()`  
**Description:**  
🔹 Récupère l'**adresse IP locale** de l'ordinateur.  
🔹 Retourne une **adresse IPv4** au format `FString`.  
🔹 Si aucune adresse n'est disponible, retourne une chaîne vide.  

**Exemple d'utilisation :**  
```cpp
FString LocalIP = UNetworkUtilities::GetLocalIpAddress();
UE_LOG(LogTemp, Log, TEXT("Adresse IP Locale : %s"), *LocalIP);
```


---

### 🛠️ `IsConnectedToInternet()`  
**Description:**  
🔹 Vérifie si l'ordinateur est **connecté à Internet**.  
🔹 Retourne un `bool` :  
   - `true` → Connexion active  
   - `false` → Pas d'accès à Internet  

**Exemple d'utilisation :**  
```cpp
if (UNetworkUtilities::IsConnectedToInternet())
{
    UE_LOG(LogTemp, Log, TEXT("L'appareil est connecté à Internet."));
}
else
{
    UE_LOG(LogTemp, Warning, TEXT("Pas de connexion Internet."));
}
```


---

### 🛠️ `GetConnectionType()`  
**Description:**  
🔹 Identifie le **type de connexion réseau actif** :  
   - **"Wi-Fi"** → Connexion sans fil  
   - **"Ethernet"** → Câble réseau  
   - **"Both"** → Connexion simultanée Wi-Fi + Ethernet  
   - **"None"** → Aucune connexion réseau détectée  

**Exemple d'utilisation :**  
```cpp
FString ConnectionType = UNetworkUtilities::GetConnectionType();
UE_LOG(LogTemp, Log, TEXT("Type de connexion : %s"), *ConnectionType);
```


---

## 🛠️ Intégration dans Unreal Engine  

1️⃣ **Ajoutez l'en-tête dans votre fichier C++**  
```cpp
#include "NetworkUtilities.h"
```


---

# Network Utilities In Blueprint


## 📌 Get Local Ip Adress

**Retrieves the player local ip adress in forme of string, can be used when building an Co-op mutliplayer local system.**

<img src="https://github.com/user-attachments/assets/d18984a4-b4b5-4f90-bef1-27d5c52fc595" width="400">



## 🎚️ Is Connected to internet

**Retrieves If player has an active internet conection on his PC.**

<img src="https://github.com/user-attachments/assets/1e513503-1b5c-4ccb-a835-2f51f7463c96" width="400">



## 🎧 Get Connection Type

**Output the player connection type in forme of string `wifi`, `Ethernet` or `Both`.**

<img src="https://github.com/user-attachments/assets/939af872-2b69-43c9-a382-33d21d95c6a7" width="400">






