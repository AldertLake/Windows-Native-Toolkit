// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MessageBoxWindows.generated.h"

UENUM(BlueprintType)
enum class EMessageBoxResult : uint8
{
    Confirmed   UMETA(DisplayName = "Confirmed (Yes/OK)"),
    Declined    UMETA(DisplayName = "Declined (No)"),
    Canceled    UMETA(DisplayName = "Canceled (Cancel/X)")
};

UENUM(BlueprintType)
enum class EMessageBoxButtons : uint8
{
    Ok                  UMETA(DisplayName = "OK Only"),
    OkCancel            UMETA(DisplayName = "OK & Cancel"),
    YesNo               UMETA(DisplayName = "Yes & No"),
    YesNoCancel         UMETA(DisplayName = "Yes, No & Cancel")
};

UENUM(BlueprintType)
enum class EWNTMessageBoxIcon : uint8
{
    None        UMETA(DisplayName = "No Icon"),
    Error       UMETA(DisplayName = "Error"),
    Warning     UMETA(DisplayName = "Warning"),
    Information UMETA(DisplayName = "Information"),
    Question    UMETA(DisplayName = "Question")
};

UENUM(BlueprintType)
enum class ECustomDialogResult : uint8
{
    FirstButton   UMETA(DisplayName = "First Button Clicked"),
    SecondButton  UMETA(DisplayName = "Second Button / Cancel")
};

UCLASS()
class SYSTEMUTILITYMODULE_API UNativeMessageBox : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //This will show a native windows message box with buttons & custom title/text.
    //Yes trigger Confirm & No trigger declined & Cancel trigger itself.
    //In case of closing the messsagebox without any button pressed, cancel will be triggered.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Message Boxes", meta = (ExpandEnumAsExecs = "Result"))
    static void ShowNativeMessageBox(
        const FString& Title,
        const FString& Message,
        EMessageBoxButtons Buttons,
        EWNTMessageBoxIcon Icon,
        EMessageBoxResult& Result
    );

    //Show a message box with some custom buttons titles & names (Less native than the the ShowNativeMessagBox
    //But you can customize buttons titles here.
    //If the messagebox is closed with nothing selected, this will trigger second button automaticaly.
    //Will not appear if buttons title is empty. always make sure the buttons have title.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Message Boxes", meta = (DisplayName = "Show Regular Message Box"), meta = (ExpandEnumAsExecs = "Result"))
    static void ShowMessageBox(
        const FString& Title,
        const FString& Message,
        EWNTMessageBoxIcon Icon,
        const FString& FirstButtonText,
        const FString& SecondButtonText,
        bool bShowSecondButton,
        ECustomDialogResult& Result
    );
};