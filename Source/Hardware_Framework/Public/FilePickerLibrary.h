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
#include "FilePickerLibrary.generated.h"

UENUM(BlueprintType)
enum class EFilePickerType : uint8
{
    File,
    Folder
};

UCLASS()
class UFilePickerLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // la node du selecteur des fichier et des dossier
    UFUNCTION(BlueprintCallable, Category = "File Picker")
    static bool OpenFileFolderPicker(
        EFilePickerType PickerType,
        bool bAllFilesTypeSupported,
        const FString& AllowedFileExtensions,
        FString& OutSelectedPath
    );

    // le layout du clavier est ici monsieur lol
    UFUNCTION(BlueprintPure, Category = "System Info")
    static FString GetCurrentKeyboardLayout();

    // et bien sur la langue de notre system
    UFUNCTION(BlueprintPure, Category = "System Info")
    static FString GetSystemLanguage();
};

