// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MessageBoxWindows.generated.h"

/** Standard result states returned by a native Windows message box. */
UENUM(BlueprintType)
enum class EMessageBoxResult : uint8
{
    Confirmed   UMETA(DisplayName = "Confirmed (Yes/OK)"),
    Declined    UMETA(DisplayName = "Declined (No)"),
    Canceled    UMETA(DisplayName = "Canceled (Cancel/X)")
};

/** Standard Windows button layouts for a message box. */
UENUM(BlueprintType)
enum class EMessageBoxButtons : uint8
{
    Ok                  UMETA(DisplayName = "OK Only"),
    OkCancel            UMETA(DisplayName = "OK & Cancel"),
    YesNo               UMETA(DisplayName = "Yes & No"),
    YesNoCancel         UMETA(DisplayName = "Yes, No & Cancel")
};

/** Icon styles supported by the native message-box helpers. */
UENUM(BlueprintType)
enum class EWNTMessageBoxIcon : uint8
{
    None        UMETA(DisplayName = "No Icon"),
    Error       UMETA(DisplayName = "Error"),
    Warning     UMETA(DisplayName = "Warning"),
    Information UMETA(DisplayName = "Information"),
    Question    UMETA(DisplayName = "Question")
};

/** Result states for the custom two-button dialog helper. */
UENUM(BlueprintType)
enum class ECustomDialogResult : uint8
{
    FirstButton   UMETA(DisplayName = "First Button Clicked"),
    SecondButton  UMETA(DisplayName = "Second Button / Cancel")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWNTMessageBoxResultEvent);

/** Async native-message-box node with direct result execution pins. */
UCLASS()
class SYSTEMUTILITYMODULE_API UAsyncNativeMessageBoxAction : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FWNTMessageBoxResultEvent OnConfirmed;

    UPROPERTY(BlueprintAssignable)
    FWNTMessageBoxResultEvent OnDeclined;

    UPROPERTY(BlueprintAssignable)
    FWNTMessageBoxResultEvent OnCanceled;

    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Message Boxes", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Show Native Message Box Async"))
    static UAsyncNativeMessageBoxAction* ShowNativeMessageBoxAsync(
        const UObject* WorldContextObject,
        const FString& Title,
        const FString& Message,
        EMessageBoxButtons Buttons,
        EWNTMessageBoxIcon Icon
    );

    virtual void Activate() override;

private:
    void Finalize(EMessageBoxResult Result);

    FString DialogTitle;
    FString DialogMessage;
    EMessageBoxButtons DialogButtons = EMessageBoxButtons::Ok;
    EWNTMessageBoxIcon DialogIcon = EWNTMessageBoxIcon::None;
    bool bAddedToRootForCompatibility = false;
};

/** Async custom-message-box node with direct result execution pins. */
UCLASS()
class SYSTEMUTILITYMODULE_API UAsyncRegularMessageBoxAction : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FWNTMessageBoxResultEvent OnFirstButton;

    UPROPERTY(BlueprintAssignable)
    FWNTMessageBoxResultEvent OnSecondButton;

    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Message Boxes", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Show Regular Message Box Async"))
    static UAsyncRegularMessageBoxAction* ShowMessageBoxAsync(
        const UObject* WorldContextObject,
        const FString& Title,
        const FString& Message,
        EWNTMessageBoxIcon Icon,
        const FString& FirstButtonText,
        const FString& SecondButtonText,
        bool bShowSecondButton
    );

    virtual void Activate() override;

private:
    void Finalize(ECustomDialogResult Result);

    FString DialogTitle;
    FString DialogMessage;
    EWNTMessageBoxIcon DialogIcon = EWNTMessageBoxIcon::None;
    FString PrimaryButtonText;
    FString SecondaryButtonText;
    bool bDisplaySecondButton = false;
    bool bAddedToRootForCompatibility = false;
};

/** Native Windows message-box helper nodes for Blueprints. */
UCLASS()
class SYSTEMUTILITYMODULE_API UNativeMessageBox : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Shows a native Windows message box and expands the result enum into Blueprint execution pins. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Message Boxes", meta = (DisplayName = "Show Native Message Box", ExpandEnumAsExecs = "Result"))
    static void ShowNativeMessageBox(
        const FString& Title,
        const FString& Message,
        EMessageBoxButtons Buttons,
        EWNTMessageBoxIcon Icon,
        EMessageBoxResult& Result
    );

    /** Shows a native task dialog with custom button text and expands the selected result into Blueprint execution pins. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Message Boxes", meta = (DisplayName = "Show Regular Message Box", ExpandEnumAsExecs = "Result"))
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

