// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

using UnrealBuildTool;
using System.IO;

public class NetworkUtilityModule : ModuleRules
{
    public NetworkUtilityModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Networking", 
            "Sockets"  
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new string[]
            {
                "iphlpapi.lib",   // Network adapter APIs
                "ws2_32.lib",     // Winsock
                "wlanapi.lib",    // WLAN (Wi-Fi)
                "wininet.lib",    // WinINet
                "setupapi.lib",   //For BT Class
                "BluetoothApis.lib" //Same bt
            });
        }
    }
}
