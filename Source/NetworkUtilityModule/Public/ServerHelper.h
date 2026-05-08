// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <atomic>
#include "ServerHelper.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTransferComplete, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTransferProgress, float, ProgressPercent);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnTransferBytes, int64, BytesTransferred, int64, TotalBytes);

/** Final result details for a network upload or download task. */
USTRUCT(BlueprintType)
struct FNetworkTransferResult
{
    GENERATED_BODY()

    /** True when the transfer completed and the output file was saved. */
    UPROPERTY(BlueprintReadOnly, Category = "Network Transfer")
    bool bSuccess = false;

    /** HTTP status code when the transfer used HTTP or HTTPS. */
    UPROPERTY(BlueprintReadOnly, Category = "Network Transfer")
    int32 StatusCode = 0;

    /** Final local file path for downloads. */
    UPROPERTY(BlueprintReadOnly, Category = "Network Transfer")
    FString SavedFilePath;

    /** Total bytes written or uploaded when available. */
    UPROPERTY(BlueprintReadOnly, Category = "Network Transfer")
    int64 BytesTransferred = 0;

    /** Expected transfer size when the server reports it. */
    UPROPERTY(BlueprintReadOnly, Category = "Network Transfer")
    int64 TotalBytes = 0;

    /** SHA1 hash of the downloaded file content when available. */
    UPROPERTY(BlueprintReadOnly, Category = "Network Transfer")
    FString SHA1;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTransferResult, FNetworkTransferResult, Result);

/** Lightweight cancel handle returned by async transfer nodes. */
USTRUCT(BlueprintType)
struct FNetworkTransferHandle
{
    GENERATED_BODY()

public:
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag;

    FNetworkTransferHandle()
    {
        CancelFlag = MakeShared<std::atomic<bool>, ESPMode::ThreadSafe>(false);
    }

    void Cancel()
    {
        if (CancelFlag.IsValid())
        {
            CancelFlag->store(true);
        }
    }

    bool IsValid() const { return CancelFlag.IsValid(); }
};

/** Native Windows network-transfer helper nodes for Blueprints. */
UCLASS()
class NETWORKUTILITYMODULE_API UServerHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Downloads a file from HTTP/HTTPS.
     * @param URL - Full file URL (e.g. https://site.com/file.zip)
     * @param SaveDirectory - JUST the folder to save in (e.g. D:\Downloads\)
     */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Server Management", meta = (AutoCreateRefTerm = "OnProgress, OnComplete", DisplayName = "Download File (HTTP/HTTPS)"))
    static FNetworkTransferHandle DownloadFileHTTP(
        FString URL,
        FString SaveDirectory,
        FOnTransferProgress OnProgress,
        FOnTransferComplete OnComplete
    );

    /** Downloads an HTTP or HTTPS file with filename override, overwrite control, bytes progress, status code, SHA1, and partial-file cleanup. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Server Management", meta = (AutoCreateRefTerm = "OnProgress, OnComplete", DisplayName = "Download Advanced", AdvancedDisplay = "FileNameOverride,bOverwrite,bDeletePartialOnFail"))
    static FNetworkTransferHandle DownloadAdvanced(
        FString URL,
        FString SaveDirectory,
        FString FileNameOverride,
        bool bOverwrite,
        bool bDeletePartialOnFail,
        FOnTransferBytes OnProgress,
        FOnTransferResult OnComplete
    );

    /**
     * Uploads a file using FTP. Handles spaces in filenames automatically.
     * @param URL - The folder URL (e.g. ftp://ftpupload.net/htdocs/)
     * @param LocalFilePath - Full path to file (e.g. C:\Images\My Photo.png)
     */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Server Management", meta = (AutoCreateRefTerm = "OnProgress, OnComplete", DisplayName = "Upload File Using FTP"))
    static FNetworkTransferHandle UploadFileFTP(
        FString URL,
        FString User,
        FString Password,
        FString LocalFilePath,
        FOnTransferProgress OnProgress,
        FOnTransferComplete OnComplete
    );

    /**
     * Downloads a file using FTP.
     * @param URL - Full file URL (e.g. ftp://site.com/file.zip)
     * @param SaveDirectory - JUST the folder to save in (e.g. D:\Downloads\)
     */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Server Management", meta = (AutoCreateRefTerm = "OnProgress, OnComplete", DisplayName = "Download File Using FTP"))
    static FNetworkTransferHandle DownloadFileFTP(
        FString URL,
        FString User,
        FString Password,
        FString SaveDirectory,
        FOnTransferProgress OnProgress,
        FOnTransferComplete OnComplete
    );

    /** Requests cancellation for an active transfer handle without deleting any already-written local or remote data. */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|Server Management", meta = (DisplayName = "Cancel Network Transfer"))
    static void CancelTransfer(FNetworkTransferHandle Handle);

};

