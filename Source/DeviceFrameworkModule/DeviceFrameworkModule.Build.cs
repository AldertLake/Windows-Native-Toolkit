using UnrealBuildTool;

public class DeviceFrameworkModule : ModuleRules
{
    public DeviceFrameworkModule(ReadOnlyTargetRules Target) : base(Target)
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
                "Winmm.lib",    // AudioSystemLibrary
                "XInput.lib",   // Hardware input polling
                "Advapi32.lib"  // Hardware framework BPLibrary (registry or sensor access)
            });
        }
    }
}
