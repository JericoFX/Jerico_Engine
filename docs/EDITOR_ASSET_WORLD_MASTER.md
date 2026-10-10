# JE Editor, Assets y World — BSP como primer flujo

**Vigente 2026-10-10.** Contrato de destino; implementación no acreditada. GOAL y contrato JE superiores. Fuente O3DE/Atom se reutiliza y migra a nombres/owners JE, no API pública AZ ni facade perpetua. Renders permitidos DX12/Vulkan, AWS/multiplayer excluidos.

## 1. UI Jerico y authoring nativo

Qt6 Widgets sobre editor/AzToolsFramework/AzQtComponents actuales adaptados. Viewport central grande compartido por workspaces, Outliner/paletas izquierda, Inspector derecho, logs/curvas/timelines/profiler abajo. Grafito/navy con cian y ámbar de selección; tipografía legible, DPI, iconos/texto además de color, shortcuts y docking persistentes. World/Geometry-BSP/Materiales primero; Assets/Import/Procedural-Geology/Physics/Motion/AI/Audio/VFX/Sequencer/UI/Luau/XR/Profiler según consumidor.

No un editor nuevo aislado ni pantallas mock para demostrar productividad. Cambiar tab mantiene scene/selection/viewport. Edit/Simulate/Play usa runtime systems reales. Diálogos de importar/guardar/cancelar/overwrite/progreso/error/renderer/device/cierre con cambios/recovery están incluidos, no esconderlos tras screenshot bonito.

Primera prueba: cuarto+corredor, cut de puerta/ventana, caras/UV/material, Undo/Redo/Save/reopen y mesh runtime. WhiteBox/Manifold CSG es base real a probar; modelar un cubo no acredita BSP completo. Herramientas avanzadas entran por operación persistente, no todo el algoritmo de golpe.

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
