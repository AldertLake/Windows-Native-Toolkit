// ---------------------------------------------------
// Copyright (c) 2025 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Support:  https://ko-fi.com/aldertlake
// ---------------------------------------------------

using UnrealBuildTool;
//using System.IO;

public class NetworkUtilityModule : ModuleRules
{
    public NetworkUtilityModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Combined all module dependencies into one array
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "Json",
            "Networking", "Sockets", "HTTP", "libcurl", "OpenSSL"
        });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            bEnableExceptions = true;

            // Combined all Windows libraries into one array
            PublicSystemLibraries.AddRange(new string[]
            {
                "iphlpapi.lib", "ws2_32.lib", "wininet.lib",     
                "wlanapi.lib", "setupapi.lib", "BluetoothApis.lib", 
                "WindowsApp.lib", "Shlwapi.lib", "RuntimeObject.lib" 
            });

            // Combined ThirdParty Static Dependencies into one line
            AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL", "zlib", "nghttp2");
        }
    }
}