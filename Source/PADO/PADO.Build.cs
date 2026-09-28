// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PADO : ModuleRules
{
	public PADO(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});

		// PhysicsCore: Fire Action 발 기록이 탄착 표면 재질(UPhysicalMaterial)을 싣는다.
		PrivateDependencyModuleNames.AddRange(new string[] { "Niagara", "PhysicsCore" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
