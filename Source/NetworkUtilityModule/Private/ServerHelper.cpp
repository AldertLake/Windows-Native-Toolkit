// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "ServerHelper.h"

#include "Async/Async.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/SecureHash.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "curl/curl.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if PLATFORM_WINDOWS
struct FCurlContext
{
    FArchive* FileArchive = nullptr;
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag;
    FOnTransferProgress ProgressDelegate;
    double LastBroadcastTime = 0.0;
    bool bIsUpload = false;
};

struct FCurlEasyHandle
{
    CURL* Handle = nullptr;

    FCurlEasyHandle()
        : Handle(curl_easy_init())
    {
    }

    ~FCurlEasyHandle()
    {
        if (Handle)
        {
            curl_easy_cleanup(Handle);
        }
    }

    CURL* Get() const
    {
        return Handle;
    }

    bool IsValid() const
    {
        return Handle != nullptr;
    }
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
        if (Context->FileArchive->IsError())
        {
            return 0;
        }
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
        if (Context->FileArchive->IsError())
        {
            return CURL_READFUNC_ABORT;
        }
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
            FOnTransferProgress DelegateCopy = Context->ProgressDelegate;

            AsyncTask(ENamedThreads::GameThread, [DelegateCopy, Percent]()
            {
                DelegateCopy.ExecuteIfBound(Percent);
            });
        }
    }
    return 0;
}

static void SetupCurlOptions(CURL* Curl, const FString& URL, const FString& User, const FString& Password, FCurlContext* Context)
{
    curl_easy_setopt(Curl, CURLOPT_URL, TCHAR_TO_UTF8(*URL));

    if (!User.IsEmpty())
    {
        FString Auth = Password.IsEmpty() ? User : (User + TEXT(":") + Password);
        curl_easy_setopt(Curl, CURLOPT_USERPWD, TCHAR_TO_UTF8(*Auth));
    }

    curl_easy_setopt(Curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(Curl, CURLOPT_TIMEOUT, 600L);

    curl_easy_setopt(Curl, CURLOPT_FTP_USE_EPSV, 0L);
    curl_easy_setopt(Curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
    curl_easy_setopt(Curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 2L);
    curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYHOST, 2L);

#if defined(CURLSSLOPT_NATIVE_CA)
    curl_easy_setopt(Curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif

    curl_easy_setopt(Curl, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(Curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(Curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
    curl_easy_setopt(Curl, CURLOPT_XFERINFODATA, Context);

#if !UE_BUILD_SHIPPING
    curl_easy_setopt(Curl, CURLOPT_VERBOSE, 1L);
#endif
}
#endif

static FString ResolveDownloadFileName(const FString& URL, const FString& FileNameOverride = FString())
{
    FString FileName = FileNameOverride.TrimStartAndEnd();
    if (FileName.IsEmpty())
    {
        FileName = FPaths::GetCleanFilename(URL);
        int32 QueryIndex = INDEX_NONE;
        if (FileName.FindChar(TEXT('?'), QueryIndex))
        {
            FileName = FileName.Left(QueryIndex);
        }
        FileName = FGenericPlatformHttp::UrlDecode(FileName);
    }

    return FPaths::GetCleanFilename(FileName);
}

static bool EnsureDirectoryExists(const FString& DirectoryPath, const TCHAR* Context)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (PlatformFile.DirectoryExists(*DirectoryPath))
    {
        return true;
    }

    if (PlatformFile.CreateDirectoryTree(*DirectoryPath))
    {
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("Error: %s failed because the destination directory could not be created: %s"), Context, *DirectoryPath);
    return false;
}

void UServerHelper::CancelTransfer(FNetworkTransferHandle Handle)
{
    Handle.Cancel();
}

FNetworkTransferHandle UServerHelper::UploadFileFTP(FString URL, FString User, FString Password, FString LocalFilePath, FOnTransferProgress OnProgress, FOnTransferComplete OnComplete)
{
    FNetworkTransferHandle Handle;
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag = Handle.CancelFlag;

    Async(EAsyncExecution::Thread, [URL, User, Password, LocalFilePath, OnProgress, OnComplete, CancelFlag]()
    {
        bool bSuccess = false;

#if PLATFORM_WINDOWS
        if (FPaths::FileExists(LocalFilePath))
        {
            FString RawFileName = FPaths::GetCleanFilename(LocalFilePath);
            FString EncodedFileName = FGenericPlatformHttp::UrlEncode(RawFileName);

            FString FinalURL = URL;
            if (!FinalURL.EndsWith("/")) FinalURL += "/";
            FinalURL += EncodedFileName;

            FCurlEasyHandle CurlPtr;
            if (CurlPtr.IsValid())
            {
                TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*LocalFilePath));
                if (Reader)
                {
                    FCurlContext Context;
                    Context.FileArchive = Reader.Get();
                    Context.CancelFlag = CancelFlag;
                    Context.ProgressDelegate = OnProgress;
                    Context.bIsUpload = true;

                    SetupCurlOptions(CurlPtr.Get(), FinalURL, User, Password, &Context);

                    curl_easy_setopt(CurlPtr.Get(), CURLOPT_UPLOAD, 1L);
                    curl_easy_setopt(CurlPtr.Get(), CURLOPT_READFUNCTION, ReadCallback);
                    curl_easy_setopt(CurlPtr.Get(), CURLOPT_READDATA, &Context);
                    curl_easy_setopt(CurlPtr.Get(), CURLOPT_INFILESIZE_LARGE, (curl_off_t)Reader->TotalSize());

                    CURLcode Res = curl_easy_perform(CurlPtr.Get());

                    if (Res == CURLE_OK) bSuccess = true;
                    else UE_LOG(LogTemp, Error, TEXT("FTP Upload Error: %hs"), curl_easy_strerror(Res));

                    Reader->Close();
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("FTP: Failed to open local file for reading: %s"), *LocalFilePath);
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("FTP: Local File Missing: %s"), *LocalFilePath);
        }
#else
        UE_LOG(LogTemp, Error, TEXT("FTP operations are currently only fully supported on Windows."));
#endif

        AsyncTask(ENamedThreads::GameThread, [OnComplete, bSuccess]() { OnComplete.ExecuteIfBound(bSuccess); });
    });

    return Handle;
}

FNetworkTransferHandle UServerHelper::DownloadFileFTP(FString URL, FString User, FString Password, FString SaveDirectory, FOnTransferProgress OnProgress, FOnTransferComplete OnComplete)
{
    FNetworkTransferHandle Handle;
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag = Handle.CancelFlag;

    Async(EAsyncExecution::Thread, [URL, User, Password, SaveDirectory, OnProgress, OnComplete, CancelFlag]()
    {
        bool bSuccess = false;

#if PLATFORM_WINDOWS
        FCurlEasyHandle CurlPtr;
        if (CurlPtr.IsValid())
        {
            const FString FileName = ResolveDownloadFileName(URL);
            if (FileName.IsEmpty())
            {
                UE_LOG(LogTemp, Error, TEXT("Error: Download File Using FTP failed because the file name could not be resolved from the URL."));
                AsyncTask(ENamedThreads::GameThread, [OnComplete]() { OnComplete.ExecuteIfBound(false); });
                return;
            }

            FString SanitizedURL = URL.TrimStartAndEnd();
            SanitizedURL.ReplaceInline(TEXT(" "), TEXT("%20"));

            FString FullSavePath = FPaths::Combine(SaveDirectory, FileName);

            if (!EnsureDirectoryExists(SaveDirectory, TEXT("Download File Using FTP")))
            {
                AsyncTask(ENamedThreads::GameThread, [OnComplete]() { OnComplete.ExecuteIfBound(false); });
                return;
            }

            IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
            TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*FullSavePath));
            if (Writer)
            {
                FCurlContext Context;
                Context.FileArchive = Writer.Get();
                Context.CancelFlag = CancelFlag;
                Context.ProgressDelegate = OnProgress;
                Context.bIsUpload = false;

                SetupCurlOptions(CurlPtr.Get(), SanitizedURL, User, Password, &Context);

                curl_easy_setopt(CurlPtr.Get(), CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(CurlPtr.Get(), CURLOPT_WRITEDATA, &Context);

                CURLcode Res = curl_easy_perform(CurlPtr.Get());

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
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Error: Download File Using FTP failed because output file could not be created on disk: %s"), *FullSavePath);
            }
        }
#else
        UE_LOG(LogTemp, Error, TEXT("FTP operations are currently only fully supported on Windows."));
#endif

        AsyncTask(ENamedThreads::GameThread, [OnComplete, bSuccess]() { OnComplete.ExecuteIfBound(bSuccess); });
    });

    return Handle;
}

FNetworkTransferHandle UServerHelper::DownloadFileHTTP(FString URL, FString SaveDirectory, FOnTransferProgress OnProgress, FOnTransferComplete OnComplete)
{
    FNetworkTransferHandle Handle;
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag = Handle.CancelFlag;

    const FString FileName = ResolveDownloadFileName(URL);
    if (FileName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Error: Download File (HTTP/HTTPS) failed because the file name could not be resolved from the URL."));
        OnComplete.ExecuteIfBound(false);
        return Handle;
    }

    if (!EnsureDirectoryExists(SaveDirectory, TEXT("Download File (HTTP/HTTPS)")))
    {
        OnComplete.ExecuteIfBound(false);
        return Handle;
    }

    const FString FullSavePath = FPaths::Combine(SaveDirectory, FileName);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(URL);
    Request->SetVerb("GET");

    Request->OnRequestProgress64().BindLambda(
        [OnProgress, CancelFlag](FHttpRequestPtr HttpRequest, uint64 BytesSent, uint64 BytesReceived)
    {
        if (CancelFlag.IsValid() && CancelFlag->load())
        {
            HttpRequest->CancelRequest();
            return;
        }

        if (HttpRequest->GetResponse().IsValid())
        {
            int32 TotalLength = HttpRequest->GetResponse()->GetContentLength();
            if (TotalLength > 0)
            {
                float Percent = (float)BytesReceived / (float)TotalLength;
                AsyncTask(ENamedThreads::GameThread, [OnProgress, Percent]()
                {
                    OnProgress.ExecuteIfBound(Percent);
                });
            }
        }
    });

    Request->OnProcessRequestComplete().BindLambda(
        [FullSavePath, OnComplete, CancelFlag](FHttpRequestPtr HttpRequest, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        bool bSuccess = false;

        if (CancelFlag.IsValid() && CancelFlag->load())
        {
        }
        else if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
        {
            TArray<uint8> ResponseData = Response->GetContent();
            if (FFileHelper::SaveArrayToFile(ResponseData, *FullSavePath))
            {
                bSuccess = true;
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Error: Download File (HTTP/HTTPS) failed because the downloaded file could not be written to disk: %s"), *FullSavePath);
            }
        }

        AsyncTask(ENamedThreads::GameThread, [OnComplete, bSuccess]()
        {
            OnComplete.ExecuteIfBound(bSuccess);
        });
    });

    Request->ProcessRequest();

    return Handle;
}

FNetworkTransferHandle UServerHelper::DownloadAdvanced(FString URL, FString SaveDirectory, FString FileNameOverride, bool bOverwrite, bool bDeletePartialOnFail, FOnTransferBytes OnProgress, FOnTransferResult OnComplete)
{
    FNetworkTransferHandle Handle;
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag = Handle.CancelFlag;

    FNetworkTransferResult EarlyResult;
    if (URL.TrimStartAndEnd().IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Error: Download Advanced failed because URL is empty."));
        OnComplete.ExecuteIfBound(EarlyResult);
        return Handle;
    }

    if (SaveDirectory.TrimStartAndEnd().IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Error: Download Advanced failed because save directory is empty."));
        OnComplete.ExecuteIfBound(EarlyResult);
        return Handle;
    }

    FString FileName = ResolveDownloadFileName(URL, FileNameOverride);
    if (FileName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Error: Download Advanced failed because no valid output filename could be resolved."));
        OnComplete.ExecuteIfBound(EarlyResult);
        return Handle;
    }

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!EnsureDirectoryExists(SaveDirectory, TEXT("Download Advanced")))
    {
        OnComplete.ExecuteIfBound(EarlyResult);
        return Handle;
    }

    const FString FullSavePath = FPaths::Combine(SaveDirectory, FileName);
    if (PlatformFile.FileExists(*FullSavePath) && !bOverwrite)
    {
        EarlyResult.SavedFilePath = FullSavePath;
        UE_LOG(LogTemp, Error, TEXT("Error: Download Advanced failed because the destination file already exists."));
        OnComplete.ExecuteIfBound(EarlyResult);
        return Handle;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(URL);
    Request->SetVerb(TEXT("GET"));

    Request->OnRequestProgress64().BindLambda(
        [OnProgress, CancelFlag](FHttpRequestPtr HttpRequest, uint64 BytesSent, uint64 BytesReceived)
        {
            if (CancelFlag.IsValid() && CancelFlag->load())
            {
                HttpRequest->CancelRequest();
                return;
            }

            int64 TotalBytes = 0;
            if (HttpRequest->GetResponse().IsValid())
            {
                TotalBytes = HttpRequest->GetResponse()->GetContentLength();
            }
            AsyncTask(ENamedThreads::GameThread, [OnProgress, BytesReceived, TotalBytes]()
            {
                OnProgress.ExecuteIfBound(static_cast<int64>(BytesReceived), TotalBytes);
            });
        });

    Request->OnProcessRequestComplete().BindLambda(
        [FullSavePath, bDeletePartialOnFail, OnComplete, CancelFlag](FHttpRequestPtr HttpRequest, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            FNetworkTransferResult Result;
            Result.SavedFilePath = FullSavePath;

            if (CancelFlag.IsValid() && CancelFlag->load())
            {
                UE_LOG(LogTemp, Error, TEXT("Error: Download Advanced transfer was canceled."));
            }
            else if (!bWasSuccessful || !Response.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Error: Download Advanced HTTP request failed."));
            }
            else
            {
                Result.StatusCode = Response->GetResponseCode();
                Result.TotalBytes = Response->GetContentLength();
                if (EHttpResponseCodes::IsOk(Result.StatusCode))
                {
                    const TArray<uint8>& Data = Response->GetContent();
                    Result.BytesTransferred = Data.Num();
                    Result.SHA1 = FSHA1::HashBuffer(Data.GetData(), Data.Num()).ToString();
                    if (FFileHelper::SaveArrayToFile(Data, *FullSavePath))
                    {
                        Result.bSuccess = true;
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Error: Download Advanced failed to save the downloaded file."));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Error: Download Advanced received HTTP status code %d."), Result.StatusCode);
                }
            }

            if (!Result.bSuccess && bDeletePartialOnFail)
            {
                FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*FullSavePath);
            }

            AsyncTask(ENamedThreads::GameThread, [OnComplete, Result]()
            {
                OnComplete.ExecuteIfBound(Result);
            });
        });

    Request->ProcessRequest();
    return Handle;
}
