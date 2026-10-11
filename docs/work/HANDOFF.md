# Handoff — última directiva JE/BSP/geología

Última directiva del propietario (2026-10-10): **motor, juego y editor sin Python**. CPython se conserva sólo como herramienta externa de compilación/codegen, nunca host, dependencia enlazada ni distribución de Editor/Runtime. Se sustituye el script propio de habitación por acción C++ nativa y se retiran EditorPythonBindings/QtForPython/PySide del recorrido, con revisión de consumidores. Las evidencias anteriores de fixture22/AST y CPython embebido son históricas; el fixture nativo requiere un nuevo freeze y validación. N0/N1 siguen abiertos.

Fecha 2026-10-10. Rama única development. **N0 IN PROGRESS; build original BLOCKED; núcleo CSG VERIFIED con alcance focal, editor/runtime NOT TESTED.**

## Handoff de ejecución local

Checkout único `F:\JericoEngine\JericoEngine`, origin JericoFX/Jerico_Engine, base `35e413726655b13e9523bd478e7de3e55d21bc92`. Clone LFS skip, fetch/pull ff-only sin cambio y estado inicial limpio comprobados. Lecturas obligatorias y masters realizadas. No tocar repos históricos ni crear otra rama/worktree/owner. Padre integra/acepta/publica; dos writers dependency/BSP; Luna único owner CMake/build/tests, sólo evidencia.

[N0_EXECUTION_2026_10_10](../evidence/n0/N0_EXECUTION_2026_10_10.md) contiene comandos, logs/hashes RED/GREEN, inventarios y bloqueos. La evidencia se publica con source; builds/cache/source terceros quedan locales. Publicación anterior a esta ampliación `6eb79db287c8da7626e0aa6537de8bc8531acc26`, fetch posterior sin divergencia; GOAL persistente ACTIVE en este chat. No había builds viejos en el checkout. El output propio vacío `python-v143` fue eliminado; dos outputs poblados e inútiles (280.206.279 bytes) permanecen inventariados por rechazo automático del borrado recursivo, cuyo único motivo comunicado fue política. No eludirlo ni limpiar repos históricos.

Hecho: resolver local 24/24 GREEN, exclusión LFS verificada sin materializar payload y hardware/source/deps inventariados. Núcleo real `JE::BSP::Csg` std+Manifold 3.5.1 Apache-2.0: geometría legacy, salida atómica, UV por esquina e identidad de superficie de 128 bits conservadas, costuras protegidas; **117 checks GREEN en dos CTests Release**. Habitación/corredor/aperturas siguen en el fixture real. Los adapters todavía pierden atributos y rechazan vacío hasta reparar consumidores render/collider; sin UUID/formato cambiado. No se ha absorbido WhiteBox Gem completo ni probado el editor.

Desbloqueos verificados: zlib 1.2.11 y Freetype 2.11.1 reconstruidos estáticos con identidades JE, consumers de compresión/errores y rasterización de glifo PASS. Python rev1 histórico **3.10.13** Debug/Release recompilado con v143, paquete de 22.341.204 bytes SHA `4370b5870c6b4da65206f6c8e82ca5540f0f2c60fe4cb2bbbd34dbf1cc7dd2d9`, consumer funcional Release 3/3 y Debug embed 1/1 PASS antes del oráculo de procedencia. Bootstrap 3.12.9 sólo generó archivos; no sustituyó runtime. Qt/PySide 6.11.2 exactos recuperados de GitHub Actions/Azure tras auditar destinos, hashes y manifests. Pin y avisos originales preservados; seguridad/redistribución completa no están cerradas.

Histórico: root Python rev1 y luego rev3 aceptaron los paquetes locales hasta Google Benchmark. Python rev2 falla banner y no se integra; rev3 producer262,024 s, paquete22.351.228 bytes SHA `12e9eb0ec73bb28a683b250a1d702bad06c963dc8d4c9f4823b8d54871599917`, Release3/3+Debugembed1/1 y manifiesto3.282/3.282 PASS. Lua3/3 y Zstd4/4+negativos PASS. Atomicwrites: RED42/43 por continuación de pluggy, fix de una línea GREEN43/43 parser/hashes/metadata PEP440, instalación offline0/2,390 s y operaciones atómicas PASS. Root histórico Google Benchmark local/recursos: exit1/26,140s, entonces LZ4 ausente. Freeze303/Benchmark174/fixture22, caché y fuentes pertinentes intactos entonces. Ningún build/frame.

Continuación actual: LZ41.9.4 y OpenSSL1.1.1o tienen producer/consumers/guards nativos GREEN y están en catálogo/cache. LZ4:167.540B, SHA `9e7e44c4e9d70307e22da51ecbfb71934770ca0ec7c4077d5ee739c2a21c2eac`; OpenSSL rev3:7.040.252B, SHA `122f6d3a20fd4e8bdc158bccae6014f129be3fac7f16704ef01e75b8a2491888`. Root44argv/freeze393 terminó exit1/13,176s por GoogleTest v1.18.0 en AzTest; inputs intactos, sólo27 archivos de extracción LZ4 esperados. Fallos anteriores de harness son NOT_RUN. GoogleTest oficial exacto ya disponible (252files/4.143.491B, freeze260 `9238D2FFB4B465D196167504CBDD9AB7134F6167D6AD2374C047C1A9F6571FDB`). Root45argv/freeze414 `A1DA254EBC36446F21E61F41220AD90D26A7CF7A99EC8174AF36AAAB5038FD75` terminó exit1/34,403s: GoogleTest local válido, siguiente Expat2.4.2rev2 en CrySystem.Static; OpenSSL1337files+stamp extraídos y demás fuentes/freezes/cache sin deltas. Pico52.621.312B sólo CMake padre, sin hijos. Dos writers asignados Expat/TIFF exactos; no desactivar XML ni codecs que conserva receta. Luna único owner de próximas validaciones. SQLiteverificador positivo/seis negativos PASS; consumidor enlaza target original, pendiente nativebuild. mcpprev1 falló por LF→CRLF demostrado; rev2 corrige escritura sin cambiar pins y pasó producer0/12,615s, archive218.928B SHA `4c4ae3b71be82837b18090029d7ec49a19af80c12b35008b70a0d6f967acc589`,20miembros/19hashes, consumers Release4/4+Debug4/4 y ocho guards. Freeze17/17 intacto; logs iniciales con argv vacíos son NOT_RUN. Catálogo/cache integrados tras revisión; outputs rev1 preservados.

Producto candidato actual: retiro de runners/hosting Python en38archivos y autoría WhiteBox C++13archivos congelados; no compilados ni ejecutados. Fixture21archivos/siete seeds, acción Tools `Author JE Room/Corridor Fixture`, formas nativas y APIs existentes de Undo/Save; el script propio de habitación se retiró. Freeze22/AST/embeddingCPython históricos. Material por cara/UV/persistencia, Undo/Redo/reapertura/cook/runtime siguen abiertos; New Project/New Component/variantes de shader requieren portado nativo para conservar funcionalidad. Splash3 dibuja gradiente opaco nativo; UI/SVG/puntero132B preservados y referenciaPNG retirada. Freeze6 `BEBD49E54ECEDC444898E4D1C17F7FC5101DB00CCB34DBCF39A2D0F2DE876DB2`; rcc Qt6.11.2 exit0/0,055s acredita sólo empaquetado, no compilación del widget ni Editor/UI. Ningún frame ni baseline Editor/AP/launcher. Probar Windows DX12/Vulkan; Linux Vulkan NOT TESTED. Después Studio/PWAS/EPLS/geología según roadmap.

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
