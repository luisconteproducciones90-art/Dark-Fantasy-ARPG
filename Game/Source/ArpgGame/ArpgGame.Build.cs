using UnrealBuildTool;

// Capa de juego sobre Unreal: actores, cámara, entrada. Las reglas viven en ArpgCore.
public class ArpgGame : ModuleRules
{
	public ArpgGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ArpgCore"
		});
	}
}
