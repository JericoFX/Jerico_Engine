# Handoff — última directiva JE/BSP/geología

Fecha 2026-10-10. Rama única development. **N0 IN PROGRESS; build original BLOCKED; núcleo CSG VERIFIED con alcance focal, editor/runtime NOT TESTED.**

## Handoff de ejecución local

Checkout único `F:\JericoEngine\JericoEngine`, origin JericoFX/Jerico_Engine, base `35e413726655b13e9523bd478e7de3e55d21bc92`. Clone LFS skip, fetch/pull ff-only sin cambio y estado inicial limpio comprobados. Lecturas obligatorias y masters realizadas. No tocar repos históricos ni crear otra rama/worktree/owner. Padre integra/acepta/publica; dos writers dependency/BSP; Luna único owner CMake/build/tests, sólo evidencia.

[N0_EXECUTION_2026_10_10](../evidence/n0/N0_EXECUTION_2026_10_10.md) contiene comandos, logs/hashes RED/GREEN, inventarios y bloqueos. La evidencia se publica con source; builds/cache/source terceros quedan locales. Publicación anterior a esta ampliación `b54a874e3d0d312d78ad73f79920e676f26b932b`, fetch posterior sin divergencia; GOAL persistente ACTIVE en este chat. No había builds viejos en el checkout. El output propio vacío `python-v143` fue eliminado; dos outputs poblados e inútiles (280.206.279 bytes) permanecen inventariados por rechazo automático del borrado recursivo, cuyo único motivo comunicado fue política. No eludirlo ni limpiar repos históricos.

Hecho: resolver local 24/24 GREEN, exclusión LFS verificada sin materializar payload y hardware/source/deps inventariados. Núcleo real `JE::BSP::Csg` std+Manifold 3.5.1 Apache-2.0: geometría legacy, salida atómica, UV por esquina e identidad de superficie de 128 bits conservadas, costuras protegidas; **117 checks GREEN en dos CTests Release**. Habitación/corredor/aperturas siguen en el fixture real. Los adapters todavía pierden atributos y rechazan vacío hasta reparar consumidores render/collider; sin UUID/formato cambiado. No se ha absorbido WhiteBox Gem completo ni probado el editor.

Desbloqueos verificados: zlib 1.2.11 y Freetype 2.11.1 reconstruidos estáticos con identidades JE, consumers de compresión/errores y rasterización de glifo PASS. Python **3.10.13** Debug/Release recompilado con v143, paquete de 22.341.204 bytes SHA `4370b5870c6b4da65206f6c8e82ca5540f0f2c60fe4cb2bbbd34dbf1cc7dd2d9`, consumer Release 3/3 y Debug embed 1/1 PASS. Bootstrap 3.12.9 sólo generó archivos; no sustituyó runtime. Qt/PySide 6.11.2 exactos recuperados de GitHub Actions/Azure tras auditar destinos, hashes y manifests. Pin y avisos originales preservados; seguridad/redistribución completa no están cerradas.

Configure raíz con Python/wheelhouse instaló requisitos/cuatro editables sin índices; pip check/list/imports/ausencia boto3/botocore/s3transfer PASS, PySide/shiboken NOT INSTALLED. Negativo dedicado corregido exit 1 / 86,968 s fatal wheelhouse ausente, venv local creado/sin requirement stamps, cache/freeze intactos; captura inicial defectuosa no acredita nativeexit/tiempo. Mimalloc 3.5.2/pin/MIT aceptado. **Último configure (BOM/CRLF final preservado) exit 1 / 9,528 s por Lua-5.4.4-rev1-windows, SHA `8ac853288712267ec9763be152a9274ce87b54728b8add97e2ba73c0fd5a0345`**, archive faltante root-package-cache, unpack original-engine-deps/packages. Fuente Lua/receta/patch ya auditados; producer pendiente. No build/frame. Banner Git de Python toma ancestor engine: afecta sys.version/_git y no version_info/ABI; corregirlo exige rebuild/package/hash/consumers nuevos.

Siguiente acción: reconstruir/aprovisionar Lua 5.4.4 tras auditoría de receta/ABI, seguir siguiente paquete concreto, corregir banner Git de Python y revalidarlo; generar grafo/compilar Editor/AP/launcher. Ruta AutomatedTesting declara WhiteBox/Atom y Base.prefab real, pero incluye multiplayer y seis TIF Atom/UV aún punteros LFS; ajustar composición/fuentes permitidas antes de la escena. Probar Windows DX12/Vulkan, Save/reopen/imagen/métricas; Linux Vulkan NOT TESTED. N1: atributos a halfedges/serialización/material Atom, grid/snap/selección/Inspector, Undo/Redo/cook/runtime/teardown. Studio/PWAS/EPLS/geología siguen en la misma escena.

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
