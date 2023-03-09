// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Cavern : ModuleRules
{
	public Cavern(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput", "MeshDescription",
			"StaticMeshDescription", "MeshConversion", "ProceduralMeshComponent" });
	}
}
