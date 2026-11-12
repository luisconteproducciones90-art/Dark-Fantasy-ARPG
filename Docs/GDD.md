# Documento maestro de diseño (GDD)

**Título provisional:** *Vhaldren: El Juramento Ceniciento* (pendiente de búsqueda de marca antes de usarlo en público)
**Género:** Action RPG hack & slash isométrico · **Plataforma:** Windows · **Motor:** Unreal Engine 5.8
**Estado del documento:** v0.1 — Fase 1. Los números de equilibrio son puntos de partida y se ajustan jugando.

## 1. Visión

Un ARPG de cámara isométrica, clic para moverse y combate rápido, con la estructura de los clásicos del
género (actos, ciudades seguras, mazmorras, botín aleatorio, inventario por cuadrícula) y tecnología actual.
Universo, nombres, criaturas, arte y sonido son propios.

Pilares, en orden de prioridad cuando entren en conflicto:

1. **Jugabilidad clásica**: control directo y preciso; cada clic responde al instante.
2. **Lectura clara**: el jugador siempre distingue su personaje, los enemigos, el botín y el peligro.
3. **El botín importa**: cada objeto puede cambiar cómo se juega, no solo subir un número.
4. **Liviano**: se instala rápido y corre fluido en equipos de gama media.
5. **Sin conexión primero**: el modo individual no pide Internet ni cuenta.

## 2. Mundo e historia

**Vhaldren** fue un reino que mantenía a raya la noche con la *Campana del Alba*. Cuando la campana se
quebró, empezó **el Marchitar**: una ceniza que no cae del cielo sino que sube de la tierra, y que despierta
lo enterrado. Bajo el reino duerme **Orrum, el Rey Hueco**, un dios vaciado por sus propios fieles.

El jugador es un **Juramentado**: alguien que hizo un voto ante las brasas de la última hoguera de
Brasavieja y que no puede morir del todo mientras el voto siga en pie (justificación de la reaparición en
la ciudad).

| Acto | Región | Ciudad segura | Entornos | Jefe de acto |
|---|---|---|---|---|
| I | Páramos de Ceniza | Brasavieja | Bosque de los Ahorcados, Camposanto de Mirel, catacumbas | La Abadesa Sin Rostro |
| II | Arenas de Sal | Oth-Kareth | Desierto, tumbas, Templo de la Sed | El Tejedor de Sed |
| III | Marca de Hierronegro | Fuerte Tregua | Castillo, cuevas, pasos de montaña | Mariscal Vordan el Desollado |
| IV | La Fragua Honda | Campamento Escoria | Región volcánica, forjas, ríos de lava | Madre Escoria |
| V | El Velo Roto | El Último Umbral | Zona demoníaca, ruinas invertidas | Orrum, el Rey Hueco |

Cada acto: 1 ciudad, 4–6 zonas exteriores, 3–5 mazmorras, 6 misiones, 1 jefe de acto y 2–3 jefes menores.

## 3. Cámara y control

- Cámara elevada fija a 45° de orientación y ~52° de inclinación, con FOV estrecho (35°) y distancia larga
  para una lectura casi isométrica. Sigue al personaje con un retardo mínimo.
- Zoom opcional y acotado (rueda). Giro opcional, desactivado por defecto, limitado a ±45°.
- Las paredes que tapan al personaje se desvanecen por material; la cámara nunca se acerca por colisión.
- **Clic izquierdo**: mover / atacar / recoger / hablar según lo que haya bajo el cursor. Clic corto = ruta
  por malla de navegación; clic mantenido = seguir el cursor.
- **Clic derecho**: habilidad secundaria. **Mayús**: atacar sin moverse.
- **1–4**: cinturón de pociones. **Q W E R / F1–F8**: asignación de habilidades. **I C T M Esc**: inventario,
  personaje, habilidades, mapa, menú. **Alt**: mostrar nombres de objetos en el suelo.
- Enemigos y objetos se resaltan con contorno al pasar el cursor.

## 4. HUD

| Zona | Contenido |
|---|---|
| Inferior izquierda | **Esfera roja de vida**: líquido animado, valor numérico, destello al recibir daño, brillo al regenerar, latido y viñeta roja por debajo del 25 % |
| Inferior derecha | **Esfera azul de maná**: líquido animado, valor numérico, onda al gastar, brillo al regenerar |
| Centro inferior | Habilidad de clic izquierdo y derecho, barra de habilidades, cinturón de 4 pociones, barra de experiencia, botones de inventario/personaje/habilidades/mapa, iconos de estados activos |
| Superior derecha | Minimapa (alternable a mapa superpuesto), nombre de la zona, dificultad |
| Superior izquierda | Misión activa y objetivo; retratos del grupo en multijugador |

Las dos esferas nunca se ocultan en combate. Estilo: hierro forjado, piedra y pergamino oscuro; tipografía
con serifa legible; escalado de interfaz de 80 % a 150 %.

## 5. Personajes

Atributos: **Fuerza** (requisitos de armas y armaduras, daño cuerpo a cuerpo), **Destreza** (índice de ataque,
defensa, requisitos de armas a distancia), **Vitalidad** (vida), **Energía** (maná).
Nivel máximo 80. Por nivel: 5 puntos de atributo y 1 de habilidad. Cada clase tiene 3 ramas de 8
habilidades (activas y pasivas) con requisitos de nivel y sinergias dentro de la rama.

| Clase | Papel | Ramas |
|---|---|---|
| **Guerrero** | Cuerpo a cuerpo, resistencia | Maestría de armas · Gritos de guerra · Furia |
| **Hechicero** | Magia elemental a distancia | Brasa (fuego) · Escarcha (frío) · Tormenta (rayo) |
| **Cazador** | Arcos, trampas, precisión | Arquería · Trampas · Instinto (pasivas y compañero halcón) |
| **Nigromante** | Invocaciones y magia oscura | Osario (invocación) · Podredumbre (veneno y hueso) · Maleficios |
| **Asesino** | Velocidad, sigilo, críticos | Hojas · Sombras · Venenos |
| **Paladín** | Defensa, auras, magia sagrada | Auras de amparo · Auras de castigo · Juicio (combate sagrado) |

Valores iniciales y crecimiento por clase: `Game/Source/ArpgCore/Private/Character/CharacterStats.cpp`.
Se pueden crear, guardar, cargar y eliminar personajes; cada uno progresa por separado.

## 6. Combate

Tipos de daño: físico, fuego, frío, rayo, veneno, sombra y sagrado.

- **Impacto** (solo ataques de arma): `IA / (IA + 0,8·DEF) × (1 + 0,02·(nivel atacante − nivel defensor))`,
  acotado entre 5 % y 95 %.
- **Bloqueo** (requiere escudo): máximo 75 %. Los hechizos no se bloquean.
- **Crítico**: 5 % base, ×1,5 de daño base; ambos mejorables.
- **Armadura**: reduce daño físico `A / (A + 50 + 10·nivel atacante)`, máximo 75 %.
- **Resistencias** elementales: de −100 % a 75 %.

Estados: **quemadura** (daño en el tiempo), **congelación** (ralentiza; a enemigos normales puede
inmovilizarlos), **veneno** (daño en el tiempo, anula regeneración), **aturdimiento**, **electrocución**
(el siguiente golpe salta a enemigos cercanos), **maldición** (baja resistencias). Los jefes tienen
duración reducida de estados de control.

Implementado y probado hoy: impacto, bloqueo, crítico, armadura y resistencias (`ArpgCore/Combat`).
Estados e invocaciones: Fase 4 y Fase 8.

## 7. Objetos e inventario

Inventario de mochila 10×4 casillas; los objetos ocupan de 1×1 a 2×4. Ranuras de equipo: casco, armadura,
guantes, botas, cinturón, arma, escudo/mano secundaria, 2 anillos, amuleto. Almacén personal 10×10 en cada
ciudad; pestaña compartida entre personajes de la misma cuenta local (opcional).

| Rareza | Propiedades aleatorias | Notas |
|---|---|---|
| Común | 0 | Base para engarzar gemas |
| Mágico | 1–2 | |
| Raro | 3–4 | |
| Épico | 4–5 | Rangos de propiedad más altos |
| Legendario | 5–6 + 1 poder fijo | Poder que altera una habilidad |
| Mítico | Fijas, diseñadas a mano | Uno equipado a la vez; definen una forma de jugar |

Cada propiedad tiene niveles según el nivel del objeto, que depende del nivel del monstruo y la zona.
Gemas (5 tipos × 5 calidades) se engarzan en huecos. Comparación automática con lo equipado, arrastrar y
soltar, compra, venta y recompra en mercaderes, comercio entre jugadores con ventana de confirmación doble.

## 8. Enemigos y jefes

Familias por acto (muertos de ceniza, bestias del páramo, cultistas de la sed, hueste de Hierronegro,
engendros de escoria, moradores del Velo). Arquetipos de comportamiento: acosador cuerpo a cuerpo, tirador
que mantiene distancia, cargador, invocador, sanador/potenciador, emboscador que sale del suelo, explosivo.

Rangos: normal, **campeón** (grupo con un modificador), **élite** (2–3 modificadores y séquito), jefe.
Los jefes de acto tienen 2–3 fases con cambio de patrón por umbral de vida, ataques telegrafiados en el
suelo y botín garantizado la primera vez.

## 9. Mundo y generación procedural

Las ciudades y las arenas de jefe son fijas y hechas a mano. Zonas y mazmorras se arman con **generación
procedural controlada**: un grafo de salas (entrada → ruta crítica → ramas opcionales → salida/jefe)
relleno con piezas modulares hechas a mano, con reglas por bioma. La semilla de la partida fija el mapa;
misma semilla, mismo mapa (necesario para multijugador y para reproducir errores).
Contenido por zona: cofres, santuarios con mejora temporal, eventos, salas secretas, portales de viaje
rápido entre ciudad y zonas descubiertas, y pergaminos de portal a la ciudad.

## 10. Progresión y dificultades

Tres dificultades que se desbloquean en orden: **Peregrino**, **Penitente**, **Condenado**. Cada una sube
nivel de monstruos, añade modificadores de élite y aplica penalización de resistencias al jugador.
Morir: se reaparece en la ciudad; parte del oro queda en el lugar de la muerte y se recupera al volver.
Modo opcional de **voto final** (muerte permanente).

## 11. Modo individual y multijugador

- **Individual**: totalmente sin conexión, guardado local, sin cuenta.
- **Multijugador** (opcional): cooperativo de hasta 4 jugadores por partida (la arquitectura no impide
  subir a 8; se decide con datos de rendimiento). Servidor dedicado autoritativo: valida movimiento, daño,
  vida, botín, inventario y comercio. Botín instanciado por jugador. Chat de grupo e invitaciones.
- Los personajes sin conexión y los personajes en línea son listas separadas: un guardado local es
  editable por el usuario y no puede entrar a la economía en línea.

## 12. Audio

Música ambiental orquestal oscura por acto, con capa de combate. Sonidos de botín diferenciados por rareza
(el jugador debe reconocer un legendario por el oído). Voces de PNJ en ciudades.

## 13. Fuera de alcance de la primera versión

PvP, temporadas, tienda dentro del juego, consolas, controles con mando. Se dejan puertas abiertas en la
arquitectura pero no se construyen.
