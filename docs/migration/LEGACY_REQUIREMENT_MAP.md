# Rescate JericoEngine2 → JericoEngine/O3DE

Este es un mapa de requisitos/procedencia, no un manifiesto de archivos autorizados para copiar. Fuente principal inspeccionada: **JericoFX/JericoEngine2@304d9d2e13704e4967e82a8d5a13a5a506be01d4**. Los repositorios anteriores permanecen sin modificar.

## Decisiones que se sustituyen

| Antes | Ahora |
|---|---|
| CryEngine 5.7.1 como implementación base | Snapshot oficial O3DE en development |
| O3DE reference-only y prohibición de Atom/AZ | O3DE es la base; aprovechar owners existentes, sin copiar arquitectura innecesaria adicional |
| JericoRenderGraph/Diligent obligatorio | Atom RPI/Pass/RHI/FrameGraph→Vulkan, una autoridad |
| Auditar/portar Cry shader antes de cada pass | Auditar Atom y fuentes públicas permitidas; Cry solo requisitos/historia |
| MNM derivado de Cry | Recast/Detour candidato, con provider físico auditado |
| Port de SVOGI hacia JSVGI | GI nueva/adaptación permisiva evaluada, sin source Cry |
| Restablecer/borrar implementación paralela antigua | No tocar el repo histórico; no ejecutar aquellas órdenes de reset |
| Topologías antiguas Astra único escritor/Sol prohibido | Última directiva Git 2026-10-09: Sol padre + dos hijos disjuntos + Luna asignada |
| Geología aparece como familia general en docs viejos | No frente activo; math/daño/cables/modular sobreviven, no geology engine |
| Pruebas viejas aceptadas en nueva base | Evidencia histórica separada; O3DE empieza NOT VERIFIED |

## Documentos de origen y destino

Cada enlace fija el SHA, no la rama mutable. Se inspeccionaron documentos completos cuando eran pequeños y tramos de contratos/destino cuando eran extensos. No se afirma auditoría completa del source ni de todos los diarios. Antes de implementar, leer el tramo relevante completo del origen y confirmar que el nuevo contrato lo representa o lo sustituye explícitamente.

Base de enlaces: https://github.com/JericoFX/JericoEngine2/tree/304d9d2e13704e4967e82a8d5a13a5a506be01d4 .

| Origen en JericoEngine2 | Rescate | Destino |
|---|---|---|
| GOAL.md | Producto/formatos, Vulkan-only, agua cerrada, native workflows | GOAL + REQUIREMENTS_MATRIX |
| AGENTS.md | Última coordinación, one-owner, evidencia/Git, no microbloqueos | AGENTS; órdenes Cry históricas sustituidas |
| docs/ROADMAP.md / CODEX_PERSISTENT_GOAL.md | Familias/objetivo persistente, no orden J viejo | ROADMAP N0-N12; un GOAL canónico |
| docs/DEFINITION_OF_DONE.md | Integrated no equivale a compiled | DEFINITION_OF_DONE |
| docs/JERICOEDITOR_MASTER.md / JERICOEDITOR_VISUAL_DESIGN.md | Shell/workspaces/Inspector/blueprints/dialogs | EDITOR_ASSET_WORLD_MASTER |
| docs/EDITOR_AUTOMATION_MCP.md | Bridge tipado/transacciones/automatización | Editor master + AGENTS |
| docs/RUNTIME_FORMATS.md | Catalogo r*/epak, bounds/versioning/dependencies | Editor/assets master; wire layouts viejos no congelados |
| docs/JERICOENGINE1_TRANSFER_MAP.md | Import/input/save/items/narrative/trust/accessibility, donors | Todos los masters/matriz; tabla de transfer viejo no acredita código |
| docs/SCRIPTING_GRAPHCORE_GAMEPLAY_MASTER.md | Luau/references/Store/triggers/input/HTN graph | SCRIPTING_AI_GAMEPLAY_MASTER |
| docs/WORLD_AI_STREAMING_MASTER.md | World/streaming/HTN/sensores/relaciones/grupos/aprendizaje | Script/AI + Editor/World masters; MNM no |
| docs/AUDIO_VFX_ANIMATION_UI_MASTER.md | Rigs/IK/fitting/bimanual/UI/timeline/audio/XR | ANIMATION_UI_AUDIO_XR_MASTER + N5/N6 gates |
| docs/OPENXR_XR_MASTER.md | XR compartido, no plugin tardío | GOAL/N0-N1 contracts/N10 aceptación |
| docs/PROCEDURAL_GEOMETRY_DESTRUCTION_MASTER.md | Splines/RMF/daño/estructuras/collision/bake | PROCEDURAL_PHYSICS_ENVIRONMENT_MASTER |
| docs/JERICO_MODULAR_NETWORK_RMOD_SPEC.md | RMod vs rpart, ports/sockets/routes/fittings/supports | Procedural master; no PipeSystem hardcoded |
| docs/JERICO_RBRUSH_IDTECH6_CLUSTERED_DECALS_SPEC.md | BRUSH/PAYLOAD/TARGET y decals correctos | Procedural + Render master |
| docs/JERICO_2D_2_5D_PIXEL_ISOMETRIC_VERTEX_VEHICLE_SPEC.md | Perfiles/sprites/normals/isométrico/vertex/vehicles | PRESENTATION_PROFILES_MASTER |
| docs/RENDERING_RASTER_SIGGRAPH_MASTER.md | Agenda, restricciones, lecciones de coste y agua | Render master; no ports Cry/Diligent obligatorios |
| docs/ENGINE_DEBUGGING_BOOK.md | Método y errores comprobados, no causas Atom supuestas | Error book inicial + referencias históricas |
| docs/DEPENDENCY_BASELINE.md | Método de pins/licencias, no descargar todo ese SDK tree | legal/PROVENANCE_AND_DEPENDENCIES |
| docs/LEGAL_PROVENANCE_BOUNDARY.md | Identidad no borra licencia | Legal master ampliado |
| docs/CRY_LUMBERYARD_O3DE_REFERENCE_ONLY.md | Lecciones de desacoplamiento, rechazo duplicados/plugin chains | Arquitectura nativa; reference-only sustituido |
| docs/CRY_TO_JERICO_TRANSFORMATION_MASTER.md / CRYENGINE_5_7_AUDIT.md | Historia, no ejecución ni fuente a trasladar | H: referencia histórica |
| docs/XML_PURGE_AND_FORMAT_MIGRATION.md | Sin XML propio en producto final | Editor/assets master; no purga de XML tercero/tool |
| docs/design/JERICO_BUOYANCY_AND_FLUIDS_DESIGN.md | Enable Buoyancy, fluid queries, fuerzas sin renderer | Procedural master, DESIGN ONLY intacto |
| docs/design/JERICO_BUOYANCY_AND_FLUIDS_CODE_AUDIT_2026_10_08.md | Dependencias/errores físicos como referencia | Cuarentena de código Cry; requisitos, no copia |
| docs/design/CRY_SUBSYSTEM_AUDIT_BACKLOG_2026_10_08.md | Checklist de auditoría por consumidores | N0 y protocolo por familia; no reparar Cry |
| docs/work/ACTIVE_SLICE.md / HANDOFF.md / evidence | Pruebas/pins/datos originales del motor anterior | Evidencia histórica solo, no activar J3A |
| images/ y referencias visuales | Dirección del editor como fuente pendiente de admisión visual/licencia | No imágenes copiadas/inspeccionadas íntegramente por este commit |

## Fuentes JericoEngine1 que siguen localizables

Repositorio `JericoFX/JericoEngine` (no confundir con el nuevo `Jerico_Engine`). Se leyeron tramos de:

- `docs/research/IDTECH6_SIGGRAPH2016_FULL_AUDIT.md`, blob observado `aa5ca244f8cf2dac67eef50172c617ef26cdeb18`.
- `docs/research/2026-07-31-siggraph-real-time-engine-atlas.md` (selección y familias; revisar HEAD/hash al descargar).
- `docs/PROCEDURAL_PWAS_EPLS_MASTER.md`, blob observado `4f89d6ba268a78148d40450ae6cdb545d15c1e8f`.

El transfer map histórico también conserva rutas de Asset Import/Cooking, runtime chunks, incremental cook/delta epak, hot reload, input 1.0, gameplay API, physics/Jolt, audio, Motion Studio/UI y evidence de Grip. Son cola de lectura por familia, no todos inspeccionados nuevamente por esta publicación. No atribuirles INTEGRATED hasta verificar aquí.

Playground `JericoFX/JericoGeologyPlayground`: consulta futura de math/cables/edge/RMF/assembly/bake y `docs/PHYSICAL_CHAIN_AND_ROCK_EDGE_MATH_2026_10_03.md` si existe en el checkout real. Esta última ruta proviene de la conversación/registro histórico y requiere localizarse antes de citar contenido; no se afirma presencia verificada en esta auditoría.

## Clasificación de material rescatable

**ADOPT CONTRACT:** intención de producto original, UX, criterios de aceptación, formatos como semántica, invariantes y decisiones actuales. Los nuevos masters los reexpresan sobre O3DE.

**AUDIT CODE/TEST CANDIDATE:** implementaciones originales previas a Cry de Luau, Jolt adapters, math/codec/command/tests. Requiere expediente por archivo, fixtures/assets/transitivas y dependencia real. Si está acoplado a Cry o su procedencia es incierta, no se copia.

**HISTORY ONLY:** resultados Cry, shaders traducidos/adaptados, MNM/SVOGI, inicialización heredada, GUI/MFC/Scaleform, WAF/builds/packages/source assets de licencias no admitidas. Se pueden formular requisitos observables independientes; no importar la implementación.

**RESEARCH ONLY:** papers/publicaciones/addons y ramas alternativas. No autoriza copiar PDFs, assets o code privado.

## Estado histórico que no se debe falsear

El 2026-10-09 el propietario cerró el agua para esa etapa, underwater quedó diferido y una repetición final source-free no se ejecutó. Al snapshot del 2026-10-10 J3A seguía NOT PASS; el sky de una escena fija consumía ~24.8 ms GPU frente a root ~28.1 ms según ACTIVE_SLICE. Ese dato sirve para recordar medir coste/consumidor real, no para atribuir ese problema a Atom.

Los reportes previos de import/LOD/cook y shaders son evidencia del producto previo y contienen límites explícitos. No copiar cifras de checks a la nueva aceptación; no usar supuesta superioridad de O3DE como resultado de benchmark.

## Antes de cualquier eliminación histórica

Esta tarea NO elimina el checkout/repo Cry. Antes de que el propietario retire archivos locales: verificar commits y objetos necesarios, preservar evidencia/documentos/artefactos no versionados con permiso y licencia, identificar cambios sin publicar, y distinguir borrar copia local de borrar repositorio GitHub/historial. No borrar el repositorio de evidencia porque otro checkout ya compile.

## Precisión obligatoria — materiales, formatos, deltas y DLC

El master `docs/ASSET_MATERIALS_FORMATS_EPAK_MASTER.md` conserva como requisitos activos: semánticas PBR/ORM y mapas separados, `.rmat/.rtext/.rmesh` con metadata explícita, conversión source→Canonical IR→Asset Processor/cook, streaming source-free, `.epak` por chunks y overlays delta con rollback, políticas de hash/firma/cifrado y DLC ContentPacks/CampaignRegistry/saves por CampaignId. Leer antes de cualquier sustitución de Material Editor, Asset Bundler o VFS. La selección de bibliotecas Zstd/BLAKE3/libsodium necesita auditoría legal y test, no copia automática de código histórico.
