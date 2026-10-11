# JE Editor, Assets y World — BSP como primer flujo

**Vigente 2026-10-10.** Contrato de destino; implementación no acreditada. GOAL y contrato JE superiores. Fuente O3DE/Atom se reutiliza y migra a nombres/owners JE, no API pública AZ ni facade perpetua. Renders permitidos DX12/Vulkan, AWS/multiplayer excluidos.

## 1. UI Jerico y authoring nativo

Qt6 Widgets sobre editor/AzToolsFramework/AzQtComponents actuales adaptados. Viewport central grande compartido por workspaces, Outliner/paletas izquierda, Inspector derecho, logs/curvas/timelines/profiler abajo. Grafito/navy con cian y ámbar de selección; tipografía legible, DPI, iconos/texto además de color, shortcuts y docking persistentes. World/Geometry-BSP/Materiales primero; Assets/Import/Procedural-Geology/Physics/Motion/AI/Audio/VFX/Sequencer/UI/Luau/XR/Profiler según consumidor.

No un editor nuevo aislado ni pantallas mock para demostrar productividad. Cambiar tab mantiene scene/selection/viewport. Edit/Simulate/Play usa runtime systems reales. Diálogos de importar/guardar/cancelar/overwrite/progreso/error/renderer/device/cierre con cambios/recovery están incluidos, no esconderlos tras screenshot bonito.

Primera prueba: cuarto+corredor, cut de puerta/ventana, caras/UV/material, Undo/Redo/Save/reopen y mesh runtime. WhiteBox/Manifold CSG es base real a probar; modelar un cubo no acredita BSP completo. Herramientas avanzadas entran por operación persistente, no todo el algoritmo de golpe.

### 1.1. Hammer 2 + Jerico custom — SPECIFIED/DESIGN

El detalle visual recuperado es **SPECIFIED/DESIGN**: no acredita implementación, captura del Editor ni producto verificado. Procedencia: `JericoFX/JericoEngine2@304d9d2e13704e4967e82a8d5a13a5a506be01d4`, `docs/JERICOEDITOR_MASTER.md` (blob `e35f62b5d56bc89ffe30dd6fd4c99df75a53cd43`) y `docs/JERICOEDITOR_VISUAL_DESIGN.md` (blob `b6a125dfa4c7f424554eb5ef48252d7d9e609836`). GOAL vigente conserva la claridad Hammer/Source 2 y la identidad Jerico custom, con el Editor Qt6 real y Atom adaptado; las decisiones históricas de Python/plugins productivos, Diligent o Vulkan-only quedan sustituidas.

La jerarquía distingue menú de aplicación, tabs de workspace, herramientas del workspace y tabs de cada panel. Se permiten tres capas compactas de chrome donde sean útiles: aplicación/contexto, menú y herramientas. Outliner/World/Assets/paletas a la izquierda, superficie de trabajo o viewport dominante al centro, Inspector a la derecha y docks inferiores con tabs de contenido, Output/Problems, graph, Simulation, Profiler y timeline/curvas según contexto; status bar persistente. Paneles delimitados, bordes finos fríos, profundidad tonal leve y controles casi cuadrados/radio pequeño facilitan lectura; no una superficie densa sin separaciones ni tarjetas grandes. [MASTER:35–63](https://github.com/JericoFX/JericoEngine2/blob/304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/JERICOEDITOR_MASTER.md#L35-L63), [VISUAL:5–55 y 161–170](https://github.com/JericoFX/JericoEngine2/blob/304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/JERICOEDITOR_VISUAL_DESIGN.md#L5-L170).

Docking/layouts persistentes, DPI y varios monitores pertenecen a esa misma aplicación. Cambiar workspace cambia paneles, herramientas y overlays, conservando documento/scene/World, selección, viewport compartido y stack de transacciones; no crea autoridades paralelas. El viewport conserva gramática de navegación, outline de selección, colores de gizmo, jerarquía de grid, header, nombres Perspective/Orthographic, modos Lit/Debug y feedback de status. Geometry admite vistas ortográficas sincronizadas cuando sean útiles, sobre el mismo contexto. [MASTER:9–17 y 51–63](https://github.com/JericoFX/JericoEngine2/blob/304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/JERICOEDITOR_MASTER.md#L9-L63), [VISUAL:57–69 y 121–122](https://github.com/JericoFX/JericoEngine2/blob/304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/JERICOEDITOR_VISUAL_DESIGN.md#L57-L122).

Tokens exactos de la paleta histórica; los overlays del dominio no cambian el chrome global. [VISUAL:19–41](https://github.com/JericoFX/JericoEngine2/blob/304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/JERICOEDITOR_VISUAL_DESIGN.md#L19-L41).

| Rol | Color | Rol | Color |
|---|---|---|---|
| Shell | `#081420` | Surface | `#0D1A26` |
| Panel | `#112334` | Raised | `#162C40` |
| Border | `#24465D` | Primary cyan | `#00C8E8` |
| Active orange | `#FF9418` | Selection | `#FFB347` |
| Primary text | `#E8EEF4` | Secondary text | `#AEBBC6` |
| Muted text | `#748897` | Grid minor | `#173347` |
| Grid major | `#29536B` | Success | `#2BC76F` |
| Warning | `#F3B43F` | Error | `#EF5350` |
| Axis X | `#EF5350` | Axis Y | `#42D37A` |
| Axis Z | `#42A5F5` | — | — |

Densidad aproximada al **100% de escala**, no medidas ya verificadas del Editor: toolbar/control **28–30 px**, filas compactas tree/table **22–24 px**, lista estándar **26–30 px**, tab strip **28–30 px**, separación de panel **6–8 px** y de grupo **8–12 px**. DPI preserva estas relaciones. El donante no fija aquí tamaños de fuente, anchos mínimos ni combinaciones de shortcuts: no inventarlos. [VISUAL:71–81](https://github.com/JericoFX/JericoEngine2/blob/304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/JERICOEDITOR_VISUAL_DESIGN.md#L71-L81).

Una capa de estilo/widgets/delegates Jerico compartida cubre tabs, árboles, secciones del Inspector, vector/transform, pickers de assets, campos color/material, sliders/números, búsqueda, tiles, icon buttons, validación/busy y drag/drop; QSS por sí solo no cumple ese contrato. El Inspector orientado a componentes expone **Entity Components** mediante el sistema de propiedades/reflection común. Todos los controles mantienen estados **Idle/Hover/Pressed/Focused/Disabled/Busy/Warning/Error/Success**, con foco de teclado visible independientemente del hover. La presentación Jerico debe ser más clara, separada y fácil de recorrer que Cry/Sandbox, reteniendo productividad y separación de tareas Hammer, sin clonarlo. [VISUAL:83–99, 136–144 y 161–170](https://github.com/JericoFX/JericoEngine2/blob/304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/JERICOEDITOR_VISUAL_DESIGN.md#L83-L170).

Scene combina Outliner/Entity Palette, viewport, Inspector y Output/Problems/Profiler; Materials combina biblioteca, preview PBR/graph y parámetros de superficie/capas; Geometry combina jerarquía/stack de acciones, perspectiva/ortho, Inspector y operación/diff/volúmenes. La velocidad de autoría recuperada comprende brushes aditivos/sustractivos, selección cara/arista/vértice, extrude/inset, bevel/chamfer, slice/knife/loop, bridge/fill, puertas/ventanas/arcos/escaleras, spline extrusion, conversión a mesh/assembly, material por cara y UV, con operaciones no destructivas donde sea práctico. Grid/snap y demás operaciones siguen el master procedural vigente; aquí no se agrega un mapa de teclas. [VISUAL:101–122](https://github.com/JericoFX/JericoEngine2/blob/304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/JERICOEDITOR_VISUAL_DESIGN.md#L101-L122), [MASTER:165–182](https://github.com/JericoFX/JericoEngine2/blob/304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/JERICOEDITOR_MASTER.md#L165-L182).

La autoridad visual declarada en el pin es `images/40_jericoeditor_blueprint_reference_board.jpg`, `images/41_jerico_brand_icon_reference.jpg`, `images/40_jericoeditor_scene_blueprint.png`, `images/41_jericoeditor_physics_blueprint.png` y `images/42_jericoeditor_materials_blueprint.png`. La marca de aplicación es la **J modular metálica naranja + blanca** suministrada; los iconos funcionales de herramientas/componentes son una familia coherente distinta. Los SVG `30_*` a `39_*` sólo describen layout/cobertura. Los blueprints se traducen a Qt6 y contratos reales, no se copian como screenshots; sus anotaciones exteriores/cotas no son chrome del producto. En este rescate se verificaron los textos/blobs y la presencia de esas referencias, **sin descargar ni inspeccionar sus píxeles**. La lectura visual pendiente y los nombres M10.5/VISUAL_REFERENCE no localizados se registran en la fila JERICOEDITOR del mapa de migración. [VISUAL:147–182](https://github.com/JericoFX/JericoEngine2/blob/304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/JERICOEDITOR_VISUAL_DESIGN.md#L147-L182).

La recuperación documental no adelanta aceptación: primero cuarto/corredor, puerta/ventana, material/UV por cara y Undo/Redo/Save/reopen reales en este Editor; después su cook/launcher. Apariencia, kernel CSG o configuración aislada no prueban ese flujo ni el diseño visible. Capturas y comportamiento del Editor siguen pendientes.

## 2. Transacciones y automatización

Selection/context→preview/diff→validate→transaction→modelo canónico→invalidation→notificación. Cancel/fallo no publican mitad de scene. Undo/Redo y overrides en instancias/prefabs respetan ownership. No escribir scene file a escondidas desde un panel.

UI, Automation Bridge/MCP local y comandos tools autorizados usan un registro tipado JE; queries no mutan, writes acotados/preview/cancel/Undo, validación de permisos/contexto. No arbitrary code desde un asset ni puertos públicos. IPC local Editor/AssetProcessor/debug no exige transporte de multiplayer; extraerlo como tool privado cuando corresponda. Shipping no incluye esas tools.

## 3. Identidad/World y transición JE

Una implementación de identity/revision/world, no otra tabla escribible para conciliar AZ IDs y GUID Jerico. Mapear source GUID/product ID/entity lifetime y scope Scene/PrefabInternal con claridad. Migrar types/names/UUIDs/reflection/codegen con writers/readers y converters: no renombre global que rompa datos ni alias JE=AZ como destino.

Refs persistentes incluyen identidad/scope/capability esperado; runtime handles incluyen contexto/generación y validación, nunca pointers serializados. Outliner→IDE crea/reutiliza self.references tipada; nombre de entidad sólo display, no identidad. Duplicate/instantiate/reparent/scene unload/remap preservan significado o reportan missing type/reference; no resolver silenciosamente por nombre.

World conserva hierarchy/prefabs/variants/layers/cells/HLOD/residency. Unload invalida jobs/callbacks/render/physics/nav de forma segura. No reescribir ECS por dogma o crear World paralelo para geología. Fields/meshes/instances que produce authoring se publican en el mismo sistema.

## 4. Import/cook y materiales

Inputs originales inmutables + settings→snapshot→Canonical IR→builders/Asset Processor→products/deps→cache→package. Adaptar pipeline existente a JE. Importadores tool-only ufbx/glTF/OBJ, texturas/heightmaps/audio según master material; un suffix no decide semántica. No Autodesk FBX SDK ni ATL/MFC impuestos.

Nombres libres de texturas, BaseColor/Normal/Roughness/Metallic/AO/Emission/Opacity explícitos, ORM R/AO G/roughness B/metal y mapas separados, color-space/normal-Y/UV/scalars editables. Source case/path traversal/symlink/size/external dependencies validados antes de reservar o publicar. Modelo pesado con progreso/cancel, no congelar escritorio.

Material por cara de BSP/UV/region IDs debe sobrevivir CSG, reimport y Undo. Geología reutiliza campos semánticos en triplanar/strata/erosión/interior; no otra representación de material. MaterialGraph/instances/variants y SRGs sobre Atom adaptado, no segundo shader VM. Physics material separado de apariencia.

Detalles obligatorios están en `ASSET_MATERIALS_FORMATS_EPAK_MASTER.md`: tangentes/handedness, mips/odd dimensions/normal filter/alpha, fuentes/formats, readers/crypto/package. No sustituirlos por extensión de archivo reconocida.

## 5. Reimport/hot reload

GUID estable, recipe/importer/version/seed/hash de fuente+deps y overrides. Candidate-first con validación del cierre, publicación del selector al final, recursos anteriores vivos mientras haya GPU/audio/physics readers. Cancel/invalid no activa partially built scene. Invalidation por dependencias/revisión, no timestamp global; texture-only no recook geometry/physics sin relación real.

Hot reload necesita fences/deferred retirement/suscripciones correctas; no liberar porque gameplay ya soltó el pointer. Repetir reload/unload/cancel y medir RAM pico/VRAM/latencia. No segundo VFS/cache/cooker independiente para formatos JE.

## 6. Formatos y distribución

Catálogo `.rproject/.rlevel/.rpref/.rmesh/.rmat/.rtext/.rcoll/.rnav/.rskel/.rrig/.ranim/.rgraph/.rseq/.rscript/.rinput/.rui/.rloc/.rfont/.raudio/.rsound/.rsoundsurface/.rpart/.rsave/.epak`. `.rpart` es VFX, `.rtext` textura. `.rmod.toml/.rbrush/.rui2d/.rui3d/.rlang` son autoría según su contrato. Sin extensión nueva por cada editor tab.

Magic/version/chunks/endian/flags/bounds/hash/deps/finite values/validator; no ABI C++/thirdparty structs/pointers ni paths de fuente. Writer/reader/negative tests+consumer real antes de VERIFIED. No adoptar wire limits experimentales de Cry ni renombrar `.pak`/material O3DE como supuesta implementación JE.

N2 incorpora primeros productos usados por BSP/material/geología, N11 completa source-free distribution. Un baseline cooked O3DE temprano no es final-format PASS. Paquetes .epak locales con revision closure/streaming/delta/rollback y políticas de protección verificadas por separado, sin plataformas de comercio/cuentas/AWS.

## 7. Aceptación

Fixture: BSP cuarto/apertura+GLB multislots/normal/ORM vs separados→editar material→instanciar prefab→reimport una textura→Undo/Save/reopen→cook→launcher aislado sin fuentes. Agregar cliff/cueva con campos materiales y collider/nav cuando entre Geology. Corrupt/missing/source cancel/unload/reload probados.

Capturas reales/timings/memoria/cook/UI responsiveness en Windows-DX12, Windows-Vulkan y Linux-Vulkan por separado. Un screenshot/render API no acredita los otros. Config/targets/generated/exports JE y dependencias privadas/no-AWS/no-multiplayer se validan conjuntamente con comportamiento.
