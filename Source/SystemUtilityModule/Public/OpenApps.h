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
    //Note that it was replaced with LaunchExternalProcess.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Process Management|External Apps", meta = (DeprecatedFunction, DeprecationMessage = "This function was replaced with LaunchExternalProcess"))
    static bool OpenApplication(const FString& ExePath);

    //Verify If An App Is Running.
    //Note that it was replaced with IsProcessRunning (using process ID insted of path)
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Process Management|External Apps", meta = (DeprecatedFunction, DeprecationMessage = "This function was replaced with IsProcessRunning"))
    static bool IsAppRunning(const FString& ExePath);

    //Launch external application process & return the app ID.
    //You can use this ID to verify if the app is still running.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management|External Apps")
    static int32 LaunchExternalProcess(const FString& ExePath, const FString& Arguments, bool bHidden);

    //Kills a process AND its entire family tree (Children/Sub-processes).
    //Note : This function is not recommanded to use & still Experimental.
    //Will fail in most cases.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management|External Apps")
    static bool KillProcessTree(int32 ProcessID);

    //Verify if a process is running or not.
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Process Management|External Apps")
    static bool IsProcessRunning(int32 ProcessID);

};