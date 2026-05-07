// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OpenApps.generated.h"

/** Native Windows external-process and window-management helper nodes for Blueprints. */
UCLASS()
class SYSTEMUTILITYMODULE_API UOpenApps : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Launches an external executable and returns the best process ID found for it. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management|External Apps", meta = (DisplayName = "Launch App"))
    static int32 LaunchExternalProcess(const FString& ExePath, const FString& Arguments, bool bHidden);

    /** Terminates a process and its child processes when the OS allows it. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management|External Apps", meta = (DisplayName = "Kill Process Tree"))
    static bool KillProcessTree(int32 ProcessID);

    /** Returns true when a process ID is still running. */
    UFUNCTION(BlueprintPure, Category = "Windows Native Toolkit|Process Management|External Apps", meta = (DisplayName = "Is Process Running"))
    static bool IsProcessRunning(int32 ProcessID);

    /** Brings the main visible window for a process to the foreground. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management|External Apps", meta = (DisplayName = "Focus App"))
    static bool BringAppToFront(int32 ProcessID);

};

