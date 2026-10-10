# JE Render — DX12/Vulkan, BSP/geología y SIGGRAPH medido

**Directiva 2026-10-10: DX12 y Vulkan son los únicos backends gráficos permitidos.** Windows prueba ambos, Linux Vulkan. No renderer RT en esta fase; Null exclusivamente test/headless. Código propio/adaptado converge a JE::Render, targets JE; Atom es implementación de partida, no namespace público final ni segundo motor.

## 1. Ruta única y fuentes reales

Consumidores JE→implementación RPI/Feature Processors/Pass System adaptada→FrameGraph/RHI→DX12 o Vulkan. Un owner de recursos/barriers/subresources/transients/queues/deferred release/PSO, no JericoRenderGraph adicional ni Diligent debajo. Una publicación Environment y una exposure/tone pipeline.

Source audit: Windows `Gems/Atom/Feature/Common/Code/Source/Platform/Windows/runtime_dependencies_clients.cmake` ya declara DX12/Vulkan/Null; Linux equivalente sólo Vulkan. Esto verifica declaraciones, no ejecución/feature parity. Auditar también tools/builders/package flags; excluir backends no permitidos y Null de selección/producto gráfico.

Inventariar RHI/DX12, RHI/Vulkan, RPI, Feature/Common, Asset/Shader, MaterialEditor/Canvas/PassCanvas, shaders/AZSL/DXC, SRGs/reflection y variants. Los nombres son localizadores del source actual. Migrar namespace/macros/CMake/codegen con usuarios y IDs; no resolver identidad con aliases indefinidos. HLSL/AZSL de entrada y binaries offline se adaptan con compiladores; no reescribir un compilador sólo para cambiar su marca.

GPU capabilities/format support/subgroups/indirect limits se consultan; no asumir equivalencia de drivers. Guardar compiler versions, shader products y features exigidas por material/pass. API unsupported produce error/fallback permitido explícito; nunca fallback a un tercer renderer.

## 2. Optimización que se vea desde BSP

No esperar N12: N1/N2/N8 aportan escenas cuarto/corredor/puerta/ventana, muchos brushes/módulos y cliff/cueva/materiales geológicos. Medir edit→preview→apply→mesh/collision update, frame CPU/GPU, draw/dispatch, memoria pico/residente, compile/cook stalls y respuesta de UI.

Por cambio: fuente/problema → reproducción con cámara/settings/hardware/backend fijos → baseline → propuesta mínima → RED/GREEN apropiado → ON/OFF/ON o A/B/A → imagen más tiempos/memoria → KEEP/REJECT. Menos draws o más checks no prueba menor frametime. Efectos visuales nuevos muestran antes/después a escala original; overlays de debug no sustituyen imagen final.

Guardar p50/p95/p99, warmup/cold versus steady y resolución; GPU query no es presentación/FPS. No comparar fixtures de 64x64 contra viewport real ni atribuir una caída del escritorio a VRAM sin medición. No ocultar trabajo leyendo todo cada frame. Readbacks del diagnóstico apagados no forman parte del camino normal.

## 3. Base material/luz/visibilidad

PBR metallic/roughness, normal/tangent, ORM/separados y color-space coherentes; material instances/graph/variants acotadas. Luces directional/point/spot/projector/rectangle/disk/tube según prueba, shadows/probes/IBL y transparencia; sombra de área exige representación/aproximación real, no promesa por blur.

JericoMaterials controla semántica y `.rmat/.rtext`, Atom adaptado ejecuta. Geología publica los campos materiales de la geometría, no otro generador visual desconectado. Decals RBrush con proyección/gradients/orden/receiver/LOD y prueba sobre metal.

Visibility GPU Scene/frustum/HZB, bounds conservadores, LOD/HLOD/instancing/indirect e invalidación local. Auditar implementación existente antes de crear buffers/scene registry nuevos. Meshlets/virtualized geometry/virtual textures sólo con beneficio medido y fallback; no necesarios para mostrar primer BSP.

## 4. Agenda de investigación preservada

P0=baseline/reusar, P1=extensión inmediata con consumidor, P2=después de foundations, R=investigación no prometida. Las 22 referencias continúan como catálogo de lectura/adopción, no 22 features automáticamente implementadas. Fuente pública o auditada ≠ benchmark de Jerico.

| ID | Referencia | Aplicación/gate JE |
|---|---|---|
| SIG-01 | idTech 666, SIGGRAPH2016 | P1: clustered lights/probes/decals, Stamp/RBrush, shadows/rough glass/particle lighting; no tuning GCN copiado |
| SIG-02 | Frostbite Moving to PBR,2014 | P0/P1: unidades/direct/IBL/material semantics, comparar Atom actual |
| SIG-03 | Area Lighting/LTC,2016 | P1: luces de área existentes primero, evaluación separada de representación de sombras |
| SIG-04 | EON rough diffuse | R/P2: fuente/version corregidas antes de implementar, white furnace y visual A/B |
| SIG-05 | OpenPBR | Terminología/layers, no obligación de implementar todo ni importar otras dependencias |
| SIG-06 | GPU-Driven Rendering Pipelines,2015 | P1/P2: scene extraction/batching/culling/indirect medidos en BSP/geología |
| SIG-07 | Clustered Deferred/Forward y culling de producción | P1: listas comunes y overflow; adaptar lo existente, no segundo registro |
| SIG-08 | SDSM/CSM scrolling/idTech caches | P1/P2: sombras estables/caches/invalidation/budget/age, sol/cámara/casters móviles |
| SIG-09 | Hillaire Sky/Atmosphere,2020 | P2: SkyAtmosphere actual/LUT/aerial perspective, sin copiar shader Cry |
| SIG-10 | Bruneton/Neyret | R: referencia de transporte/unidades/precision; no error gates de otro modelo impuestos |
| SIG-11 | Frostbite Unified Volumetrics,2015 | P2: medios coherentes/luces/partículas, buffers/temporal/coste |
| SIG-12 | Nubis2017/2022/2023 | P2: nubes/weather/shadows con quality ladder y ghosting tests |
| SIG-13 | Horizon Forbidden West Water,2022 | P2 diferido: costa/curvas/perfiles/deformación/espuma, no solver underwater/buoyancy |
| SIG-14 | Uncharted4 Rapids/Tessendorf | P2: flujo local authorable; FFT sólo por valor medido |
| SIG-15 | Temporal AA/Filmic SMAA/post de producción | P1/P2: histories/jitter/motion/disocclusion, misma scene/cámara, sin exposure duplicado |
| SIG-16 | Large Scale Terrain/Geometry Clipmaps | P2: tiles/residency/seams/materials; terrain existente antes de virtual texturing |
| SIG-17 | Destiny Particle Architecture/idTech particle atlas | P2: lighting desacoplado del sprite después de emitter/cook real |
| SIG-18 | Smolder, índice SIGGRAPH2026 | R/P2: lectura focal y evaluación, no framework completo ni claim de coste |
| SIG-19 | HypeHype stochastic tile lighting | R: sólo ante costo real de muchas luces; ruido/bias/colores/movimiento |
| SIG-20 | SLIM/adaptive tessellation/subdivision del índice2026 | R: lectura/version/alcance aún por verificar íntegramente, no required foundation |
| SIG-21 | Valve SDF/vector text y producción de sprites | P2: UI/2D/batching/overdraw, no otro engine |
| SIG-22 | Destiny animation/motion matching/second-order response | Referencia por dominio; motion matching/neural diferidos, tutorial no se etiqueta paper |

Conservar atlas Jerico histórico con CoD materials, geometry/terrain/deformation, Journey y otras familias. RT-only no entra a producción por figurar en el atlas. DiligentFX puede ser referencia/fragmento permitido, nunca añadir otro RHI/scheduler para tomar un efecto.

## 5. idTech6 por piezas

Clustered común: lights/probes/decals donde reutilizar listas evita trabajo; límites/overflow visibles. No megashader obligatorio. Decals: OBB/proyección, explicit gradients/mips, channel blending, sort determinista, palette/stamp editable; RBrush no se reduce a decal system. Cámara rasante, metal, overlap y movimiento con authoring/cook reales.

Shadows: static/dynamic split, atlas/cache/time slicing y actualización por necesidad; empty drawlist no acredita contenido vacío de shadow map. Rough glass: pirámide filtrada compartida, depth/silhouette y temporales; Particle lighting: frecuencia/resolución adaptativas separadas de la presentación full-res. Coherencia/subgroups/async: capability y fallback, colas sólo si hay overlap/latencia mejorada medidos.

Principio: evaluar una señal a la menor frecuencia/resolución/precisión compatible con su error visible. Presupuestos DOOM/PS4/GCN históricos no son constantes JE.

## 6. GI, agua y entorno

No copiar SVOGI/MNM/shaders de Cry. JSVGI conserva intención de GI raster propia con sparse regions/bricks/budgets si hay necesidad; no port autorizado ni primer bloque antes del BSP. DiffuseProbeGrid realtime usa RT; baked puede requerir RT para producir datos y no autoriza esa ruta automáticamente. AO/IBL no se llaman GI completa.

Environment único para sol/luna/sky/fog/cloud/weather/wind/water y tiempos por consumidor documentados. Agua local/superior primero; under water/costa siguen diferidos y flotabilidad DESIGN ONLY. No conservar código caro sólo por haber pasado tests en Cry. Epipolar/Yusov2013 es GDC, no SIGGRAPH; comparar sólo la técnica necesaria y licencias de implementación.

## 7. Tomar ideas de otros engines

Permitido: estudiar diseño público de Hammer/Source para edición BSP, idTech/Frostbite/Decima para rendering, motores abiertos para memoria/jobs/streaming y herramientas. Prohibido: copiar source/assets propietarios por acceso disponible, trasladar arquitectura entera, cambiar tolerancias para imitar un screenshot o ocultar dependencia bajo JE.

Ficha mínima: problema JE; fuente exacta/versión/sección; idea útil; qué se rechaza; código reutilizable y licencia o implementación original; owner y consumers; experimento/imagen/timing/memoria/fallback; resultado. Una referencia no es una orden de implementar todo. Si cambia un contrato de material/formato/reflection, actualizar el master con pruebas.

## 8. Evidencia y fuentes

Acceptance matrix: Windows-DX12, Windows-Vulkan, Linux-Vulkan, con shader products/cook/runtime/source-free por pares y representative scenes. Presupuesto de authoring geológico no se confunde con frametime: ambos registrados. Sin GPU/hardware no hay claim de benchmark ni runtime.

Fuentes base: `docs/research/JE_DEPENDENCY_AUDIT_2026_10_10.md`; docs oficiales Atom https://www.docs.o3de.org/docs/atom-guide/dev-guide/rhi/ y https://www.docs.o3de.org/docs/user-guide/visualization/rendering/ . Índices de papers: https://advances.realtimerendering.com/s2016/ , /s2015/, /s2022/, /s2025/, /s2026/ en el mismo dominio; publicaciones del autor https://sebh.github.io/publications/ . El atlas/auditorías del repo histórico siguen siendo mapas de lectura, no afirmación de relectura completa aquí de todos los PDFs. Estado actual: planificación y source audit, ningún render nuevo implementado.
