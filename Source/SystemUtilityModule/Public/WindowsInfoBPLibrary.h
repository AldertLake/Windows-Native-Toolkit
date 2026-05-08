// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WindowsInfoBPLibrary.generated.h"

/** Native Windows operating-system information helper nodes for Blueprints. */
UCLASS()
class SYSTEMUTILITYMODULE_API UWindowsInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Returns the current Windows product family, such as Windows 10 or Windows 11. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|Windows Details", meta = (DisplayName = "Get Windows Version"))
    static FString GetWindowsVersion();

    /** Returns the Windows build number including UBR when available. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|Windows Details", meta = (DisplayName = "Get Windows Build"))
    static FString GetWindowsBuild();

    /** Returns the installed Windows edition, such as Home, Pro, or Enterprise. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|Windows Details", meta = (DisplayName = "Get Windows Edition"))
    static FString GetWindowsEdition();

    /** Returns the local computer name. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|Windows Details", meta = (DisplayName = "Get Computer Name"))
    static FString GetPCName();

    /** Returns the current local user name. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|Windows Details", meta = (DisplayName = "Get User Name"))
    static FString GetLocalUserName();

    /** Returns the current Windows keyboard layout identifier, for example 00000409 for US English. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|Windows Details", meta = (DisplayName = "Get Keyboard Layout"))
    static FString GetCurrentKeyboardLayout();

    /** Returns the current Windows system language locale name, for example en-US or fr-FR. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Information|Windows Details", meta = (DisplayName = "Get System Language"))
    static FString GetSystemLanguage();

private:
    static FString ReadRegistryString(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);

    static uint32 ReadRegistryDWORD(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);
};

