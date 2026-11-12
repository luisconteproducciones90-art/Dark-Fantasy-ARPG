# Compilación y distribución

## Entorno analizado (12-11-2026)

El desarrollo asistido se ejecuta en un contenedor Linux en la nube:

| Herramienta | Estado |
|---|---|
| CMake 3.28, GCC 13, Clang 18, Ninja | Disponibles — compilan y prueban `ArpgCore` |
| Git + Git LFS, PostgreSQL 16, Docker, Go, Node, Python | Disponibles |
| **Unreal Engine, Windows, Visual Studio** | **No disponibles** |
| Blender | No disponible |

Consecuencia: aquí se verifica todo `ArpgCore`. El código de `ArpgGame` y la configuración de Unreal se
escriben aquí pero **solo se pueden compilar y probar en una máquina Windows con Unreal 5.8**.

## Requisitos de la máquina de desarrollo (Windows)

- Windows 10/11 de 64 bits, ~120 GB libres en SSD.
- Unreal Engine 5.8.x desde Epic Games Launcher.
- Visual Studio 2022 o 2026 con las cargas "Desarrollo de juegos con C++" y "Desarrollo de escritorio con C++".
- Git y Git LFS.

## Primer arranque del proyecto Unreal

1. Clonar el repositorio y ejecutar `git lfs install`.
2. Clic derecho en `Game/DarkFantasyARPG.uproject` → *Generate Visual Studio project files*.
3. Abrir el `.uproject`; aceptar compilar los módulos.
4. Crear un nivel con un suelo y un **NavMeshBoundsVolume** que lo cubra, colocar un *Player Start* y
   pulsar Play. El modo de juego ya asigna personaje y controlador isométricos.
5. Si algo no compila, copiar el error completo: se corrige en la siguiente sesión.

## Pruebas del núcleo (cualquier sistema operativo)

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

GitHub Actions ejecuta lo mismo en Linux y Windows en cada push.

## Empaquetado (Fase 14)

```
RunUAT.bat BuildCookRun -project="...\Game\DarkFantasyARPG.uproject" -platform=Win64 ^
  -clientconfig=Shipping -build -cook -stage -pak -iostore -compressed -prereqs -archive ^
  -archivedirectory="...\Dist"
```

## Instalador (Fase 14)

- **Inno Setup**: un único `Setup.exe`, sin dependencias, con asistente, acceso directo en el escritorio y
  desinstalador. Alternativas: NSIS (equivalente, peor mantenimiento de scripts) y WiX/MSI (pensado para
  despliegue corporativo, innecesario aquí).
- El único requisito externo es el paquete de prerrequisitos de Unreal (runtime de Visual C++), que el
  instalador ejecuta en silencio.
- Primer arranque: detección de hardware (`UGameUserSettings::RunHardwareBenchmark`) para elegir perfil
  gráfico inicial.
- Guardados en `%LOCALAPPDATA%\DarkFantasyARPG\Saved\SaveGames`; el desinstalador pregunta antes de borrarlos.
- **Verificación de archivos y actualizador**: manifiesto firmado con hash SHA-256 por archivo; el lanzador
  compara, descarga solo lo que cambió y puede reparar la instalación.

## Servidor dedicado (Fase 12)

Compilar el target de servidor dedicado exige Unreal **compilado desde el código fuente** (acceso al
repositorio de Epic en GitHub), no la versión del Launcher. Es un requisito de Epic y hay que planificarlo:
~200 GB de disco y varias horas de primera compilación.
