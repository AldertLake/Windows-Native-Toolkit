// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NativeMessageBox.generated.h"

UENUM(BlueprintType)
enum class ENativeMessageBoxIcon : uint8
{
    None            UMETA(DisplayName = "None"),
    Information     UMETA(DisplayName = "Information"),
    Warning         UMETA(DisplayName = "Warning"),
    Error           UMETA(DisplayName = "Error"),
    Question        UMETA(DisplayName = "Question")
};

USTRUCT(BlueprintType)
struct FNativeMessageBoxResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Message Box")
    bool bFirstButtonPressed;

    UPROPERTY(BlueprintReadOnly, Category = "Message Box")
    bool bWasClosedWithoutSelection;

    FNativeMessageBoxResult()
    {
        bFirstButtonPressed = false;
        bWasClosedWithoutSelection = false;
    }
};

UCLASS()
class SYSTEMUTILITYMODULE_API UNativeMessageBox : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //Initialize a message box with title and other custom info
    UFUNCTION(BlueprintCallable, Category = "Message Box", meta = (Keywords = "messagebox dialog windows"))
    static FNativeMessageBoxResult ShowNativeMessageBox(
        const FString& Title,
        const FString& Content,
        ENativeMessageBoxIcon IconType,
        bool bShowSecondButton);
};