/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
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
class WINDOWS_NATIVE_TOOLKIT_API UToastNotificationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Toast Notification")
    static void ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType = EToastIconType::Info);

    UFUNCTION(BlueprintCallable, Category = "Toast Notification")
    static void CleanupTrayIcon();

private:
    // Internal helper to initialize and show the tray notification
    static void DisplayTrayNotification(const FString& Title, const FString& Message, EToastIconType IconType);

    // Helper to get the game title from configuration
    static FString GetGameTitle();

    // Static initialization and cleanup for the library
    static void StaticInitialize();
    static void StaticShutdown();
};