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
            "Engine"
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Networking",
            "Sockets",
            "HTTP",
            "Json" 
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "libcurl",
            "OpenSSL"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
           
        });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new string[]
            {
                "iphlpapi.lib",      // Network Adapter Info (GetAdaptersAddresses)
                "ws2_32.lib",        // Winsock 2 (Sockets, IP conversion)
                "wlanapi.lib",       // Wi-Fi Specifics
                "wininet.lib",       // Internet Extensions
                "setupapi.lib",      // Device Setup (Bluetooth/Hardware IDs)
                "BluetoothApis.lib"  // Bluetooth
            });

            AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "zlib");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "nghttp2");
        }
    }
}