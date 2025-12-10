// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ServerHelper.generated.h"

// --- DELEGATES --- ITS THE BEST METHODE TO AVOID PAUSING GAME THREAD
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnFTPComplete, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnFTPProgress, float, ProgressPercent);

USTRUCT(BlueprintType)
struct FFTPTransferHandle
{
    GENERATED_BODY()

public:
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag;

    FFTPTransferHandle()
    {
        CancelFlag = MakeShared<std::atomic<bool>, ESPMode::ThreadSafe>(false);
    }

    void Cancel()
    {
        if (CancelFlag.IsValid())
        {
            *CancelFlag = true;
        }
    }

    bool IsValid() const { return CancelFlag.IsValid(); }
};

UCLASS()
class NETWORKUTILITYMODULE_API UFTPManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Uploads a file. Handles spaces in filenames automatically.
     * @param URL - The folder URL (e.g. ftp://ftpupload.net/htdocs/)
     * @param LocalFilePath - Full path to file (e.g. C:\Images\My Photo.png)
     */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|FTP Server Management", meta = (AutoCreateRefTerm = "OnProgress, OnComplete", DisplayName = "Upload File Using FTP"))
    static FFTPTransferHandle UploadFile(
        FString URL,
        FString User,
        FString Password,
        FString LocalFilePath,
        FOnFTPProgress OnProgress,
        FOnFTPComplete OnComplete
    );

    /**
     * Downloads a file.
     * @param URL - Full file URL (e.g. ftp://site.com/file.zip)
     * @param SaveDirectory - JUST the folder to save in (e.g. D:\Downloads\)
     */
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|FTP Server Management", meta = (AutoCreateRefTerm = "OnProgress, OnComplete", DisplayName = "Download File Using FTP"))
    static FFTPTransferHandle DownloadFile(
        FString URL,
        FString User,
        FString Password,
        FString SaveDirectory,
        FOnFTPProgress OnProgress,
        FOnFTPComplete OnComplete
    );

    //Cancel transfer of the file - note that cancel does not mean delete meaning in the server side the transfer will be canceled & not deleted.
    UFUNCTION(BlueprintCallable, Category = "Windows Native Toolkit|Network & Connectivity|FTP Server Management", meta = (DisplayName = "Cancel FTP Operation"))
    static void CancelTransfer(FFTPTransferHandle Handle);

};