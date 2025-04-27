/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OpenApps.generated.h"

UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UOpenApps : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Windows Utilities")
    static bool OpenApps(const FString& ExePath);

    UFUNCTION(BlueprintCallable, Category = "Windows Utilities")
    static bool IsAppRunning(const FString& ExePath);
};