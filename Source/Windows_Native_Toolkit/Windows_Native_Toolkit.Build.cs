/************************************************************************************
 *																					*
 * Copyright (c) 2025 AldertLake. All Rights Reserved.								*
 * GitHub:	https://github.com/AldertLake/Windows-Native-Toolkit					*
 *																					*
 ************************************************************************************/

using System.IO;
using UnrealBuildTool;

public class Windows_Native_Toolkit : ModuleRules
{
    public Windows_Native_Toolkit(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Include paths
        PublicIncludePaths.AddRange(new string[] { });
        PrivateIncludePaths.AddRange(new string[] { });
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/Core/Public/Windows"));

        // Dependency modules
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Slate",
            "SlateCore",
            "DesktopPlatform",
            "Networking",
            "Sockets",
            // GPU VRAM dependencies
            "D3D11RHI",
            "D3D12RHI",
            "DX12"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore"
        });

        // Dynamic modules
        DynamicallyLoadedModuleNames.AddRange(new string[] { });

        // Windows-specific configuration
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // System libraries
            PublicSystemLibraries.AddRange(new string[]
            {
                "dxgi.lib",
                "d3d11.lib",
                "d3d12.lib",
                "XInput.lib",
                "Advapi32.lib",
                "wbemuuid.lib",  
                "oleaut32.lib",  
                "Shell32.lib", 
            });

            // Include paths
            PublicIncludePaths.AddRange(new string[]
            {
                Path.Combine(EngineDirectory, "Source/Runtime/Windows/D3D11RHI/Public"),
                Path.Combine(EngineDirectory, "Source/Runtime/D3D12RHI/Public"),
            });

            // Definitions
            PublicDefinitions.AddRange(new string[]
            {
                "WITH_DX12=1",
                "D3D12_CORE=1",
                "WIN32_LEAN_AND_MEAN",
                "NOMINMAX",
                "NTDDI_VERSION=NTDDI_WIN10",
                "_WIN32_WINNT=0x0A00"
            });
        }

        // Additional libraries
        PublicAdditionalLibraries.AddRange(new string[]
        {

        });
    }
}