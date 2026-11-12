// Único archivo del módulo que toca Unreal: registra el módulo.
// El build de CMake (pruebas sin motor) lo excluye.
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, ArpgCore);
