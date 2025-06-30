/************************************************************************************
 *                                                                                  *
 * Copyright (c) 2025 AldertLake. All Rights Reserved.                              *
 * GitHub: https://github.com/AldertLake/Windows-Native-Toolkit                    *
 *                                                                                  *
 ************************************************************************************/

using UnrealBuildTool;

public class Windows_Native_Toolkit : ModuleRules
{
    public Windows_Native_Toolkit(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Essential Unreal Engine module dependencies
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",    // For UInputDetectionLibrary
            "Slate",       // For UFilePickerLibrary (FSlateApplication)
            "SlateCore"    // Required by Slate
        });

        // Windows-specific configuration
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // System libraries required by the plugin for various Windows APIs
            PublicSystemLibraries.AddRange(new string[]
            {
                "Winmm.lib",      // Audio device management (UAudioDeviceManager)
                "XInput.lib",     // Gamepad detection (UInputDetectionLibrary, USystemInfoBPLibrary)
                "Shell32.lib",    // File operations, notifications, app launching (UFileSystemBlueprintLibrary, UToastNotificationLibrary, UOpenApps)
                "Comctl32.lib",   // Task dialogs (UMessageBoxWindows)
                "Advapi32.lib",   // Registry access (USystemInfoBPLibrary)
                "dxgi.lib",       // GPU information (USystemInfoBPLibrary)
                "Iphlpapi.lib",   // Network adapters (UNetworkUtilities)
                "Ws2_32.lib",     // Winsock (UNetworkUtilities)
                "Wlanapi.lib"     // WLAN APIs (UNetworkUtilities)
            });
        }
    }
}