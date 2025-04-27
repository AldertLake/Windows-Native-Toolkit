/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ToastNotificationLibrary.generated.h"

UENUM(BlueprintType)
enum class EToastIconType : uint8
{
    Info        UMETA(DisplayName = "Info"),       // Default info icon
    Warning     UMETA(DisplayName = "Exclamation"), // Exclamation mark
    Error       UMETA(DisplayName = "Error")       // Error (red X)
};

UCLASS()
class UToastNotificationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Displays a system tray notification with a title, message, and customizable icon
    UFUNCTION(BlueprintCallable, Category = "Toast Notification")
    static void ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType = EToastIconType::Info);

    // Cleans up the system tray icon (call this when shutting down or as needed)
    UFUNCTION(BlueprintCallable, Category = "Toast Notification")
    static void CleanupTrayIcon();

private:
    // Internal helper to initialize and show the tray notification
    static void DisplayTrayNotification(const FString& Title, const FString& Message, EToastIconType IconType);

    // Platform check helper
    static bool IsRunningOnWindows();

    // Helper to get the game title
    static FString GetGameTitle();

    // Static initialization and cleanup for the library
    static void StaticInitialize();
    static void StaticShutdown();

    // Register shutdown callback
    static bool bIsShutdownRegistered;
};









