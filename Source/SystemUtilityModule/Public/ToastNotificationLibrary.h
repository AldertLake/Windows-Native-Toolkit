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
    Info                UMETA(DisplayName = "Information"),
    Warning             UMETA(DisplayName = "Warning"),
    Error               UMETA(DisplayName = "Error"),
    None                UMETA(DisplayName = "No Icon"),
    ExternalImage       UMETA(DisplayName = "External Image"),
    TextureReference    UMETA(DisplayName = "Texture Reference")
};

/** Native Windows tray-notification helper nodes for Blueprints. */
UCLASS()
class SYSTEMUTILITYMODULE_API UToastNotificationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Shows a native Windows tray notification. When using Texture Reference, pass any UTexture2D from any source. When using External Image, pass a file path to a PNG/JPG/BMP on disk. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management|Notifications", meta = (DisplayName = "Show Notification", AdvancedDisplay = "CustomImagePath, TextureReference"))
    static void ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType, const FString& CustomImagePath = TEXT(""), class UTexture2D* TextureReference = nullptr);

    /** Destroys all cached custom notification icons and frees the associated GDI resources. Call this if you want to force-reload icons after changing textures or image files at runtime. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management|Notifications", meta = (DisplayName = "Invalidate Notification Icon Cache"))
    static void InvalidateIconCache();

    static void CleanupTrayIcon();
};
