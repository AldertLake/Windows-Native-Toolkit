// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WindowsInfoBPLibrary.generated.h"

UCLASS()
class SYSTEMUTILITYMODULE_API UWindowsInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    //Output windwos current version
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Windows Details")
    static FString GetWindowsVersion();

    //Get Windows Build
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Windows Details")
    static FString GetWindowsBuild();

    //Get widows edition name eg: Proffesionel, Home, LTSC..
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Windows Details")
    static FString GetWindowsEdition();

    //Get the PC Name eg DESKTOP-XXXXX
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Windows Details")
    static FString GetPCName();

    //Get local user name 
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|System Informations|Windows Details")
    static FString GetLocalUserName();

private:
    // Helper to read a string from the Windows registry
    static FString ReadRegistryString(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);

    // Helper to read a DWORD from the Windows registry
    static uint32 ReadRegistryDWORD(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);
};