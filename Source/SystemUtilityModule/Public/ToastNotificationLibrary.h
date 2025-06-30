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

/**
 * Blueprint function library for displaying Windows system tray notifications.
 * Provides functions to show notifications and clean up the tray icon.
 * Note: This library is designed exclusively for Windows platforms.
 */
UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UToastNotificationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Displays a system tray notification with a title, message, and customizable icon.
     * @param Title The title of the notification (max 64 characters).
     * @param Message The message body of the notification (max 256 characters).
     * @param IconType The type of icon to display (Info, Warning, Error).
     */
    UFUNCTION(BlueprintCallable, Category = "Toast Notification")
    static void ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType = EToastIconType::Info);

    /**
     * Removes the system tray icon. Automatically called on application exit, but can be invoked manually if needed.
     */
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