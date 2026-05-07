// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

using UnrealBuildTool;

public class NetworkUtilityModule : ModuleRules
{
    public NetworkUtilityModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "Json",
            "Networking", "Sockets", "HTTP"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "FileIOUtilityModule"
        });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new string[]
            {
                "iphlpapi.lib", "ws2_32.lib", "wininet.lib",
                "wlanapi.lib", "setupapi.lib", "BluetoothApis.lib",
                "WindowsApp.lib", "Shlwapi.lib", "RuntimeObject.lib",
                "Ole32.lib", "OleAut32.lib"
            });

            AddEngineThirdPartyPrivateStaticDependencies(Target, "libcurl", "OpenSSL", "zlib", "nghttp2");
        }
    }
}


