// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/SoftObjectPtr.h"
#include "ConfigSaveLibrary.generated.h"

/**
 * Blueprint access to Unreal config files with wildcard value serialization,
 * native config arrays, per-value AES encryption, and asset path helpers.
 */
UCLASS()
class FILEIOUTILITYMODULE_API UConfigSaveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Writes a single Blueprint value to a config key.
	 * @param Section Config section name, for example "Player Settings".
	 * @param Key Config key name inside the section, for example "Mouse Sensitivity".
	 * @param Value Connect the value you want to save. The pin changes type to match your variable.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the value was serialized and written to GConfig. If Automatically Flush Config is enabled, the file is also flushed to disk.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Windows Native Toolkit|Config Files|Values", meta=(DisplayName="Write Config Value", CustomStructureParam="Value", AutoCreateRefTerm="Value", ReturnDisplayName="Success"))
	static bool WriteConfigValue(const FString& Section, const FString& Key, const int32& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execWriteConfigValue);

	/**
	 * Reads a single Blueprint value from a config key.
	 * @param Section Config section name, for example "Player Settings".
	 * @param Key Config key name inside the section, for example "Mouse Sensitivity".
	 * @param Value Output value read from config. Drag from this output pin and promote it to a variable, or connect it directly to later nodes.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the key existed and the text value was imported into the output pin.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Windows Native Toolkit|Config Files|Values", meta=(DisplayName="Read Config Value", CustomStructureParam="Value", ReturnDisplayName="Success"))
	static bool ReadConfigValue(const FString& Section, const FString& Key, int32& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execReadConfigValue);

	/**
	 * Writes a Blueprint array using Unreal's native repeated +Key=Value config array format.
	 * @param Section Config section name that owns the array.
	 * @param Key Config array key. Existing entries for this key are replaced.
	 * @param Values Array values to save. The pin changes type to match your array.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the array was serialized and written to GConfig. If Automatically Flush Config is enabled, the file is also flushed to disk.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Windows Native Toolkit|Config Files|Arrays", meta=(DisplayName="Write Config Array", ArrayParm="Values", ReturnDisplayName="Success"))
	static bool WriteConfigArray(const FString& Section, const FString& Key, const TArray<int32>& Values, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execWriteConfigArray);

	/**
	 * Reads a Blueprint array from Unreal's native repeated +Key=Value config array format.
	 * @param Section Config section name that owns the array.
	 * @param Key Config array key to read.
	 * @param Values Output array filled from config. Drag from this output pin and promote it to a variable, or connect it directly to later nodes.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if one or more config array entries were found and imported.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Windows Native Toolkit|Config Files|Arrays", meta=(DisplayName="Read Config Array", ArrayParm="Values", ReturnDisplayName="Success"))
	static bool ReadConfigArray(const FString& Section, const FString& Key, TArray<int32>& Values, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execReadConfigArray);

	/**
	 * Adds one value to a native config array only if an equivalent value is not already present.
	 * @param Section Config section name that owns the array.
	 * @param Key Config array key to update.
	 * @param Value Value to add. The pin changes type to match your variable.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the value was added. False means the value already existed or the write failed.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Windows Native Toolkit|Config Files|Arrays", meta=(DisplayName="Add Unique To Config Array", CustomStructureParam="Value", AutoCreateRefTerm="Value", ReturnDisplayName="Added"))
	static bool AddUniqueToConfigArray(const FString& Section, const FString& Key, const int32& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execAddUniqueToConfigArray);

	/**
	 * Removes all matching values from a native config array.
	 * @param Section Config section name that owns the array.
	 * @param Key Config array key to update.
	 * @param Value Value to remove. The pin changes type to match your variable.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if one or more values were removed.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Windows Native Toolkit|Config Files|Arrays", meta=(DisplayName="Remove From Config Array", CustomStructureParam="Value", AutoCreateRefTerm="Value", ReturnDisplayName="Removed"))
	static bool RemoveFromConfigArray(const FString& Section, const FString& Key, const int32& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execRemoveFromConfigArray);

	/**
	 * Encrypts one string value with the project AES key and writes the Base64 ciphertext to config. Use it for lightweight local config privacy, not as a secure secret vault.
	 * @param Section Config section name.
	 * @param Key Config key name inside the section.
	 * @param Value Plain text string to encrypt and save.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if encryption and config write succeeded. If Automatically Flush Config is enabled, the file is also flushed to disk.
	 */
	UFUNCTION(BlueprintCallable, Category="Windows Native Toolkit|Config Files|Encryption", meta=(DisplayName="Write Encrypted String", ReturnDisplayName="Success"))
	static bool WriteEncryptedString(const FString& Section, const FString& Key, const FString& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Reads a Base64 ciphertext config value and decrypts it with the project AES key.
	 * @param Section Config section name.
	 * @param Key Config key name inside the section.
	 * @param Value Output plain text string after decrypting the config value.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the key existed and decryption succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category="Windows Native Toolkit|Config Files|Encryption", meta=(DisplayName="Read Encrypted String", ReturnDisplayName="Success"))
	static bool ReadEncryptedString(const FString& Section, const FString& Key, FString& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Converts an asset object reference to a soft object path string.
	 * @param Asset Asset object to convert.
	 * @return Soft object path string suitable for saving to config.
	 */
	UFUNCTION(BlueprintPure, Category="Windows Native Toolkit|Config Files|Asset Paths", meta=(DisplayName="Convert Asset To Path", ReturnDisplayName="Path"))
	static FString ConvertAssetToPath(UObject* Asset);

	/**
	 * Converts a class reference to a soft class path string.
	 * @param Class Class to convert.
	 * @return Soft class path string suitable for saving to config.
	 */
	UFUNCTION(BlueprintPure, Category="Windows Native Toolkit|Config Files|Asset Paths", meta=(DisplayName="Convert Class To Path", ReturnDisplayName="Path"))
	static FString ConvertClassToPath(UClass* Class);

	/**
	 * Converts a soft object path string to a soft asset reference without loading the asset.
	 * @param Path Soft object path string, for example "/Game/Folder/Asset.Asset".
	 * @param Asset Soft asset reference output. Use Unreal's async load nodes if you need the loaded object.
	 * @return True if Path was a valid soft object path string.
	 */
	UFUNCTION(BlueprintPure, Category="Windows Native Toolkit|Config Files|Asset Paths", meta=(DisplayName="Convert Path To Asset", ReturnDisplayName="Success"))
	static bool ConvertPathToSoftAssetReference(const FString& Path, UPARAM(DisplayName="Asset") TSoftObjectPtr<UObject>& Asset);

	/**
	 * Converts a soft class path string to a soft class reference without loading the class.
	 * @param Path Soft class path string.
	 * @param Class Soft class reference output. Use Unreal's async load nodes if you need the loaded class.
	 * @return True if Path was a valid soft class path string.
	 */
	UFUNCTION(BlueprintPure, Category="Windows Native Toolkit|Config Files|Asset Paths", meta=(DisplayName="Convert Path To Class", ReturnDisplayName="Success"))
	static bool ConvertPathToSoftClassReference(const FString& Path, UPARAM(DisplayName="Class") TSoftClassPtr<UObject>& Class);

	/**
	 * Removes a scalar value or every repeated array entry for one key.
	 * @param Section Config section name.
	 * @param Key Config key to remove.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the key existed and was removed. If Automatically Flush Config is enabled, the file is also flushed to disk.
	 */
	UFUNCTION(BlueprintCallable, Category="Windows Native Toolkit|Config Files|Utilities", meta=(DisplayName="Clear Config Key", ReturnDisplayName="Success"))
	static bool ClearConfigKey(const FString& Section, const FString& Key, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Removes every key from one config section.
	 * @param Section Config section name to clear.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the section existed and was cleared. If Automatically Flush Config is enabled, the file is also flushed to disk.
	 */
	UFUNCTION(BlueprintCallable, Category="Windows Native Toolkit|Config Files|Utilities", meta=(DisplayName="Clear Config Section", ReturnDisplayName="Success"))
	static bool ClearConfigSection(const FString& Section, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Removes a whole config section and all keys in it.
	 * @param Section Config section name to remove.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the section existed and was removed. If Automatically Flush Config is enabled, the file is also flushed to disk.
	 */
	UFUNCTION(BlueprintCallable, Category="Windows Native Toolkit|Config Files|Utilities", meta=(DisplayName="Remove Config Section", ReturnDisplayName="Success"))
	static bool RemoveConfigSection(const FString& Section, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Deletes a generated project config file from disk and unloads it from GConfig when possible.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the generated config file existed under the project's generated config directory and was deleted.
	 */
	UFUNCTION(BlueprintCallable, Category="Windows Native Toolkit|Config Files|Utilities", meta=(DisplayName="Delete Config File", ReturnDisplayName="Success"))
	static bool DeleteConfigFile(UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Checks whether a key exists in a config file. If Section is empty, all sections are searched.
	 * @param Section Optional config section name. Leave empty to search every section in the file.
	 * @param Key Config key name to find.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the key exists in the requested section, or anywhere in the file when Section is empty.
	 */
	UFUNCTION(BlueprintPure, Category="Windows Native Toolkit|Config Files|Utilities", meta=(DisplayName="Does Config Key Exist", ReturnDisplayName="Exists"))
	static bool DoesConfigKeyExist(const FString& Section, const FString& Key, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Checks whether a config file exists on disk.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the resolved file exists under the generated config directory, or at the supplied absolute path.
	 */
	UFUNCTION(BlueprintPure, Category="Windows Native Toolkit|Config Files|Utilities", meta=(DisplayName="Does Config File Exist", ReturnDisplayName="Exists"))
	static bool DoesConfigFileExist(UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Gets all section names currently known for a config file.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return Section names found in the resolved config file.
	 */
	UFUNCTION(BlueprintPure, Category="Windows Native Toolkit|Config Files|Utilities", meta=(DisplayName="Get Config Sections", ReturnDisplayName="Sections"))
	static TArray<FString> GetConfigSections(UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Forces the resolved config file to be written to disk.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if GConfig was available, the config file was loaded, and the flush request was issued.
	 */
	UFUNCTION(BlueprintCallable, Category="Windows Native Toolkit|Config Files|Utilities", meta=(DisplayName="Flush Config", ReturnDisplayName="Success"))
	static bool FlushConfig(UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
};



