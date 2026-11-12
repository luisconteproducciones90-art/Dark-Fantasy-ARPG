# Objetivos de rendimiento y plan de medición

**Ningún número de este documento es un requisito mínimo publicado.** Son objetivos de trabajo. Los
requisitos mínimos y recomendados se escribirán en la Fase 13 a partir de capturas reales en máquinas
reales; hasta entonces no existen.

## Objetivos de trabajo (hipótesis a verificar)

| Perfil | Resolución / fps objetivo | Clase de GPU prevista | Técnicas |
|---|---|---|---|
| Bajo | 1080p (escalado desde ~67 %) / 60 | Integrada moderna o dedicada de ~2017 | Sin iluminación global dinámica, sombras en cascada cortas, DX11 opcional, partículas al 40 % |
| Medio | 1080p / 60 | Gama media de ~2019 | Lumen Lite, sombras virtuales calidad media, TSR |
| Alto | 1440p / 60 | Gama media-alta actual | Lumen, sombras virtuales altas, niebla volumétrica |
| Ultra | 4K (TSR) / 60 | Gama alta actual | Lumen alta calidad, MegaLights, partículas completas |

Presupuestos de escena (cámara isométrica, peor caso de combate):

- 80 enemigos animados en pantalla en Medio sin bajar de 60 fps.
- Tiempo de carga de zona < 8 s en SSD SATA.
- Memoria: < 6 GB RAM y < 4 GB VRAM en Medio.
- Instalación < 15 GB en la primera versión.

## Por qué la cámara isométrica ayuda

La distancia a cámara es casi constante y el horizonte nunca se ve: el radio visible es pequeño y conocido.
Eso permite distancias de dibujado cortas, un solo nivel de detalle dominante por objeto, sombras de corto
alcance y carga progresiva por celdas alrededor del jugador.

## Técnicas previstas

| Requisito | Implementación |
|---|---|
| LOD | Nanite en entorno estático (perfiles Medio+); LOD clásicos de 3 niveles en personajes y en perfil Bajo |
| Culling | Frustum + oclusión del motor; distancia máxima de dibujado por categoría; ocultar techos y pisos superiores |
| Instancing | Piezas modulares de mazmorra como mallas instanciadas (ISM/HISM); utilería repetida instanciada |
| Texturas | Compresión BC7/BC5, streaming de texturas con presupuesto por perfil; 2K máximo salvo jefes |
| Iluminación | Lumen Lite/Lumen según perfil; luces de antorcha sin sombra salvo las N más cercanas |
| Sombras | Calidad, distancia y número de luces con sombra configurables |
| Partículas | Niagara con niveles de calidad por perfil y tope global de emisores por pantalla |
| Animación | Presupuesto de animación (Animation Budget Allocator), actualización reducida fuera de pantalla |
| Memoria / carga | World Partition o streaming por salas; carga asíncrona de assets; pooling de proyectiles y enemigos |

Los valores concretos de cada perfil (cvars) se fijan en la Fase 13 midiendo, no antes: escribirlos ahora
sería inventarlos.

## Cómo se mide

1. Escena de prueba fija con semilla fija: 80 enemigos, 10 habilidades con efectos, 60 s de recorrido
   automático.
2. Captura con `stat unit`, `stat gpu`, Unreal Insights y CSV Profiler en build **Shipping** empaquetado.
3. Se registra fps medio, percentil 1 % y memoria en al menos tres máquinas reales (baja, media, alta).
4. Los resultados se guardan en `Docs/mediciones/` con fecha, hardware y commit.
