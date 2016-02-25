// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class trnetdemo : ModuleRules
{
	public trnetdemo(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "OnlineSubsystem", "OnlineSubsystemUtils" });
        DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
    }
}
