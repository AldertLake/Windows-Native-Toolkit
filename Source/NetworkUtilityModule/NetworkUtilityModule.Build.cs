// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

using UnrealBuildTool;

public class NetworkUtilityModule : ModuleRules
{
    public NetworkUtilityModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new string[]
            {
                "Iphlpapi.lib",    // NetworkAdapters
                "Ws2_32.lib",     // Winsock for networking
                "Wlanapi.lib",   // WLAN (WiFi scanning)
                "Wininet.lib"   // Internet functions (e.g. InternetCheckConnection)
            });
        }
    }
}
