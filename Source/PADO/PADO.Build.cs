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


			// GameplayTag
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",

			// Network
			"DeveloperSettings",
			"ReusableSteamSessions",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"CoreOnline",
			
			// GMS(Gameplay Message Router)
			"GameplayMessageRuntime",

			// UI(UMG, CommonUI, MVVM)
			"UMG",
			"CommonUI",
			"CommonInput",
			"ModelViewViewModel",
			
			// Mass
			"MassCore",
			"MassEntity",
			"MassCommon",
			"MassMovement",
			"MassSpawner",
			"MassAIBehavior",
			"MassSignals",
			"StateTreeModule",
		});

		// PhysicsCore: Fire Action 발 기록이 탄착 표면 재질(UPhysicalMaterial)을 싣는다.
		PrivateDependencyModuleNames.AddRange(new string[] { "Niagara", "PhysicsCore" });

		// UI 위젯 내부 구현용 Slate
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
