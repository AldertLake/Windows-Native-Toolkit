// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "DeviceFrameworkModule.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <objbase.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace WNT_Private
{
#if PLATFORM_WINDOWS
    /** RAII wrapper for COM initialization that correctly handles refcount semantics. */
    struct FScopedComInit
    {
        HRESULT Result = E_FAIL;
        bool bNeedsUninitialize = false;

        FScopedComInit()
        {
            Result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            // Only uninitialize when we performed a *new* initialization (S_OK).
            // S_FALSE means COM was already initialized in the same mode — calling
            // CoUninitialize here would decrement the refcount and potentially tear
            // down COM from under another caller on this thread.
            bNeedsUninitialize = (Result == S_OK);
        }

        ~FScopedComInit()
        {
            if (bNeedsUninitialize)
            {
                CoUninitialize();
            }
        }

        /** Returns true when COM is available on this thread (either freshly initialized or already present). */
        bool IsUsable() const
        {
            return SUCCEEDED(Result) || Result == RPC_E_CHANGED_MODE;
        }

        FScopedComInit(const FScopedComInit&) = delete;
        FScopedComInit& operator=(const FScopedComInit&) = delete;
    };
#endif

    /** Unified error logger for all WNT subsystems. */
    static void LogWntError(const TCHAR* Context, const FString& Message)
    {
        UE_LOG(LogWNT, Error, TEXT("Error: %s failed. %s"), Context, *Message);
    }
}
