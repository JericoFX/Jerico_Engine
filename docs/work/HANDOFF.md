# Handoff — última directiva JE/BSP/geología

Fecha 2026-10-10. Rama única development. **N0 IN PROGRESS; build original BLOCKED; núcleo CSG VERIFIED con alcance focal, editor/runtime NOT TESTED.**

## Handoff de ejecución local

Checkout único `F:\JericoEngine\JericoEngine`, origin JericoFX/Jerico_Engine, base `35e413726655b13e9523bd478e7de3e55d21bc92`. Clone LFS skip, fetch/pull ff-only sin cambio y estado inicial limpio comprobados. Lecturas obligatorias y masters realizadas. No tocar repos históricos ni crear otra rama/worktree/owner. Padre integra/acepta/publica; dos writers dependency/BSP; Luna único owner CMake/build/tests, sólo evidencia.

[N0_EXECUTION_2026_10_10](../evidence/n0/N0_EXECUTION_2026_10_10.md) contiene comandos, logs/hashes RED/GREEN, inventarios y bloqueos. La evidencia se publica con source; builds/cache/source terceros quedan locales. No había builds viejos en el checkout; fixture propio accidental eliminado sin recursión. No limpiar otros repositorios.

Hecho: resolver package local con cache/hash y rechazo de destinos remotos/fallback, 24/24 casos GREEN; exclusión LFS verificada en un puntero raw sin materializar payload/hook/config cambiado; inventario hardware/source/deps. Núcleo CSG productivo real `JE::BSP::Csg` std+Manifold 3.5.1 Apache-2.0; vacío válido, errores/output atómico, 33 checks GREEN incluyendo habitación/corredor/aperturas. Adapter integrado por fuente conserva vacío rechazado hasta reparar consumidores render/collider; sin UUID/formato cambiado. No se ha absorbido WhiteBox Gem completo.

Bloqueo original demostrado: configure exit 1 / 11,839 s por zlib-1.2.11-rev5-windows, luego Freetype. Python privado 3.10.13 ausente, Python host 3.14.6 no sustituye el pin. Qt/PySide 6.11.2 del fork no cuentan con receta coincidente en el snapshot público revisado. Aprovisionamiento/SHA/rutas permitidas exactas en expediente y package-catalog.csv; no CloudFront/S3, lfs pull ni instalación automática.

Configure final del patch (mismo setup protegido, cache de compiler existente): exit 1 / 0,160 s, fatal específico primer zlib/SHA/rutas, sin cascada/fallback y sin build. No medir mejora cold/steady comparando esos dos intentos. Goal persistente creado en este chat por petición explícita del propietario; permanece ACTIVE con todos los pendientes de GOAL/ROADMAP.

Siguiente acción: obtener los paquetes coincidentes locales o auditar/reconstruir recetas con nuevo pin revisado; repetir configure protegido sólo al cambiar esos inputs, generar grafo y compilar Editor/Asset Processor/launcher. Probar Windows DX12 y Vulkan, guardar/abrir escena y registrar imagen/métricas reales. Linux Vulkan NOT TESTED por plataforma ausente. Después N1 material por cara/UV/lineage, grid/snap/selección/Inspector, Undo/Redo/Save/reopen/cook/runtime, teardown vacío; Procedural Studio/PWAS/EPLS y geología siguen el roadmap sobre esa escena.

No afirmar N0/N1 completos, engine ligero ni ausencia AWS/multiplayer por estos tests. Todavía existen composition/targets/red de juego heredados; clasificar consumidores/IPC, link/deploy/runtime antes de retirarlos. CMake upstream/pip/PATCH_FILES y licencias/SBOM siguen abiertos para cierre transitivo. Los apartados siguientes registran la revisión documental anterior y no sustituyen este estado ejecutado.

## Cambio del propietario incorporado

Sin AWS ni multiplayer en producto, no optional/deferred. Código propio/adaptado JE:: real, no AZ final ni alias decorativo. Solo DX12/Vulkan (Windows ambos, Linux Vulkan). Gems necesarios absorbidos dentro del engine. BSP/UI/materiales visibles primero, geología reactivada y optimización SIGGRAPH continua; ideas de otros motores permitidas conforme a licencia.

Conservar materiales PBR/ORM/separados, formatos r*/epak, gameplay/animation/UI/physics/AI/XR locales existentes. No expandir a plataformas externas. Buoyancy sigue DESIGN ONLY; RT/underwater y BSP→OpenVDB no se activan por la nueva orden.

## Hecho en esta revisión

Lectura real de dependencias CMake/CSG/paquetes: AzFramework→AzNetworking→OpenSSL/RapidXML/codegen; WhiteBox→Atom/editor/OpenMesh/Manifold y tests PhysX; CSG existente con geometría/validación y gaps de aceptación; Recast deps/pin v1.6.0/FetchContent; selección DX12/Vulkan Windows, Vulkan Linux; CloudFront/S3 default de packages y CloudFront LFS. Ver DEP-01..12 con blobs y alcance de lectura.

Actualizados los objetivos, identidad/composición, exclusiones, planes de ejecución y orden visible. Ningún namespace/source/build/backend/Gem ha sido convertido por escribir estos documentos. Ninguna prueba GPU/offline/linker ha sido ejecutada. Los commits históricos 013e887/a536fb7 contienen la planificación anterior y no son autoridad cuando contradicen GOAL actual.

## Próximo trabajo de Codex

N0 por el plan nuevo: repo/status/sync, dependencias permitidas locales, graph efectivo y baseline por pares disponibles. No consumir AWS para resolver build. Luego N1 cuarto/corredor/puerta/ventana con editor real, material/UV/Undo/save/cook/runtime y owner JE; N2/N4/N8 según necesidades, geología antes de completar todo gameplay.

No redescubrir cada vez el roadmap ni bloquear por un tipo abstracto. Publicar resultados visibles con evidence y seguir el siguiente vertical no bloqueado. Cuando falte hardware/paquete, conservar NOT TESTED/BLOCKED específico y avanzar tareas independientes.

## Pendientes que no se ocultan

Build/Editor/Asset Processor/runtime y métricas reales, cierre completo offline/transitivo/licencias/SBOM, refactor IPC/red, identidad JE y migración UUID/codegen/cache, absorción de Gem resources/builders, conservación UV/material de CSG, Jolt/physics/nav y geología GPU/mesher/cook. Default branch administrativo no se afirma cambiado por este conector; development explícito es la rama de trabajo. Ningún otro repositorio o rama fue borrado.

## Ampliación posterior a 54a6c98 — engine ligero con herramientas propias

El propietario pidió achicar/optimizar el motor y confirmó PWAS, EPLS (también escrito ELSP) y nuestras herramientas. Se añade `docs/JE_LEAN_ENGINE_AND_AUTHORING.md` y se enlaza desde GOAL/CODEX_START/índice. Procedural Studio fabrica, PWAS distribuye, EPLS aplica capas ambientales; Geometry/Jolt/Material siguen sus owners. Se conservan todas las capacidades y formatos comprometidos, sin añadir plataforma externa.

Lecturas nuevas: CMake raíz (rango de composición/dependencies/runtime generation), Code/CMakeLists y contratos originales PWAS/EPLS y Procedural Studio por secciones. La documentación oficial Asset Bundler confirma selección por dependencias de seeds; no se da por completa en JE ni por equivalente a .epak. No se afirma auditoría íntegra de todos los módulos ni de anexos originales.

N0 debe medir tamaños source/SDK/runtime/símbolos/cache y build/idle/CPU/GPU/RAM/VRAM por separado. Demo siguiente: BSP+pieza Studio+placement PWAS+capas EPLS, con locks/Apply/Cancel/Undo/save/reopen/cook/source-free. No regeneración global por capa ni recipes de autoría en el juego. LEAN-01..04/PWE-01..04 complementan JER existentes.

Esta ampliación es sólo Markdown: no recortó binarios/dependencias/source, no midió ahorros ni implementó PWAS/EPLS. Preservar source y evidencia; publicar cifras únicamente cuando existan mediciones reproducibles.
