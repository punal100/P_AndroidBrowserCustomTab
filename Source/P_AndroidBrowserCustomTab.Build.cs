/*
* @Author: Punal Manalan
* @Description: Android Browser Custom Tab Plugin.
* @Date: 09/10/2025
*/

using UnrealBuildTool;
using System.IO;

// Build configuration for the ViewShed Analysis runtime module
public class P_AndroidBrowserCustomTab : ModuleRules
{
	public P_AndroidBrowserCustomTab(ReadOnlyTargetRules Target) : base(Target)
	{
		// Use explicit precompiled headers for better performance
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Public include paths - these are exposed to other modules
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);

		// Private include paths - internal to this module only
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);

		// Public dependencies - modules that are required by public headers
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"RenderCore",
				"RHI",
				"ProceduralMeshComponent",
				"Json",
				"JsonUtilities"
				// ... add other public dependencies that you statically link with here ...
			}
			);

		// Private dependencies - modules only needed for implementation
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);

		// Dynamically loaded modules - loaded at runtime when needed
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			string PluginPath = Path.Combine(ModuleDirectory, "Android", "ChromeCustomTabs_UPL.xml");
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", PluginPath);
		}
	}
}
