// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "ConfigSaveLibrary.h"

#include "FileIOUtilityModule.h"
#include "WindowsNativeToolkitSettings.h"
#include "Containers/StringConv.h"
#include "CoreGlobals.h"
#include "HAL/FileManager.h"
#include "Misc/AES.h"
#include "Misc/Base64.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/PropertyPortFlags.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Stack.h"
#include "UObject/UnrealType.h"

namespace WNTConfig::Private
{
	constexpr int32 AESKeyByteLength = FAES::FAESKey::KeySize;
	constexpr int32 PlaintextLengthHeaderBytes = sizeof(int32);

	FString GetDefaultConfigFilename()
	{
		const UWindowsNativeToolkitSettings* Settings = GetDefault<UWindowsNativeToolkitSettings>();
		const FString ConfigName = Settings ? Settings->DefaultConfigFilename.TrimStartAndEnd() : FString();
		return ConfigName.IsEmpty() ? FString(TEXT("Game")) : ConfigName;
	}

	bool ShouldAutomaticallyFlushConfig()
	{
		const UWindowsNativeToolkitSettings* Settings = GetDefault<UWindowsNativeToolkitSettings>();
		return !Settings || Settings->bAutomaticallyFlushConfig;
	}

	bool ShouldAutomaticallyHandleSoftReferencePaths()
	{
		const UWindowsNativeToolkitSettings* Settings = GetDefault<UWindowsNativeToolkitSettings>();
		return !Settings || Settings->bAutomaticallyHandleSoftReferencePaths;
	}

	FString ResolveConfigFilename(const FString& Filename)
	{
		FString Resolved = Filename.TrimStartAndEnd();
		if (Resolved.IsEmpty())
		{
			Resolved = GetDefaultConfigFilename();
		}

		const bool bHasDirectory = !FPaths::GetPath(Resolved).IsEmpty();
		if (!bHasDirectory)
		{
			const FString BaseName = FPaths::GetBaseFilename(Resolved);
			if (GConfig)
			{
				FString ConfigFilename = GConfig->GetConfigFilename(*BaseName);
				return ConfigFilename.EndsWith(TEXT(".ini"))
					? FConfigCacheIni::NormalizeConfigIniPath(ConfigFilename)
					: ConfigFilename;
			}

			return FConfigCacheIni::NormalizeConfigIniPath(FConfigCacheIni::GetDestIniFilename(*BaseName, nullptr, *FPaths::GeneratedConfigDir()));
		}

		if (FPaths::GetExtension(Resolved, false).IsEmpty())
		{
			Resolved += TEXT(".ini");
		}

		if (FPaths::IsRelative(Resolved))
		{
			Resolved = FPaths::Combine(FPaths::GeneratedConfigDir(), Resolved);
		}

		return FConfigCacheIni::NormalizeConfigIniPath(Resolved);
	}

	bool IsConfigCachePath(const FString& ResolvedFilename)
	{
		return ResolvedFilename.EndsWith(TEXT(".ini"));
	}

	FString GetDiskConfigFilename(const FString& ResolvedFilename)
	{
		if (IsConfigCachePath(ResolvedFilename))
		{
			return FConfigCacheIni::NormalizeConfigIniPath(ResolvedFilename);
		}

		return FConfigCacheIni::NormalizeConfigIniPath(FConfigCacheIni::GetDestIniFilename(*ResolvedFilename, nullptr, *FPaths::GeneratedConfigDir()));
	}

	bool IsPathInsideDirectory(const FString& Filename, const FString& Directory)
	{
		FString FullFilename = FPaths::ConvertRelativePathToFull(Filename);
		FString FullDirectory = FPaths::ConvertRelativePathToFull(Directory);
		FPaths::NormalizeFilename(FullFilename);
		FPaths::NormalizeDirectoryName(FullDirectory);

		if (FullFilename.Equals(FullDirectory, ESearchCase::IgnoreCase))
		{
			return true;
		}

		if (!FullDirectory.EndsWith(TEXT("/")))
		{
			FullDirectory += TEXT("/");
		}

		return FullFilename.StartsWith(FullDirectory, ESearchCase::IgnoreCase);
	}

	bool ValidateSectionAndKey(const TCHAR* Operation, const FString& Section, const FString& Key)
	{
		if (Section.TrimStartAndEnd().IsEmpty())
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Section is empty."), Operation);
			return false;
		}

		if (Key.TrimStartAndEnd().IsEmpty())
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Key is empty. Section='%s'."), Operation, *Section);
			return false;
		}

		return true;
	}

	bool ValidateSection(const TCHAR* Operation, const FString& Section)
	{
		if (Section.TrimStartAndEnd().IsEmpty())
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Section is empty."), Operation);
			return false;
		}

		return true;
	}

	bool HasConfig()
	{
		if (!GConfig)
		{
			UE_LOG(LogWNTConfig, Error, TEXT("GConfig is not available."));
			return false;
		}

		return true;
	}

	bool DoesResolvedConfigFileExist(const FString& ResolvedFilename)
	{
		return IFileManager::Get().FileExists(*GetDiskConfigFilename(ResolvedFilename));
	}

	bool EnsureConfigFileReadyForWrite(const TCHAR* Operation, const FString& ResolvedFilename, const bool bCreateIfMissing)
	{
		if (!HasConfig())
		{
			return false;
		}

		const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
		const FString Directory = FPaths::GetPath(DiskFilename);
		if (!Directory.IsEmpty() && !IFileManager::Get().DirectoryExists(*Directory))
		{
			if (!IFileManager::Get().MakeDirectory(*Directory, true))
			{
				UE_LOG(LogWNTConfig, Error, TEXT("%s failed: Could not create config directory. File='%s', Directory='%s'."),
					Operation, *DiskFilename, *Directory);
				return false;
			}
		}

		FConfigFile* ConfigFile = GConfig->FindConfigFile(ResolvedFilename);
		if (!ConfigFile && IsConfigCachePath(ResolvedFilename))
		{
			if (IFileManager::Get().FileExists(*DiskFilename))
			{
				GConfig->LoadFile(ResolvedFilename);
			}
			else if (bCreateIfMissing)
			{
				FConfigFile NewConfigFile;
				NewConfigFile.Name = FName(*FPaths::GetBaseFilename(DiskFilename));
				GConfig->Add(ResolvedFilename, NewConfigFile);
			}

			ConfigFile = GConfig->FindConfigFile(ResolvedFilename);
		}

		if (!ConfigFile)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Config file is not loaded in GConfig%s. ConfigName='%s', File='%s'."),
				Operation,
				bCreateIfMissing ? TEXT(" and could not be created") : TEXT(""),
				*ResolvedFilename,
				*DiskFilename);
			return false;
		}

		ConfigFile->NoSave = false;
		return true;
	}

	bool FinalizeConfigWrite(const TCHAR* Operation, const FString& ResolvedFilename, const bool bAllowMissingAfterFlush = false)
	{
		if (!ShouldAutomaticallyFlushConfig())
		{
			return true;
		}

		if (!HasConfig())
		{
			return false;
		}

		const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
		GConfig->Flush(false, ResolvedFilename);

		if (!DoesResolvedConfigFileExist(ResolvedFilename))
		{
			if (bAllowMissingAfterFlush)
			{
				UE_LOG(LogWNTConfig, Log, TEXT("%s completed: Config file is not present after flush because the operation left no values to save. ConfigName='%s', File='%s'."),
					Operation, *ResolvedFilename, *DiskFilename);
				return true;
			}

			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: GConfig accepted the write but no config file exists on disk after flush. ConfigName='%s', File='%s'. This can happen when the written array is empty or config writes are globally disabled."),
				Operation, *ResolvedFilename, *DiskFilename);
			return false;
		}

		return true;
	}

	bool DoesConfigSectionExist(const FString& Section, const FString& ResolvedFilename)
	{
		return GConfig && GConfig->DoesSectionExist(*Section, ResolvedFilename);
	}

	FConfigFile* FindConfigFileForRead(const FString& ResolvedFilename)
	{
		if (!GConfig)
		{
			return nullptr;
		}

		return GConfig->Find(ResolvedFilename);
	}

	bool DoesConfigKeyExistInSection(const FString& Section, const FString& Key, const FString& ResolvedFilename)
	{
		const FConfigFile* ConfigFile = FindConfigFileForRead(ResolvedFilename);
		const FConfigSection* ConfigSection = ConfigFile ? ConfigFile->FindSection(Section) : nullptr;
		return ConfigSection && ConfigSection->Contains(FName(*Key));
	}

	bool DoesConfigKeyExistInFile(const FString& Section, const FString& Key, const FString& ResolvedFilename)
	{
		if (Key.TrimStartAndEnd().IsEmpty())
		{
			return false;
		}

		const FConfigFile* ConfigFile = FindConfigFileForRead(ResolvedFilename);
		if (!ConfigFile)
		{
			return false;
		}

		if (!Section.TrimStartAndEnd().IsEmpty())
		{
			const FConfigSection* ConfigSection = ConfigFile->FindSection(Section);
			return ConfigSection && ConfigSection->Contains(FName(*Key));
		}

		TArray<FString> Sections;
		ConfigFile->GetKeys(Sections);
		for (const FString& ExistingSection : Sections)
		{
			const FConfigSection* ConfigSection = ConfigFile->FindSection(ExistingSection);
			if (ConfigSection && ConfigSection->Contains(FName(*Key)))
			{
				return true;
			}
		}

		return false;
	}

	void LogMissingConfigLocation(const TCHAR* Operation, const FString& Section, const FString& Key, const FString& ResolvedFilename)
	{
		const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
		if (!DoesResolvedConfigFileExist(ResolvedFilename))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Config file does not exist. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
				Operation, *ResolvedFilename, *DiskFilename, *Section, *Key);
			return;
		}

		if (!DoesConfigSectionExist(Section, ResolvedFilename))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Section was not found. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
				Operation, *ResolvedFilename, *DiskFilename, *Section, *Key);
			return;
		}

		if (!DoesConfigKeyExistInSection(Section, Key, ResolvedFilename))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Key was not found. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
				Operation, *ResolvedFilename, *DiskFilename, *Section, *Key);
			return;
		}

		UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: GConfig could not read the value even though the file, section, and key exist. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
			Operation, *ResolvedFilename, *DiskFilename, *Section, *Key);
	}

	void LogMissingConfigSection(const TCHAR* Operation, const FString& Section, const FString& ResolvedFilename)
	{
		const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
		if (!DoesResolvedConfigFileExist(ResolvedFilename))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Config file does not exist. ConfigName='%s', File='%s', Section='%s'."),
				Operation, *ResolvedFilename, *DiskFilename, *Section);
			return;
		}

		if (!DoesConfigSectionExist(Section, ResolvedFilename))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Section was not found. ConfigName='%s', File='%s', Section='%s'."),
				Operation, *ResolvedFilename, *DiskFilename, *Section);
			return;
		}

		UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: GConfig could not update the section even though it exists. ConfigName='%s', File='%s', Section='%s'."),
			Operation, *ResolvedFilename, *DiskFilename, *Section);
	}

	bool RemoveConfigSectionFromFile(const TCHAR* Operation, const FString& Section, const FString& ResolvedFilename)
	{
		if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, false))
		{
			return false;
		}

		FConfigFile* ConfigFile = GConfig ? GConfig->FindConfigFile(ResolvedFilename) : nullptr;
		if (!ConfigFile || !ConfigFile->FindSection(Section))
		{
			LogMissingConfigSection(Operation, Section, ResolvedFilename);
			return false;
		}

		if (ConfigFile->Remove(Section) <= 0)
		{
			LogMissingConfigSection(Operation, Section, ResolvedFilename);
			return false;
		}

		ConfigFile->Dirty = true;
		ConfigFile->NoSave = false;
		return FinalizeConfigWrite(Operation, ResolvedFilename, true);
	}

	bool IsEmptySoftReferenceText(const FString& Value)
	{
		const FString TrimmedValue = Value.TrimStartAndEnd();
		return TrimmedValue.IsEmpty() || TrimmedValue.Equals(TEXT("None"), ESearchCase::IgnoreCase);
	}

	bool ValidateSoftObjectPathText(const FString& Value)
	{
		if (IsEmptySoftReferenceText(Value))
		{
			return true;
		}

		const FSoftObjectPath SoftObjectPath(Value.TrimStartAndEnd());
		return SoftObjectPath.IsValid();
	}

	bool ValidateSoftClassPathText(const FString& Value)
	{
		if (IsEmptySoftReferenceText(Value))
		{
			return true;
		}

		const FSoftClassPath SoftClassPath(Value.TrimStartAndEnd());
		return SoftClassPath.IsValid();
	}

	bool ExportPropertyValueToString(const FProperty* Property, const void* ValueAddress, FString& OutValue, UObject* OwnerObject)
	{
		if (!Property || !ValueAddress)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard export failed: property or value address is invalid."));
			return false;
		}

		if (CastField<FSoftClassProperty>(Property))
		{
			if (!ShouldAutomaticallyHandleSoftReferencePaths())
			{
				UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard export failed: automatic soft reference path handling is disabled. Use Convert Class To Path and write the path string manually."));
				return false;
			}

			const FSoftObjectPtr* SoftObjectPtr = reinterpret_cast<const FSoftObjectPtr*>(ValueAddress);
			OutValue = SoftObjectPtr ? SoftObjectPtr->ToSoftObjectPath().ToString() : FString();
			return true;
		}

		if (CastField<FSoftObjectProperty>(Property))
		{
			if (!ShouldAutomaticallyHandleSoftReferencePaths())
			{
				UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard export failed: automatic soft reference path handling is disabled. Use Convert Asset To Path and write the path string manually."));
				return false;
			}

			const FSoftObjectPtr* SoftObjectPtr = reinterpret_cast<const FSoftObjectPtr*>(ValueAddress);
			OutValue = SoftObjectPtr ? SoftObjectPtr->ToSoftObjectPath().ToString() : FString();
			return true;
		}

		if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
		{
			if (!ShouldAutomaticallyHandleSoftReferencePaths())
			{
				UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard export failed: automatic soft reference path handling is disabled. Use Convert Class To Path and write the path string manually."));
				return false;
			}

			const UClass* Class = Cast<UClass>(ClassProperty->GetObjectPropertyValue(ValueAddress));
			OutValue = Class ? FSoftClassPath(Class).ToString() : FString();
			return true;
		}

		if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			if (!ShouldAutomaticallyHandleSoftReferencePaths())
			{
				UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard export failed: automatic soft reference path handling is disabled. Use Convert Asset To Path and write the path string manually."));
				return false;
			}

			const UObject* Object = ObjectProperty->GetObjectPropertyValue(ValueAddress);
			OutValue = Object ? FSoftObjectPath(Object).ToString() : FString();
			return true;
		}

		OutValue.Reset();
		Property->ExportTextItem_Direct(OutValue, ValueAddress, nullptr, OwnerObject, PPF_None);
		return true;
	}

	bool ImportPropertyValueFromString(const FProperty* Property, void* ValueAddress, const FString& Value, UObject* OwnerObject)
	{
		if (!Property || !ValueAddress)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard import failed: property or value address is invalid."));
			return false;
		}

		if (const FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
		{
			if (!ShouldAutomaticallyHandleSoftReferencePaths())
			{
				UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard import failed: automatic soft reference path handling is disabled. Use Convert Path To Class and handle the soft class reference manually."));
				return false;
			}

			if (!ValidateSoftClassPathText(Value))
			{
				UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard import failed: value '%s' is not a valid soft class path for property '%s'."),
					*Value, *Property->GetName());
				return false;
			}

			if (IsEmptySoftReferenceText(Value))
			{
				SoftClassProperty->ClearValue(ValueAddress);
				return true;
			}

			const FString TrimmedValue = Value.TrimStartAndEnd();
			FSoftObjectPtr* SoftObjectPtr = reinterpret_cast<FSoftObjectPtr*>(ValueAddress);
			*SoftObjectPtr = FSoftObjectPtr(FSoftClassPath(TrimmedValue));
			return true;
		}

		if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
		{
			if (!ShouldAutomaticallyHandleSoftReferencePaths())
			{
				UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard import failed: automatic soft reference path handling is disabled. Use Convert Path To Asset and handle the soft asset reference manually."));
				return false;
			}

			if (!ValidateSoftObjectPathText(Value))
			{
				UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard import failed: value '%s' is not a valid soft object path for property '%s'."),
					*Value, *Property->GetName());
				return false;
			}

			if (IsEmptySoftReferenceText(Value))
			{
				SoftObjectProperty->ClearValue(ValueAddress);
				return true;
			}

			const FString TrimmedValue = Value.TrimStartAndEnd();
			FSoftObjectPtr* SoftObjectPtr = reinterpret_cast<FSoftObjectPtr*>(ValueAddress);
			*SoftObjectPtr = FSoftObjectPtr(FSoftObjectPath(TrimmedValue));
			return true;
		}

		if (CastField<FClassProperty>(Property))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard import failed: reading config paths into hard Class Reference pins would require synchronous loading. Read into a Soft Class Reference pin, then use Unreal's async load nodes if a loaded class is needed."));
			return false;
		}

		if (CastField<FObjectPropertyBase>(Property))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard import failed: reading config paths into hard Object Reference pins would require synchronous loading. Read into a Soft Object Reference pin, then use Unreal's async load nodes if a loaded object is needed."));
			return false;
		}

		const TCHAR* Result = Property->ImportText_Direct(*Value, ValueAddress, OwnerObject, PPF_None);
		if (!Result)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard import failed: value '%s' could not be imported into property '%s' of type '%s'."),
				*Value, *Property->GetName(), *Property->GetCPPType());
			return false;
		}

		return true;
	}

	bool ExportArrayToStrings(const FArrayProperty* ArrayProperty, void* ArrayAddress, TArray<FString>& OutValues, UObject* OwnerObject)
	{
		if (!ArrayProperty || !ArrayAddress)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config array export failed: array property or address is invalid."));
			return false;
		}

		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddress);
		const FProperty* InnerProperty = ArrayProperty->Inner;
		OutValues.Reset(ArrayHelper.Num());

		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			FString ExportedValue;
			if (!ExportPropertyValueToString(InnerProperty, ArrayHelper.GetRawPtr(Index), ExportedValue, OwnerObject))
			{
				return false;
			}

			OutValues.Add(MoveTemp(ExportedValue));
		}

		return true;
	}

	bool ImportPropertyValueFromStringStaged(const FProperty* Property, void* ValueAddress, const FString& Value, UObject* OwnerObject)
	{
		if (!Property || !ValueAddress)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config wildcard import failed: property or value address is invalid."));
			return false;
		}

		FDefaultConstructedPropertyElement TempValue(Property);
		if (!ImportPropertyValueFromString(Property, TempValue.GetObjAddress(), Value, OwnerObject))
		{
			return false;
		}

		Property->CopyCompleteValue(ValueAddress, TempValue.GetObjAddress());
		return true;
	}

	bool ImportStringsToArray(const FArrayProperty* ArrayProperty, void* ArrayAddress, const TArray<FString>& Values, UObject* OwnerObject)
	{
		if (!ArrayProperty || !ArrayAddress)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config array import failed: array property or address is invalid."));
			return false;
		}

		const FProperty* InnerProperty = ArrayProperty->Inner;
		if (!InnerProperty)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config array import failed: array inner property is invalid."));
			return false;
		}

		void* TempArrayAddress = ArrayProperty->AllocateAndInitializeValue();
		ON_SCOPE_EXIT
		{
			ArrayProperty->DestroyAndFreeValue(TempArrayAddress);
		};

		FScriptArrayHelper TempArrayHelper(ArrayProperty, TempArrayAddress);
		TempArrayHelper.Resize(Values.Num());

		for (int32 Index = 0; Index < Values.Num(); ++Index)
		{
			if (!ImportPropertyValueFromString(InnerProperty, TempArrayHelper.GetRawPtr(Index), Values[Index], OwnerObject))
			{
				UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config array import failed: config value '%s' could not be converted to array element type '%s'."),
					*Values[Index], *InnerProperty->GetCPPType());
				return false;
			}
		}

		ArrayProperty->CopyCompleteValue(ArrayAddress, TempArrayAddress);
		return true;
	}

	bool IsSerializedValueEquivalent(const FProperty* Property, const FString& SerializedValue, const void* ValueAddress, UObject* OwnerObject)
	{
		if (!Property || !ValueAddress)
		{
			return false;
		}

		FDefaultConstructedPropertyElement TempValue(Property);
		return ImportPropertyValueFromString(Property, TempValue.GetObjAddress(), SerializedValue, OwnerObject)
			&& Property->Identical(TempValue.GetObjAddress(), ValueAddress, PPF_None);
	}

	bool GetAESKey(FAES::FAESKey& OutKey)
	{
		const UWindowsNativeToolkitSettings* Settings = GetDefault<UWindowsNativeToolkitSettings>();
		const FString KeyString = Settings ? Settings->AESEncryptionKey : FString();
		const FTCHARToUTF8 KeyBytes(*KeyString);

		if (KeyString.Len() != AESKeyByteLength || KeyBytes.Length() != AESKeyByteLength)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config AES Encryption Key must be exactly %d characters and %d UTF-8 bytes. Current length: %d characters, %d bytes."), AESKeyByteLength, AESKeyByteLength, KeyString.Len(), KeyBytes.Length());
			return false;
		}

		OutKey.Reset();
		FMemory::Memcpy(OutKey.Key, reinterpret_cast<const uint8*>(KeyBytes.Get()), AESKeyByteLength);
		return true;
	}

	TArray<uint8> StringToUTF8Bytes(const FString& Value)
	{
		const FTCHARToUTF8 Converted(*Value);

		TArray<uint8> Bytes;
		Bytes.SetNumUninitialized(Converted.Length());
		if (Converted.Length() > 0)
		{
			FMemory::Memcpy(Bytes.GetData(), reinterpret_cast<const uint8*>(Converted.Get()), Converted.Length());
		}

		return Bytes;
	}

	FString UTF8BytesToString(const uint8* Bytes, const int32 ByteCount)
	{
		if (ByteCount <= 0)
		{
			return FString();
		}

		const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes), ByteCount);
		return FString(Converted.Length(), Converted.Get());
	}

	bool EncryptStringToBase64(const FString& Plaintext, FString& OutBase64)
	{
		FAES::FAESKey Key;
		if (!GetAESKey(Key))
		{
			return false;
		}

		const TArray<uint8> PlaintextBytes = StringToUTF8Bytes(Plaintext);
		const int32 PlaintextByteCount = PlaintextBytes.Num();
		const int32 UnpaddedByteCount = PlaintextLengthHeaderBytes + PlaintextByteCount;
		const int32 PaddedByteCount = Align(UnpaddedByteCount, static_cast<int32>(FAES::AESBlockSize));

		TArray<uint8> EncryptedBytes;
		EncryptedBytes.SetNumZeroed(PaddedByteCount);
		FMemory::Memcpy(EncryptedBytes.GetData(), &PlaintextByteCount, PlaintextLengthHeaderBytes);
		if (PlaintextByteCount > 0)
		{
			FMemory::Memcpy(EncryptedBytes.GetData() + PlaintextLengthHeaderBytes, PlaintextBytes.GetData(), PlaintextByteCount);
		}

		FAES::EncryptData(EncryptedBytes.GetData(), EncryptedBytes.Num(), Key);
		OutBase64 = FBase64::Encode(EncryptedBytes);
		return true;
	}

	bool DecryptBase64ToString(const FString& Base64, FString& OutPlaintext)
	{
		FAES::FAESKey Key;
		if (!GetAESKey(Key))
		{
			return false;
		}

		TArray<uint8> EncryptedBytes;
		if (!FBase64::Decode(Base64, EncryptedBytes))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config failed to decode encrypted value from Base64."));
			return false;
		}

		if (EncryptedBytes.Num() < PlaintextLengthHeaderBytes || (EncryptedBytes.Num() % FAES::AESBlockSize) != 0)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config encrypted value has invalid byte length: %d."), EncryptedBytes.Num());
			return false;
		}

		FAES::DecryptData(EncryptedBytes.GetData(), EncryptedBytes.Num(), Key);

		int32 PlaintextByteCount = 0;
		FMemory::Memcpy(&PlaintextByteCount, EncryptedBytes.GetData(), PlaintextLengthHeaderBytes);

		if (PlaintextByteCount < 0 || PlaintextByteCount > EncryptedBytes.Num() - PlaintextLengthHeaderBytes)
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit Config decrypted value has an invalid plaintext length."));
			return false;
		}

		OutPlaintext = UTF8BytesToString(EncryptedBytes.GetData() + PlaintextLengthHeaderBytes, PlaintextByteCount);
		return true;
	}

	bool GenericWriteConfigValue(const FString& Section, const FString& Key, const FProperty* ValueProperty, const void* ValueAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Write Config Value");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		FString SerializedValue;
		if (!ExportPropertyValueToString(ValueProperty, ValueAddress, SerializedValue, OwnerObject))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Could not serialize wildcard Value. Section='%s', Key='%s'."),
				Operation, *Section, *Key);
			return false;
		}

		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, true))
		{
			return false;
		}

		GConfig->SetString(*Section, *Key, *SerializedValue, ResolvedFilename);
		return FinalizeConfigWrite(Operation, ResolvedFilename);
	}

	bool GenericReadConfigValue(const FString& Section, const FString& Key, const FProperty* ValueProperty, void* ValueAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Read Config Value");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		FString SerializedValue;
		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		if (!GConfig->GetString(*Section, *Key, SerializedValue, ResolvedFilename))
		{
			LogMissingConfigLocation(Operation, Section, Key, ResolvedFilename);
			return false;
		}

		if (!ImportPropertyValueFromStringStaged(ValueProperty, ValueAddress, SerializedValue, OwnerObject))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Config value could not be converted to the connected output pin type. ConfigName='%s', File='%s', Section='%s', Key='%s', RawValue='%s'."),
				Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key, *SerializedValue);
			return false;
		}

		return true;
	}

	bool GenericWriteConfigArray(const FString& Section, const FString& Key, const FArrayProperty* ArrayProperty, void* ArrayAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Write Config Array");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		TArray<FString> SerializedValues;
		if (!ExportArrayToStrings(ArrayProperty, ArrayAddress, SerializedValues, OwnerObject))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Could not serialize wildcard Values array. Section='%s', Key='%s'."),
				Operation, *Section, *Key);
			return false;
		}

		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, true))
		{
			return false;
		}

		if (SerializedValues.IsEmpty())
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s wrote an empty array. Native config arrays do not persist an explicit empty-array marker, so a later read returns false until at least one value is saved. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
				Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key);
		}

		GConfig->SetArray(*Section, *Key, SerializedValues, ResolvedFilename);
		return FinalizeConfigWrite(Operation, ResolvedFilename, SerializedValues.IsEmpty());
	}

	bool GenericReadConfigArray(const FString& Section, const FString& Key, const FArrayProperty* ArrayProperty, void* ArrayAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Read Config Array");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		TArray<FString> SerializedValues;
		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		const int32 NumValues = GConfig->GetArray(*Section, *Key, SerializedValues, ResolvedFilename);

		if (NumValues <= 0)
		{
			LogMissingConfigLocation(Operation, Section, Key, ResolvedFilename);
			return false;
		}

		if (!ImportStringsToArray(ArrayProperty, ArrayAddress, SerializedValues, OwnerObject))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Config array entries could not be converted to the connected output array type. ConfigName='%s', File='%s', Section='%s', Key='%s', EntryCount=%d."),
				Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key, SerializedValues.Num());
			return false;
		}

		return true;
	}

	bool GenericAddUniqueToConfigArray(const FString& Section, const FString& Key, const FProperty* ValueProperty, const void* ValueAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Add Unique To Config Array");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		FString SerializedValue;
		if (!ExportPropertyValueToString(ValueProperty, ValueAddress, SerializedValue, OwnerObject))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Could not serialize wildcard Value. Section='%s', Key='%s'."),
				Operation, *Section, *Key);
			return false;
		}

		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, true))
		{
			return false;
		}

		TArray<FString> SerializedValues;
		GConfig->GetArray(*Section, *Key, SerializedValues, ResolvedFilename);

		for (const FString& ExistingValue : SerializedValues)
		{
			if (ExistingValue == SerializedValue || IsSerializedValueEquivalent(ValueProperty, ExistingValue, ValueAddress, OwnerObject))
			{
				UE_LOG(LogWNTConfig, Warning, TEXT("%s returned false: Value already exists in the config array. ConfigName='%s', File='%s', Section='%s', Key='%s', Value='%s'."),
					Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key, *SerializedValue);
				return false;
			}
		}

		SerializedValues.Add(MoveTemp(SerializedValue));
		GConfig->SetArray(*Section, *Key, SerializedValues, ResolvedFilename);
		return FinalizeConfigWrite(Operation, ResolvedFilename);
	}

	bool GenericRemoveFromConfigArray(const FString& Section, const FString& Key, const FProperty* ValueProperty, const void* ValueAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Remove From Config Array");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		FString SerializedValue;
		if (!ExportPropertyValueToString(ValueProperty, ValueAddress, SerializedValue, OwnerObject))
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Could not serialize wildcard Value. Section='%s', Key='%s'."),
				Operation, *Section, *Key);
			return false;
		}

		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		TArray<FString> SerializedValues;
		GConfig->GetArray(*Section, *Key, SerializedValues, ResolvedFilename);

		if (SerializedValues.IsEmpty())
		{
			LogMissingConfigLocation(Operation, Section, Key, ResolvedFilename);
			return false;
		}

		const int32 RemovedCount = SerializedValues.RemoveAll([ValueProperty, ValueAddress, OwnerObject, &SerializedValue](const FString& ExistingValue)
		{
			return ExistingValue == SerializedValue || IsSerializedValueEquivalent(ValueProperty, ExistingValue, ValueAddress, OwnerObject);
		});

		if (RemovedCount > 0)
		{
			if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, false))
			{
				return false;
			}

			GConfig->SetArray(*Section, *Key, SerializedValues, ResolvedFilename);
			return FinalizeConfigWrite(Operation, ResolvedFilename, SerializedValues.IsEmpty());
		}
		else
		{
			UE_LOG(LogWNTConfig, Warning, TEXT("%s returned false: Value was not found in the config array. ConfigName='%s', File='%s', Section='%s', Key='%s', Value='%s'."),
				Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key, *SerializedValue);
		}

		return false;
	}
}

using namespace WNTConfig::Private;

bool UConfigSaveLibrary::WriteConfigValue(const FString& Section, const FString& Key, const int32& Value, const FString& Filename)
{
	UE_LOG(LogWNTConfig, Error, TEXT("Write Config Value failed: This wildcard node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 signature."));
	return false;
}

DEFINE_FUNCTION(UConfigSaveLibrary::execWriteConfigValue)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	const void* ValueAddress = Stack.MostRecentPropertyAddress;

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericWriteConfigValue(Section, Key, ValueProperty, ValueAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigSaveLibrary::ReadConfigValue(const FString& Section, const FString& Key, int32& Value, const FString& Filename)
{
	UE_LOG(LogWNTConfig, Error, TEXT("Read Config Value failed: This wildcard node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 signature."));
	return false;
}

DEFINE_FUNCTION(UConfigSaveLibrary::execReadConfigValue)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	void* ValueAddress = Stack.MostRecentPropertyAddress;

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericReadConfigValue(Section, Key, ValueProperty, ValueAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigSaveLibrary::WriteConfigArray(const FString& Section, const FString& Key, const TArray<int32>& Values, const FString& Filename)
{
	UE_LOG(LogWNTConfig, Error, TEXT("Write Config Array failed: This wildcard array node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 array signature."));
	return false;
}

DEFINE_FUNCTION(UConfigSaveLibrary::execWriteConfigArray)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	void* ArrayAddress = Stack.MostRecentPropertyAddress;
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayProperty)
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("Write Config Array failed: Values pin is not a valid array property."));
		Stack.bArrayContextFailed = true;
		return;
	}

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericWriteConfigArray(Section, Key, ArrayProperty, ArrayAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigSaveLibrary::ReadConfigArray(const FString& Section, const FString& Key, TArray<int32>& Values, const FString& Filename)
{
	UE_LOG(LogWNTConfig, Error, TEXT("Read Config Array failed: This wildcard array node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 array signature."));
	return false;
}

DEFINE_FUNCTION(UConfigSaveLibrary::execReadConfigArray)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	void* ArrayAddress = Stack.MostRecentPropertyAddress;
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayProperty)
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("Read Config Array failed: Values pin is not a valid array property."));
		Stack.bArrayContextFailed = true;
		return;
	}

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericReadConfigArray(Section, Key, ArrayProperty, ArrayAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigSaveLibrary::AddUniqueToConfigArray(const FString& Section, const FString& Key, const int32& Value, const FString& Filename)
{
	UE_LOG(LogWNTConfig, Error, TEXT("Add Unique To Config Array failed: This wildcard node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 signature."));
	return false;
}

DEFINE_FUNCTION(UConfigSaveLibrary::execAddUniqueToConfigArray)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	const void* ValueAddress = Stack.MostRecentPropertyAddress;

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericAddUniqueToConfigArray(Section, Key, ValueProperty, ValueAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigSaveLibrary::RemoveFromConfigArray(const FString& Section, const FString& Key, const int32& Value, const FString& Filename)
{
	UE_LOG(LogWNTConfig, Error, TEXT("Remove From Config Array failed: This wildcard node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 signature."));
	return false;
}

DEFINE_FUNCTION(UConfigSaveLibrary::execRemoveFromConfigArray)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	const void* ValueAddress = Stack.MostRecentPropertyAddress;

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericRemoveFromConfigArray(Section, Key, ValueProperty, ValueAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigSaveLibrary::WriteEncryptedString(const FString& Section, const FString& Key, const FString& Value, const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Write Encrypted String");

	if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
	{
		return false;
	}

	FString EncryptedValue;
	if (!EncryptStringToBase64(Value, EncryptedValue))
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Plain text value could not be encrypted. Section='%s', Key='%s'."),
			Operation, *Section, *Key);
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, true))
	{
		return false;
	}

	GConfig->SetString(*Section, *Key, *EncryptedValue, ResolvedFilename);
	return FinalizeConfigWrite(Operation, ResolvedFilename);
}

bool UConfigSaveLibrary::ReadEncryptedString(const FString& Section, const FString& Key, FString& Value, const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Read Encrypted String");

	if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
	{
		return false;
	}

	FString EncryptedValue;
	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	if (!GConfig->GetString(*Section, *Key, EncryptedValue, ResolvedFilename))
	{
		LogMissingConfigLocation(Operation, Section, Key, ResolvedFilename);
		return false;
	}

	if (!DecryptBase64ToString(EncryptedValue, Value))
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Config value exists but could not be decrypted. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
			Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key);
		return false;
	}

	return true;
}

FString UConfigSaveLibrary::ConvertAssetToPath(UObject* Asset)
{
	if (!Asset)
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("Convert Asset To Path returned an empty path: Asset is None."));
	}

	return Asset ? FSoftObjectPath(Asset).ToString() : FString();
}

FString UConfigSaveLibrary::ConvertClassToPath(UClass* Class)
{
	if (!Class)
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("Convert Class To Path returned an empty path: Class is None."));
	}

	return Class ? FSoftClassPath(Class).ToString() : FString();
}

bool UConfigSaveLibrary::ConvertPathToSoftAssetReference(const FString& Path, TSoftObjectPtr<UObject>& Asset)
{
	Asset = TSoftObjectPtr<UObject>();

	const FString TrimmedPath = Path.TrimStartAndEnd();
	if (TrimmedPath.IsEmpty())
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("Convert Path To Soft Asset Reference failed: Path is empty."));
		return false;
	}

	const FSoftObjectPath SoftObjectPath(TrimmedPath);
	if (!SoftObjectPath.IsValid())
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("Convert Path To Soft Asset Reference failed: Path is not a valid soft object path. Path='%s'."), *TrimmedPath);
		return false;
	}

	Asset = TSoftObjectPtr<UObject>(SoftObjectPath);
	return true;
}

bool UConfigSaveLibrary::ConvertPathToSoftClassReference(const FString& Path, TSoftClassPtr<UObject>& Class)
{
	Class = TSoftClassPtr<UObject>();

	const FString TrimmedPath = Path.TrimStartAndEnd();
	if (TrimmedPath.IsEmpty())
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("Convert Path To Soft Class Reference failed: Path is empty."));
		return false;
	}

	const FSoftClassPath SoftClassPath(TrimmedPath);
	if (!SoftClassPath.IsValid())
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("Convert Path To Soft Class Reference failed: Path is not a valid soft class path. Path='%s'."), *TrimmedPath);
		return false;
	}

	Class = TSoftClassPtr<UObject>(SoftClassPath);
	return true;
}

bool UConfigSaveLibrary::ClearConfigKey(const FString& Section, const FString& Key, const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Clear Config Key");

	if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
	{
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, false))
	{
		return false;
	}

	if (!GConfig->RemoveKey(*Section, *Key, ResolvedFilename))
	{
		LogMissingConfigLocation(Operation, Section, Key, ResolvedFilename);
		return false;
	}

	return FinalizeConfigWrite(Operation, ResolvedFilename, true);
}

bool UConfigSaveLibrary::ClearConfigSection(const FString& Section, const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Clear Config Section");

	if (!HasConfig() || !ValidateSection(Operation, Section))
	{
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	return RemoveConfigSectionFromFile(Operation, Section, ResolvedFilename);
}

bool UConfigSaveLibrary::RemoveConfigSection(const FString& Section, const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Remove Config Section");

	if (!HasConfig() || !ValidateSection(Operation, Section))
	{
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	return RemoveConfigSectionFromFile(Operation, Section, ResolvedFilename);
}

bool UConfigSaveLibrary::DeleteConfigFile(const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Delete Config File");

	if (!HasConfig())
	{
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
	if (!IsPathInsideDirectory(DiskFilename, FPaths::GeneratedConfigDir()))
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Delete Config File only deletes generated config files under the project's generated config directory. ConfigName='%s', File='%s', AllowedDirectory='%s'."),
			Operation, *ResolvedFilename, *DiskFilename, *FPaths::GeneratedConfigDir());
		return false;
	}

	if (!FPaths::GetExtension(DiskFilename, false).Equals(TEXT("ini"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Resolved file is not an .ini file. ConfigName='%s', File='%s'."),
			Operation, *ResolvedFilename, *DiskFilename);
		return false;
	}

	if (!IFileManager::Get().FileExists(*DiskFilename))
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Config file does not exist. ConfigName='%s', File='%s'."),
			Operation, *ResolvedFilename, *DiskFilename);
		return false;
	}

	GConfig->UnloadFile(ResolvedFilename);
	const bool bDeleted = IFileManager::Get().Delete(*DiskFilename, true, true, false);
	if (!bDeleted)
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Unreal file manager could not delete the file. ConfigName='%s', File='%s'."),
			Operation, *ResolvedFilename, *DiskFilename);
		return false;
	}

	GConfig->UnloadFile(ResolvedFilename);
	GConfig->Remove(ResolvedFilename);
	UE_LOG(LogWNTConfig, Log, TEXT("%s completed. ConfigName='%s', File='%s'."), Operation, *ResolvedFilename, *DiskFilename);
	return true;
}

bool UConfigSaveLibrary::DoesConfigKeyExist(const FString& Section, const FString& Key, const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Does Config Key Exist");

	if (!HasConfig())
	{
		return false;
	}

	if (Key.TrimStartAndEnd().IsEmpty())
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("%s returned false: Key is empty."), Operation);
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	const bool bExists = DoesConfigKeyExistInFile(Section, Key, ResolvedFilename);
	if (!bExists)
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("%s returned false: Key was not found. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
			Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key);
	}

	return bExists;
}

bool UConfigSaveLibrary::DoesConfigFileExist(const FString& Filename)
{
	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
	const bool bExists = DoesResolvedConfigFileExist(ResolvedFilename);
	if (!bExists)
	{
		const FString Directory = FPaths::GetPath(DiskFilename);
		const bool bDirectoryExists = IFileManager::Get().DirectoryExists(*Directory);
		UE_LOG(LogWNTConfig, Warning, TEXT("Does Config File Exist returned false: Config file was not found on disk. ConfigName='%s', File='%s', Directory='%s', DirectoryExists=%s."),
			*ResolvedFilename,
			*DiskFilename,
			*Directory,
			bDirectoryExists ? TEXT("true") : TEXT("false"));
	}

	return bExists;
}

TArray<FString> UConfigSaveLibrary::GetConfigSections(const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Get Config Sections");

	TArray<FString> Sections;
	if (!HasConfig())
	{
		return Sections;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
	if (!DoesResolvedConfigFileExist(ResolvedFilename))
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("%s failed: Config file does not exist. ConfigName='%s', File='%s'."), Operation, *ResolvedFilename, *DiskFilename);
		return Sections;
	}

	GConfig->GetSectionNames(ResolvedFilename, Sections);
	if (Sections.IsEmpty())
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("%s returned zero sections: File exists but no sections were loaded. ConfigName='%s', File='%s'."),
			Operation, *ResolvedFilename, *DiskFilename);
	}

	return Sections;
}

bool UConfigSaveLibrary::FlushConfig(const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Flush Config");

	if (!HasConfig())
	{
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, false))
	{
		return false;
	}

	GConfig->Flush(false, ResolvedFilename);
	UE_LOG(LogWNTConfig, Log, TEXT("%s completed. ConfigName='%s', File='%s'."),
		Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename));
	return true;
}



