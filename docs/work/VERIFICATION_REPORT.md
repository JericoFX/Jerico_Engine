# Verificación JE — 2026-10-10

Última directiva del propietario (2026-10-10): **motor, juego y editor sin Python**. CPython se conserva sólo como herramienta externa de compilación/codegen, nunca host, dependencia enlazada ni distribución de Editor/Runtime. Se sustituye el script propio de habitación por acción C++ nativa y se retiran EditorPythonBindings/QtForPython/PySide del recorrido, con revisión de consumidores. Las evidencias anteriores de fixture22/AST y CPython embebido son históricas; el fixture nativo requiere un nuevo freeze y validación. N0/N1 siguen abiertos.

## Ejecución local posterior a la revisión documental

Base local `35e413726655b13e9523bd478e7de3e55d21bc92`, development. [Expediente N0](../evidence/n0/N0_EXECUTION_2026_10_10.md) e inventarios/logs/hashes asociados sustituyen el estado de «no ejecución» para sus scopes concretos.

Clone/sync/preservación comprobados; última publicación anterior a esta ampliación `6eb79db287c8da7626e0aa6537de8bc8531acc26`, SHA remoto verificado. Resolver24/24 y CSG117 reutilizan inputs sin cambios. Zlib/FT1/1 cada uno, Lua3/3 y Zstd4/4+negativos PASS. Python rev2 FAIL retenido; rev3 producer/package y Release3/3+Debugembed1/1 PASS con nueva procedencia de herramienta externa. Qt/PySide exactos adquiridos, PySide/shiboken NOT INSTALLED. El root histórico303 acepta Google Benchmark local y termina exit1/26,140s por LZ4 entonces ausente, sin build. Root303/303, Benchmark174/174, fixture histórico22/22, caché y fuentes pertinentes intactos entonces. Atomicwrites: RED42/43, fix de una línea GREEN43/43 parser/hashes/metadata PEP440, instalación offline y operaciones atómicas, sin upgrade de pins.

Zstd JE: producer exit0/30,094 s, paquete567.544B SHA `672edc4a41e31c6f45675b84a314cf1a31caa75428c543ea67a27253d9588793`, manifiesto82/82, consumer4/4 y seis guards PASS; root lo encontró1.3.5. Python rev3 producer262,024 s, paquete22.351.228B SHA `12e9eb0ec73bb28a683b250a1d702bad06c963dc8d4c9f4823b8d54871599917`, inputs11/11 y consumers Release3/3+Debug1/1. Memoria pico no medida; no se repite build por esa métrica. Fixture histórico22/22 con AST/seis JSON PASS. Manifest anterior20 es obsoleto por README19/20; su exit0 no establece20/20. Recursos227/228 materializados, Git227/227 y dos Resources con blobs existentes; splash Editor sólo puntero132B con endpoints404. Producto INTEGRATED NOT VERIFIED, sin Editor/AssetProcessor/frame.

CSG productivo JE::BSP::Csg con Manifold local: **117 checks GREEN en dos CTests Release**, incluidos errores atómicos, cuarto/corredor/aperturas, UV/costuras e identidad íntegra de superficies. RED, diagnóstico y fix preservados; tests/tolerancias sin cambiar después del RED. Inputs congelados antes/después y diffs/evidencia revisados por el padre. Adapters/editor sin compilación ni aceptación runtime; sus UV/material/serialización todavía no usan estos atributos.

Ampliación actual: LZ41.9.4 producer31,210s, Release/Debug4/4CTest y siete guards PASS; OpenSSL1.1.1o rev3 producer197,550s, seis CTests (cinco funcionales y siete guards agrupados) PASS; ambos integrados en catálogo/cache tras revisión. SQLite3.53.4 archivo/fourblobs y seis negativos PASS con siete inputs intactos; cuatro tests funcionales enlazan el target original pero aún no se ejecutaron. Helper de metadata de fuente release tiene RED real exit1 y GREEN schema2, matriz7/7; commit schema1 sigue intacto. mcpprev1 FAIL por LF→CRLF demostrado; rev2 producer0/12,615s, archive218.928B/20miembros/19hashes, Release4/4+Debug4/4 y ocho guards PASS, freeze17 intacto. Catálogo/cache integrado tras revisión del padre; captura anterior con argv vacíos es NOT_RUN. Advertencias upstream preservadas, no aceptación ShaderBuilder/Editor.

Retiro de Python productivo38 y autoría C++ WhiteBox13 implementados por fuente, freeze38/13 verificados por el padre; no se acredita compilación/link/deploy/ausenciaVM cargada. Fixture21/siete seeds actual; freeze22 y pruebas AST/Pythonembedding son históricas. New Project/New Component/variantes shader nativos siguen pendientes. Root44argv/freeze393 ejecutado una vez: **exit1/13,176s, primer fatal GoogleTest v1.18.0 ausente en AzTest**. Freeze393/393, Benchmark174/174, cache20/20, wheelhouse44/44 y fuentes privadas sin deltas; sólo27 archivos esperados por instalación LZ4 en engine packages. Pico medido39.567.360B corresponde sólo al proceso CMake padre, sin agregar hijos. Dos fallos anteriores del harness antes de CMake son NOT_RUN. GoogleTest ya aprovisionado y splash3 revisado con RCC0/0,055s sólo de empaquetado. Root siguiente414/45argv ejecutado una vez: exit1/34,403s, GoogleTest local resuelto, primer fatal Expat2.4.2rev2 en CrySystem.Static; OpenSSL1337files+stamp es única delta esperada. Freeze414/260/174/17/6, SQLite/cache/wheelhouse/pristines intactos. Pico52.621.312B sólo CMake padre. Expat/TIFF en preparación; engine sin build.

Windows-DX12 y Windows-Vulkan BLOCKED antes de generar targets; Linux-Vulkan NOT TESTED. No Editor/Asset Processor/launcher/frame/Undo/save/reopen/cook ni métricas RAM/VRAM/CPU/GPU; no cierre AWS/multiplayer/grafo/deploy/SBOM/source-free. N0/GOAL permanecen activos. Publicación local requiere diff/status/input hashes/evidencia, fetch/sync seguro, commit development/push normal y SHA remoto coincidente.

Outputs útiles preservados. Un output propio vacío se eliminó; dos outputs propios obsoletos poblados (280.206.279 bytes) permanecen inventariados porque la revisión automática rechazó el borrado recursivo por política, sin razón más específica. No se usó otro mecanismo para eludirlo. Los snapshots históricos/paquetes nuevos no se confunden con la futura build del engine.

## Registro histórico de la revisión documental

**Tipo: documentación y auditoría remota de source. No engine build ni ejecución.**

## Base comprobada

Development leído en `a536fb78c6195c3edf10afa34bfdf65c6f841ee5`, tree `d5405df115bd099310d8dbf69678c2b1f7ba5522`. Compare desde `241c78b8064898a709cf1c5dd3535c6406763ef3`: 2 commits, 25 Markdown afectados, ningún source/build/assets modificado. Ramas adicionales y repos históricos no se modifican por este encargo.

## Auditoría de source realizada

Hallazgos DEP-01..12 con archivos/blobs y límites en `docs/research/JE_DEPENDENCY_AUDIT_2026_10_10.md`: CloudFront/LFS/package defaults/S3, AzFramework→AzNetworking y libs/codegen, WhiteBox CMake/CSG/Manifold/editor/tests, Recast CMake/pin/FetchContent y listas runtime Windows/Linux de Atom. Csg.cpp y 3rdPartyPackages.cmake leídos por rangos, no íntegramente. Las dudas de CSG son pruebas propuestas, no bugs runtime reproducidos.

Documentación pública oficial consultada sobre RHI DX12/Vulkan/plataformas y licencias Qt. No relectura completa de todos los papers/diapositivas ni auditoría total de transitive dependencies. No se publican tokens/URLs temporales de las respuestas GitHub.

## Revisión de política

Última orden prevalece: JE:: real (no alias AZ), Gems dentro del engine, AWS/multiplayer EXCLUDED, Windows DX12/Vulkan y Linux Vulkan, BSP/UI temprano, geología activa, formatos/materiales propios preservados, SIGGRAPH/ideas de otros motores por beneficio/licencia. No activar RT/BSP→VDB/buoyancy por asociación. Los masters/rutas técnicas anteriores se reconciliaron con esta precedencia y con la matriz JER.

## No realizado / no acreditado

No CMake configure/build; no linker map/deployment/runtime imports; no namespace/source/Gem migration; no retirada de AWS/multiplayer/RHI; no descarga/materialización LFS; no captura de red ni prueba offline; no Editor/Asset Processor/launcher/Jolt/Luau/GPU/headset; no mediciones FPS/RAM/VRAM; no SBOM exhaustivo ni certificado legal. No independencia ni optimización ya conseguida.

## Gate de publicación

La edición se crea sobre el tree padre inspeccionado, afecta únicamente Markdown y usa commit parent/ref development esperados, sin force. Antes de mover la ref se compara el candidato con a536fb7, se revisan archivos/cambios y se vuelve a leer HEAD. Después se comprueba ref y contenidos. Es revisión Git remota, no afirmar `git pull`/`git diff --check` local ni tests del motor que no se ejecutaron.

Codex debe convertir cada pendiente N0 y cada DEP en evidencia real del scope pertinente. No sustituir este informe por PASS global tras un build o una captura.
