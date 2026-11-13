# Hoja de ruta y estado

Regla: una función solo se marca **hecha** si funciona y está verificada. "Escrito, sin compilar" es un
estado distinto y se dice.

| Fase | Contenido | Reglas (`ArpgCore`, probadas) | Juego (Unreal, **sin compilar**) |
|---|---|---|---|
| 1 | Arquitectura y GDD | **Hecha** | — |
| 2 | Proyecto Unreal | — | Escrito |
| 3 | Personaje y cámara | — | Escrito: cámara isométrica con zoom acotado y giro opcional |
| 4 | Movimiento y combate | Golpes, 7 estados, criatura de combate, búsqueda de caminos | Escrito: clic para moverse, atacar con clic |
| 5 | HUD | — | Escrito: esferas de vida y maná, barra central, minimapa, objetivo. Dibujado por código, sin arte |
| 6 | Enemigos y recompensas | 7 monstruos, escalado, IA de 7 comportamientos, botín | Escrito: monstruos, botín en el suelo, experiencia |
| 7 | Inventario y equipo | Cuadrícula, 10 ranuras, requisitos, estadísticas | Escrito: panel con equipar, quitar, soltar y comparar. Falta arrastrar y soltar, gemas, almacén, comercio |
| 8 | Clases y habilidades | 6 clases, una habilidad activa por clase | Escrito: habilidad con clic derecho. Faltan los árboles de talentos |
| 9 | Mapas | Generador de mazmorras | Escrito: mazmorra con cubos instanciados. Faltan biomas, ciudades y piezas modulares |
| 10 | Misiones y jefes | Pendiente | Un jefe por mazmorra, sin fases. Misiones pendientes |
| 11 | Guardado e individual | Formato de guardado | Escrito: guardar y cargar un personaje. Falta menú de personajes |
| 12 | Multijugador | Pendiente | Pendiente |
| 13 | Optimización | — | Pendiente (necesita mediciones reales) |
| 14 | Instalador | — | Pendiente |

"Escrito" significa exactamente eso: el código existe y pasó una revisión de sintaxis contra una imitación
de las cabeceras de Unreal, pero **no se ha compilado ni ejecutado con Unreal**. Hasta que compile y se
juegue, ninguna de esas casillas cuenta como hecha.

## Verificado hoy

- `ArpgCore` compila sin avisos con GCC 13 y Clang 18 (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`,
  sin excepciones ni RTTI).
- 68 pruebas automáticas pasan, también bajo AddressSanitizer y UBSan, y en modo *unity build* (como compila Unreal).
- GitHub Actions compila y pasa las mismas pruebas en Linux y en Windows (MSVC, `/W4 /WX`).

## Primera compilación real (Unreal 5.8, MSVC 14.44, 12-11-2026)

- `ArpgCore` compiló y enlazó dentro de Unreal sin errores.
- `ArpgGame`: 9 de 10 archivos compilaron a la primera; `ArpgGameMode.cpp` falló por una constante mal
  nombrada (`FILEOPEN_Silent` en lugar de `FILEREAD_Silent`), ya corregida. Pendiente de confirmar que con
  eso compila y enlaza todo, y de ejecutar el juego.

## No verificado

- Todo `Game/Source/ArpgGame`, los `.Target.cs`, `.Build.cs`, el `.uproject` y `Game/Config`: nunca han
  pasado por Unreal Build Tool. Lo esperable es que la primera compilación dé errores que habrá que corregir.
- Que `ArpgCore` compile dentro de Unreal (sí compila con MSVC por separado y en modo agrupado).
- Todo lo que solo se sabe jugando: que la iluminación se vea bien, que el ritmo de combate sea agradable,
  que los monstruos no se atasquen en esquinas, que el equilibrio numérico tenga sentido.

## Limitaciones conocidas de la versión de prueba

- Los ataques a distancia son instantáneos y atraviesan muros: aún no hay proyectiles.
- Invocadores y apoyos atacan en vez de invocar o curar.
- Los cofres no son un objeto: su contenido aparece en el suelo de la sala de tesoro.
- Morir no tiene penalización todavía.
- Solo hay una mazmorra que se repite más profunda; no hay ciudad, mercader ni misiones.
- Los efectos visuales son líneas y esferas de depuración, que no aparecen en una build *Shipping*.

## Pendientes inmediatos

1. **Compilar y abrir el proyecto en Windows con Unreal 5.8; corregir lo que falle.** Nada más avanza de
   verdad hasta esto.
2. Ajustar con lo que se vea al jugar: iluminación, velocidades, daño, densidad de monstruos.
3. Proyectiles, fases de jefe, invocaciones y curación de aliados.
4. Ciudad con mercader y almacén; misiones.
5. Datos de equilibrio en archivos (ADR-5) y árboles de talentos.

## Lo que el código no resuelve

Modelos, animaciones, texturas, efectos, música y voces de calidad comercial son producción artística:
hacen falta artistas o un flujo con Blender y herramientas generativas, y tiempo. Hasta entonces el juego
usará figuras provisionales. La dirección de arte se fijará con la imagen de referencia cuando se reciba.
