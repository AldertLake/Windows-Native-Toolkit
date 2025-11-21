// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConfigSaveLibrary.generated.h"

UCLASS()
class FILEIOUTILITYMODULE_API UConfigSaveLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    
public:

    // Save console integer variables to engine.ini
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Engine GConfig Library", meta = (DisplayName = "Save Integer To Engine Config"))
    static void SaveIntegerToEngineConfig(const FString& Section, const FString& Key, int32 Value);

    // Save Float console variables to engine.ini
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Engine GConfig Library", meta = (DisplayName = "Save Float To Engine Config"))
    static void SaveFloatToEngineConfig(const FString& Section, const FString& Key, float Value);

    // Saves a string value to the specified section and key in the engine's config file
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Engine GConfig Library", meta = (DisplayName = "Save String To Engine Config"))
    static void SaveStringToEngineConfig(const FString& Section, const FString& Key, const FString& Value);

    // I use this to remove specific key from the engine.ini config file
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Engine GConfig Library", meta = (DisplayName = "Remove Saved Key From Engine Config"))
    static void RemoveEngineConfigKey(const FString& Section, const FString& Key);

};