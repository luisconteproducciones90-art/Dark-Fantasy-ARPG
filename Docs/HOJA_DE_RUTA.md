# Hoja de ruta y estado

Regla: una función solo se marca **hecha** si funciona y está verificada. "Escrito, sin compilar" es un
estado distinto y se dice.

| Fase | Contenido | Estado |
|---|---|---|
| 1 | Arquitectura y GDD | **Hecha** |
| 2 | Proyecto Unreal y configuración | Escrita, **sin compilar** (falta Unreal en el entorno) |
| 3 | Personaje y cámara isométrica | Escrita, **sin compilar** |
| 4 | Movimiento y combate | Reglas de golpe y estados (quemadura, escarcha, congelación, veneno, aturdimiento, electrocución, maldición) hechas y probadas; criatura de combate (vida, maná, regeneración, muerte, estados al impactar) hecha y probada; clic para moverse escrito sin compilar; ataque con clic pendiente |
| 5 | HUD con esferas | Pendiente |
| 6 | Enemigos y recompensas | Reglas de botín hechas y probadas (rareza por rango de monstruo y hallazgo mágico, propiedades aleatorias, botín por rango con garantía de jefe); 7 monstruos del Acto I con escalado por nivel, rango y dificultad, y decisión de IA de los 7 comportamientos, hechos y probados; falta conectarlos a actores de Unreal |
| 7 | Inventario y equipamiento | Reglas hechas y probadas (cuadrícula, 10 ranuras de equipo, requisitos, estadísticas con equipo, valor de venta); interfaz, gemas, almacén y comercio pendientes |
| 8 | Clases y habilidades | Datos base de las 6 clases hechos; habilidades pendientes |
| 9 | Mapas, ciudades y mazmorras | Generador de mazmorras hecho y probado (salas, pasillos, entrada, jefe, tesoros, población); falta instanciarlo en Unreal con piezas modulares, biomas y ciudades |
| 10 | Misiones y jefes | Pendiente |
| 11 | Guardado y modo individual | Formato de guardado hecho y probado (versión, CRC, validación completa); falta escritura a disco, menú de personajes y flujo de partida |
| 12 | Multijugador y servidores | Pendiente |
| 13 | Optimización | Pendiente |
| 14 | Instalador y distribución | Pendiente |

## Verificado hoy

- `ArpgCore` compila sin avisos con GCC 13 y Clang 18 (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`,
  sin excepciones ni RTTI).
- 60 pruebas automáticas pasan, también bajo AddressSanitizer y UBSan, y en modo *unity build* (como compila Unreal).
- GitHub Actions compila y pasa las mismas pruebas en Linux y en Windows (MSVC, `/W4 /WX`).

## No verificado

- Todo `Game/Source/ArpgGame`, los `.Target.cs`, `.Build.cs`, el `.uproject` y `Game/Config`: nunca han
  pasado por Unreal Build Tool.

## Pendientes inmediatos

1. Compilar y abrir el proyecto en Windows con Unreal 5.8; corregir lo que falle.
2. `ArpgCore`: habilidades (definiciones, coste de maná, enfriamientos, árbol con requisitos) y misiones.
3. `ArpgCore`: pasar las tablas de clases, objetos, propiedades y monstruos a archivos de datos (ADR-5).
4. `ArpgGame`: componente que conecta un actor con `ArpgCore` (vida, maná, golpes) y ataque básico con clic.
5. `ArpgUI`: esferas de vida y maná con material animado.

## Lo que el código no resuelve

Modelos, animaciones, texturas, efectos, música y voces de calidad comercial son producción artística:
hacen falta artistas o un flujo con Blender y herramientas generativas, y tiempo. Hasta entonces el juego
usará figuras provisionales. La dirección de arte se fijará con la imagen de referencia cuando se reciba.
