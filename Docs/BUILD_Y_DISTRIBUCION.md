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
3. Abrir el `.uproject` y aceptar compilar los módulos.
4. Pulsar **Play**. No hay que crear niveles ni assets: el juego genera la mazmorra, la luz, el personaje y
   los monstruos al empezar, sobre el mapa vacío del motor.
5. Si algo no compila, copiar el error completo de la ventana *Output* de Visual Studio (o el archivo
   `Game/Saved/Logs/*.log`): se corrige en la siguiente sesión. Es normal que haya errores la primera vez;
   este código nunca ha pasado por el compilador de Unreal.

### Controles de la versión de prueba

| Entrada | Acción |
|---|---|
| Clic izquierdo en el suelo | Caminar (clic corto: ruta esquivando muros; mantenido: seguir el cursor) |
| Clic izquierdo en un monstruo | Acercarse y atacar hasta que muera |
| Clic izquierdo en botín | Ir a recogerlo |
| Clic derecho | Habilidad de la clase (gasta maná, tiene enfriamiento) |
| 1 / 2 | Poción de vida / de maná |
| I | Inventario: clic equipa, clic derecho suelta, clic en lo equipado lo quita, `+` sube atributos |
| Rueda | Zoom |
| Consola (`º` o `~`): `ArpgNuevoPersonaje N` | Personaje nuevo: 0 Guerrero, 1 Hechicero, 2 Cazador, 3 Nigromante, 4 Asesino, 5 Paladín |

### Qué debería verse

- Una mazmorra oscura de suelo y muros bajos, con el personaje (un cilindro) iluminado por su antorcha.
- Monstruos como conos: grises los normales, azules los campeones, amarillos los élites y uno rojo grande,
  el jefe. Los "excavadores" asoman apenas del suelo hasta que te acercas.
- Esfera roja de vida abajo a la izquierda y azul de maná abajo a la derecha, barra central, minimapa
  arriba a la derecha y objetivo arriba a la izquierda.
- Al matar al jefe se baja a una mazmorra nueva, más grande y con monstruos de más nivel. La partida se
  guarda al bajar y al salir, en `Game/Saved/SaveGames/Personaje.vhsv` (con copia `.bak`).

Las figuras geométricas y las líneas de colores de los ataques son provisionales: sustituyen a modelos,
animaciones y partículas que todavía no existen.

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
