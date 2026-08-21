// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#include "ToastNotificationLibrary.h"
#include "SystemUtilityModule.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "ImageUtils.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include "Windows/HideWindowsPlatformTypes.h"


static const UINT TRAY_ICON_ID = 5500;
static bool bIsCleanupRegistered = false;

/** Cached HWND from the first NIM_ADD call so CleanupTrayIcon can reliably
    remove the tray icon even after the game viewport has been destroyed. */
static HWND CachedTrayIconHwnd = nullptr;

/** Maps a cache key (hash) to a generated HICON. Prevents redundant GDI object
    creation when the same source is used for repeated notifications. */
static TMap<uint32, HICON> CachedCustomIcons;

/** Target pixel size for generated notification icons. */
static constexpr int32 IconTargetSize = 64;

/** Maximum number of cached icons before the oldest entries are evicted. */
static constexpr int32 MaxCachedIcons = 32;

/** Maximum file size (bytes) we will attempt to decode as a notification icon. */
static constexpr int64 MaxImageFileSize = 16 * 1024 * 1024; // 16 MB


static void DestroyCachedIcons()
{
    for (const auto& Pair : CachedCustomIcons)
    {
        if (Pair.Value)
        {
            DestroyIcon(Pair.Value);
        }
    }
    CachedCustomIcons.Empty();
}

static void EvictOldestIfNeeded()
{
    if (CachedCustomIcons.Num() < MaxCachedIcons)
    {
        return;
    }

    TMap<uint32, HICON>::TIterator It = CachedCustomIcons.CreateIterator();
    if (It)
    {
        if (It.Value())
        {
            DestroyIcon(It.Value());
        }
        It.RemoveCurrent();
    }
}

static void CacheIcon(uint32 Key, HICON hIcon)
{
    EvictOldestIfNeeded();
    CachedCustomIcons.Add(Key, hIcon);
}


static void EnforceOpaquePixels(TArray<FColor>& Pixels)
{
    for (FColor& P : Pixels)
    {
        if (P.A < 255)
        {
            P.R = 0;
            P.G = 0;
            P.B = 0;
        }
        P.A = 255;
    }
}


static HICON CreateHIconFromPixels(const TArray<FColor>& SourcePixels, int32 Width, int32 Height)
{
    if (SourcePixels.Num() == 0 || Width <= 0 || Height <= 0)
    {
        return nullptr;
    }

    if (SourcePixels.Num() != Width * Height)
    {
        UE_LOG(LogWNT, Warning, TEXT("Pixel count (%d) does not match dimensions (%dx%d)."), SourcePixels.Num(), Width, Height);
        return nullptr;
    }

    TArray<FColor> ScaledPixels;
    ScaledPixels.SetNumZeroed(IconTargetSize * IconTargetSize);

    int32 DestX = 0, DestY = 0;
    int32 DestW = IconTargetSize, DestH = IconTargetSize;

    if (Width > Height)
    {
        DestH = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Height) / Width * IconTargetSize));
        DestY = (IconTargetSize - DestH) / 2;
    }
    else if (Height > Width)
    {
        DestW = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Width) / Height * IconTargetSize));
        DestX = (IconTargetSize - DestW) / 2;
    }

    TArray<FColor> ResizedPixels;
    FImageUtils::ImageResize(Width, Height, SourcePixels, DestW, DestH, ResizedPixels, false);

    if (ResizedPixels.Num() != DestW * DestH)
    {
        UE_LOG(LogWNT, Warning, TEXT("ImageResize produced unexpected pixel count."));
        return nullptr;
    }

    for (int32 Y = 0; Y < DestH; ++Y)
    {
        for (int32 X = 0; X < DestW; ++X)
        {
            ScaledPixels[(DestY + Y) * IconTargetSize + (DestX + X)] = ResizedPixels[Y * DestW + X];
        }
    }

    EnforceOpaquePixels(ScaledPixels);

    HICON hIcon = nullptr;
    HDC hDC = GetDC(NULL);
    if (!hDC)
    {
        UE_LOG(LogWNT, Warning, TEXT("GetDC(NULL) failed."));
        return nullptr;
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = IconTargetSize;
    bmi.bmiHeader.biHeight = -IconTargetSize; // Negative = top-down row order
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* Bits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &Bits, NULL, 0);

    if (!hBitmap || !Bits)
    {
        UE_LOG(LogWNT, Warning, TEXT("CreateDIBSection failed."));
        ReleaseDC(NULL, hDC);
        return nullptr;
    }

    FMemory::Memcpy(Bits, ScaledPixels.GetData(), IconTargetSize * IconTargetSize * sizeof(FColor));

    const int32 MaskStride = ((IconTargetSize + 31) / 32) * 4;
    const int32 MaskSize = MaskStride * IconTargetSize;
    TArray<uint8> ZeroedMask;
    ZeroedMask.AddZeroed(MaskSize);
    HBITMAP hMask = CreateBitmap(IconTargetSize, IconTargetSize, 1, 1, ZeroedMask.GetData());

    if (!hMask)
    {
        UE_LOG(LogWNT, Warning, TEXT("CreateBitmap for mask failed."));
        DeleteObject(hBitmap);
        ReleaseDC(NULL, hDC);
        return nullptr;
    }

    ICONINFO IconInfo = {0};
    IconInfo.fIcon = true;
    IconInfo.hbmMask = hMask;
    IconInfo.hbmColor = hBitmap;

    hIcon = CreateIconIndirect(&IconInfo);

    DeleteObject(hMask);
    DeleteObject(hBitmap);
    ReleaseDC(NULL, hDC);

    if (!hIcon)
    {
        UE_LOG(LogWNT, Warning, TEXT("CreateIconIndirect failed."));
    }

    return hIcon;
}


static HICON CreateHIconFromImage(const FString& ImagePath)
{
    if (ImagePath.IsEmpty())
    {
        UE_LOG(LogWNT, Warning, TEXT("Custom icon path is empty."));
        return nullptr;
    }

    if (!FPaths::FileExists(ImagePath))
    {
        UE_LOG(LogWNT, Warning, TEXT("Custom icon file does not exist: %s"), *ImagePath);
        return nullptr;
    }

    const uint32 PathHash = GetTypeHash(ImagePath);
    if (HICON* Cached = CachedCustomIcons.Find(PathHash))
    {
        return *Cached;
    }

    const int64 FileSize = IFileManager::Get().FileSize(*ImagePath);
    if (FileSize <= 0)
    {
        UE_LOG(LogWNT, Warning, TEXT("Custom icon file is empty or unreadable: %s"), *ImagePath);
        return nullptr;
    }
    if (FileSize > MaxImageFileSize)
    {
        UE_LOG(LogWNT, Warning, TEXT("Custom icon file exceeds %lld MB size limit: %s"), MaxImageFileSize / (1024 * 1024), *ImagePath);
        return nullptr;
    }

    TArray<uint8> RawFileData;
    if (!FFileHelper::LoadFileToArray(RawFileData, *ImagePath))
    {
        UE_LOG(LogWNT, Warning, TEXT("Failed to read image file: %s"), *ImagePath);
        return nullptr;
    }

    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    const EImageFormat Format = ImageWrapperModule.DetectImageFormat(RawFileData.GetData(), RawFileData.Num());
    if (Format == EImageFormat::Invalid)
    {
        UE_LOG(LogWNT, Warning, TEXT("Unsupported image format: %s"), *ImagePath);
        return nullptr;
    }

    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(Format);
    if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
    {
        UE_LOG(LogWNT, Warning, TEXT("Failed to decode image: %s"), *ImagePath);
        return nullptr;
    }

    TArray<uint8> UncompressedBGRA;
    if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
    {
        UE_LOG(LogWNT, Warning, TEXT("Failed to decompress image to BGRA: %s"), *ImagePath);
        return nullptr;
    }

    const int32 Width = ImageWrapper->GetWidth();
    const int32 Height = ImageWrapper->GetHeight();
    if (Width <= 0 || Height <= 0)
    {
        UE_LOG(LogWNT, Warning, TEXT("Image has invalid dimensions (%dx%d): %s"), Width, Height, *ImagePath);
        return nullptr;
    }

    const int32 ExpectedBytes = Width * Height * static_cast<int32>(sizeof(FColor));
    if (UncompressedBGRA.Num() < ExpectedBytes)
    {
        UE_LOG(LogWNT, Warning, TEXT("Decompressed data size mismatch for: %s"), *ImagePath);
        return nullptr;
    }

    TArray<FColor> PixelData;
    PixelData.SetNumUninitialized(Width * Height);
    FMemory::Memcpy(PixelData.GetData(), UncompressedBGRA.GetData(), ExpectedBytes);

    HICON hIcon = CreateHIconFromPixels(PixelData, Width, Height);
    if (hIcon)
    {
        CacheIcon(PathHash, hIcon);
    }
    return hIcon;
}


static HICON CreateHIconFromTexture(UTexture2D* Texture)
{
    if (!Texture || !IsValid(Texture))
    {
        UE_LOG(LogWNT, Warning, TEXT("Texture Reference is null or pending kill."));
        return nullptr;
    }

    const uint32 TexHash = GetTypeHash(Texture->GetPathName());
    if (HICON* Cached = CachedCustomIcons.Find(TexHash))
    {
        return *Cached;
    }

    int32 Width = 0, Height = 0;
    TArray<FColor> PixelData;

#if WITH_EDITORONLY_DATA

    if (!Texture->Source.IsValid())
    {
        UE_LOG(LogWNT, Warning, TEXT("Texture '%s' has no source data."), *Texture->GetName());
        return nullptr;
    }

    Width = Texture->Source.GetSizeX();
    Height = Texture->Source.GetSizeY();

    if (Width <= 0 || Height <= 0)
    {
        UE_LOG(LogWNT, Warning, TEXT("Texture '%s' has invalid dimensions (%dx%d)."), *Texture->GetName(), Width, Height);
        return nullptr;
    }

    const ETextureSourceFormat SourceFormat = Texture->Source.GetFormat();

    TArray64<uint8> RawSourceData;
    if (!Texture->Source.GetMipData(RawSourceData, 0))
    {
        UE_LOG(LogWNT, Warning, TEXT("Failed to read source mip data for '%s'."), *Texture->GetName());
        return nullptr;
    }

    if (RawSourceData.Num() == 0)
    {
        UE_LOG(LogWNT, Warning, TEXT("Source mip data is empty for '%s'."), *Texture->GetName());
        return nullptr;
    }

    PixelData.SetNumUninitialized(Width * Height);

    if (SourceFormat == TSF_BGRA8)
    {
        const int64 ExpectedBytes = static_cast<int64>(Width) * Height * sizeof(FColor);
        if (RawSourceData.Num() < ExpectedBytes)
        {
            UE_LOG(LogWNT, Warning, TEXT("Texture '%s' source data too small (%lld < %lld)."),
                *Texture->GetName(), RawSourceData.Num(), ExpectedBytes);
            return nullptr;
        }
        FMemory::Memcpy(PixelData.GetData(), RawSourceData.GetData(), Width * Height * sizeof(FColor));
    }
    else if (SourceFormat == TSF_G8)
    {
        const int64 ExpectedBytes = static_cast<int64>(Width) * Height;
        if (RawSourceData.Num() < ExpectedBytes)
        {
            UE_LOG(LogWNT, Warning, TEXT("Texture '%s' grayscale source data too small."), *Texture->GetName());
            return nullptr;
        }
        const uint8* Src = RawSourceData.GetData();
        for (int32 i = 0; i < Width * Height; ++i)
        {
            PixelData[i].R = PixelData[i].G = PixelData[i].B = Src[i];
            PixelData[i].A = 255;
        }
    }
    else
    {
        UE_LOG(LogWNT, Warning, TEXT("Texture '%s' uses unsupported source format (%d). Use a standard 8-bit BGRA texture."),
            *Texture->GetName(), static_cast<int32>(SourceFormat));
        return nullptr;
    }

#else
    FTexturePlatformData* PlatformData = Texture->GetPlatformData();
    if (!PlatformData || PlatformData->Mips.Num() == 0)
    {
        UE_LOG(LogWNT, Warning, TEXT("Texture '%s' has no platform data. Set CompressionSettings to UserInterface2D."), *Texture->GetName());
        return nullptr;
    }

    FTexture2DMipMap& Mip = PlatformData->Mips[0];
    Width = Mip.SizeX;
    Height = Mip.SizeY;

    if (Width <= 0 || Height <= 0)
    {
        UE_LOG(LogWNT, Warning, TEXT("Texture '%s' mip has invalid dimensions (%dx%d)."), *Texture->GetName(), Width, Height);
        return nullptr;
    }

    const void* RawData = Mip.BulkData.Lock(LOCK_READ_ONLY);
    if (!RawData)
    {
        UE_LOG(LogWNT, Warning, TEXT("Failed to lock bulk data for '%s'. Data may have been discarded."), *Texture->GetName());
        Mip.BulkData.Unlock();
        return nullptr;
    }

    const int64 ExpectedBytes = static_cast<int64>(Width) * Height * sizeof(FColor);
    const int64 BulkDataSize = Mip.BulkData.GetBulkDataSize();
    if (BulkDataSize < ExpectedBytes)
    {
        UE_LOG(LogWNT, Warning, TEXT("Texture '%s' bulk data too small (%lld < %lld). Set CompressionSettings to UserInterface2D."),
            *Texture->GetName(), BulkDataSize, ExpectedBytes);
        Mip.BulkData.Unlock();
        return nullptr;
    }

    PixelData.SetNumUninitialized(Width * Height);
    FMemory::Memcpy(PixelData.GetData(), RawData, static_cast<int32>(ExpectedBytes));
    Mip.BulkData.Unlock();
#endif

    HICON hIcon = CreateHIconFromPixels(PixelData, Width, Height);
    if (hIcon)
    {
        CacheIcon(TexHash, hIcon);
    }
    return hIcon;
}
#endif


void UToastNotificationLibrary::ShowToastNotification(const FString& Title, const FString& Message, EToastIconType IconType, const FString& CustomImagePath, UTexture2D* TextureReference)
{
#if PLATFORM_WINDOWS

    if (!GEngine || !GEngine->GameViewport) return;

    TSharedPtr<SWindow> WindowPtr = GEngine->GameViewport->GetWindow();
    if (!WindowPtr.IsValid()) return;

    TSharedPtr<FGenericWindow> NativeWindow = WindowPtr->GetNativeWindow();
    if (!NativeWindow.IsValid()) return;

    HWND ParentWindow = (HWND)NativeWindow->GetOSWindowHandle();
    if (!ParentWindow) return;

    NOTIFYICONDATAW Nid = { 0 };
    Nid.cbSize = sizeof(NOTIFYICONDATAW);
    Nid.hWnd = ParentWindow;
    Nid.uID = TRAY_ICON_ID;

    Nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_INFO;
    Nid.uCallbackMessage = WM_USER + 300;

    HICON GameIcon = (HICON)GetClassLongPtr(ParentWindow, GCLP_HICON);
    Nid.hIcon = GameIcon ? GameIcon : LoadIcon(NULL, IDI_APPLICATION);

    const TCHAR* ProjName = FApp::GetProjectName();
    if (ProjName)
    {
        StringCchCopyW(Nid.szTip, ARRAYSIZE(Nid.szTip), (LPCWSTR)ProjName);
    }
    else
    {
        StringCchCopyW(Nid.szTip, ARRAYSIZE(Nid.szTip), L"Unreal Game");
    }

    StringCchCopyW(Nid.szInfoTitle, ARRAYSIZE(Nid.szInfoTitle), (LPCWSTR)*Title);
    StringCchCopyW(Nid.szInfo, ARRAYSIZE(Nid.szInfo), (LPCWSTR)*Message);

    if (IconType == EToastIconType::TextureReference || IconType == EToastIconType::ExternalImage)
    {
        HICON CustomHIcon = nullptr;

        if (IconType == EToastIconType::TextureReference)
        {
            CustomHIcon = CreateHIconFromTexture(TextureReference);
        }
        else
        {
            CustomHIcon = CreateHIconFromImage(CustomImagePath);
        }

        if (CustomHIcon)
        {
            Nid.hBalloonIcon = CustomHIcon;
            Nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON | NIIF_RESPECT_QUIET_TIME;
        }
        else
        {
            UE_LOG(LogWNT, Warning, TEXT("Custom icon generation failed. Falling back to standard Info icon."));
            Nid.dwInfoFlags = NIIF_INFO | NIIF_RESPECT_QUIET_TIME;
        }
    }
    else
    {
        switch (IconType)
        {
        case EToastIconType::Info:    Nid.dwInfoFlags = NIIF_INFO | NIIF_RESPECT_QUIET_TIME; break;
        case EToastIconType::Warning: Nid.dwInfoFlags = NIIF_WARNING | NIIF_RESPECT_QUIET_TIME; break;
        case EToastIconType::Error:   Nid.dwInfoFlags = NIIF_ERROR | NIIF_RESPECT_QUIET_TIME; break;
        default:                      Nid.dwInfoFlags = NIIF_NONE | NIIF_RESPECT_QUIET_TIME; break;
        }
    }

    BOOL bSuccess = Shell_NotifyIconW(NIM_MODIFY, &Nid);

    if (!bSuccess)
    {
        bSuccess = Shell_NotifyIconW(NIM_ADD, &Nid);

        if (bSuccess)
        {
            Nid.uVersion = NOTIFYICON_VERSION_4;
            Shell_NotifyIconW(NIM_SETVERSION, &Nid);

            CachedTrayIconHwnd = ParentWindow;

            if (!bIsCleanupRegistered)
            {
                FCoreDelegates::OnPreExit.AddStatic(&UToastNotificationLibrary::CleanupTrayIcon);
                bIsCleanupRegistered = true;
            }
        }
        else
        {
            UE_LOG(LogWNT, Warning, TEXT("Shell_NotifyIconW(NIM_ADD) failed."));
        }
    }
#endif
}


void UToastNotificationLibrary::InvalidateIconCache()
{
#if PLATFORM_WINDOWS
    const int32 Count = CachedCustomIcons.Num();
    DestroyCachedIcons();
    if (Count > 0)
    {
        UE_LOG(LogWNT, Log, TEXT("Invalidated %d cached notification icon(s)."), Count);
    }
#endif
}


void UToastNotificationLibrary::CleanupTrayIcon()
{
#if PLATFORM_WINDOWS
    NOTIFYICONDATAW Nid = { 0 };
    Nid.cbSize = sizeof(NOTIFYICONDATAW);
    Nid.uID = TRAY_ICON_ID;

    if (CachedTrayIconHwnd)
    {
        Nid.hWnd = CachedTrayIconHwnd;
    }
    else if (GEngine && GEngine->GameViewport)
    {
        TSharedPtr<SWindow> Win = GEngine->GameViewport->GetWindow();
        if (Win.IsValid() && Win->GetNativeWindow().IsValid())
        {
            Nid.hWnd = (HWND)Win->GetNativeWindow()->GetOSWindowHandle();
        }
    }

    Shell_NotifyIconW(NIM_DELETE, &Nid);
    CachedTrayIconHwnd = nullptr;

    DestroyCachedIcons();
#endif
}
