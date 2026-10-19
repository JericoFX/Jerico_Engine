# JericoEngine — objetivo vigente: JE, BSP y motor integrado

**Directiva posterior del propietario, 2026-10-19 — conservar DDGI y ray tracing:** «Ray tracing y DDGI, ya que van bien de la mano». Esta orden sustituye la exclusión temporal de RT. Se conservan y reutilizan DDGI y ray tracing de Atom sobre Vulkan, junto con VR/OpenXR sobre Vulkan. Revertir sólo la desactivación de RT aún no publicada; conservar la corrección de acceso seguro a assets faltantes y los assets nativos ya integrados. La disponibilidad y el funcionamiento se verifican en hardware y runtime reales; la orden no acredita por sí sola GI visible ni cierra DiffuseProbeGrid.

**Aclaración del propietario, 2026-10-18 — materiales sin pantalla intermedia:** «Generar material» desde una textura crea el archivo y lo selecciona en el Asset Browser, sin abrir automáticamente Material Workspace ni una pantalla de edición. El doble clic o apertura explícita lleva directamente a ese material con su preview y propiedades. Se conserva la elección explícita ante conflictos, Cancel sin escritura y la protección de ajustes manuales, fuentes y destinos existentes; no aplica el material a un objeto. Flujo verificado de forma acotada en Windows/Profile/Vulkan R50, con86/86casos y comprobación GUI; no acredita toda la Gem ni otros backends/plataformas. El Inspector del Asset Browser ya abierto puede actualizar sus metadatos/preview con la selección. Esta orden sustituye la presentación automática en Workspace descrita más abajo.

**Dirección de materiales aceptada por el propietario, 2026-10-19:** UI Hammer2+custom más clara, pestañas Propiedades | Nodos dentro del mismo material y guardado de conexiones/constantes; creación desde conjuntos de texturas, maestros/instancias, mezclas y diagnóstico comprensible. Reutilizar los sistemas nativos existentes. El rediseño, la pestaña Nodos y su asociación/persistencia de material/grafo continúan pendientes; las imágenes conceptuales solicitadas no son implementación ni aceptación. Continuar el GOAL y la absorción nativa sin pausarlos para esas imágenes.

**Directiva del propietario: 2026-10-10, posterior a a536fb7. Estado: ACTIVE / IMPLEMENTACIÓN NO VERIFICADA.**
Repositorio único: `JericoFX/Jerico_Engine`. Rama única de trabajo/publicación: `development`.

**Prioridad reafirmada por el propietario, 2026-10-12:** integrar el engine sin Gems activables por proyecto. La implementación activa absorbe geometría/BSP y materiales con registro único, assets, consumidores y separación Runtime/Authoring; no ampliar el desvío LUT. Compilación, pruebas y escena del corte nativo deben verificarse antes de aceptarlo. La integración completa del resto de subsistemas sigue siendo parte obligatoria del GOAL, no un objetivo descartado tras este primer corte.

## 1. Decisiones obligatorias, no opciones futuras

1. **Sin AWS ni multiplayer.** No plataforma AWS, servicios/cuentas/SDKs, GameLift, matchmaking, sesiones de red, replicación, servidores de juego, comercio ni backend online. No quedan como extras opcionales o fases diferidas. No se añade infraestructura para soportarlos.
2. **Nomenclatura JE.** Todo código propiedad/adaptación del motor converge a `JE::`, con subnamespaces por responsabilidad. Targets propios `JE::Core`, `JE::Render`, etc.; macros propias `JE_`. No `AZ::`, `AZStd`, `AzFramework`, `Gem::` o nombres Cry como identidad técnica propia final. No basta un alias `namespace JE = AZ` ni una fachada que oculte para siempre las autoridades antiguas. Avisos legales y APIs genuinas de dependencias se preservan; ver contrato de identidad.
3. **Solo DirectX 12 y Vulkan como backends gráficos.** Windows: DX12 y Vulkan; Linux: Vulkan. Los tres pares son gates distintos. No DX11/OpenGL/Metal/WebGPU ni móvil. Null puede existir únicamente como dispositivo de tests/headless, nunca como tercer renderer de producto ni prueba gráfica.
4. **Gems necesarios dentro del engine.** Reutilizar su implementación e integrar sus módulos, assets, builders, registro y ciclos de vida. El producto final no instala ni activa Gems por proyecto ni depende de catálogos remotos. No es borrar todos los Gems ni crear Jerico como un Gem gigante junto a O3DE.
5. **BSP/UI primero y resultados visibles continuos.** La primera transformación debe permitir construir/editar una habitación, puerta, ventana y corredor en el viewport Jerico, con materiales y persistencia. No aplazar el editor útil hasta terminar todos los subsistemas.
6. **Geología reactivada expresamente.** Roca/familia de rocas/cliff/overhang/cueva, capas/estratos/erosión/fracturas y materiales geológicos vuelven al alcance junto a BSP, RMod, PWAS/EPLS y daño. Se conserva la cancelación específica de BSP→OpenVDB como base general; no se revive ese backend por inferencia.
7. **Optimización SIGGRAPH integrada y medible.** Reutilizar primero lo que Atom resuelve y aplicar mejoras a escenas BSP/geología/materiales reales. Cada mejora muestra resultado visual, coste CPU/GPU, memoria y límites. Se permiten ideas de otros motores y código solo cuando su licencia/procedencia permitan reutilizarlo.
8. **Achicar y optimizar el engine sin recortar nuestras capacidades.** Reducir dependencias, tamaño de SDK/runtime, build/incremental link, RAM/VRAM y trabajo inútil de ejecución/autoría con mediciones separadas. No borrar historia, tests, avisos o herramientas para simular una mejora ni prometer porcentajes sin baseline.
9. **PWAS, EPLS y nuestras herramientas integradas son obligatorios.** Procedural Studio fabrica, PWAS propone/distribuye/ensambla, EPLS añade capas ambientales; BSP estructura, Geology genera forma/campos y RBrush aplica cobertura. Comparten los owners JE existentes sin ser Gems instalables, engines paralelos ni evaluadores de autoría permanentes en Shipping. `ELSP` es la variante escrita del mismo EPLS, no otro sistema.

10. **Motor, juego y editor sin Python.** Directiva posterior del propietario en este chat, 2026-10-10: no CPython embebido, EditorPythonBindings, QtForPython/PySide, terminal ni ejecución de scripts Python en producto. Autoría BSP y herramientas integradas usan sus APIs C++ nativas; gameplay mantiene Luau strict según este GOAL. Python puede actuar únicamente como herramienta externa de compilación/codegen, con dependencias locales auditadas y sin copiar intérprete, librerías ni paquetes al Editor/Runtime distribuido. Los paquetes y pruebas Python anteriores conservan valor histórico de bootstrap; no acreditan este nuevo gate. Retirar consumidores y registro/carga/deploy, sin borrar implementaciones o formatos que otras herramientas necesitan antes de migrarlos.

11. **DX12, Vulkan y VR/OpenXR obligatorios.** Reafirmación del propietario en este chat: VR comparte Atom y los subsistemas del motor mediante OpenXR; no es un tercer renderer. Su aceptación exige runtime y visor reales, vistas estereoscópicas, swapchain, seguimiento y acciones/input. Las capturas de escritorio DX12/Vulkan no prueban VR, y la ausencia de visor no elimina este requisito.

Esta directiva **sustituye** Vulkan-only, DX12 diferido, geología no activa, multiplayer opcional/diferido y permiso para conservar nomenclatura AZ indefinidamente. La prioridad no se deduce de documentos anteriores. Autoridad: última instrucción del propietario > GOAL > AGENTS > contrato JE > masters > ROADMAP/ACTIVE_SLICE.

## 2. Qué significa independencia

Sin plataformas externas, cuentas, marketplace, servicios obligatorios ni descarga implícita de Gems/dependencias para usar el motor. Bibliotecas elegidas (Qt, Jolt, Luau, ufbx, Manifold/OpenMesh, Recast, audio y codecs admitidos) se integran como dependencias privadas con versión/hash/licencia y obtención reproducible bajo control del proyecto. No se reemplazan por motores remotos ni se exponen como contratos públicos del juego.

No se afirma que podamos eliminar el sistema operativo, drivers, SDKs gráficos o toolchain. El producto debe arrancar y funcionar offline. El build reproducible debe usar fuentes/paquetes locales auditados, sin fallback AWS/S3/CloudFront ni descarga silenciosa. Si falta un paquete, informar cuál falta: no buscarlo en Amazon por detrás.

La comunicación local Editor↔Asset Processor/MCP/debug no es multiplayer. Auditar y extraer solo el IPC local necesario, sin conservar el stack completo de transporte/replicación por comodidad. Shipping no carga los servicios de desarrollo.

## 3. Base y método

Base O3DE: `241c78b8064898a709cf1c5dd3535c6406763ef3`, tree `db4b7d4a5d784d2283c95903f461e311d69704c8`; snapshot inspeccionado antes de esta edición: `a536fb78c6195c3edf10afa34bfdf65c6f841ee5`. Los dos commits intermedios cambian solo Markdown. Ese checkpoint documentaba auditoría y planificación; la instrucción vigente autoriza implementar y verificar N0 y el roadmap en development. No se acredita una conversión funcional por documentarla.

No borrar repositorios, ramas, código, licencias, assets ni evidencias con esta publicación. No importar fuente/shaders/historial de CryEngine2. Las otras ramas siguen como referencias inactivas; no merge masivo. Migrar código real posteriormente en development, por verticales con resultado observable y rollback por commit, no worktrees ni motores paralelos.

Para cada subsistema: localizar productores/consumidores y dependencias reales → elegir un owner JE → integrar código/registro/datos/builders → migrar consumidores → demostrar editor/runtime/cook y APIs gráficas aplicables → retirar ruta antigua y aliases temporales. No rechazar tecnología útil por su nombre ni mantener dos autoridades por evitar cambiar consumidores.

Estado N0, 2026-10-11: libsamplerate JE rev2 integrado. OpenImageIO/OpenColorIO nativo sin bindings Python acepta18 pruebas de API/codecs/tools (9Release+9Debug) y su archive rev7 de12.939.864B se integra en el catálogo Windows/cache44, retirando PythonPlugins de esa asociación. Se reutilizan los binarios del productor rev4; sólo se corrige configuración de búsquedas privadas y metadata de alfa del fixture, sin cambiar datos ni igualdad exacta. El crash anterior se conserva como no reproducido, sin causa reparada demostrada. El recorrido C++ de autoría, reapertura y capturas Atom de la habitación sigue revisado por fuente, con39 pruebas focales y Editor aún NOT_RUN; la habitación candidata ahora exige dos materiales Atom reales y bindings por cara, sin acreditar todavía AP/prefab/runtime. Root posterior a la selección transitiva corregida: exit1/21,238s por PhysX5 rev4 ausente. Ocho casos con RED original pasan; la lista real de45 directorios excluye NvCloth y conserva Atom/WhiteBox. El build completo sigue bloqueado; después WhiteBox.Editor.Tests y Editor/AP/escena DX12/Vulkan. Primer QSS real del shell integrado, sin validación visual; no cierra UX/UI ni reduce los demás requisitos. N0/N1 y métricas/producto siguen abiertos. La última directiva aportada exige padre Sol6.1 high y dos escritores Sol6.1 high con scopes disjuntos; Luna sólo apoyo de revisión/compilación asignada, sin fuente/docs/config/Git/aceptación. La preferencia no acredita un cambio de modelo efectivo. Aprovisionar paquetes no decide su permanencia: retirada por consumidores y pruebas de funciones, apartado7. Detalle y evidencia en ACTIVE_SLICE/HANDOFF.

Actualización posterior del 2026-10-11: el estado de bloqueo y Editor NOT_RUN del párrafo anterior es histórico. La escena BSP ya produjo capturas Atom de Editor y game mode en Windows DX12 (`build/n0-validation/editor-fixture-dx12-attempt9/verification.json`) y Vulkan, incluida la variante UV guardada/reabierta (`build/n0-validation/editor-uv-runtime-vulkan-attempt1/verification.json`). Ambos registros acreditan el recorrido de captura con 30 bindings, no aceptación completa del producto, standalone, rendimiento ni VR. El baseline/BSP completo y las migraciones sin Python siguen pendientes de integración/publicación; HDR LUT nativo compila y las suites revisadas pasan73+22 pruebas; la generación real ya produjo .3dl/.azasset y producto AP. La acumulación de LUTs entre actualizaciones tiene RED/GREEN: cinco regresiones antes fallaban, Atom78/78 y CommonFeatures24/24 pasan tras la reparación. Metadata shaper corregida y compilada; CommonFeatures27/27 pasa. UI Vulkan generó/activó None y Log2_48Nits con shaper correcto, Undo/Redo por menú y log sin exceso LUT/timeout. None recorta el suelo en esta escena; Log2 conserva detalle. Save/reopen Vulkan verificado en copia JELutValidation (EBB764...4994), con LUT/shaper2/metadata válida persistidos y original FEE558...17AA intacto; evidencia hdr-lut-save-reopen-attempt1. Rango custom, multicapa y equivalencia cuantitativa siguen pendientes. Material Editor Vulkan abre WhiteBoxDefault con Shaderball/Neutral Urban iluminados; edición de rugosidad y Undo observados sin guardar (material-editor-vulkan-preview-attempt2). Recuperación de assets originales no acredita absorción Gems ni reparación del error de buffer vacío; Material Canvas, thumbnail y flujo de autoría completo siguen pendientes. Publicaciones focales confirmadas en development: 9dc5a6f084 corrige acumulación Look y 6e38b27a06 evita iniciar Python para manifests nativos; SceneData43/43 y reproceso Shaderball0errores/1warning sin Python. ACTIVE_SLICE/HANDOFF conservan los resultados focales y límites actuales. N0/N1 y el GOAL global permanecen abiertos.

Checkpoint nativo 2026-10-13, integración funcional publicada en development 388e618aab651cf02e9fe01125f5428514639751 y SHA remoto confirmado: MaterialGraph/PassGraph y configuraciones de nodo comparten Editor, Inspector e historial JE; Save/reopen real en Windows DX12/Vulkan y148 casos únicos verificados. El SDK instalado exporta tres owners nativos distintos, excluye seis aplicaciones de ALL/exports/install y un consumidor privado importa/enlaza/carga sus DLL con PATH limitado al SDK/Windows. No activa factories ni acredita SDK limpio completo;558 pointers SDK y AP global siguen pendientes. Generación/aplicación Pass, default-open sin SourceUUID, absorción restante de Gems, Shipping y VR siguen abiertos. Evidencia y siguiente scope en ACTIVE_SLICE/HANDOFF y docs/evidence/n0/native-pass-workspace-and-sdk-2026-10-13. Cuatro copias privadas obsoletas de SDK retiradas; build original y repositorios previos conservados.

## 4. Stack y owners finales

| Área | Destino |
|---|---|
| Core/World/Assets | O3DE adaptado a JE; una identidad, un World, un asset graph y un registro de revisión |
| Render | Implementación Atom adaptada dentro de `JE::Render`; un FrameGraph/scheduler/device por contexto, DX12/Vulkan |
| Editor | Qt6 Widgets, workflow Jerico; viewport compartido, herramientas BSP prioritarias |
| Geometry/Geology | `JE::Geometry`, `JE::BSP`, `JE::Geology`; White Box/Manifold como base auditada y matemáticas originales permitidas |
| Physics | `JE::Physics` con Jolt privado; PhysX solo transición identificada, nunca doble simulación del mismo body |
| Script | `JE::Script` con Luau strict; una VM de gameplay/UI |
| Animation/Sequencer | EMotionFX/Maestro auditados e integrados en Motion Studio y Sequencer JE |
| UI/Audio/VFX | JericoUI/Luau, JericoAudio backend privado y JericoParticles; no middleware/servicios obligatorios |
| Navigation/AI | Recast/Detour privado, HTN explicable con memoria/relaciones; grupos de NPC no son multiplayer |
| XR | OpenXR nativo, mismo World/input/interaction/audio; bindings de DX12/Vulkan según plataforma probada |
| Packaging | Formatos `.r*` y `.epak` propios, readers/bounds/versiones, cook y ejecución sin fuentes |

Diligent queda como referencia o reutilización puntual permitida, no capa obligatoria debajo de Atom. DDGI y ray tracing de Atom se conservan, por orden posterior del propietario de 2026-10-19; Vulkan sigue siendo el único backend activo. Las bibliotecas externas no se renombrarán fraudulentamente; sus tipos se encapsulan y su procedencia se conserva.

## 5. Requisitos conservados

### Editor, BSP y materiales

JericoEditor conserva el flujo UI/UX inspirado en Hammer 2/Source 2 con diseño propio Jerico, detallado en [el master de editor](docs/EDITOR_ASSET_WORLD_MASTER.md). Dirección grafito/navy, acento cian y herramienta/selección ámbar; tipografía legible, DPI, atajos, docking y accesibilidad. Viewport principal compartido; Outliner/paletas a la izquierda, Inspector a la derecha, logs/curvas/timeline abajo. Edit/Simulate/Play usan el engine real. Cambiar tab no destruye escena ni selección.

BSP de autoría: brushes, caras/aristas/vértices, grid/snap, extrude/inset/bevel, slice/hollow/union/subtract/intersect, UV/material por cara, salas/corredores/puertas/ventanas/escaleras/arcos/columnas. White Box ya declara Manifold y contiene código CSG; probar antes de reescribir. Mantener semántica de superficies/regiones y GUID al editar. Runtime consume mallas/colliders/nav, no necesita brush tree espacial histórico.

Materiales PBR propios sobre Atom: BaseColor, Normal, Roughness, Metallic, AO, Emission, Opacity, material instances/graph/layers y Physical Material separado. Nombres de texturas libres. ORM y mapas separados obligatorios, metadata color-space/canales/normal Y/UV explícita, mips completos y tangentes correctas. Reimport mantiene overrides y no reescribe fuentes. `.rmat/.rtext/.rmesh` son contratos reales, no extensiones renombradas.

### Assets y persistencia

GLB/glTF2, FBX por ufbx Tool-only, OBJ/MTL; PNG/JPEG/TGA/TIFF/HDR/EXR/DDS/KTX2/Basis; heightmaps PNG16/RAW16/32/EXR/TIFF/GeoTIFF; WAV/FLAC/OGG por admisión y pruebas. No Autodesk FBX SDK ni ATL/MFC impuestos. Source snapshot→Canonical IR→Asset Processor/builders integrados→productos→paquete; una dependencia/revisión canónica, invalidación incremental y publicación candidate-first.

GUID persistente, handles runtime seguros; ámbito Scene/PrefabInternal y remap de instancias. Outliner→IDE crea/reutiliza referencia tipada en `self.references`; self se enlaza nativamente. Commands solicitan, owners deciden, Events informan. Store opt-in registra cambios confirmados sin repetir Store.set. Save/Load por providers/versiones con restore transaccional, corrupción/rollback y sin punteros/closures.

### Geología, procedural y física

Masa/BSP y espacio jugable manual son la base; forma Rock/RockFamily/Cliff/Overhang/Cave independiente de material. Operation stack reversible: ruido/warp/estratos/juntas/erosión térmica o hidráulica admitida/sedimentación/fracturas/daño. SDF/campos dispersos y Surface Nets+Hermite+QEF QR/SVD son candidatos propios auditados; preservar cortes analíticos y no redondear planos por forzar todo al muestreo.

Materiales triplanares/world-space reutilizan los campos de geología: capas, pendiente, curvatura, cavidad, erosión, sedimentos y FreshInterior; no ruido independiente que contradiga la forma. Dirty regions/voxels, preview/cancel, límites de trabajo/readback, bake/LOD/collision/nav y publicación única. Prueba visible de roca, cliff estratificado con undercut y cueva jugable; no ruido genérico como supuesto sistema geológico.

RMod con piezas/ports/sockets y fitting de redes, intersecciones manuales, flanges, soportes suelo/pared/techo, snap con histéresis, UV por longitud de arco/RMF; cables y cadenas catenarias con Jolt. PWAS/EPLS proponen con locks/masks/seed/overrides sin romper layout manual. RBrush separa BRUSH/PAYLOAD/TARGET: vertex/material/mask/decal/daño tienen outputs distintos.

Jolt posee body/collider/constraints/COM/inertia, queries/trigger/character/ragdoll/vehicle, materiales físicos y fragmentos. Destrucción prefracture/bonds/local activation antes de fractura arbitraria costosa. Viento físico opt-in por body independiente de bending visual; fans son emisores del Environment compartido. **Flotabilidad y Enable Buoyancy por WaterVolume siguen DESIGN ONLY** hasta permiso específico; reactivar geología no los activa.

### Animación, scripting, UI, sonido e IA

Motion Studio: `.rskel` estructura, `.rrig` controles/IK/FK, `.ranim` clips, `.rgraph` evaluación; huesos/pesos/retarget/pausar-corregir-guardar. GripPoints/GrabPoints, SceneSockets/MatePoints/FitProfiles; una InteractionSession bimanual, fitting por etapas/tolerancias/simetrías, Jolt manda e IK representa. Llaves/pilas/tapas no crean solvers físicos separados.

Luau strict, input semántico teclado/mouse/gamepad/XR, contexts/rebinding/focus loss/haptics; triggers de shape/proximidad/click/input/evento/condición con activador/filtro/Once/cooldown. Inventory/Items/Item Studio separados de presentación; Dialogue/Quest/Journal y replay local QA conservados, no red de juego.

Sequencer `.rseq` con cámaras/curvas/eventos/audio/UI y restauración al stop/skip/cancel; no control total del player por defecto. UI `.rui2d/.rui3d`→`.rui`, grid/hover/drag/preview3D/tweens, world-space/render-target; `.rlang/.rloc/.rfont` y accesibilidad. Audio espacial/SoundEvents/mixer/zonas/portales/oclusión/reverb, backend abierto privado; VFX/flipbooks/culling/overdraw, `.rpart` reservado a partículas.

Navigation separado de HTN; sensores, memoria, relaciones/especies/grupos/territorio, reservas y acciones cancelables. Bait/manada/colonia explicables y guardables. GraphCore visual común con evaluadores de dominio, no otra VM. 2D/2.5D/pixel/isométrico/vehículos y VR comparten el motor; son objetivos existentes secundarios al BSP/geología visible.

## 6. Calidad, cierre y límites

Cada checkpoint enseña escena real y cambios aplicados, no solo mocks/headers/conteos de checks. Métricas comparables por escena/cámara/resolución/hardware/backend, p50/p95/p99 CPU/GPU, RAM/VRAM, carga/recook y UI responsiveness. Una mejora puede aceptarse para un par de plataforma/backend y quedar NOT TESTED en otro; no acredita el gate completo.

SIGGRAPH/idTech6: decals/stamps/clustered, PBR, shadow cache, GPU visibility/HZB, rough glass, partículas/volumétricos/atmósfera y frecuencia de costes; optimización por problemas medidos, no listado infinito de papers. Ideas de otros engines permitidas con ficha fuente/licencia/porción útil/no copiar/experimento/fallback. No clonar arquitecturas ni incorporar código propietario por traducirlo.

Cierre global requiere `JE::` real en código propio, módulos integrados dentro del engine y sin Gem manager productivo, ausencia de AWS/multiplayer/backends no permitidos en build/package/runtime, formatos propios source-free, licencias/avisos/SBOM, pruebas negativas y recorridos completos. No basta deshabilitar un checkbox ni ocultar un DLL. Las dependencias privadas admitidas y notices legales no son faltas de identidad.

Ver `docs/architecture/JE_IDENTITY_AND_NATIVE_MODULES.md`, `docs/research/JE_DEPENDENCY_AUDIT_2026_10_10.md` y `docs/ROADMAP.md`. Toda especificación anterior se aplica solo donde sea compatible con esta decisión; conservar detalles técnicos válidos, nunca reactivar una exclusión.

## 7. Ampliación: motor ligero con PWAS/EPLS y nuestras herramientas

**Directiva posterior a 54a6c98:** aplicar `docs/JE_LEAN_ENGINE_AND_AUTHORING.md` como criterio transversal de N0–N12. La reducción no permite quitar capacidades comprometidas ni convertir PWAS/EPLS en extensiones externas. Mantener Procedural Studio, Knowledge Library, BSP, Geology, RBrush, Smart Materials, RMod/Splines, import/physics/Motion/UI/AI/audio/VFX/automation y los formatos propios con sus owners existentes.

Primer recorrido conjunto: cuarto/corredor BSP → pieza fabricada en Procedural Studio → PWAS propone y coloca respetando locks/clearances → EPLS aplica polvo/humedad/óxido mediante campos y máscaras → Undo/Save/reopen → cook/runtime. Una operación local no regenera todo; los jobs viejos/cancelados no publican encima de nuevas revisiones. EPLS no corta estructura ni duplica placement/física: solicita esas operaciones al owner correspondiente.

Separar targets y dependencias efectivas de Editor/Tools/Runtime, sin otro framework de perfiles. Las herramientas permanecen integradas, pero Shipping carga resultados cocinados, no editores de nodos, generadores PWAS/EPLS/geología, knowledge/recipes ni importers innecesarios. Efectos dinámicos ya admitidos usan datos/consumidores runtime mínimos; no se elimina gameplay por adelgazar. Inicialización diferida de recursos de panel nunca impide registrar tipos necesarios para preservar escenas.

Directiva posterior del propietario en este chat: después del primer baseline funcional, retirar progresivamente las dependencias heredadas innecesarias. Aprovisionar una dependencia para N0 no decide su permanencia. Cada retirada identifica consumidores, alternativa o integración en su owner JE, coste medido y pruebas de las funciones afectadas. GoogleTest y runners pertenecen a targets de prueba; importadores/compiladores/generadores de autoría pertenecen a Editor/Tools según consumidor; las bibliotecas necesarias quedan privadas en su subsistema. La distribución final verifica grafo, link, despliegue y módulos cargados, junto con tamaño/RAM/VRAM. Absorber Gems y reducir duplicación preserva las capacidades comprometidas.

Medir aparte source activo, SDK/editor, runtime/cooked assets, .git/LFS/cache/terceros, símbolos y tiempos de configure/full/incremental build/link. Medir también idle/carga/import/cook/preview/CPU/GPU/RAM/VRAM. Los mismos contenidos y funciones se comparan antes/después; sin porcentajes/GB/FPS inventados ni limpieza destructiva de evidencia para lograr cifras. LEAN-01..04 y PWE-01..04 del contrato complementan los IDs JER existentes y la Definition of Done; no crean otro roadmap.
