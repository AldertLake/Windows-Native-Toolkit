#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WindowsInfoBPLibrary.generated.h"

/**
 * Blueprint function library for retrieving Windows system information.
 * Provides functions to query Windows version, build, edition, PC name, and user name.
 * Note: Some functions are Windows-only; unsupported platforms return default values.
 */
UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UWindowsInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Gets the Windows version (e.g., "Windows 11", "Windows 10").
     * @return Version name or "Unknown" if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "Windows Info")
    static FString GetWindowsVersion();

    /**
     * Gets the Windows build number (e.g., "22000.1234").
     * @return Build number or "Unknown" if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "Windows Info")
    static FString GetWindowsBuild();

    /**
     * Gets the Windows edition (e.g., "Pro", "Home").
     * @return Edition name or "Unknown" if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "Windows Info")
    static FString GetWindowsEdition();

    /**
     * Gets the PC's computer name.
     * @return Computer name or "Unknown" if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "Windows Info")
    static FString GetPCName();

    /**
     * Gets the local user name.
     * @return User name or "Unknown" if unavailable.
     */
    UFUNCTION(BlueprintPure, Category = "Windows Info")
    static FString GetLocalUserName();

private:
    // Helper to read a string from the Windows registry
    static FString ReadRegistryString(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);

    // Helper to read a DWORD from the Windows registry
    static uint32 ReadRegistryDWORD(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);
};