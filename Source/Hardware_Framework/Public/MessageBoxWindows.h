/*

 * Copyright (C) 2025 AldertLake. All rights reserved.
 *
 * This file is part of the Windows Native ToolKit, an Unreal Engine Plugin.
 *
 * Unauthorized copying, distribution, or modification of this file is strictly prohibited.
 *
 * Anyone who bought this project has the full right to modify it like he want.
 *
 *
 * Author: AldertLake
 * Date: 2025/1/9
 ______________________________________________________________________________________________________________

  AAAAAAA     L          DDDDDDD     EEEEEEE     RRRRRRR    TTTTTTT    L          AAAAAAA     KKKKKK    EEEEEEE
 A       A    L          D       D   E           R     R       T       L         A       A    K     K   E
 AAAAAAAAA    L          D       D   EEEEE       RRRRRRR       T       L         AAAAAAAAA    KKKKKK    EEEE
 A       A    L          D       D   E           R   R         T       L         A       A    K   K     E
 A       A    LLLLLLL    DDDDDDD     EEEEEEE     R    R        T       LLLLLLL   A       A    K    K    EEEEEEE
 ______________________________________________________________________________________________________________
*/

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

    UPROPERTY(BlueprintReadOnly, Category = "MessageBox")
    bool bFirstButtonPressed;

    UPROPERTY(BlueprintReadOnly, Category = "MessageBox")
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
    UFUNCTION(BlueprintCallable, Category = "Windows|MessageBox", meta = (Keywords = "messagebox dialog windows"))
    static FMessageBoxResult ShowMessageBox(
        const FString& Title,
        const FString& Content,
        EMessageBoxIcon IconType,
        bool bShowSecondButton,
        const FString& FirstButtonText = TEXT("OK"),
        const FString& SecondButtonText = TEXT("Cancel"));
};