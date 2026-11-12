# Dark Fantasy ARPG

Action RPG isométrico de fantasía oscura para Windows, sobre Unreal Engine 5.8. Universo original.

- [Documento de diseño (GDD)](Docs/GDD.md)
- [Arquitectura técnica](Docs/ARQUITECTURA.md)
- [Objetivos de rendimiento](Docs/RENDIMIENTO.md)
- [Compilación y distribución](Docs/BUILD_Y_DISTRIBUCION.md)
- [Hoja de ruta y estado real](Docs/HOJA_DE_RUTA.md)

## Probar el núcleo de reglas (sin Unreal)

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Abrir el juego

Requiere Windows, Unreal Engine 5.8 y Visual Studio. No hacen falta niveles ni assets: se pulsa Play y el
juego genera la mazmorra. Pasos y controles en
[Compilación y distribución](Docs/BUILD_Y_DISTRIBUCION.md#primer-arranque-del-proyecto-unreal).
