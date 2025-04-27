/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
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

UCLASS()
class UMessageBoxWindows : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Message Box", meta = (Keywords = "messagebox dialog windows"))
    static FMessageBoxResult ShowMessageBox(
        const FString& Title,
        const FString& Content,
        EMessageBoxIcon IconType,
        bool bShowSecondButton,
        const FString& FirstButtonText = TEXT("OK"),
        const FString& SecondButtonText = TEXT("Cancel"));
};