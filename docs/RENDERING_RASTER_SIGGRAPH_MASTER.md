# Atom/Vulkan y agenda SIGGRAPH de JericoEngine

Fecha: 2026-10-10. Estado: **contrato y selección de investigación; ningún algoritmo nuevo integrado por esta publicación**. Owners: N2/N4/N7/N8 y rendimiento continuo.

## 1. Cambio respecto al master Cry

Se sustituye `Cry high-level → JericoRenderGraph → Diligent` por la ruta canónica de O3DE: consumidores Jerico → Atom RPI/FeatureProcessors/Pass System → Atom RHI/FrameGraph → Vulkan. No crear otra autoridad de recursos, device, exposición, scene extraction o scheduling.

Conservar el pipeline de shaders AZSL/HLSL y toolchain offline disponible; auditar DXC/SPIR-V, SRGs, reflection, variants y cache antes de reemplazarlo. Luau de gameplay no sustituye automáticamente los Lua material functors internos. Shader tools son offline; Shipping no necesita compilar fuentes. Producto final de materiales se integra con el Asset Processor y formatos del master de assets.

Vulkan Windows/Linux es la única aceptación actual. No incorporar RT de render, path tracing, BLAS/TLAS o ray queries como requisito. Preservar código de otras APIs sin abrir su desarrollo. DiligentFX puede compararse como referencia de un algoritmo o fuente permitida puntual; no introducir su device/graph/renderer completo bajo Atom.

## 2. Inventario técnico que debe cerrar N0/N2

Leer bajo `Gems/Atom`: RHI/Vulkan, RPI, Feature/Common, Asset/Shader, Tools/MaterialCanvas/MaterialEditor/PassCanvas. Seguir productores/consumidores para: frame attachments y subresources, estados/barriers/queue ownership, transient heaps/aliasing, deferred destruction, SRG/descriptors/bindless, PSO/cache/shader variants, CPU jobs/submit, swapchain/frame pacing, streaming/budgets y diagnóstico.

Registrar las features reales del dispositivo; no asumir subgroup size, formatos storage/atomic, indirect limits o memoria coherente. No malloc/free/readback repetido por objeto si ya existe pooling/lifetime adecuado. No confundir memory budget del wire con pico host/VRAM ni GPU query con tiempo de presentación.

Antes de rediseñar un pass: mapa actual → escena/fallo/coste → publicación/fuente permitida → cambio mínimo → comparación → decisión. Una implementación más nueva o compute/async no es automáticamente más rápida.

## 3. Contrato de render base

PBR metallic/roughness, normas/unidades explícitas, direct/IBL coherentes; normal/ORM y mapas separados. Cámara/proyección/depth convention/reverse-Z y posición reconstruida documentados. Una exposición/tone mapping y un Environment.

Luces directional/point/spot/projector y area rectangle/disk/tube según integración verificada; materiales, sombras y probes compartidos por opacos/transparencias donde corresponda. Las sombras de emisor finito necesitan representación/approximation real, no declarar resueltas por blur. Fallback punctual claramente identificado.

Visibility: bounds conservadores/frustum/HZB, LOD/HLOD y culling/compaction/indirect a partir de owner existente; meshlets/virtualized geometry solo después de beneficio probado. No importar `multi-device-resources` o `standalone_meshinfo` en bloque por su nombre.

Shadows: estabilidad, static/dynamic separation, atlas/cache invalidation, budgets y age/debt. SSR/probes/IBL tienen límites de información oculta; fallback coherente sin doble energía. GI difusa es diferente de AO y reflejo especular.

## 4. Agenda priorizada: fuente, aplicación y gate

P0 = auditar/reutilizar baseline; P1 = primera extensión con consumidor; P2 = después de foundations; R = investigación sin compromiso. Las aplicaciones/gates son decisiones Jerico, no capacidades atribuidas automáticamente a los papers.

| Ref | Fuente y época | Aplicación propuesta | Prioridad / condición |
|---|---|---|---|
| SIG-01 | idTech 666, SIGGRAPH 2016 | Clustered común, stamps/decals, shadows, rough glass, particle lighting, coherencia y frecuencia de costes | P1 por bloques; no copiar tuning GCN/PS4 |
| SIG-02 | Moving Frostbite to PBR, 2014 | Semánticas/unidades, direct/IBL, normal/roughness y authoring | P0/P1; comparar con materiales Atom existentes |
| SIG-03 | Real-Time Area Lighting/LTC, 2016 | Evaluación rectangle/disk/polygon, aproximación Fresnel y luces artísticas | P1; auditar LTC/area lights actuales, sombras aparte |
| SIG-04 | EON rough diffuse, JCGT 2025, actualización 2026-02-04 | Comparar rough diffuse y energía con baseline | R/P2; fuente corregida y white furnace; no reemplazo cosmético |
| SIG-05 | OpenPBR | Taxonomía/layers/material semantics | Referencia; no obligación de implementar todo el modelo ni sus dependencias |
| SIG-06 | GPU-Driven Rendering Pipelines, 2015 | Scene records, batching, culling e indirect | P1/P2; medir submission/compaction, no nuevo GPU Scene paralelo |
| SIG-07 | Clustered Deferred and Forward Shading; culling de producción | Listas de lights/probes/decals y overflow diagnosticable | P1; reutilizar LightCulling/Decals actuales |
| SIG-08 | SDSM / CSM scrolling / cachés idTech | Cascadas estables, invalidación y updates presupuestados | P1/P2; cámara/sol/occluder móviles y memoria |
| SIG-09 | Hillaire, A Scalable and Production Ready Sky and Atmosphere Rendering Technique, 2020 | Comparar LUT/sky-view/aerial perspective con SkyAtmosphere Atom | P1/P2; no portar el shader Cry caro |
| SIG-10 | Bruneton/Neyret y referencias atmosféricas públicas | Oracle de unidades/transportes y límites numéricos | R; no tolerancias copiadas de fixtures Cry como contrato de otro modelo |
| SIG-11 | Frostbite Unified Volumetric Rendering, 2015 | Fog local/medios coherentes con luces/partículas | P2; extinction/transmittance/HDR y budget de volumen |
| SIG-12 | Nubis 2017/2022/2023 | Clouds/weather/temporal/shadowing escalables | P2; una atmósfera, quality ladder y ghosting tests |
| SIG-13 | Rendering Water in Horizon Forbidden West, 2022 | Costa/curvas/perfiles de ola/deformación y espuma authored/cooked | P2 diferido; no solver general underwater/flotabilidad |
| SIG-14 | Rendering Rapids in Uncharted 4, 2016; Tessendorf | Flujo artístico/acotado; FFT solo donde mejore coste/calidad | P2; agua local y control temporal primero |
| SIG-15 | Temporal AA/Filmic SMAA y post de producción | Historial, jitter, motion vectors, disocclusion y filtros | P1/P2; inspeccionar post Atom, no histories/exposure duplicados |
| SIG-16 | Large Scale Terrain Rendering; Geometry Clipmaps | Celdas, streaming, costuras y materiales por distancia | P2; Terrain existente antes de virtual texture |
| SIG-17 | Destiny Particle Architecture; atlas lighting idTech | Graph/cook y coste de iluminación desacoplado del sprite | P2 tras emitter/VFX real |
| SIG-18 | Smolder, SIGGRAPH 2026 | Estudiar integración de efectos volumétricos con lighting/transparent/fog | R/P2; no importar framework completo, ni promesa de coste |
| SIG-19 | Stochastic Tile-Based Lighting in HypeHype, 2025 | Candidato para alto coste medido de muchas luces | R; noise/bias/colores/movimiento antes de elegir |
| SIG-20 | SLIM; Adaptive Tessellation and Subdivision, 2026 | Investigar escala de mundos/triangulación cuando el caso modular lo requiera | R; fuente localizada, no auditada íntegramente ni requerida |
| SIG-21 | Valve SDF/vector text; producción de sprites/materiales | Texto/iconos y reducción de overdraw/batching | P2 dentro de UI/2D, no motor separado |
| SIG-22 | Destiny animation; motion matching; second-order response | Layers/rig/secondary motion como referencias de authoring | Motion matching/neurales diferidos; tutorial matemático no se etiqueta paper |

Mantener también el atlas antiguo con materiales CoD WWII, GI/iluminación de producción, terrain/deformation, Journey, ReSTIR/RT y familias vendor; conservar descubrimiento no significa activar cada técnica. Los trabajos RT-only (incluidos path tracing/ORCA, GI idTech8/MegaLights cuando requieran RT) quedan fuera de la fase activa. No usar el catálogo para introducir hardware RT bajo otra etiqueta.

## 5. idTech 6: rescate completo, no solo decals

La auditoría histórica de JericoEngine1 mapea las 58 páginas; se conserva como fuente, sin copiar el PDF ni assets. Su hash registrado es histórico, no recalculado en esta publicación.

- Cluster/light/probe/decal: revisar las listas y resolución de Atom; consolidar solo si evita trabajo y no introduce megashader/permutation explosion.
- Decals/Stamp: paleta, cursor OBB, parámetros, canales y resultado editable. RBrush BRUSH/PAYLOAD/TARGET se define en el master procedural. Gradientes correctos, atlas/arrays, ordering/receiver filtering y LOD. Probar metal puro, grazing angles y overlaps.
- Shadows: cache/time-slicing con invalidación comprobable; mapa vacío, mapa cacheado no actualizado y mapa válido son estados distintos.
- Rough glass: pirámide filtrada compartida con política de edge/depth/roughness; medir halos, coste/memoria y estabilidad.
- Particle lighting atlas: resolver luz a menor frecuencia/resolución sin aumentar errores visibles; evaluar después del VFX base, no antes.
- Coherencia y subgroups: primero algoritmo portable; features/tamaño/operaciones consultados, fallback y tests.
- Dynamic resolution/temporal/async: presupuestos de latencia y p95/p99, no solo media GPU; preservar nitidez, input y XR.

El principio de coste conservado es evaluar cada señal con la resolución/frecuencia/precisión suficiente para su error visible. El dataset 2016, sus máximos de decals, atlas y bits no se vuelven constantes de Jerico.

## 6. GI sin importar Cry ni activar RT

No copiar SVOGI ni shaders de CryEngine2. JSVGI es intención de GI propia con regiones sucias, sparse bricks/clipmaps/residency e iluminación difusa presupuestada, no un port autorizado ni foundation inmediata.

Auditar DiffuseProbeGrid con especial atención: realtime es ray traced; modo baked puede consumir datos sin RT, pero generar esos datos puede requerir RT. Con la fase actual, no usar Auto Select ni dejar RT como dependencia oculta. Si hace falta GI temprana, seleccionar un método compatible documentado y probarlo; IBL/AO no se etiquetan GI completa. Un futuro permiso para baking RT se trataría aparte, no se presume.

## 7. Entorno y agua

Preservar un único clock/Environment/sun/camera/exposure. SkyAtmosphere/Stars existentes primero; compare shadowed shafts/fog con fuentes independientes cuando corresponda. Epipolar de Egor Yusov, Practical Implementation of Light Scattering Effects, **GDC 2013**, es referencia distinta de SIGGRAPH; DiligentFX es implementación potencialmente útil con licencia/pin, no un nuevo renderer.

Agua vista desde arriba es el alcance conservado del juego. O3DE necesita su propia integración y prueba; la aceptación previa Cry no viaja. Local volumes, surface reflection/transparency/contact, motion y material budget por escala. Underwater queda diferido; costa Horizon es follow-up y no resuelve todo el agua. Flotabilidad continúa DESIGN ONLY en el master físico.

## 8. Rendimiento y aprendizaje de la base anterior

ACTIVE_SLICE histórico 2026-10-10 documenta una escena fija a 1236x320 con sky aproximadamente 24.8 ms y root 28.1 ms, aun con cientos de miles de checks correctos. Es evidencia reportada del renderer Cry/Diligent, no medida de Atom ni benchmark comparable. Su utilidad aquí es exigir coste real desde N0; no portar esa implementación por su volumen de tests.

Registrar GPU/driver/backend/build/settings/scene/camera/resolution, warmup/steady windows, CPU frame y GPU timings, p50/p95/p99, peak/resident RAM/VRAM, shader compile/cook e input responsiveness. Diferenciar cold-load y steady, tiempos de espera y presentación. No claim de leak o ISA/counters no accesibles.

Optimización: baseline → control negativo → cambio → A/B/A → coste/calidad/memoria → aceptación o rechazo. No aumentar tolerancias para salvar un shader. Denoising/temporal altera imagen deliberadamente y necesita métricas apropiadas, no igualdad byte a byte universal.

## 9. Fuentes y estado de lectura

Auditorías privadas leídas por secciones y conservadas por referencia: JericoEngine2@304d9d2e13704e4967e82a8d5a13a5a506be01d4 `docs/RENDERING_RASTER_SIGGRAPH_MASTER.md` (incluido tramo de destino y follow-up costero), `docs/JERICO_RBRUSH_IDTECH6_CLUSTERED_DECALS_SPEC.md`; JericoEngine `docs/research/IDTECH6_SIGGRAPH2016_FULL_AUDIT.md`, `2026-07-31-siggraph-real-time-engine-atlas.md`. No se afirma relectura íntegra de todos los PDFs ni de todos los diarios de diagnóstico.

Fuentes primarias verificadas o índices de localización para la próxima lectura focal:

- https://advances.realtimerendering.com/s2016/index.html — idTech, LTC, rapids y temporal.
- https://advances.realtimerendering.com/s2015/index.html — GPU-driven y volumétricos.
- https://sebh.github.io/publications/ — trabajos de atmósfera/volumétricos del autor.
- https://advances.realtimerendering.com/s2022/index.html — Nubis y Rendering Water in Horizon Forbidden West.
- https://advances.realtimerendering.com/s2025/index.html — HypeHype y trabajos a clasificar por dependencia RT.
- https://advances.realtimerendering.com/s2026/index.html — Smolder, SLIM y subdivisión; abstracts/index, no implementación auditada.
- https://jcgt.org/published/0014/01/06/ — EON, publicado 2025 y actualizado 2026-02-04; usar versión corregida.
- https://docs.o3de.org/docs/atom-guide/ — puntos de entrada oficiales de Atom.

Antes de implementar, guardar título/autores/año, URL primaria, versión/hash del material/código usado, sección leída, licencia, aplicación mínima, owner Atom, prueba/fallback, límites y criterio de rechazo en la ficha del vertical. No republicar PDFs sin permiso ni llamar nuevo paper a una auditoría local.
