# 🖥️ Windows Native Toolkit - Refresh Rate
The **Refresh Rate** module allows interaction with the **screen refresh rate** on Windows.  
✔ Get the **current refresh rate**  
✔ Change the **refresh rate**  
✔ List **supported refresh rates**  

⚠ **Note:** These functions are **Windows-exclusive**.  



## 📜 Available Functions  

### 🛠️ `GetCurrentRefreshRate()`  
**Description:**  
🔹 Retrieves the **current refresh rate** of the primary display.  

**Example Usage:**  
```cpp
int32 CurrentRefreshRate = URefreshRateFunctionLibrary::GetCurrentRefreshRate();
UE_LOG(LogTemp, Log, TEXT("Current refresh rate: %d Hz"), CurrentRefreshRate);

```



---

### 🛠️ `SetRefreshRate(int32 NewRefreshRate, int32 Width = -1, int32 Height = -1)`  
**Description:**  
🔹 Sets a **new refresh rate** for the display.  
🔹 Optionally adjusts **resolution**.  
🔹 Returns `true` if successful, `false` otherwise.  

**⚠ Warning:**  
🔹 Windows **prevents unsupported refresh rates**.  
🔹 Use `GetSupportedRefreshRates()` to check valid rates.  

**Example Usage:**  
```cpp
bool bSuccess = URefreshRateFunctionLibrary::SetRefreshRate(144);
if (bSuccess)
{
    UE_LOG(LogTemp, Log, TEXT("Successfully changed refresh rate to 144 Hz."));
}
else
{
    UE_LOG(LogTemp, Error, TEXT("Failed to change refresh rate."));
}
```






### 🛠️ `GetSupportedRefreshRates()`  
**Description:**  
🔹 Retrieves all **supported refresh rates** for the current resolution.  

**Example Usage:**  
```cpp
TArray<int32> SupportedRates = URefreshRateFunctionLibrary::GetSupportedRefreshRates();
for (int32 Rate : SupportedRates)
{
    UE_LOG(LogTemp, Log, TEXT("Supported rate: %d Hz"), Rate);
}
```














---

# Refresh Rate In Blueprint


## 📌 Set refresh rate

**Used to change the player display refresh rate in full screen mode, the`width` and `height` represente the resolution X and Y if both `=-1` then if will change refresh rate while keeping the original resolution,
if modified it will apply the new refresh rate with the new display resolution, the boolean return value tell if the refresh rate got applied or not (unsupported refresh rates may not apply).**

<img src="https://github.com/user-attachments/assets/1eefa3a8-0cca-4219-86be-fd4c803c75d3" width="400">



## ⚙️ Get supported refresh rates

**Retrieves list of supported refresh rates by player display can be collected using an `Loop with break Node`.**

<img src="https://github.com/user-attachments/assets/97616559-2cfa-43e8-a117-04deb984c7fc" width="400">



## 🖥️ Get Current refresh rate

**Output the player display current active refresh rate.**

<img src="https://github.com/user-attachments/assets/f8514168-e2ff-488a-a1fc-220bddee47b1" width="400">

