# JE — motor más pequeño, PWAS/EPLS y herramientas integradas

**Directiva adicional del propietario: 2026-10-10, posterior a 54a6c98. Estado: CONTRACT ADOPTED / NO IMPLEMENTACIÓN NI AHORRO MEDIDO.**

Este contrato transversal complementa GOAL/AGENTS y los masters existentes; no crea otro motor, otro roadmap ni otro owner procedural. Objetivo: reducir tamaño, dependencias, trabajo de compilación, memoria y coste de ejecución **sin quitar PWAS, EPLS, BSP, geología ni nuestras herramientas**. Una única rama development. Código propio/adaptado JE::, sólo DX12/Vulkan, sin AWS ni multiplayer; permanecen las exclusiones/permisos de GOAL.

## 1. Las herramientas no son el lastre que se elimina

Jerico debe ser una herramienta potente de autoría con runtime proporcionado a lo que el juego consume. Integrar una capacidad en el motor significa que está soportada y disponible sin instalar Gems; no que toda su UI, compiladores, datos de preview y generadores deban cargarse en cada ejecutable o ejecutar trabajo todo el tiempo.

Mantener las fronteras ya acordadas, bajo nombres propios JE:

| Capacidad | Autoridad real | Resultado normal |
|---|---|---|
| JericoBSP / Hard-Surface | Construye arquitectura, solids/brush tree y operaciones de malla derivada explícitas | Mesh/regiones/materiales/colliders/nav |
| Geology | Genera forma rocosa/estratos/cueva/erosión/daño y campos semánticos | Mesh/campos/bake/colisión/LOD |
| Procedural Studio | Fabrica piezas paramétricas y conjuntos reutilizables: puertas, ventanas, mesas, cajones, perfiles, redes y estructuras | Assets/prefabs/parts reutilizables |
| Knowledge Library | Describe conceptos/perfiles/estilos/materiales y reglas aceptadas de construcción | Datos de autoría para productores, no inferencia oculta runtime |
| PWAS | Propone distribución, ensamblado y colocación de contenido en el mundo usando productores existentes | Instancias/prefabs/relaciones/variantes aceptadas |
| EPLS | Environmental Procedural Layer System: historia ambiental por capas de superficie | Masks/material layers/decals/bake y solicitudes de colocación |
| RBrush | Gesto/cobertura sobre payload y target elegidos | Strokes transaccionales, no segundo material/geometry engine |
| Splines/RMod/Strands | Camino/frame/ports/sockets/fittings/soportes/cables/cadenas con owners existentes | Mesh/instancias/prefabs/constraints/cooked LOD |
| Smart Materials | Apariencia por semánticas/campos y capas explícitas, material físico separado | .rmat/.rtext y parámetros del renderer |
| Tools restantes | Import/Assets, Physics, Motion, Sequencer, UI, Luau IDE, AI/Nav, Audio/VFX, automation/profiler | Comparten editor/runtime/cook; no aplicaciones/engines paralelos |

EPLS es el nombre canónico presente en los documentos originales. `ELSP` en las conversaciones designa ese mismo sistema, no otro módulo, serializer o namespace. PWAS no es un layout de viewports y EPLS no es sólo un editor de curvas temporales. Procedural Studio **no se fusiona conceptualmente con PWAS**: fabrica; PWAS distribuye. Compartir infraestructura no transfiere ownership.

## 2. Integración PWAS/EPLS, sin reescribir lo que ya sirve

Flujo de autoría compartido, no un pipeline rígido que obligue a ejecutar todo:

```text
BSP / Geology / modelos importados
    -> regiones, surface roles, bounds, campos y puntos de conexión
Procedural Studio / biblioteca de piezas
    -> assets reutilizables
PWAS
    -> propuesta de colocación/ensamblado y diff editable
EPLS + Smart Materials + RBrush
    -> capas de historia ambiental y detalle
World / Transactions / Asset Processor / Cook únicos
    -> productos .r* y .epak
```

Cada capacidad puede usarse independientemente cuando sus inputs existan. Cambiar musgo no obliga a volver a generar roca, collider, navmesh, muebles y tuberías. Las dependencias explícitas determinan qué recalcular.

### PWAS: propuesta controlada

Lee layout manual, habitaciones/portales/surface roles, volúmenes/splines, bounds, ports/sockets y constraints/clearances. Selecciona piezas/variantes de la biblioteca, solicita a Procedural Studio lo que deba fabricarse y propone distribución/transformaciones compatibles. No introduce otro route solver si RMod ya lo posee ni otro registry/scene.

Conserva recipe GUID/generator version/seed/input hashes, include/exclude masks, locks/manual overrides e identidad de resultados aceptados. Preview/Diff/Apply/Cancel mediante las transacciones canónicas. Regenerar una región no cambia IDs de piezas bloqueadas ni desplaza puertas/ventanas/camino jugable para acomodar dressing. Un cambio estructural es otra operación explícita.

Si faltan altura/collision/nav/material tags exigidos por una regla, diagnosticar el input faltante y no asumirlo. Conflictos sin solución se muestran como gaps, no se reparan moviendo a escondidas objetos manuales. Cambiar seed no es autorización para rehacer todo el mundo.

### EPLS: capas de superficie, no otro simulador del mundo

Capas previstas: humedad, óxido, musgo/hongos, nieve/hielo, suciedad/polvo/hollín, filtraciones, grietas/daño superficial, telarañas y crecimiento biológico. Fuentes: orientación/pendiente, altura/cavidad/curvatura, contacto/distancia, volúmenes/splines, material/tags, campos geológicos/daño, agua/temperatura/viento cuando existan y pintura manual.

Compone máscaras/layers/decals o solicita outputs geométricos/instancias a sus owners. **La colocación de objetos/conjuntos pasa por PWAS; la construcción de geometría pasa por Geometry/Procedural Studio; la simulación física pasa por Jolt.** No implementar un segundo scatter/mesh generator/physics state dentro de EPLS para simplificar una llamada.

EPLS no corta topología estructural ni perfora una pared silenciosamente. Un efecto barato de normal/mask/decal no cambia silhouette/collision. Bevel/chamfer/cavity/through-hole/physical break se solicita como operación explícita del owner de geometry/destruction, preservando FreshInterior y las pruebas de colisión. Si una capa afecta tags de audio/material físico, publicar por el contrato del owner correspondiente, no reemplazarlo.

Las capas procedurales respetan regiones pintadas autoritativas (`NoGenerate`, superficies excluidas, etc.). Regenerar no elimina overrides. Los materiales geológicos usan los mismos campos de estratos/erosión/depósito que la forma, no un ruido visual independiente que los contradiga.

### UX de nuestras herramientas

Herramientas integradas en el mismo shell Qt6: marcar volumen/caras/spline/objetos, elegir preset/regla, ajustar Inspector/manipuladores y ver Preview/Diff. Ventana Procedural Studio acoplable/desacoplable sigue siendo parte del mismo editor. El graph avanzado se comparte como frontend por dominio; no obligar al autor a escribir fórmulas/nodos para poner una puerta o aplicar musgo.

El panel PWAS muestra propuestas/clearances/gaps, locks y variantes; EPLS muestra stack de capas, máscara individual/cobertura, source inputs y coste; RBrush conserva BRUSH/PAYLOAD/TARGET. World, material editor y profiler enseñan el mismo resultado/revisión. No segundos viewports/runtimes recreados por cada tab.

## 3. Primera demostración conjunta, no sólo paneles vacíos

Integrar el primer uso de PWAS/EPLS con la escena BSP/materiales de N1/N2/N4 y completar N8 por capacidades. No esperar audio/IA/packaging avanzado para ver autoría real; tampoco implementar todo simultáneamente. ACTIVE_SLICE elige un resultado completo cada vez.

**Demo A — habitación/mina:** construir cuarto/corredor/puerta/ventana con BSP. Procedural Studio fabrica una repisa/puerta o tramo de conducto; PWAS propone piezas/soportes/mobiliario sin tapar el paso. Congelar una pieza y regenerar sólo región no bloqueada. EPLS aplica polvo en caras altas, humedad en zonas bajas, óxido bajo uniones y filtraciones desde un input explícito. Deshacer capa conserva geometría/placement.

**Demo B — geología conectada:** cliff/cueva con estratos/FreshInterior. PWAS coloca piezas aprobadas en zonas válidas; EPLS usa campos de superficie y masks para detalle, sin ruido desconectado ni regeneración completa. Colisión y espacio navegable coherentes donde ya estén admitidos.

**Demo C — persistencia/cook:** Preview cancel no cambia el World; Apply/Undo/Redo/Save/cerrar/reopen conserva receta, IDs y overrides. Cook al mismo .rmesh/.rmat/.rtext/.rcoll/.rpref/.rlevel y .epak según consumer. Runtime aislado sin recipes/generadores/UI de autoría reproduce los resultados aceptados. Probar refs eliminadas, jobs cancelados y una revisión vieja que termina después de una nueva: no publica datos obsoletos.

Comparar imágenes/settings y gates por Windows-DX12, Windows-Vulkan, Linux-Vulkan. La existencia de un diseño o output histórico de Playground no acredita estas demos.

## 4. Achicar: separar qué pesa y dónde

No dar una cifra o porcentaje de reducción sin medición. Registrar estos grupos por separado para no confundir un checkout grande con un juego pesado:

| Medida | Alcance |
|---|---|
| Source activo | Archivos de código y assets versionados necesarios, aparte de historia |
| Historia y LFS | .git, objetos LFS locales y caches, sin borrarlos para simular mejora de producto |
| SDK/editor instalado | Executables/libs/tools/recursos de autoría y símbolos, separados |
| Runtime instalado | Binarios/bibliotecas/cooked content necesarios del juego; símbolos aparte y preservados para depuración |
| Dependencias/toolchain | Fuentes y artefactos de terceros, licencias, paquetes y cache reproducible |
| Build | Configure, full/incremental compile/link, pico RAM, artefactos y tamaño de cache |
| Uso | Arranque/carga/reimport/cook y editing latency, CPU/GPU p50/p95/p99, RAM pico/residente y VRAM |

No prometer bajar, por ejemplo, a un número arbitrario de GB. Capturar bytes/conteos/milisegundos y explicar diferencias con mismo build/config/escena/hardware/cache policy. Coste de nuevas funciones y ahorro por optimización son cuentas distintas: comparar PWAS/EPLS optimizado contra PWAS/EPLS con las mismas funciones, no contra un engine vacío.

## 5. Separación real entre Editor, Tools y Runtime

Usar targets existentes/adaptados, **no otro sistema genérico de perfiles, plugins o feature managers**. El engine proporciona todas nuestras herramientas; la distribución del juego no incluye sus ventanas, compiladores, fuentes o recipes innecesarias.

- Editor: UI Qt, BSP/Geology/PWAS/EPLS/Procedural Studio/Material Graph/inspectores/import authoring y previews; recursos pesados de workspaces se crean al usarse, con estados persistentes y límites explícitos.
- Tools/Cook: builders/importers, evaluadores de recipes/graphs, geometry preprocessing, bake/LOD/collision/nav y packaging; ejecución local y automatizable. Compartir código bibliotecario con el editor, no otro authority de cook.
Directiva posterior del propietario, 2026-10-10: motor, juego y editor sin Python embebido. Herramientas de autoría mediante C++ nativo; gameplay mediante la VM Luau ya elegida. Python externo puede ejecutar codegen/build, pero el gate de Editor/Runtime exige ausencia de CPython, PySide/shiboken, módulos de bindings, terminal y scripts productivos en targets enlazados, módulos cargados y despliegue. No basta quitar seeds del proyecto si AtomToolsFramework sigue enlazándolo.

- Runtime: World, render, física, script, animación, audio/UI/IA y readers/productos que necesita el contenido; no Qt de edición, MaterialCanvas/GraphCanvas de autoría, Python tool host, import FBX/glTF, shader compiler o builders por accidente. Requisitos legítimos de una dependencia se auditan antes de excluirla.

Los resultados de PWAS/EPLS estáticos se cocinan como instancias, meshes, capas/masks/texturas/material parameters/decals y deps. El runtime no evalúa el graph de fabricación/placement cada frame ni carga Knowledge Library de autoría. Un cambio dinámico ya admitido (wetness, viento o daño) usa datos/algoritmo mínimo del owner runtime, no reintroduce el editor/generador completo. No invalidar el save/gameplay por cambiar quality tier.

Widgets ocultos no ejecutan previews/miniaturas/rebuild continuamente. Diferir recursos costosos, **no retrasar registro de tipos/serializers hasta perder datos al abrir un documento**. Mantener tipos mínimos para leer/preservar o rechazar explícitamente edición no soportada. Procesos herramientas en idle no se cuentan sólo por CPU del exe principal: medir también Asset Processor/workers necesarios.

## 6. Dieta de dependencias y binarios

Mantener sólo lo seleccionado: nada AWS/multiplayer/server/matchmaking/cloud/commerce ni DX11/OpenGL/Metal/WebGPU/móvil. Quitar del build/deployment generadores, examples y tests no requeridos por el runtime, conservando suite/fuentes de prueba y evidencia en sus ámbitos. No borrar repo/history/branches/licencias/SDKs sin manifiesto, clasificación y autorización de alcance.

Clasificar target/include/public/private/transitive graph, generated registration, resource bundles, linker map/imports y runtime-loaded modules. Presencia en carpeta no prueba que se compile, y target no prueba que se cargue. Una dependencia PUBLIC demasiado amplia se corrige migrando interfaz/consumers, no sólo cambiando PUBLIC a PRIVATE si rompe ABI/headers.

Shared geometry kernels, atlas/mips/meshes/instances, buffers y assets por GUID/hash sin copias duplicadas innecesarias. Cache con presupuesto/eviction/ref lifetimes; no retener todas las revisiones para siempre. Reutilizar memoria de jobs/frame arenas sólo con lifetime correcto. Eliminar espejo CPU de recurso GPU sólo cuando ningún consumer/cook/save/debug admitido lo necesite; readbacks no sustituyen memoria ahorrada sin medir stalls.

Link-time optimization/IPO, dead stripping y consolidación de DLLs son candidatos medidos por target/config. Verificar soporte de compilador/CMake, exports y registros por reflexión antes de usarlos; una clase registrada por nombre puede no aparecer en referencias estáticas. No `whole-archive` global ni forzar static link de todo; conservar condiciones de redistribución de terceros y symbol bundles de depuración. Comprobar tiempo de link/incremental/peak RAM además de bytes. No prometer rendimiento por renombrar módulos o reducir el número de DLLs.

## 7. Menos trabajo, además de menos archivos

PWAS: spatial queries/index existentes, reevaluar sólo regiones sucias, identificación estable y cache de resultados válidos; no escanear todos los objetos cada frame. EPLS: reutilizar campos/masks/bakes de material/geología, limitar capas/coste visible, no reservar atlas por cada instancia sin necesidad. Los límites de una propuesta deben producir reporte de truncado/cancel/fallback, no perder contenido silenciosamente.

Geology/CSG: jobs incrementales cancelables con revisions, regiones/voxels sucios, buffers reutilizables, preview de detalle acotado y cook final explícito. Calidad de preview no se transforma a escondidas en producto final. No mantener geometría high/low/CPU/GPU duplicada sin owner/residency y propósito.

Render: culling/HZB/instancing/LOD/HLOD, shadow cache, temporal/resolution budgets, capas/masks/decals y variantes material controladas en el renderer único. SIGGRAPH/idTech aporta algoritmos para problemas observados, no obligación de adoptar frameworks/VT/mesh shaders/RT. Sólo DX12/Vulkan, hardware features consultadas y fallbacks. Bake en lugar de geometría cuando no cambie silhouette/collision/gameplay y la política artística lo admita.

Workflow responsive: reimport/cook/preview no reconstruyen el engine; avanzar por jobs y publicar candidatos atómicos; despachar sólo a subscribers/dirty revisions. Limitar parallel jobs por RAM/IO medidos para no inmovilizar el escritorio. Jobs background reales del engine no significan trabajo asíncrono de este asistente.

## 8. Qué inspección ya sustenta la propuesta

Snapshot de producto: `54a6c98424fd39099049139a078175eb5f9ae6ca`.

- `CMakeLists.txt`, blob `b04c7699b7970691baecd8530eab575370ac166b`, líneas 95–fin: distingue INSTALLED_ENGINE, añade subdirectorios de Code/Assets/python/Tools/Templates, procesa dependencies Gem diferidas, genera StaticModules.inl/settings registries y dependencias runtime. Esto da puntos reales de integración/recorte, **no demuestra que todos se compilen/carguen**.
- `Code/CMakeLists.txt`, blob `8b64ec8280bda4544396cce7779a2e1621cf7e23`: agrega Framework/LauncherUnified/Legacy/Editor/Tools. Ver gates/targets efectivos dentro de cada subárbol antes de afirmar cuánto cuesta.
- Hallazgos previos DEP-01..12 sobre AWS/LFS/package defaults, networking, WhiteBox/Recast/RHI siguen en `research/JE_DEPENDENCY_AUDIT_2026_10_10.md`; no se midió tamaño/build/runtime en aquella auditoría.
- Donor `JericoFX/JericoEngine` `docs/PROCEDURAL_PWAS_EPLS_MASTER.md`, blob `4f89d6ba268a78148d40450ae6cdb545d15c1e8f`, secciones 1–11 consultadas: locks/manual layout, source/cooked, NoDraw, Smart Materials y Environmental Procedural Layer System. No se afirma revisión completa de todos los anexos/implementaciones.
- Donor `docs/JERICO_PROCEDURAL_STUDIO_1_0.md`, blob `2d85bcfe8d667fdf5b8698781d0afbab4aab99da`, líneas 1–135: Procedural Studio fabrica/PWAS coloca/BSP estructura; ventana integrada, graph avanzado no obligatorio en uso normal y productos cocinados. Se adoptan contratos originales, no se portó código.

Documentación primaria pública revisada: O3DE Asset Bundler release bundling permite seleccionar assets dependientes de seeds, no necesita empaquetar todo el proyecto. JE debe completar dependencias explícitas/indirectas usadas por scripts/presets/campañas antes de excluir un asset; nunca recortar sólo por no aparecer en la escena visible. No copiar el formato .pak como si ya fuera .epak.

Fuentes: https://docs.o3de.org/docs/user-guide/packaging/asset-bundler/bundle-assets-for-release/ ; https://www.docs.o3de.org/docs/user-guide/packaging/asset-bundler/verifying-bundles/ ; https://cmake.org/cmake/help/latest/module/CheckIPOSupported.html ; https://cmake.org/cmake/help/latest/prop_tgt/INTERPROCEDURAL_OPTIMIZATION.html . Las versiones/flags concretos se verifican en la toolchain efectiva; esta lectura no es benchmark ni mejora aplicada.

## 9. Gate de aceptación ligado al roadmap existente

No añadir un ciclo de auditorías que bloquee BSP. En N0, levantar inventario real de tamaño/targets/startup y las exclusiones; en N1/N2/N4/N8 medir el resultado visible de cada reducción/feature; N11/N12 cierran deployment/portabilidad/regresiones.

| Gate | Requisito/owner existente | Prueba |
|---|---|---|
| LEAN-01 | JER-003/004/095/098 | Herramientas completas nativas sin duplicar kernel/registry/engine ni conexión externa |
| LEAN-02 | JER-068/069/071/096 | Cierre runtime sin UI/recipes/builders innecesarios/AWS/multiplayer/extra RHI, manifest+link+ejecución |
| LEAN-03 | JER-067/074/101 | Bytes source/SDK/runtime/cache/símbolos y compile/idle/frame/cook por separado, antes/después medidos |
| LEAN-04 | JER-015/016/092/101 | Cambiar capa local no rehace mundo/geometry/physics sin dependencia; cancel y stale job no publican |
| PWE-01 | JER-045/049/053/099 | BSP manual + pieza Studio + propuesta PWAS con locks/clearance/seed y IDs persistentes |
| PWE-02 | JER-013/046/047/054/100 | EPLS masks/material/decals según inputs, no corte estructural oculto ni otro placement |
| PWE-03 | JER-005/068/097 | Apply/Undo/Save/reopen/cook/source-free con mismos resultados en los tres pares gráficos |
| PWE-04 | JER-076/094/095 | Nombres propios JE y recursos/código integrados, no Gems independientes ni alias JE=AZ |

Para aceptar una optimización debe mejorar su métrica objetivo reproduciblemente sin degradación no justificada de capacidad, calidad, latencia, memoria o seguridad. Mostrar tradeoffs explícitos si otra métrica empeora. No ocultar contenido/pérdida de features ni vaciar la escena para conseguir una cifra. No porcentajes inventados, promesas de GB o claims de LTO/async/no-leak sin pruebas.

**Esta publicación sólo fija alcance y pruebas. No se ha reducido el binario, medido el consumo, implementado PWAS/EPLS ni portado código de otras bases.** No elimina source/SDKs/caches/history ni cambia dependencias/CMake. Mantener evidencias previas; actualizar HANDOFF con mediciones reales cuando Codex ejecute el vertical.
