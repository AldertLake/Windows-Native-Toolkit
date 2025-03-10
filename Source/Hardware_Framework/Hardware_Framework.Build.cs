

using System.IO;
using UnrealBuildTool;

public class Hardware_Framework : ModuleRules
{
	public Hardware_Framework(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
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
                //These To Get GPU Vram 
                "D3D11RHI",
				"D3D12RHI",
				"DX12"
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		// Link against required Windows libraries
		PublicAdditionalLibraries.Add("wbemuuid.lib"); // WMI library
		PublicAdditionalLibraries.Add("oleaut32.lib"); // For SysAllocString/SysFreeString



        /////// These Are Only Windows Difinitions /////////////////////////////////////////////////////////////////////////WINDOWS//////////////



        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(
                new string[]
                {
                    "dxgi.lib",
                    "d3d11.lib",
                    "d3d12.lib",
                    "XInput.lib",
                    "Advapi32.lib",
                }
            );

            PublicIncludePaths.AddRange(
                new string[] {
                    Path.Combine(EngineDirectory, "Source/Runtime/Windows/D3D11RHI/Public"),
                    Path.Combine(EngineDirectory, "Source/Runtime/D3D12RHI/Public"),
                }
            );

            // Fixed: Use PublicDefinitions instead of Definitions
            PublicDefinitions.Add("WITH_DX12=1");
            PublicDefinitions.Add("D3D12_CORE=1");
        }

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // Fixed: Use PublicDefinitions for platform macros
            PublicDefinitions.Add("WIN32_LEAN_AND_MEAN");
            PublicDefinitions.Add("NOMINMAX");
            PublicDefinitions.Add("NTDDI_VERSION=NTDDI_WIN10");
            PublicDefinitions.Add("_WIN32_WINNT=0x0A00");
        }


    }
}
