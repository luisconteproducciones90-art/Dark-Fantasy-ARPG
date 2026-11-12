using UnrealBuildTool;

// Núcleo de reglas del juego. No debe depender de nada más que "Core"
// (y solo para registrar el módulo): ver Docs/ARQUITECTURA.md.
public class ArpgCore : ModuleRules
{
	public ArpgCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		PublicDependencyModuleNames.Add("Core");
	}
}
