// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

using UnrealBuildTool;

public class SystemUtilityModule : ModuleRules
{
    public SystemUtilityModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore"
        });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new string[]
            {
                "Shell32.lib",    // OpenApps, ToastNotificationLibrary
                "Comctl32.lib",   // MessageBoxWindows
                "Advapi32.lib",   // WindowsInfoBPLibrary
                "Winmm.lib",      // Audio fallbacks
                "XInput.lib",     // May be used in message-based user notifications
                "dxgi.lib",      // GPU info (part of WindowsInfoBPLibrary)
            });
        }
    }
}