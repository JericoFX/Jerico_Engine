# Vertical activo — N0: baseline JE permitido y preparación de BSP visible

Última directiva del propietario (2026-10-10): **motor, juego y editor sin Python**. CPython se conserva sólo como herramienta externa de compilación/codegen, nunca host, dependencia enlazada ni distribución de Editor/Runtime. Se sustituye el script propio de habitación por acción C++ nativa y se retiran EditorPythonBindings/QtForPython/PySide del recorrido, con revisión de consumidores. Las evidencias anteriores de fixture22/AST y CPython embebido son históricas; el fixture nativo requiere un nuevo freeze y validación. N0/N1 siguen abiertos.

Fecha 2026-10-10. Estado **N0 IN PROGRESS / ORIGINAL ENGINE BUILD BLOCKED**. development solamente. Checkout único `F:\JericoEngine\JericoEngine`, base `35e413726655b13e9523bd478e7de3e55d21bc92`. [Expediente y evidencia real](../evidence/n0/N0_EXECUTION_2026_10_10.md).

## Ejecución actual

Clone/fetch/pull ff-only y estado inicial limpio comprobados, documentos obligatorios leídos. Integración publicada anterior a esta ampliación `6eb79db287c8da7626e0aa6537de8bc8531acc26`; GOAL persistente ACTIVE en este chat. Inventarios de toolchain/source/LFS/packages registrados; repos históricos intactos. Se eliminó el output propio vacío `python-v143`; dos outputs propios poblados e inútiles (280.206.279 bytes) siguen inventariados porque la revisión automática rechazó su borrado recursivo por política, sin motivo más específico. No se intentó eludir el rechazo.

Paquetes focales verificados: zlib 1.2.11, Freetype 2.11.1, Lua 5.4.4, Zstd 1.3.5, LZ4 1.9.4 y OpenSSL 1.1.1o, con identidades JE. LZ4 pasó cuatro CTests en Release y Debug más siete guards; OpenSSL rev3 pasó seis CTests, incluidos siete guards y TLS por BIO en memoria. Catálogo/cache actualizados sólo tras GREEN. Python 3.10.13 rev3 y requirements43/43 son herramientas externas auditadas; sus pruebas de embedding son históricas y no autorizan Python en producto. Qt 6.11.2 exacto disponible; PySide/shiboken no instalados y excluidos del producto. Google Benchmark local ya se verificó. Root393 histórico terminó exit1/13,176s por GoogleTest entonces ausente; dos fallos previos del harness son NOT_RUN. El configure actual45argv/freeze414 terminó **exit1/34,403s por Expat2.4.2rev2 requerido por CrySystem.Static**. GoogleTest resolvió localmente y OpenSSL se extrajo/validó; fuentes/freezes/cache permanecieron intactos. Expat y TIFF (mismo target consumidor) están asignados a los dos writers, sin desactivar XML/codecs. Todavía no hay build del engine ni cierre transitivo.

Implementados resolver local sin default CloudFront/S3/fallback Git y `.lfsconfig` sin URL heredada, con exclusión de materialización. Suite CMake focal final 24/24 PASS; LFS devuelve un puntero real byte/hash idéntico, hooks/config intactos. Configure final exit 1 / 0,160 s con error específico zlib/SHA/rutas. Límites de cierre transitivo en evidencia. No equivalen a ausencia AWS/multiplayer en grafo/link/deploy/runtime; esa retirada sigue pendiente.

Núcleo productivo CSG migrado realmente a `JE::BSP::Csg`, sin AzCore/alias; conserva salida atómica, vacío válido y legacy geométrico. Añade UV por esquina e identidad de superficie de dos enteros de 64 bits, con costuras protegidas. RED/diagnóstico/GREEN reales: Release build exit 0 / 4,585 s, dos CTests exit 0 / 0,103 s, **117 checks PASS**; incluye habitación/corredor/puerta/ventana y negativos. Adapters White Box integrados por fuente **NOT VERIFIED** y todavía pierden los atributos; guard de vacío mantiene escena hasta teardown render/collider probado. Material Atom por cara, UV del editor y captura real pendientes.

`Projects/JEEditorFixture` actual tiene siete seeds y 21 archivos propios; el script propio `author_room.py` fue retirado. WhiteBox incorpora por fuente una acción C++ `Author JE Room/Corridor Fixture`, cinco operaciones CSG, selección nativa de formas y Undo/serialización/Save mediante APIs existentes, con guards de proyecto/prefab/ownership/locks. Freeze del writer13 SHA `F6C14C9F513A242034D7ADEA30C38BA38F3929CA9B0B73F9E48AE78EB18091E5`; tests de autoría pendientes de compilación/ejecución real. El freeze22/AST anterior es histórico. Recursos227 y dos ICO/BMP originales conservados; el PNG splash restante es un puntero LFS de132B, no una imagen, con cuatro endpoints404. **INTEGRATED NOT VERIFIED**: no Editor/AP/cook/runtime, Undo/Redo real, reapertura ni captura.

Retirada de Python del recorrido productivo implementada por fuente en38 archivos: Editor, AtomToolsFramework/tools, plantilla y composición. Freeze38 SHA `5774C11ADA2FBD2E68DA666CCD396305F93533DB6996AEB6F0D8DDD6B96D8143`; no compilación ni prueba de link/deploy/carga. Open/Edit Project usan APIs C++ existentes. New Project, New Component wizard y algoritmos de variantes antes implementados en Python siguen pendientes de portado nativo; no se dan por reemplazados al ocultar sus runners.

SQLite usa el wrapper original3.53.4, no el paquete legado3.37.2: verificador positivo y seis negativos PASS con siete inputs intactos. Los cuatro CTests nativos añadidos enlazan exclusivamente el target original y todavía no se ejecutaron. mcpp `_az.2` mantiene fuente/patch exactos; rev1 falló por LF→CRLF demostrado en once archivos. Rev2 escribeLF y comprueba hashes: producer0/12,615s, archive218.928B SHA `4c4ae3b71be82837b18090029d7ec49a19af80c12b35008b70a0d6f967acc589`,20miembros/19hashes, Release4/4+Debug4/4 y ocho guards PASS, freeze17 intacto. Catálogo/cache integrados tras revisión; captura previa de argv vacíos es NOT_RUN. No se cambia el pin para aceptar el fallo.

Owner: padre integración/aceptación/Git, writers dependency/BSP congelados al validar, Luna único owner CMake/build/tests. Ninguna plataforma/consumidor se acredita por el fixture aislado.

## Resultado de N0

Inspeccionar/configurar la base con dependencias privadas permitidas y locales, sin AWS/multiplayer productivos. Mostrar escena base con Editor/Asset Processor/runtime y registrar métricas. Pares obligatorios: Windows-DX12, Windows-Vulkan, Linux-Vulkan. Par no disponible queda NOT TESTED y no bloquea desarrollo visible del disponible; no se declara N0 portable completo.

## Lectura y tareas concretas

Leer GOAL/AGENTS, `docs/architecture/JE_IDENTITY_AND_NATIVE_MODULES.md`, `docs/research/JE_DEPENDENCY_AUDIT_2026_10_10.md` y N0_FOUNDATION_PLAN.

GoogleTest v1.18.0 ya aprovisionado desde archive oficial con SHA `6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5` y commit `063de7e9578f82b369302001269680b4b1553359`:252files/BSD3, override local existente, wrapper sin cambios. Splash3 nativo conserva40UI/SVG/puntero; RCC Qt6.11.2 exit0 sólo valida empaquetado, widget/Editor real pendientes. Root45argv/freeze414 `A1DA254EBC36446F21E61F41220AD90D26A7CF7A99EC8174AF36AAAB5038FD75` verificó GoogleTest y bloqueó por Expat; implementar y validar sus providers mínimos Expat/TIFF antes del siguiente configure. Generar targets/compilar Editor/AP/launcher y tests WhiteBox para JEEditorFixture. Continuar clasificación efectiva/IPC/servidor/multiplayer/RHI: AzSock de Editor↔AP es distinto de AzNetworking genérico; no retirar IPC ni consumidores de depuración Lua sin alternativa y pruebas.

Registrar fuente/configuración/build/códigos de salida, fixture/imagen real, coste cold/steady CPU/GPU/RAM/VRAM y limits. Lectura CMake no es linker/runtime. No generar placeholders JE para marcar avance.

## Siguiente vertical prioritario

N1 UI/BSP: cuarto/corredor/apertura puerta/ventana sobre WhiteBox CSG auditado; UV/material/Undo/Save/reopen y runtime. Integración del owner a JE dentro del engine sin project Gem install. Después N2 materiales y N4 collision, N8 geología adelantado; no esperar todos los sistemas de juego.

## Prohibiciones

Nada AWS/multiplayer/servidores/backend online. Solo DX12/Vulkan como render. No alias JE=AZ como solución final. Geología está activa; BSP→OpenVDB y RT de render no. Buoyancy continúa DESIGN ONLY. No copiar source/shaders de Cry ni borrar otros repositorios/ramas/evidencia.
