// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "WindowsNativeToolkitSettings.h"

#include "FileIOUtilityModule.h"
#include "Containers/StringConv.h"

#define LOCTEXT_NAMESPACE "WindowsNativeToolkitSettings"

namespace
{
	static FString NormalizeHttpUrlOrDefault(const FString& Value, const TCHAR* DefaultValue)
	{
		FString Normalized = Value.TrimStartAndEnd();
		if (Normalized.IsEmpty())
		{
			return FString(DefaultValue);
		}

		if (!Normalized.StartsWith(TEXT("http://"), ESearchCase::IgnoreCase) &&
			!Normalized.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase))
		{
			return FString(DefaultValue);
		}

		return Normalized;
	}

	static float ClampTimeoutOrDefault(const float Value, const float DefaultValue)
	{
		return Value > 0.1f ? Value : DefaultValue;
	}
}

UWindowsNativeToolkitSettings::UWindowsNativeToolkitSettings()
	: DefaultConfigFilename(TEXT("Game"))
	, bAutomaticallyFlushConfig(true)
	, bAutomaticallyHandleSoftReferencePaths(true)
	, AESEncryptionKey(TEXT(""))
	, DefaultInternetAccessURL(TEXT("http://clients3.google.com/generate_204"))
	, DefaultInternetAccessTimeoutSeconds(2.0f)
	, DefaultPingTimeoutSeconds(2.0f)
	, DefaultPublicIPTimeoutSeconds(4.0f)
	, IfConfigPublicIPURL(TEXT("https://ifconfig.me/ip"))
	, AmazonPublicIPURL(TEXT("https://checkip.amazonaws.com"))
	, ICanHazIPPublicIPURL(TEXT("https://icanhazip.com/"))
{
}

FName UWindowsNativeToolkitSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FName UWindowsNativeToolkitSettings::GetSectionName() const
{
	return TEXT("WindowsNativeToolkit");
}

#if WITH_EDITOR
FText UWindowsNativeToolkitSettings::GetSectionText() const
{
	return LOCTEXT("SectionText", "Windows Native Toolkit");
}

FText UWindowsNativeToolkitSettings::GetSectionDescription() const
{
	return LOCTEXT("SectionDescription", "Project settings for Windows Native Toolkit config file and network utility nodes.");
}

void UWindowsNativeToolkitSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (DefaultConfigFilename.TrimStartAndEnd().IsEmpty())
	{
		DefaultConfigFilename = TEXT("Game");
	}

	DefaultInternetAccessURL = NormalizeHttpUrlOrDefault(DefaultInternetAccessURL, TEXT("http://clients3.google.com/generate_204"));
	DefaultInternetAccessTimeoutSeconds = ClampTimeoutOrDefault(DefaultInternetAccessTimeoutSeconds, 2.0f);
	DefaultPingTimeoutSeconds = ClampTimeoutOrDefault(DefaultPingTimeoutSeconds, 2.0f);
	DefaultPublicIPTimeoutSeconds = ClampTimeoutOrDefault(DefaultPublicIPTimeoutSeconds, 4.0f);
	IfConfigPublicIPURL = NormalizeHttpUrlOrDefault(IfConfigPublicIPURL, TEXT("https://ifconfig.me/ip"));
	AmazonPublicIPURL = NormalizeHttpUrlOrDefault(AmazonPublicIPURL, TEXT("https://checkip.amazonaws.com"));
	ICanHazIPPublicIPURL = NormalizeHttpUrlOrDefault(ICanHazIPPublicIPURL, TEXT("https://icanhazip.com/"));

	const FTCHARToUTF8 KeyBytes(*AESEncryptionKey);
	if (!AESEncryptionKey.IsEmpty() && (AESEncryptionKey.Len() != 32 || KeyBytes.Length() != 32))
	{
		UE_LOG(LogWNTConfig, Warning, TEXT("Windows Native Toolkit AES Encryption Key must be exactly 32 characters and 32 UTF-8 bytes for AES-256. Current length: %d characters, %d bytes."), AESEncryptionKey.Len(), KeyBytes.Length());
	}
}
#endif

#undef LOCTEXT_NAMESPACE

