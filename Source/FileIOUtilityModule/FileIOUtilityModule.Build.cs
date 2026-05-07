// -----------------------------------------------------
// Copyright   (c) 2025 AldertLake. All Rights Reserved.
// GitHub:     https://github.com/AldertLake/
// Discord:    https://discord.gg/QpPPfh6WVn
// -----------------------------------------------------

using System.IO;
using UnrealBuildTool;

public class FileIOUtilityModule : ModuleRules
{
    public FileIOUtilityModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore"
        });


        if (File.Exists(Path.Combine(EngineDirectory, "Source", "Runtime", "AES", "AES.Build.cs")))
        {
            PrivateDependencyModuleNames.Add("AES");
        }
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new string[]
            {
                "Shell32.lib"
            });
        }
    }
}


