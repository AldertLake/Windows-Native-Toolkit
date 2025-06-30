#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OpenApps.generated.h"

/**
 * Blueprint function library for opening and checking running applications on Windows.
 * Provides functions to launch executables and verify if they are running.
 * Note: Functions are Windows-only; unsupported platforms return false.
 */
UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UOpenApps : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Opens an application at the specified executable path.
     * @param ExePath Full path to the executable (e.g., "C:/Program Files/App.exe").
     * @return True if the application was launched successfully, false otherwise (e.g., invalid path, file not found).
     */
    UFUNCTION(BlueprintCallable, Category = "Windows Utilities")
    static bool OpenApps(const FString& ExePath);

    /**
     * Checks if an application is running based on its executable name.
     * @param ExePath Full path to the executable (e.g., "C:/Program Files/App.exe"); only the filename is used for matching.
     * @return True if a process matching the executable name is running, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "Windows Utilities")
    static bool IsAppRunning(const FString& ExePath);
};