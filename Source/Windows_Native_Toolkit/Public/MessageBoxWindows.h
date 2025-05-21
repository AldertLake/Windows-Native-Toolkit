/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MessageBoxWindows.generated.h"

UENUM(BlueprintType)
enum class EMessageBoxIcon : uint8
{
    None            UMETA(DisplayName = "None"),
    Information     UMETA(DisplayName = "Information"),
    Warning         UMETA(DisplayName = "Warning"),
    Error           UMETA(DisplayName = "Error"),
    Question        UMETA(DisplayName = "Question")
};

USTRUCT(BlueprintType)
struct FMessageBoxResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Message Box")
    bool bFirstButtonPressed;

    UPROPERTY(BlueprintReadOnly, Category = "Message Box")
    bool bWasClosedWithoutSelection;

    FMessageBoxResult()
    {
        bFirstButtonPressed = false;
        bWasClosedWithoutSelection = false;
    }
};

/**
 * Blueprint function library for displaying customizable message boxes on Windows.
 * Provides a function to show a message box with a title, content, icon, and optional second button.
 * Note: This library is designed exclusively for Windows platforms.
 */
UCLASS()
class WINDOWS_NATIVE_TOOLKIT_API UMessageBoxWindows : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Displays a message box with the specified title, content, icon, and buttons.
     * @param Title The title of the message box.
     * @param Content The main content or message of the message box.
     * @param IconType The icon to display in the message box (None, Information, Warning, Error, Question).
     * @param bShowSecondButton Whether to show a second button.
     * @param FirstButtonText The text for the first button (default: "OK").
     * @param SecondButtonText The text for the second button if shown (default: "Cancel").
     * @return A struct indicating whether the first button was pressed or if the dialog was closed without selection.
     */
    UFUNCTION(BlueprintCallable, Category = "Message Box", meta = (Keywords = "messagebox dialog windows"))
    static FMessageBoxResult ShowMessageBox(
        const FString& Title,
        const FString& Content,
        EMessageBoxIcon IconType,
        bool bShowSecondButton,
        const FString& FirstButtonText = TEXT("OK"),
        const FString& SecondButtonText = TEXT("Cancel"));
};