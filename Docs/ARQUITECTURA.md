# Arquitectura técnica

## Resumen

```
┌───────────────────────── Juego (Unreal Engine 5.8) ─────────────────────────┐
│  ArpgUI · ArpgAudio · ArpgSettings      presentación                        │
│  ArpgGame · ArpgNet                     actores, cámara, entrada, réplica    │
│  ─────────────────────────────────────────────────────────────────────────  │
│  ArpgCore                               REGLAS: C++20 puro, sin Unreal       │
└──────────────────────────────────────────────────────────────────────────────┘
        ▲ el mismo ArpgCore                       ▲
        │                                         │
  Pruebas sin motor (CMake, CI)        Servidor dedicado UE + Server/ (cuentas, PostgreSQL)
```

Las dependencias solo bajan: UI → Game → Core. `ArpgCore` no conoce a nadie.

## Decisiones (ADR)

### ADR-1 · Las reglas viven en un núcleo C++ sin motor
**Decisión:** estadísticas, daño, botín, inventario, progresión, generación de mapas y serialización de
guardado se escriben en `ArpgCore`: C++20 estándar, sin excepciones, sin RTTI, sin cabeceras de Unreal.
Unreal lo consume como un módulo más.

**Alternativa descartada:** Gameplay Ability System (GAS) de Unreal para todo. Es el camino habitual y trae
réplica hecha, pero ata las reglas al editor: no se pueden probar sin motor ni compilar en CI barato.

**Por qué:** (1) un solo código de reglas para individual, cliente y servidor, que es el requisito de "no
mantener dos juegos"; (2) pruebas automáticas en segundos; (3) el servidor valida con la misma función que
el cliente predice; (4) el entorno de desarrollo actual no puede ejecutar Unreal, y así el corazón del
juego queda verificado de todos modos.

**Coste:** la réplica de estado y el enganche de animaciones/efectos a habilidades se escriben a mano en
`ArpgGame`/`ArpgNet`. Es reversible mientras no haya habilidades hechas (Fase 8).

### ADR-2 · Unreal Engine 5.8.x
Última versión estable (5.8.4 a noviembre de 2026) y último major planificado de UE5, por lo que es la base
más duradera. Aporta Lumen Lite (iluminación global a mitad de coste) y MegaLights en estado de producción,
ambos alineados con "moderno pero liviano".

### ADR-3 · Determinismo por semilla
Todo lo aleatorio pasa por `arpg::Rng` (PCG32). Mapas y botín se reproducen a partir de la semilla. En
multijugador las semillas solo existen en el servidor.

### ADR-4 · Entrada definida en código
Las acciones de Enhanced Input se crean en C++ en vez de en assets, para que el control funcione desde un
clon limpio del repositorio. Cuando exista menú de reasignación de teclas (Settings) pasarán a datos.

### ADR-5 · Datos de equilibrio
Hoy son tablas `constexpr` en `ArpgCore`. En la Fase 7 pasan a archivos de datos (JSON) cargados al inicio,
con validación en pruebas, para equilibrar sin recompilar.

## Mapa de módulos pedidos → dónde viven

| Módulo del plan | Ubicación | Estado |
|---|---|---|
| Core | `ArpgCore/Core` | RNG y tipos hechos |
| CharacterSystem | `ArpgCore/Character` + `ArpgGame` (actor) | Atributos, clases, experiencia hechos |
| CombatSystem | `ArpgCore/Combat` | Resolución de golpes y estados hechos |
| ItemSystem, InventorySystem | `ArpgCore/Item`, `ArpgCore/Inventory` | Generación de objetos, cuadrícula y equipo hechos; gemas, almacén y comercio pendientes |
| SkillSystem | `ArpgCore/Skill` + `ArpgGame` | Fase 8 |
| EnemySystem | `ArpgCore/Enemy` (datos) + `ArpgGame` (IA) | Fase 6 |
| QuestSystem | `ArpgCore/Quest` | Fase 10 |
| WorldSystem, MapSystem | `ArpgCore/Map` (generación) + `ArpgGame` (instanciado) | Fase 9 |
| SaveSystem | `ArpgCore/Save` (formato) + `ArpgGame` (archivos) | Fase 11 |
| UI | módulo UE `ArpgUI` | Fase 5 |
| Audio, Settings | módulos UE `ArpgAudio`, `ArpgSettings` | Fases 5 y 13 |
| MultiplayerSystem | módulo UE `ArpgNet` | Fase 12 |
| ServerSystem | `Server/` (servicio de cuentas y persistencia, PostgreSQL) | Fase 12 |
| Installer | `Installer/` (Inno Setup + actualizador) | Fase 14 |

Los sistemas de reglas son carpetas y espacios de nombres dentro de `ArpgCore` en lugar de 17 módulos de
Unreal separados: cada módulo UE añade tiempo de compilación y enlazado, y la frontera que importa
(reglas / motor / interfaz) ya está garantizada por el compilador. Dentro de `ArpgCore` el orden permitido
es `Core ← Character ← Combat ← Item ← Inventory ← Skill ← Enemy ← Quest ← Map ← Save`; un módulo solo
incluye cabeceras de los que están a su izquierda.

## Estructura de carpetas

```
CMakeLists.txt            build sin motor de ArpgCore + pruebas
Docs/                     GDD, arquitectura, rendimiento, build, hoja de ruta
Game/                     proyecto Unreal
  DarkFantasyARPG.uproject
  Config/
  Content/                assets (Git LFS)
  Source/
    ArpgCore/             reglas (Public/ArpgCore/<Sistema>/, Private/<Sistema>/)
    ArpgGame/             actores, cámara, entrada
Tests/ArpgCore/           pruebas automáticas del núcleo
Server/                   (Fase 12) servicio en línea + esquema PostgreSQL
Installer/                (Fase 14) instalador y actualizador
.github/workflows/        CI: compila y prueba ArpgCore en Linux y Windows
```

## Modelo de red (Fase 12, ya condiciona el diseño)

- El cliente envía **intenciones** (mover a X, usar habilidad N sobre objetivo T); nunca resultados.
- El servidor ejecuta `ArpgCore` y replica estado. El cliente predice movimiento y animación, no daño.
- En individual, el mismo proceso hace de servidor (listen server local sin red): un solo camino de código.
- `Server/` guarda cuentas, personajes en línea e inventarios en PostgreSQL; las transacciones de comercio
  son transacciones SQL atómicas.
