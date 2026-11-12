using UnrealBuildTool;

public class DarkFantasyARPGTarget : TargetRules
{
	public DarkFantasyARPGTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "ArpgCore", "ArpgGame" });
	}
}
