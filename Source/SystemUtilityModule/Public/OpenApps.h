// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OpenApps.generated.h"

UCLASS()
class SYSTEMUTILITYMODULE_API UOpenApps : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //Open An App by paths
    UFUNCTION(BlueprintCallable, Category = "Windows Utilities")
    static bool OpenApps(const FString& ExePath);

    //Verify If An App Is Running
    UFUNCTION(BlueprintCallable, Category = "Windows Utilities")
    static bool IsAppRunning(const FString& ExePath);
};