// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "HAL/ThreadSafeBool.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncGetFolderSize.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFolderSizeCalculated, int64, FolderSizeBytes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFolderSizeError, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFolderSizeCanceled);

/** Async Blueprint action that scans a folder size on a background thread. */
UCLASS()
class FILEIOUTILITYMODULE_API UAsyncGetFolderSize : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    /** Called periodically while folder contents are scanned. */
    UPROPERTY(BlueprintAssignable)
    FOnFolderSizeCalculated OnProgress;

    /** Called when folder size calculation completes. */
    UPROPERTY(BlueprintAssignable)
    FOnFolderSizeCalculated OnSuccess;

    /** Called when calculation fails. FolderSizeBytes is 0 for compatibility. */
    UPROPERTY(BlueprintAssignable)
    FOnFolderSizeCalculated OnFail;

    /** Called with a readable failure reason when calculation fails. */
    UPROPERTY(BlueprintAssignable)
    FOnFolderSizeError OnError;

    /** Called when Cancel is requested before the scan completes. */
    UPROPERTY(BlueprintAssignable)
    FOnFolderSizeCanceled OnCanceled;

    /** Calculates a folder size on a background thread. Prefer the world-context version in Blueprints. */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "Windows Native Toolkit|Files Management")
    static UAsyncGetFolderSize* GetFolderSizeAsync(const FString& FolderPath);

    /** Calculates a folder size on a background thread and keeps the async action alive through the GameInstance. */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Folder Size Async"), Category = "Windows Native Toolkit|Files Management")
    static UAsyncGetFolderSize* GetFolderSizeAsyncWithWorldContext(const UObject* WorldContextObject, const FString& FolderPath);

    /** Requests cancellation. Completion delegates are always broadcast on the game thread. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Files Management")
    void Cancel();

    virtual void Activate() override;

private:
    FString TargetFolderPath;
    TSharedPtr<FThreadSafeBool, ESPMode::ThreadSafe> bCancelRequested;
    bool bAddedToRootForCompatibility = false;
};

