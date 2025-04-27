/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "WindowsInfoBPLibrary.generated.h"

UCLASS()
class UWINDOWSInfoBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Windows Info", meta = (Platforms = "Win64"))
    static FString GetWindowsVersion();

    UFUNCTION(BlueprintPure, Category = "Windows Info", meta = (Platforms = "Win64"))
    static FString GetWindowsBuild();

    UFUNCTION(BlueprintPure, Category = "Windows Info", meta = (Platforms = "Win64"))
    static FString GetWindowsEdition();

    UFUNCTION(BlueprintPure, Category = "Windows Info", meta = (Platforms = "Win64"))
    static FString GetPCName();

    UFUNCTION(BlueprintPure, Category = "Windows Info", meta = (Platforms = "Win64"))
    static FString GetLocalUserName();

private:
    static FString ReadRegistryString(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);
    static uint32 ReadRegistryDWORD(const FString& KeyPath, const FString& ValueName, bool bLocalMachine = true);
};