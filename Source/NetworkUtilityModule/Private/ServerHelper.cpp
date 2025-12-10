// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

#include "ServerHelper.h"

#include "Async/Async.h"
#include "HAL/PlatformFilemanager.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "curl/curl.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

// --- FTP HELPERS --- THOSE ARE THE CORE FTP SYSTEM FUNCTIONS

struct FCurlContext
{
    FArchive* FileArchive = nullptr;
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag;
    FOnFTPProgress ProgressDelegate;
    double LastBroadcastTime = 0.0;
    bool bIsUpload = false; 
};

static size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* stream)
{
    FCurlContext* Context = static_cast<FCurlContext*>(stream);

    if (Context && Context->CancelFlag.IsValid() && Context->CancelFlag->load())
    {
        return 0; 
    }

    if (Context && Context->FileArchive)
    {
        int64 BytesToWrite = (int64)(size * nmemb);
        Context->FileArchive->Serialize(ptr, BytesToWrite);
        return (size_t)BytesToWrite;
    }
    return 0;
}

static size_t ReadCallback(void* ptr, size_t size, size_t nmemb, void* stream)
{
    FCurlContext* Context = static_cast<FCurlContext*>(stream);

    if (Context && Context->CancelFlag.IsValid() && Context->CancelFlag->load())
    {
        return CURL_READFUNC_ABORT; 
    }

    if (Context && Context->FileArchive && !Context->FileArchive->AtEnd())
    {
        int64 BytesToRead = (int64)(size * nmemb);
        int64 TotalSize = Context->FileArchive->TotalSize();
        int64 CurrentPos = Context->FileArchive->Tell();
        int64 Remaining = TotalSize - CurrentPos;

        if (BytesToRead > Remaining) BytesToRead = Remaining;

        Context->FileArchive->Serialize(ptr, BytesToRead);
        return (size_t)BytesToRead;
    }
    return 0;
}

static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    FCurlContext* Context = static_cast<FCurlContext*>(clientp);
    if (!Context) return 0;

    if (Context->CancelFlag.IsValid() && Context->CancelFlag->load())
    {
        UE_LOG(LogTemp, Warning, TEXT("FTP: Transfer Aborted by User via Progress Callback."));
        return 1; 
    }

    float Percent = 0.0f;
    if (Context->bIsUpload)
    {
        if (ultotal > 0) Percent = (float)ulnow / (float)ultotal;
    }
    else
    {
        if (dltotal > 0) Percent = (float)dlnow / (float)dltotal;
    }

    double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - Context->LastBroadcastTime > 0.1)
    {
        Context->LastBroadcastTime = CurrentTime;
        if (Context->ProgressDelegate.IsBound())
        {
            FOnFTPProgress DelegateCopy = Context->ProgressDelegate;
            Async(EAsyncExecution::TaskGraph, [DelegateCopy, Percent]()
                {
                    DelegateCopy.ExecuteIfBound(Percent);
                });
        }
    }
    return 0;
}

static void SetupCurlOptions(CURL* Curl, const FString& URL, const FString& User, const FString& Password, FCurlContext* Context)
{
    FString Auth = User + ":" + Password;

    curl_easy_setopt(Curl, CURLOPT_URL, TCHAR_TO_ANSI(*URL));
    curl_easy_setopt(Curl, CURLOPT_USERPWD, TCHAR_TO_ANSI(*Auth));
    curl_easy_setopt(Curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(Curl, CURLOPT_TIMEOUT, 600L);

    // Robustness
    curl_easy_setopt(Curl, CURLOPT_FTP_USE_EPSV, 0L);
    curl_easy_setopt(Curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
    curl_easy_setopt(Curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 2L);
    curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Progress
    curl_easy_setopt(Curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(Curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
    curl_easy_setopt(Curl, CURLOPT_XFERINFODATA, Context);

    // Debugging bs
#if !UE_BUILD_SHIPPING
    curl_easy_setopt(Curl, CURLOPT_VERBOSE, 1L);
#endif
}

// --- FTP UPLOAD & DOWNLOAD FUNCTIONS --- 

void UFTPManager::CancelTransfer(FFTPTransferHandle Handle)
{
    Handle.Cancel();
}

FFTPTransferHandle UFTPManager::UploadFile(FString URL, FString User, FString Password, FString LocalFilePath, FOnFTPProgress OnProgress, FOnFTPComplete OnComplete)
{
    FFTPTransferHandle Handle;
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag = Handle.CancelFlag;

    Async(EAsyncExecution::Thread, [URL, User, Password, LocalFilePath, OnProgress, OnComplete, CancelFlag]()
        {
            bool bSuccess = false;

            if (FPaths::FileExists(LocalFilePath))
            {
                FString RawFileName = FPaths::GetCleanFilename(LocalFilePath);
                FString EncodedFileName = FGenericPlatformHttp::UrlEncode(RawFileName);

                FString FinalURL = URL;
                if (!FinalURL.EndsWith("/")) FinalURL += "/";
                FinalURL += EncodedFileName;

                CURL* Curl = curl_easy_init();
                if (Curl)
                {
                    TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*LocalFilePath));
                    if (Reader)
                    {
                        FCurlContext Context;
                        Context.FileArchive = Reader.Get();
                        Context.CancelFlag = CancelFlag;
                        Context.ProgressDelegate = OnProgress;
                        Context.bIsUpload = true; 

                        SetupCurlOptions(Curl, FinalURL, User, Password, &Context);

                        curl_easy_setopt(Curl, CURLOPT_UPLOAD, 1L);
                        curl_easy_setopt(Curl, CURLOPT_READFUNCTION, ReadCallback);
                        curl_easy_setopt(Curl, CURLOPT_READDATA, &Context);
                        curl_easy_setopt(Curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)Reader->TotalSize());

                        CURLcode Res = curl_easy_perform(Curl);

                        if (Res == CURLE_OK) bSuccess = true;
                        else UE_LOG(LogTemp, Error, TEXT("FTP Upload Error: %hs"), curl_easy_strerror(Res));

                        Reader->Close();
                    }
                    curl_easy_cleanup(Curl);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("FTP: Local File Missing: %s"), *LocalFilePath);
            }

            Async(EAsyncExecution::TaskGraph, [OnComplete, bSuccess]() { OnComplete.ExecuteIfBound(bSuccess); });
        });

    return Handle;
}

FFTPTransferHandle UFTPManager::DownloadFile(FString URL, FString User, FString Password, FString SaveDirectory, FOnFTPProgress OnProgress, FOnFTPComplete OnComplete)
{
    FFTPTransferHandle Handle;
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag = Handle.CancelFlag;

    Async(EAsyncExecution::Thread, [URL, User, Password, SaveDirectory, OnProgress, OnComplete, CancelFlag]()
        {
            bool bSuccess = false;
            CURL* Curl = curl_easy_init();

            if (Curl)
            {
                FString FileName = FPaths::GetCleanFilename(URL);

                FileName = FGenericPlatformHttp::UrlDecode(FileName);

                FString FullSavePath = FPaths::Combine(SaveDirectory, FileName);

                IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
                if (!PlatformFile.DirectoryExists(*SaveDirectory))
                {
                    PlatformFile.CreateDirectoryTree(*SaveDirectory);
                }

                TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*FullSavePath));
                if (Writer)
                {
                    FCurlContext Context;
                    Context.FileArchive = Writer.Get();
                    Context.CancelFlag = CancelFlag;
                    Context.ProgressDelegate = OnProgress;
                    Context.bIsUpload = false; 

                    SetupCurlOptions(Curl, URL, User, Password, &Context);

                    curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                    curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Context);

                    CURLcode Res = curl_easy_perform(Curl);

                    if (Res == CURLE_OK)
                    {
                        bSuccess = true;
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("FTP Download Failed: %hs"), curl_easy_strerror(Res));
                    }

                    Writer->Close();


                    if (!bSuccess || (CancelFlag.IsValid() && CancelFlag->load()))
                    {
                        UE_LOG(LogTemp, Log, TEXT("FTP: Cleaning up partial file %s"), *FullSavePath);
                        PlatformFile.DeleteFile(*FullSavePath);
                        bSuccess = false; 
                    }
                }
                curl_easy_cleanup(Curl);
            }

            Async(EAsyncExecution::TaskGraph, [OnComplete, bSuccess]() { OnComplete.ExecuteIfBound(bSuccess); });
        });

    return Handle;
}

// --- HTTPS SERVER OPERATIONS --- 

//Comming soon, i just need server for testing since my free hosting shity host is not allowing automated upload without a browser.
