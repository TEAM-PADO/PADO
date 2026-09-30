using UnrealBuildTool;

public class ReusableSteamSessions : ModuleRules
{
	public ReusableSteamSessions(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "OnlineSubsystem", "OnlineSubsystemUtils", "CoreOnline"
		});
	}
}
