// Copyright UEMaster. All Rights Reserved.

using UnrealBuildTool;

public class AssetOrganizer : ModuleRules
{
    public AssetOrganizer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "Slate",
                "SlateCore",
                "EditorStyle",
                "AssetRegistry",
                "AssetTools",
                "UnrealEd",
                "EditorScriptingUtilities",
                "Projects",
                "ContentBrowser",
                "ToolMenus",
                "WorkspaceMenuStructure",
                "UMG",
                "Niagara",
                "LevelSequence",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "InputCore",
                "UnrealEd",
                "ToolMenus",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "EditorStyle",
                "AssetRegistry",
                "AssetTools",
                "EditorScriptingUtilities",
                "ContentBrowser",
                "WorkspaceMenuStructure",
                "Json",
                "JsonUtilities",
                "MessageLog",
            }
        );
    }
}
