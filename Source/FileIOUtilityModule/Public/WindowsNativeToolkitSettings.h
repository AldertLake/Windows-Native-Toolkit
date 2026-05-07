// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WindowsNativeToolkitSettings.generated.h"

/**
 * Global project settings for Windows Native Toolkit Blueprint libraries.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Windows Native Toolkit"))
class FILEIOUTILITYMODULE_API UWindowsNativeToolkitSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWindowsNativeToolkitSettings();

	/**
	 * Config file name used when a Blueprint node receives an empty File Name pin.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Config Files", meta=(DisplayName="Default Config File Name", ToolTip="Config file name used when a Blueprint node receives an empty File Name pin."))
	FString DefaultConfigFilename;

	/**
	 * When enabled, write and clear config nodes flush immediately after changing config.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Config Files", meta=(DisplayName="Automatically Flush Config", ToolTip="When enabled, write and clear config nodes flush immediately after changing config. When disabled, call Flush Config manually."))
	bool bAutomaticallyFlushConfig;

	/**
	 * When enabled, wildcard config read and write nodes save and read soft references as path strings.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Config Files", meta=(DisplayName="Automatically Handle Soft Reference Paths", ToolTip="When enabled, wildcard config read and write nodes save and read Soft Object Reference and Soft Class Reference pins as path strings. When disabled, use the manual conversion nodes."))
	bool bAutomaticallyHandleSoftReferencePaths;

	/**
	 * AES-256 key used by encrypted config value nodes. Must be exactly 32 characters and 32 UTF-8 bytes. Use for lightweight local config privacy, not as a secure secret vault.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Config Files", meta=(DisplayName="AES Encryption Key", PasswordField=true, ToolTip="AES-256 key used by encrypted config value nodes. Must be exactly 32 characters and 32 UTF-8 bytes. Use for lightweight local config privacy, not as a secure secret vault."))
	FString AESEncryptionKey;

	/**
	 * Default URL used by Query Player Internet Access when TargetURL is left empty.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Network", meta=(DisplayName="Default Internet Access URL", ToolTip="Default URL used by Query Player Internet Access when TargetURL is left empty. The endpoint should return a fast success response when internet access is available."))
	FString DefaultInternetAccessURL;

	/**
	 * Default timeout used by Query Player Internet Access when Timeout is zero or negative.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Network", meta=(ClampMin="0.1", DisplayName="Default Internet Access Timeout", ToolTip="Default timeout in seconds used by Query Player Internet Access when Timeout is zero or negative."))
	float DefaultInternetAccessTimeoutSeconds;

	/**
	 * Default timeout used by Ping URL or IP when Timeout is zero or negative.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Network", meta=(ClampMin="0.1", DisplayName="Default Ping Timeout", ToolTip="Default timeout in seconds used by Ping URL or IP when Timeout is zero or negative."))
	float DefaultPingTimeoutSeconds;

	/**
	 * Default timeout used by Get Public IP when Timeout is zero or negative.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Network", meta=(ClampMin="0.1", DisplayName="Default Public IP Timeout", ToolTip="Default timeout in seconds used by Get Public IP when Timeout is zero or negative."))
	float DefaultPublicIPTimeoutSeconds;

	/**
	 * Provider URL used by Get Public IP when the provider mode is IfConfig.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Network", meta=(DisplayName="IfConfig Public IP URL", ToolTip="Provider URL used by Get Public IP when the provider mode is IfConfig. Leave empty to restore the built-in default."))
	FString IfConfigPublicIPURL;

	/**
	 * Provider URL used by Get Public IP when the provider mode is Amazon.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Network", meta=(DisplayName="Amazon Public IP URL", ToolTip="Provider URL used by Get Public IP when the provider mode is Amazon. Leave empty to restore the built-in default."))
	FString AmazonPublicIPURL;

	/**
	 * Provider URL used by Get Public IP when the provider mode is ICanHazIP.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Network", meta=(DisplayName="ICanHazIP Public IP URL", ToolTip="Provider URL used by Get Public IP when the provider mode is ICanHazIP. Leave empty to restore the built-in default."))
	FString ICanHazIPPublicIPURL;

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

