// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ToastNotificationLibrary.generated.h"

UENUM(BlueprintType)
enum class EToastIconType : uint8
{
    Info        UMETA(DisplayName = "Info"),
    Warning     UMETA(DisplayName = "Exclamation"),
    Error       UMETA(DisplayName = "Error")
};

UCLASS()
class SYSTEMUTILITYMODULE_API UToastNotificationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //Toast notification with custom title and content
    UFUNCTION(BlueprintCallable, Category = "Toast Notification")
    static void ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType = EToastIconType::Info);

    //This should be called each time the show notification node ended
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

    // Tracks whether shutdown callback is registered
    static bool bIsShutdownRegistered;
};