# 🖥️ Windows Info
 
Le module **Windows Info** fournit des informations système essentielles sur **Windows**, permettant aux développeurs de :  
✔ Obtenir la **version de Windows**  
✔ Récupérer le **numéro de build**  
✔ Identifier l'**édition de Windows**  
✔ Connaître le **nom du PC** et **l'utilisateur connecté**  


## 📜 Features  

### 🛠️ `GetWindowsVersion()`  
**Description:**  
🔹 Retourne la **version de Windows** (Windows 11, 10, 8, 7, etc.).  
🔹 Utilise `FPlatformMisc::GetOSVersion()` et, si nécessaire, lit le **registre Windows**.  

**Exemple d'utilisation :**  
```cpp
FString WindowsVersion = UWINDOWSInfoBPLibrary::GetWindowsVersion();
UE_LOG(LogTemp, Log, TEXT("Version de Windows : %s"), *WindowsVersion);
```

---

### 🛠️ `GetWindowsBuild()`  
**Description:**  
🔹 Récupère le **numéro de build de Windows**.  
🔹 Lit les valeurs **CurrentBuildNumber** et **UBR** depuis le registre.  

**Exemple d'utilisation :**  
```cpp
FString WindowsBuild = UWINDOWSInfoBPLibrary::GetWindowsBuild();
UE_LOG(LogTemp, Log, TEXT("Build Windows : %s"), *WindowsBuild);


---

### 🛠️ `GetWindowsEdition()`  
**Description:**  
🔹 Retourne l'**édition de Windows** (Home, Pro, Enterprise, etc.).  
🔹 Lit la valeur **EditionID** depuis le registre.  

**Exemple d'utilisation :**  
```cpp
FString WindowsEdition = UWINDOWSInfoBPLibrary::GetWindowsEdition();
UE_LOG(LogTemp, Log, TEXT("Édition de Windows : %s"), *WindowsEdition);
```


---

### 🛠️ `GetPCName()`  
**Description:**  
🔹 Retourne le **nom de l'ordinateur**.  

**Exemple d'utilisation :**  
```cpp
FString PCName = UWINDOWSInfoBPLibrary::GetPCName();
UE_LOG(LogTemp, Log, TEXT("Nom du PC : %s"), *PCName);
```


---

### 🛠️ `GetLocalUserName()`  
**Description:**  
🔹 Retourne le **nom d'utilisateur de la session Windows**.  

**Exemple d'utilisation :**  
```cpp
FString UserName = UWINDOWSInfoBPLibrary::GetLocalUserName();
UE_LOG(LogTemp, Log, TEXT("Utilisateur connecté : %s"), *UserName);
```


---

## 🛠️ Fonctions internes  

### 🔧 `ReadRegistryString()`  
**Description:**  
🔹 Lit une **chaîne de caractères** dans le registre Windows.  

**Signature :**  
```cpp
FString ReadRegistryString(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);
```

---

### 🔧 `ReadRegistryDWORD()`  
**Description:**  
🔹 Lit une **valeur DWORD** dans le registre Windows.  

**Signature :**  
```cpp
uint32 ReadRegistryDWORD(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);
```



---

# Windows Info In BluePrint


## 📌 Get Windows, PC Info

**Retrieves User Windows System Build, Edition, Version, PC Name, Local User Name in forme of string.**

<img src="https://github.com/user-attachments/assets/aee39a6d-e431-4ea5-8a15-9583b780965a" width="1050">




