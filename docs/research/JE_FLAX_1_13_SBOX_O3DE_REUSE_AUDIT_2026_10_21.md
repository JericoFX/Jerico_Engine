# Auditoría conjunta: Flax 1.13 + s&box contra O3DE y JericoEngine

**Fecha:** 2026-10-21. **Fuente JE:** [development, commit 28fd7be](https://github.com/JericoFX/Jerico_Engine/commit/28fd7be7422968b98e2d9c9372e7dd681484025b).
**Estado:** RESEARCH ONLY / STATIC REVIEW / NO PRODUCT PASS. No se compilaron ni ejecutaron las tecnologías externas, ni se midió paridad de rendimiento.

## Contrato antes de cualquier uso

Este documento es **una lista de referencias, no un plan de implementación**. Manda GOAL.md, AGENTS.md, ROADMAP.md y docs/work/ACTIVE_SLICE.md. Un único repositorio JericoFX/Jerico_Engine y rama de trabajo development; integrar tecnología O3DE en módulos nativos JE:: sin crear Gems activables por proyecto ni otro motor/editor. Sin AWS, multiplayer, Python de producto ni C# de gameplay: Luau sigue siendo la VM autorizada. Vulkan es el backend actualmente activo, con DDGI/RT y OpenXR; DX12 permanece en fuente para su eventual recuperación según órdenes vigentes.

La regla de evaluación es **existente en fuente / parcialmente equivalente / carencia no demostrada / fuera de alcance**. Encontrar una clase NO demuestra que su sistema esté integrado, activo ni probado en runtime. Cada hallazgo debe pasar por productor, consumidor, assets, configuración, licencia, build, Undo/Save/reopen y benchmark cuando corresponda.

## I. Flax 1.13: render y editor frente a O3DE / JE

Características desde el [anuncio oficial de Flax 1.13](https://flaxengine.com/blog/flax-1-13-released/), 21/10/2026.

| Referencia Flax | Implementación que O3DE/JE ya proporciona | Diferencia / decisión |
| --- | --- | --- |
| Occlusion Queries y HZB GPU | RPI View/Culling tiene MaskedOcclusionCulling por CPU con búfer jerárquico y planos manuales, además de frustum. | **PARCIAL.** No confundir Hi-Z software con HZB GPU de Flax. Benchmark antes de cualquier extensión; evitar dos owners. |
| PCSS, penumbra variable según tamaño de luz | Atom tiene sombras PCF, ESM, ESM+PCF, filtros BoundarySearch/Bicubic. | **PARCIAL.** Evaluar un modo PCSS sobre el shader de sombras, sin crear otro sistema. |
| DDGI filtrado/half-resolution y DDGI especular rugoso | DiffuseProbeGrid Vulkan RT + ReflectionProbe + ReflectionScreenSpace están en JE; DDGI tiene integración focal documentada 20/10. | **PARCIAL.** No rehacer GI ni SSR; medir leakage, rugosidad, VRAM y coste antes de extender. |
| Reversed-Z | JE cámara, Math/MatrixUtils y frustum contemplan reverse depth. | **YA EXISTE. NO REIMPLEMENTAR.** Solo validar uso coherente en shaders/pases. |
| GPU Memory Profiler con preview/export de textures | JE contiene ImGuiGpuProfiler, memoria de recursos y captura Save/Load, enlazado desde AtomImGuiToolsSystemComponent. | **PARCIAL.** Un visor de render targets puede agregarse al panel actual si falta. |
| Properties Panel Search | FilteredDPE, ValueStringFilter y ComponentEditor filtran propiedades usando QLineEdit. | **YA EXISTE EN FUENTE.** Conectar a Jerico Inspector y verificar UI; no duplicar. |
| Instancing de personajes animados mediante buffer global de huesos | MeshFeatureProcessor y MeshInstanceManager existen; la cvar r_meshInstancingEnabled se declara false por defecto; SkinnedMeshInstance y GPU skinning están en fuente. | **NO DEMOSTRADA PARIDAD.** Instancing de malla regular no demuestra batching de skinned meshes. Medir antes de programar. |
| TAAU/FSR 4 | TaaPass con jitter y temporal history; no hay paridad TAAU/FSR 4 validada. | **PARCIAL.** Prioridad baja, VR se prueba aparte. |
| Wireframe overlay mejorado y nodos redimensionables | Viewport debug / GraphModel / EditorTools parcialmente integrados. | **CARACTERÍSTICAS A AUDITAR**, no reemplazar el editor o GraphCanvas. |
| Debug vivo de Behavior Trees | AnimGraph de EMotionFX no es BT; JE tiene IA/HTN prevista. | **IDEA PARA N9.** Colorear decisiones/sensores en depuración, no importar BT de Flax. |
| Referencias de objeto tipadas, serializables y prefab-safe | JE tiene GUID, serialización, Prefab, referencias Scene/PrefabInternal y contratos Luau previstos. | **PARCIAL.** Extender referencias existentes. |
| UI Post Process con RenderToTexture | LyShine/UiCanvasComponent y Camera RTT ya soportan renderizado a textura. | **BASE EXISTENTE.** Añadir material de postproceso en JericoUI solo con consumidor real. |
| .NET 10, online platform, C#14 y multiplayer | Incompatibles con restricciones de producto o no prioritarios. | **FUERA DE ALCANCE.** |

**Fuente JE precisa:** [Culling.cpp](https://github.com/JericoFX/Jerico_Engine/blob/development/Engine/Render/Integration/Source/RPI/Public/Culling.cpp) (el comentario del TestRect limita el buffer a planos colocados manualmente); [View.cpp](https://github.com/JericoFX/Jerico_Engine/blob/development/Engine/Render/Integration/Source/RPI/Public/View.cpp); [Shadow.azsli](https://github.com/JericoFX/Jerico_Engine/blob/development/Engine/Render/Features/Assets/ShaderLib/Atom/Features/Shadow/Shadow.azsli); [DiffuseProbeGrid](https://github.com/JericoFX/Jerico_Engine/blob/development/Engine/Render/Features/Source/DiffuseProbeGrid/Render/DiffuseProbeGrid.cpp); [ReflectionProbe](https://github.com/JericoFX/Jerico_Engine/tree/development/Engine/Render/Features/Source/ReflectionProbe); [ReflectionScreenSpace](https://github.com/JericoFX/Jerico_Engine/tree/development/Engine/Render/Features/Source/ReflectionScreenSpace); [GPU Profiler](https://github.com/JericoFX/Jerico_Engine/blob/development/Gems/Atom/Utils/Code/Source/ImGuiGpuProfiler.cpp); [FilteredDPE](https://github.com/JericoFX/Jerico_Engine/blob/development/Code/Framework/AzToolsFramework/AzToolsFramework/UI/DocumentPropertyEditor/FilteredDPE.cpp); [FeatureCvars](https://github.com/JericoFX/Jerico_Engine/blob/development/Engine/Render/Features/Source/FeatureCvars.cpp); [SkinnedMesh](https://github.com/JericoFX/Jerico_Engine/tree/development/Engine/Render/Features/Source/SkinnedMesh); [TaaPass](https://github.com/JericoFX/Jerico_Engine/blob/development/Engine/Render/Features/Source/PostProcessing/TaaPass.cpp).

## II. s&box: estudiar su experiencia de autoría, no portar Source 2

La documentación actual de s&box distingue **Scene Mapping moderno** de **Hammer nativo**. El primero ofrece modos de primitivas, edición de vértices/aristas/caras, Vertex Paint, Displacement y gizmos de posición, rotación, escala, resize, pivote y shear. No debe suponerse que las DLL de Hammer/ModelDoc son herramientas compatibles o redistribuibles en JE.

| Herramienta o experiencia s&box | Base O3DE / JE disponible | Decisión |
| --- | --- | --- |
| Scene Mapping con primitivas, vertex/edge/face, UV/texturas y gizmos | **Engine/Geometry** ya contiene EditorWhiteBoxComponent y WhiteBoxToolApi, junto a AzToolsFramework Component Modes. | **ALTA PRIORIDAD DE UX**. Aprovechar White Box/BSP; mejorar modo de edición y tests de geometría, materiales por cara y persistencia. No otro mesh editor. |
| Vertex Paint, Displacement y brushes | AzFramework/PaintBrush, GradientSignal, Terrain y TerrainMacroMaterial; RBrush y geología en GOAL. | **PARCIAL.** Reutilizar brush, filtros y dirty regions; integrar en tools JE con Undo/cook, sin evaluación paralela permanente. |
| Texture Generators (color, degradado, texto, SVG, generators C#), CacheToDisk | Asset Processor, C++ Asset Builders, ImageProcessingAtom y OIIO/OCIO nativo en JE. | **OPORTUNIDAD REAL.** Diseñar autoría C++ de textura procedural cocinada y cacheada si un consumidor lo pide; no meter C# ni Python runtime. |
| Shader Graph, Material Editor, editor de variables y errores | Atom Material Canvas + Material Editor + GraphCanvas; integración focal GraphModel105/EditorTools/AtomToolsFramework. | **REUTILIZAR.** Conectar Propiedades y Nodos al mismo asset/material existente, respetando grafo, constantes, Save/reopen y preview. |
| ModelDoc nodal y edición de assets de modelos | Scene Processing, EMotionFX, Scene Settings, material slots, skeleton/rig. | **MEJORAR WORKFLOW**, no importar herramientas nativas de Source 2. Reusar origen de datos, colisiones, sockets, import y preview JE. |
| Asset Previews por tipo con cámara y escena | AtomTools PreviewRenderer / Asset Browser / Material Editor. | **REUTILIZAR.** Corregir defectos como Preview mip8 y adaptar previews de rocas, puertas, clips y materiales. |
| Custom Editor Tools, docks, InspectorWidgets | Qt/AzQtComponents, AzToolsFramework, ComponentModes, DocumentPropertyEditor y EditorModeFeedback JE. | **MEJORAR UX/APIs propias JE** y no duplicar sistema de ventanas o adoptar widgets C#. |
| UndoScope granular por objetos/propiedades | AzToolsFramework/UndoSystem, WhiteBoxMeshAssetUndoCommand y GraphModel history. | **NO REINVENTAR.** Usar transacciones actuales; mejorar granularidad por operación cuando sea necesario. |
| Prefab templates, nested prefabs, overrides | AzToolsFramework/Prefab/Overrides, PrefabSystem, SpawnableEntitiesManager y GUID. | **NO REINVENTAR.** Optimizar UI y flujo, conservar referencias/overrides. |
| **MCP Server local de editor**: escena, assets, Play, capturas, consola; descubrimiento dinámico de tools | JE especifica commands/events y IPC local de herramientas, pero no se confirmó un servidor MCP C++ con paridad s&box dentro del checkout. | **CANDIDATO INTERESANTE, FUTURO.** Si se autoriza: loopback, read-only primero, mutaciones transaccionales/Undo, seguridad, tool discovery dinámico y contratos JE. No activar ahora. |
| **Movie Maker**: animación de cualquier propiedad, cámara, rig/IK, gameplay recording, nested sequences | Maestro y TrackView, EMotionFX/Motion Studio y futuro rseq. | **MUY ÚTIL COMO REFERENCIA UX.** Reutilizar timeline/curvas/rig, estudiar grabación y secuencias anidadas. No replicar todo Movie Maker ni el renderizador de vídeo. |
| ActionGraph visual | ScriptCanvas/GraphCanvas en fuente O3DE; JE usa Luau para gameplay. | **SOLO PATRONES VISUALES**. No añadir otra VM de juego. |
| C# hotload, red/cloud/servicios de s&box | O3DE ya tiene recook incremental y hot reload de assets; no es lo mismo que recompilación C#. | **FUERA DE ALCANCE** del producto; conservar Luau, asset pipeline y C++ nativo. |

**Documentación primaria s&box:** [Scene Mapping](https://sbox.game/dev/doc/editor/mapping/); [Texture Generators](https://sbox.game/dev/doc/editor/texture-generators); [MCP](https://sbox.game/dev/doc/editor/mcp-server); [Shader Graph](https://sbox.game/dev/doc/rendering/shader-graph/); [Editor Widgets](https://sbox.game/dev/doc/editor/editor-widgets); [Custom Editors](https://sbox.game/dev/doc/editor/custom-editors); [Undo System](https://sbox.game/dev/doc/editor/undo-system); [Asset Previews](https://sbox.game/dev/doc/editor/asset-previews); [Prefabs](https://sbox.game/dev/doc/scene/prefabs/); [Movie Maker](https://sbox.game/dev/doc/movie-maker/); [Sequences](https://sbox.game/dev/doc/movie-maker/sequences); [Model Editor](https://sbox.game/dev/doc/editor/model-editor).

**Código equivalente ya presente en JE:** [EditorWhiteBoxComponent](https://github.com/JericoFX/Jerico_Engine/blob/development/Engine/Geometry/Source/EditorWhiteBoxComponent.cpp); [WhiteBoxToolApi](https://github.com/JericoFX/Jerico_Engine/blob/development/Engine/Geometry/Include/JE/Geometry/WhiteBoxToolApi.h); [PaintBrush](https://github.com/JericoFX/Jerico_Engine/blob/development/Code/Framework/AzFramework/AzFramework/PaintBrush/PaintBrush.cpp); [GradientSignal](https://github.com/JericoFX/Jerico_Engine/tree/development/Gems/GradientSignal); [Terrain](https://github.com/JericoFX/Jerico_Engine/tree/development/Gems/Terrain); [MaterialCanvas](https://github.com/JericoFX/Jerico_Engine/tree/development/Gems/Atom/Tools/MaterialCanvas); [UndoSystem](https://github.com/JericoFX/Jerico_Engine/blob/development/Code/Framework/AzToolsFramework/AzToolsFramework/Undo/UndoSystem.cpp); [Prefab Overrides](https://github.com/JericoFX/Jerico_Engine/tree/development/Code/Framework/AzToolsFramework/AzToolsFramework/Prefab/Overrides); [TrackView](https://github.com/JericoFX/Jerico_Engine/tree/development/Code/Editor/TrackView); [Maestro](https://github.com/JericoFX/Jerico_Engine/tree/development/Gems/Maestro); [EMotionFX](https://github.com/JericoFX/Jerico_Engine/tree/development/Gems/EMotionFX). Revisar el mapa de archivos actual antes de editar: algunas Gems ya están absorbidas en Engine/ y otras permanecen como fuentes históricas.

## III. Qué NO debe reinventarse

- **BSP/editor:** White Box, Component Modes, gizmos y Undo antes de programar modos de malla desde cero.
- **Materiales:** MaterialCanvas/GraphModel, Material Editor, ImageProcessing y AssetProcessor antes de un sistema de texturas o graph nuevo.
- **Render:** DDGI, SSR, probes, shadows, culling, instancing y TAA existentes antes de módulos duplicados.
- **Animación/secuencias:** Maestro, TrackView y EMotionFX antes de sustituirlos por un Movie Maker propio completo.
- **Sistema de mundos:** Prefab/Overrides/Spawnables/GUID/AssetManager antes de tablas propias paralelas.
- **MCP:** si se autoriza en una etapa futura, una interfaz local sobre comandos JE existentes; nunca otro editor, otro mundo ni un servidor de escucha pública.

## IV. Fixtures propuestos — NO EJECUTADOS

1. **BSP.MappingRoom:** habitación→pasillo→puerta/ventana→editar vértices/caras→material/UV por cara→vertex paint→Undo/Redo→Save/reopen→cook/Runtime. Ver colisión y topología/manifold.
2. **Rendering.Occlusion:** pasillo en L, puerta móvil, 100/1000/10000 props y cámara brusca; frustum-only vs culling jerárquico software vs propuesta GPU si hay evidencia. CPU/GPU p50/p95/p99, VRAM, draw calls, popping, falsos ocultos.
3. **Lighting.WetFloor:** piso mojado, metal, rugosidades variadas, focos y lámparas; SSR/probes/DDGI/PCF actual vs propuesta PCSS/DDGI specular; imagen, MS/VRAM y leakage.
4. **Assets.TextureGenerator:** gradiente/ruido con seed, cache determinista, cambio de parámetro, AP, color-space, cancelar sin publicar mitad, reimport/Save/reopen, KTX/formato aceptado y cook.
5. **Animation.SkinnedInstancing:** 10/100/1000 arañas mismo rig/material, poses distintas, sombras y motion vectors; comprobar draw calls y skinning antes de decidir extensión.
6. **Editor.MCP (futuro):** leer estado/escena/assets/console/capturas, mutación autorizada con diff, Undo y Save/reopen; control local y sin exponer autoridad de GPU/física/archivo arbitraria.

## V. Licencias y exclusiones

- **Flax**: fuente disponible públicamente, pero [LICENSE.md oficial](https://github.com/FlaxEngine/FlaxEngine/blob/master/LICENSE.md) remite a [Flax EULA](https://flaxengine.com/licensing/); no copiar fuente ni shaders al árbol JE sin permiso válido y revisión legal. Preferir investigación de ideas y desarrollo independiente.
- **s&box**: el [README oficial de sbox-public](https://github.com/Facepunch/sbox-public) declara MIT para fuente del repositorio, **no para los binarios nativos de Source 2 en game/bin**, sujetos a [s&box EULA](https://facepunch.com/legal/sbox/eula). Assets, herramientas nativas y terceros tienen derechos separados. Cualquier reutilización de fuente MIT exige comprobación archivo por archivo, notices y dependencias, y no autoriza incluir C#/.NET ni bins Source 2 en producto JE.
- **O3DE**: conservar avisos y licencias Apache-2.0/MIT y terceros según [procedencia JE](../legal/PROVENANCE_AND_DEPENDENCIES.md). No clonar, instalar ni enlazar Flax/s&box como consecuencia de esta auditoría.

## VI. Instrucción para Codex

> Esta ficha es solo investigación. Antes de añadir una capacidad inspirada en Flax 1.13 o s&box, inspecciona el owner real de O3DE/JE, API, consumidores, assets, licencias, pruebas y estado de integración. Clasifica EXISTENTE EN FUENTE, PARCIAL o NO DEMOSTRADO, sin confundir presencia con funcionamiento. Prioriza reutilizar e integrar antes que reemplazar. Contrasta con GOAL y ACTIVE_SLICE; no crear nuevas ramas/Gems/VMs/renderers ni ampliar alcance por esta lista. No usar Flax EULA ni binarios Source 2. Solo proponer extensiones medibles y con pruebas/rollback cuando el corte vigente las requiera.

**Conclusión:** Flax sirve como comparación de **render y diagnósticos**; s&box sirve como comparación de **herramientas y workflows del editor**. O3DE ya contiene muchas de las piezas: la oportunidad de JE es integrarlas, optimizarlas y ofrecer una UX clara sin reinventar otro motor.
