/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

// the header of the emty useless class created by epic for no reason, please dont remove it, W Epic adding junk to developers work.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Windows_Native_ToolkitBPLibrary.generated.h"


UCLASS()
class UWindows_Native_ToolkitBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Execute Sample function", Keywords = "Windows_Native_Toolkit sample test testing"), Category = "Windows_Native_ToolkitTesting")
	static float Windows_Native_ToolkitSampleFunction(float Param);
};
