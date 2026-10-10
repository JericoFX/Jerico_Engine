# Editor, assets, mundo y formatos Jerico

Estado: contrato de destino, no implementación acreditada. Owners y etapas: N1-N3, N11-N12 de ROADMAP. Fuente histórica: GOAL, RUNTIME_FORMATS, JERICOEDITOR_MASTER y JERICOENGINE1_TRANSFER_MAP de JericoEngine2 en el SHA fijado en GOAL.

## 1. Editor real y experiencia de autoría

Reutilizar `Code/Editor`, AzToolsFramework/AzQtComponents y Atom Tools. Un shell Qt 6 Widgets, un viewport/contexto de escena compartido y paneles según tarea. No generar diferentes aplicaciones demo para Assets, Física o Scripts.

Dirección visual conservada: grafito/navy oscuro, cian/teal para identidad, ámbar/naranja para herramienta/selección; información legible, escalado DPI, icono/texto además de color. Workspaces: World, Assets, Import, Geometry/BSP, Procedural, Terrain, Materials, Physics, Rig/Motion, AI/Nav, Audio, VFX, Sequencer, UI, Luau IDE, XR, Profiler e Item/Narrative cuando se incorporen. Docking restaurable; no reconstruir viewport al cambiar tab ni perder selección/historial.

Cada acción de edición sigue: selección/contexto → propuesta/preview → validación → transacción → modelo canónico → invalidación derivada → notificación a paneles. Cancel no deja productos activos huérfanos. Undo/Redo, save/reopen, prefab overrides y revisiones se prueban en la ruta real. Nada de modificar archivos de escena directamente desde un panel evitando las transacciones.

Los diálogos heredados forman parte de la migración: errores, import, guardar, overwrite, selección de assets, progreso/cancel, renderer/device, navegación entre escenas, cierre con cambios. No ocultarlos ni aceptar una captura del viewport como prueba de toda la experiencia.

## 2. Integración nativa y automatización

Una plantilla/proyecto limpio debe recibir los subsistemas Jerico de la distribución del motor, sin instalar/habilitar Gems Jerico. Se puede utilizar registro interno de O3DE; queda fijado y administrado por el producto, no por cada diseñador.

Editor UI, Luau tools permitidos y Automation Bridge usan el mismo registro tipado de comandos/propiedades. MCP es adaptador local, con capabilities, validación, límites de alcance y respuestas trazables. Queries no mutan. Mutaciones soportan preview/diff/Apply/Cancel y Undo. No ejecutar código arbitrario obtenido de un asset/documento. No inventar tools de Blender o de cualquier MCP: descubrir schemas reales.

Ejemplo de aceptación: seleccionar una puerta, añadir hinge, mover el pivot, cambiar el límite y deshacer; el mismo comando desde la automatización debe producir igual modelo y persistencia. La captura del viewport confirma resultado, no la intención de un comando enviado.

## 3. Una identidad persistente, un mundo

Conservar la infraestructura de entidades/componentes, prefab y asset IDs de O3DE mientras se integra el contrato Jerico. Diseñar un único mapeo de identidad persistente si AZ EntityId, asset IDs y GUID Jerico tienen semánticas diferentes; no añadir dos tablas escribibles de autoridad.

Referencia persistente = identidad estable + ámbito + capacidad/tipo esperado. Handle runtime = identidad validada/generación/contexto de mundo, no puntero serializado. Resolver Scene versus PrefabInternal, instancias, duplicación y remap. Una referencia ausente informa error/contexto; no cae silenciosamente en la primera entidad de igual nombre.

World conserva jerarquías, parent transforms, layers, prefabs/variantes, streaming de celdas, HLOD y residencia. Mover, duplicar y reimportar no cambia identidades aceptadas arbitrariamente. Unload invalida handles/callbacks/physics/nav/render de forma coordinada; volver a cargar usa el modelo persistente.

No reescribir ECS para pasar una prueba aislada. Auditar las rutas reales de AZ::Entity/AzFramework y medir antes de sustituir almacenamiento, reflection o eventos.

## 4. Importación y cook

Una sola ruta: fuente inmutable + metadata/recipe → Canonical IR → builders del Asset Processor → productos versionados/dependencias → cache → paquete/runtime. No mantener un segundo cooker/asset database en paralelo porque ya exista código antiguo de Jerico.

Inputs de destino: glTF 2.0/GLB, FBX por ufbx tool-only y OBJ/MTL; PNG/JPEG/TGA/TIFF/HDR/EXR/DDS/KTX2/Basis; heightmaps PNG16/RAW16/32/EXR/TIFF/GeoTIFF; WAV/FLAC/OGG. Incorporar capacidades por pruebas específicas de formato y no declarar cobertura total tras un GLB sencillo. El engine Shipping no analiza fuentes glTF, FBX, KTX2 o recipes.

Escenas con jerarquías/submeshes/material slots/instancias, skins, clips, morph targets y extensiones glTF requieren estado individual de soporte. Extensión desconocida requerida: rechazo con mensaje; opcional: política explícita. Materiales y texturas externos: normalizar paths, rechazar escapes/links/collisiones de case y respetar presupuesto. Un modelo válido pero enorme debe cargar con progreso/cancel, sin congelar el escritorio.

No exigir FBX SDK de Autodesk ni ATL/MFC nuevos. Comprobar los targets heredados antes de declarar eliminada esa dependencia. ufbx y otros imports no entran al ABI público ni al runtime.

## 5. Semánticas de material y textura

El nombre de archivo solo puede sugerir mapping. La autoridad es metadata editable: BaseColor(sRGB), normal(linear + convención), roughness/metallic/AO(linear), emissive/color-space, opacity y channels. Aceptar ORM R=AO/G=roughness/B=metallic y mapas separados equivalentes. El cooker empaqueta sin exigir al autor rehacer texturas.

Un usuario puede cambiar de ORM a fuentes separadas y conservar apariencia, scalars y overrides. Reimport cambia únicamente lo derivado de la fuente modificada. Tests: nombres arbitrarios, textura compartida con semánticas incompatibles, UV set distinto, missing map, normal Y flip y materiales múltiples.

Transformación geométrica debe tratar normales por inverse-transpose y tangentes/handedness de forma coherente, incluyendo escalas negativas. Generación tangente usa implementación admitida/MikkTSpace cuando corresponda. UV degenerada con normal map debe diagnosticarse, no inventar un tangente silencioso aceptado.

Mips completos incluyendo dimensiones impares; normals en espacio vectorial y renormalización; datos sin sRGB; alpha policy explícita. Compresión/transcoding según plataforma y formato real soportado. Normal variance/specular AA se evalúa aparte, no se acredita con renormalizar. Budget de archivo no equivale a RAM pico: registrar copias temporales y la residencia GPU efectiva.

## 6. Reimport, caché y hot reload

GUID estable, recipe version, seed, importer version, hashes de fuente y dependencias, overrides de autoría. Build candidato sin cambiar selección activa. Publicación válida al final; error/cancel mantiene el producto anterior. Invalidación por dependencias/revisión, no recook de todo por cada propiedad.

Hot reload retira recursos tras el fence/lifetime seguro, no porque nadie en gameplay los use. GPU y tareas asíncronas pueden conservar referencias. Material/texture/mesh reload y scene unload tienen tests de repetición y memoria. Decidir slots de buffer, frame arenas y deferred release desde Atom/AzCore existentes, no copiar pooling de UE4.

## 7. Catálogo de formatos finales

| Formato | Responsabilidad |
|---|---|
| .rproject | Arranque/proyecto resuelto |
| .rlevel / .rpref | World/celdas y composición prefab |
| .rmesh / .rtext / .rmat | Geometría/LOD/streams, texturas/mips, material/producto de graph |
| .rcoll / .rnav | Colisión y navegación cocinadas |
| .rskel / .rrig / .ranim / .rgraph | Skeleton, rig, clip y graph de animación |
| .rseq | Secuencia reusable con ciclo de vida propio |
| .rscript / .rinput | Módulo Luau validado y perfil de input |
| .rui / .rloc / .rfont | UI runtime, textos y fuente/metrics/fallbacks |
| .raudio / .rsound / .rsoundsurface | Audio, SoundEvents y respuesta de superficie física |
| .rpart | Partículas/VFX; NO piezas modulares |
| .rsave | Snapshot de proveedores de save |
| .epak | Paquete y delta con cierre de dependencias |

Autoría: `.rui2d/.rui3d`, `.rlang`, `.rbrush`, `.rmod.toml` y documentos de graph/recipe. No inventar `.rfoo` por panel: nueva extensión exige owner, lifecycle y justificación.

Estos nombres conservan intención de producto, **no congelan el wire layout limitado del prototipo Cry**. Revisar límites/chunks y diseñar migración explícita antes de reutilizar codecs. No renombrar un `.pak` o asset O3DE para afirmar que se implementó `.epak/.rmesh`.

Cada reader verifica magic/version, endian, file size, chunks, counts/offsets/overflow antes de reservar, dependencias/hash y flags requeridos. Formatos independientes de ABI C++/punteros/tipos de tercero. Versiones viejas se migran o rechazan claramente. Hash != firma/autenticación.

## 8. Packaging, seguridad y aceptación

Aprovechar Asset Bundler y VFS existentes. La distribución final no depende de paths del checkout, fuentes, XML propio, process de Asset Processor ni compiladores. Executable, `.epak` y librerías de producto en un directorio; recursos de terceros conservan su layout exigido.

Fases tempranas pueden verificar runtime cocinado original O3DE; etiquetar BASELINE O3DE, no FINAL JERICO FORMAT PASS. N11 cierra los formatos/instalación finales y source-free.

Prueba vertical representativa: importar GLB con varios materiales y normales → ORM/separados equivalentes → prefab instanciado → cambio de una textura → reimport → Undo/Redo → Save → cerrar todo → reopen → cook → ejecutar desde carpeta aislada sin fuentes. Medir tiempo de respuesta, RAM pico y errores. Añadir corruptos/cancel/asset missing/unload/reload. Windows y Linux por separado.
