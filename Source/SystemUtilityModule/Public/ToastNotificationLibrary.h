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
    Info        UMETA(DisplayName = "Information"),
    Warning     UMETA(DisplayName = "Warning"),
    Error       UMETA(DisplayName = "Error"),
    None        UMETA(DisplayName = "No Icon")
};

UCLASS()
class SYSTEMUTILITYMODULE_API UToastNotificationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //This function will push notification with custom title & descreption & icon.
    //Note that the process name & icon of this notification follow the packaged game settings.
    //This can crash your editor if you abuse it alot in editor but 100% Safe in packaged game.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management|Notifications", meta = (DisplayName = "Push System Notification"))
    static void ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType);

    //This function is used to clear the tray icon created by ShowToastNotification.
    //Note : This function is old & you should not use it. starting from 2.2 the tray management is automated !
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Process Management|Notifications", meta = (DeprecatedFunction, DeprecationMessage = "Tray is managed automaticaly & this function is Deprecated !"))
    static void CleanupTrayIcon();
};