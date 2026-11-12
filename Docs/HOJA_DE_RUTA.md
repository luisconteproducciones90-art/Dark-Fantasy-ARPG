# Hoja de ruta y estado

Regla: una función solo se marca **hecha** si funciona y está verificada. "Escrito, sin compilar" es un
estado distinto y se dice.

| Fase | Contenido | Estado |
|---|---|---|
| 1 | Arquitectura y GDD | **Hecha** |
| 2 | Proyecto Unreal y configuración | Escrita, **sin compilar** (falta Unreal en el entorno) |
| 3 | Personaje y cámara isométrica | Escrita, **sin compilar** |
| 4 | Movimiento y combate | Reglas de golpe hechas y probadas; clic para moverse escrito sin compilar; ataques y estados pendientes |
| 5 | HUD con esferas | Pendiente |
| 6 | Enemigos y recompensas | Pendiente |
| 7 | Inventario y equipamiento | Pendiente |
| 8 | Clases y habilidades | Datos base de las 6 clases hechos; habilidades pendientes |
| 9 | Mapas, ciudades y mazmorras | Pendiente |
| 10 | Misiones y jefes | Pendiente |
| 11 | Guardado y modo individual | Pendiente |
| 12 | Multijugador y servidores | Pendiente |
| 13 | Optimización | Pendiente |
| 14 | Instalador y distribución | Pendiente |

## Verificado hoy

- `ArpgCore` compila sin avisos con GCC 13 y Clang 18 (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`,
  sin excepciones ni RTTI).
- 19 pruebas automáticas pasan, también bajo AddressSanitizer y UBSan.

## No verificado

- Todo `Game/Source/ArpgGame`, los `.Target.cs`, `.Build.cs`, el `.uproject` y `Game/Config`: nunca han
  pasado por Unreal Build Tool.
- La compilación de `ArpgCore` con MSVC (la hará GitHub Actions en el primer push).

## Pendientes inmediatos

1. Compilar y abrir el proyecto en Windows con Unreal 5.8; corregir lo que falle.
2. `ArpgCore`: estados (quemadura, congelación, veneno, aturdimiento), objetos con propiedades aleatorias e
   inventario por cuadrícula — todo comprobable sin motor.
3. `ArpgGame`: componente que conecta un actor con `ArpgCore` (vida, maná, golpes) y ataque básico con clic.
4. `ArpgUI`: esferas de vida y maná con material animado.

## Lo que el código no resuelve

Modelos, animaciones, texturas, efectos, música y voces de calidad comercial son producción artística:
hacen falta artistas o un flujo con Blender y herramientas generativas, y tiempo. Hasta entonces el juego
usará figuras provisionales. La dirección de arte se fijará con la imagen de referencia cuando se reciba.
