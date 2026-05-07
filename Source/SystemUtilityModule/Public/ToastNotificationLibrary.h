// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ToastNotificationLibrary.generated.h"

/** Icon styles supported by the native Windows notification helper. */
UENUM(BlueprintType)
enum class EToastIconType : uint8
{
    Info        UMETA(DisplayName = "Information"),
    Warning     UMETA(DisplayName = "Warning"),
    Error       UMETA(DisplayName = "Error"),
    None        UMETA(DisplayName = "No Icon")
};

/** Native Windows tray-notification helper nodes for Blueprints. */
UCLASS()
class SYSTEMUTILITYMODULE_API UToastNotificationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Shows a native Windows tray notification using the game window icon when available. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management|Notifications", meta = (DisplayName = "Show Notification"))
    static void ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType);

    static void CleanupTrayIcon();
};

