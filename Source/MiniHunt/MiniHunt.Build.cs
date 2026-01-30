// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MiniHunt : ModuleRules
{
	public MiniHunt(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput", "UMG", "Slate", "SlateCore", "PhysicsCore", "OnlineSubsystem", "OnlineSubsystemUtils" });
	}
}
