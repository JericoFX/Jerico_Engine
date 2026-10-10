# N0 — ejecución local y preparación del BSP

Fecha: 2026-10-10. **N0 IN PROGRESS; baseline original BLOCKED por dependencias locales ausentes. N1/editor/runtime NOT TESTED.** Este expediente distingue las pruebas ejecutadas del trabajo de producto pendiente.

`logs/copy-manifest.csv` registra las 75 copias del primer lote; entonces `evidence-files.sha256` contenía 88 outputs/inventarios. La ampliación copia **321 archivos / 13.566.958 bytes**, con SHA idénticos a sus orígenes, registrada en `local-packages/copy-manifest.csv`; el manifest SHA global cubre ambos lotes y los manifests de copia. `.gitattributes` conserva bytes/EOL y espaciado original de transcripts `.log`; source/Markdown conservan el chequeo de whitespace. Los logs son evidencia de esta plataforma/configuración, no aceptación portable.

## Checkout, preservación y autoridad

- Checkout único: `F:\JericoEngine\JericoEngine`; origin `https://github.com/JericoFX/Jerico_Engine.git`; rama `development`.
- Base: `35e413726655b13e9523bd478e7de3e55d21bc92`. La carpeta inicial `F:\JericoEngine` contenía sólo `graft/`, no un checkout: se preservó y el clone se realizó en su subcarpeta, con `GIT_LFS_SKIP_SMUDGE=1`; la variable se retiró después.
- Se verificaron remotos/rama/HEAD/status; `fetch` y `pull --ff-only` no cambiaron la base. El checkout estaba limpio antes de editar. No se crearon ramas/worktrees/forks ni se modificaron repositorios históricos.
- Padre integrador/revisor; dos writers con scopes separados (resolver CMake y CSG); Luna como único dueño de CMake/build/tests y evidencia. No se afirma un cambio de modelo de agentes ya existentes por modificar configuración.
- No había builds/caches/símbolos anteriores en este checkout. Los outputs de `build/n0-validation` pertenecen a esta ejecución y se conservan para reproducción. La invocación inicial incorrecta que usó literalmente `$outdir` creó tres archivos de fixture: el padre eliminó esos tres archivos exactos y sus directorios vacíos, sin borrado recursivo ni tocar builds de otros repositorios.

Lectura obligatoria completada: AGENTS, GOAL, CODEX_START, docs/README, ROADMAP, ACTIVE_SLICE, HANDOFF, JE_LEAN_ENGINE_AND_AUTHORING, JE_IDENTITY_AND_NATIVE_MODULES, JE_DEPENDENCY_AUDIT_2026_10_10 y N0_FOUNDATION_PLAN. También se consultaron Definition of Done, libro de errores, matrices y masters de BSP/editor, geología, materiales/formatos, PWAS/EPLS y raster/SIGGRAPH. Las decisiones existentes se reutilizan; no se rediseña el roadmap.

## Inventario medido y límites

Ver [toolchain.json](toolchain.json), [source-inventory.json](source-inventory.json), [package-catalog.csv](package-catalog.csv) y [lfs-inventory.csv](lfs-inventory.csv).

| Medición | Resultado | Alcance |
|---|---|---|
| Blobs Git del HEAD base | 31.082 / 312.477.625 bytes | Árbol versionado, no tamaño del engine compilado |
| Source candidato | 21.261 / 151.524.116 bytes | Clasificación heurística por path/extensión |
| Punteros LFS | 2.906 / 2.513.790.779 bytes de payload declarado | Cero payloads locales/materializados, verificados por OID/hash |
| Binarios del engine disponibles | Ninguno | No hay cifra de SDK/Shipping/símbolos aceptable todavía |
| Toolchain usado | VS BuildTools 17.14.35, MSVC 19.44.35228, SDK 10.0.26100, CMake 4.3.4 | Windows x64, generador VS 17 2022 |
| GPU/driver | RTX 3090, 24.576 MiB físicos, driver 616.92 | Inventario de hardware; no uso de VRAM del engine |
| Vulkan | SDK 1.4.350.0, `vulkaninfo --summary` exit 0 | Enumera GPU; dos avisos de layers de overlay ausentes; no render JE |

No se midieron RAM/VRAM del Editor o juego, carga cold/steady, CPU/GPU frame time, FPS/p95/p99 ni ahorro de binarios. La duración de configure y del fixture CSG no representa una compilación completa ni rendimiento del producto.

## Ampliación ejecutada: paquetes locales y atributos BSP

Última integración publicada antes de esta ampliación: `b54a874e3d0d312d78ad73f79920e676f26b932b`, development. Los paquetes reconstruidos reciben identidad y SHA propios: no se atribuye el hash del binario O3DE original a una build distinta. Fuentes/archivos inmutables permanecen en `F:\JericoEngine\Dependencies`; sólo copias de trabajo bajo `build/n0-validation` se compilan. Se revisaron las recetas O3DE del snapshot `70f0b7b5257bf7cf935bf1cfd270e1b53ccead18`; no se ejecutaron sus scripts de descarga/upload/limpieza.

| Paquete incorporado | Bytes tar.xz | SHA256 | Verificación real |
|---|---:|---|---|
| zlib-1.2.11-je-rev1-windows | 92.556 | `4ff69c9c7f99c917afce6d04cdf3fc6f2a65c0038abf2c677ef418182851bef7` | Producer/consumer/CTest 1/1 PASS; roundtrip, buffer corto y stream corrupto |
| freetype-2.11.1-je-rev1-windows | 712.192 | `a6999aa779cd5e37e415eb784188f73763d396eaaca58f5b9485e2a5568b9802` | Producer/consumer/CTest 1/1 PASS; FreeType con zlib requerido |
| python-3.10.13-je-rev1-windows | 22.341.204 | `4370b5870c6b4da65206f6c8e82ca5540f0f2c60fe4cb2bbbd34dbf1cc7dd2d9` | Debug/Release compilados; consumer CTest Release 3/3, Debug embed 1/1 PASS |
| qt-6.11.2-rev1-windows | 201.790.724 | `f09edc180df0ded92827279c475167be13cbb517de182a5b10ec1337e68dd834` | Hash original exacto; 4.798 entradas de manifest verificadas; Qt real 6.11.2 alcanzado por root configure |
| pyside6-6.11.2-py3.10-rev1-windows | 22.663.592 | `cd0272eb5ce46a3437132344a4a6b7022e3aee152da888963cdd9d025cbdf506` | Hash original exacto; 305 entradas verificadas; setup/Find revisados, import raíz pendiente |

### Fuentes, build y consumers

zlib fuente `madler/zlib@cacf7f1d4e3d44d871b605da3b647f07d718623f`, archivo 645.528 bytes SHA `6d4d6640ca3121620995ee255945161821218752b551a1a180f4215f7d124d45`; FreeType `freetype/freetype@3f83daeecb1a78d851b660eed025eeba362c0e4a`, archivo 2.189.273 bytes SHA `e01c4b6eef4b6896d3e4822c5a8d7871dd5dcc39d25103ed3b338914ca7af69d`. Licencias Zlib/FTL y alternativas originales preservadas. Se mantiene la configuración de features de la receta: FT estático/PIC con zlib, sin Bzip/PNG/HarfBuzz/Brotli. El consumer rasteriza Arial `A` a 48 px: 34×35, avance 26.6 de 2.048, 447 muestras no nulas, cobertura 93.295, FNV1a64 `c2fede9f900d13a4`. Font local leído con hash, sin copiar/distribuir. Detalles, libs/exes/tamaños, siete negativos auténticos y snapshots de fuentes en `local-packages/final-validation-summary.json`.

Python fuente `python/cpython@49965601d6afedafe47cc85556d99b7a24981051`, archivo 26.359.731 bytes SHA `12c564214c8a3b109d294143cb439e8bccb1cf6be677b52e98c5e2c41941ea44`. Siete externals CPython fijados por commit/archive/hash: bzip2 1.0.8, SQLite 3.40.1.0, xz 5.2.5, zlib 1.2.13, libffi 3.3.0, OpenSSL 1.1.1u y Tcl/Tk 8.6.12.0. Expat se conserva en 2.4.6 por overlay de la receta original; no se presenta como actualización de seguridad. Pins/URLs/licencias y el único enlace interno de README materializado como copia regular están en `python-input-acquisition.json`, `python-externals-manifest.json`, `python-input-tar-audit.json` y `python-execution-input-audit.json`.

Se usó MSVC v143 por disponibilidad real, no v141 ni un binario original atribuido falsamente; bootstrap Python 3.12.9 sólo genera fuentes/layout, runtime exacto **3.10.13**. No se ejecutaron get_externals/find_python/NuGet/upgrade pip remotos. Primer producer falló por la unión ExternalsDir; rev2 compiló Debug/Release y layout en 238,080 s, pero tar falló por longitud de argv de Windows. Fix demostrado del helper: file list externa; empaquetado repetido **sin recompilar**, exit 0 / 42,303 s. 3.282 hashes de payload/metadata/licencia/provenance verifican; extracción desde el archive 3.283 archivos / 104.981.177 bytes. Consumers enlazan C API y prueban compresión, SQLite, SSL, ctypes, Expat, Tcl y venv/ensurepip locales. Release creó 191 `.pyc` sólo en la copia extraída; archivos declarados, archive y `.contents` permanecieron intactos. Resumen `python-package-retry-final-summary.json`; fuentes congeladas `source-freeze-python-parent-rev2.json`.

El helper común cambió a SHA `B067C707919808DDA7697CE79EFEC94A31029FF03B6CC383E374209CC8B68E38`; los paquetes zlib/FT ya verificados fueron generados con su SHA anterior `AB1C32742EA8885CAFA3AA2EE134A252DE86B030D47382D6A4DAEE7DC17604FD`. No se inventa regeneración de esos archivos. Los negativos de argv corrupto se conservan como incidentes del harness, **no** PASS; el negativo válido de salida preexistente protege los outputs/hash. Ver error book O04–O08.

Deuda de metadatos Python demostrada: `sys.version/sys._git` anuncian `heads/development-dirty:b54a874e3d` del repo engine. `_GetBuildInfo` ejecuta Git sin root explícito y descubre el ancestor del source-work sin `.git`. Los cinco archivos CPython pertinentes coinciden con source snapshot; el pin fuente y `sys.version_info/hexversion` 3.10.13/ABI son correctos. Propiedades globales GitVersion/GitTag/GitBranch permiten corregir esos campos en una futura reconstrucción; no omiten los tres procesos Git. **No se ejecutó ese candidato ni se atribuye el banner actual al commit upstream**; corregirlo cambia DLL/PDB/archive/SHA y requiere consumers/root nuevos. `python-git-provenance-audit.json` registra causa/alcance/opciones; queda pendiente antes de cerrar procedencia del SDK.

### Qt/PySide exactos y Python offline

Artefactos GitHub Actions `9829252229` y `9917233169`, runs `33578418170`/`33817214219`, commits `4199ae654199cf08fa9415c08905485df2824c0b`/`78c04902198c8fe5210a51799458c13ac8251ac7`. Se inspeccionó el redirect de la API antes de descargar; GET manual a `productionresultssa1.blob.core.windows.net` / `productionresultssa11.blob.core.windows.net`, sin auth enviada al host binario, redirects automáticos ni URL firmada retenida. SHA de archives coincide exactamente con catálogo, aunque el estado abierto de PR de packaging no acredita promoción. Recibos `artifact-destination-head.json`, `exact-actions-artifacts.json`, `exact-package-tar-audit.json`; cinco hardlinks Qt se comprobaron contra targets internos regulares. Find/config/setup revisados antes de su uso; ningún download/upload activo en esas rutas. El `ExternalProject` observado usa sólo SOURCE_DIR de ejemplos locales. Avisos LGPL y fuente/patches para redistribución siguen pendientes; no se obtiene el source offer S3 heredado.

Se auditó el destino de **cada** uno de los 44 archivos Python antes de GET directo a `files.pythonhosted.org`, sin redirects automáticos. 60.285.263 bytes; 43 requisitos base Windows CPython 3.10 exactos más wheel 0.38.4 sólo Tools para construir atomicwrites 1.4.1 sdist offline. 43 wheels: RECORD completo; 171 PE todos AMD64; sdist setup sin install_requires/setup_requires/pyproject ni downloader. `python-wheelhouse-audit.json`, `python-wheelhouse-acquisition.json` y `python-wheelhouse-payload-summary.json` registran integridad/closure estática/avisos. DCC opcional separado no se adquirió. Requests 2.32.0 está yanked: deuda de seguridad registrada, no upgrade silencioso.

Se retiraron sólo boto3/botocore/s3transfer de `python/requirements.txt` tras revisar consumidores AWS de CI/TIA/metrics. No se acredita funcionamiento de esos consumidores excluidos. `LYPython.cmake` exige roots/wheelhouse absolutos locales, instala con no-index/no-deps/no-build-isolation/no-cache-dir y config pip deshabilitada; pip check fatal precede stamp. Wrappers directos fuera de CMake conservan fallback heredado; Linux/ROS no se ejecutaron ni se declara su cierre transitivo.

Root configure con Python JE/wheelhouse local: **exit 1 / 58,869 s**, después de instalar requisitos y cuatro paquetes editables (LyTestTools, RemoteConsole, EditorPythonTestTools y o3de) sin índices; pip check sin dependencias rotas. Primer siguiente fatal específico: **mimalloc** no aprovisionado, pin `db5c4aaaf356edfeb1aa236b3a052fe3c67d01419db36613f783999341ed5619`, commit `636510a36ab743f76a582067142f29d15b024c90`. No se ejecutó build tras ese fallo. Auditoría posterior del venv real `original-engine-deps/Python/venv/31e81382` confirmó runtime 3.10.13, pip check/list exit 0 e imports numpy 1.23.0, scipy 1.8.1, Pillow 10.3.0, pytest 7.2.0, requests 2.32.0, GitPython 3.1.44 y los cuatro editables; boto3/botocore/s3transfer sin distribución ni module spec. PySide/shiboken todavía no se instalaron ni importaron. Capturas `original-engine-wheelhouse-positive.*` y auditoría posterior asociada; freeze raíz de 16 inputs sin cambios. Acredita esta ruta Python, no closure offline integral del engine.

Mimalloc v3.5.2 se adquirió luego manualmente del tag oficial codeload: 1.453.996 bytes, SHA exactamente igual al pin consumidor. Tar auditado antes de extraer: 411 miembros / 382 archivos / 5.931.383 bytes, ningún enlace; licencia MIT conservada. Fuente local `F:\JericoEngine\Dependencies\mimalloc-3.5.2`; receipts `mimalloc-source-{acquisition,tar-audit,extraction}.json`. El consumidor usa `NO_ADD_SUBDIR`: no configura CMake upstream; declara 21 fuentes CXX en target estático. Metadata heredada se corrige a 3.5.2/hash reales. Configure con `FETCHCONTENT_SOURCE_DIR_MIMALLOC` explícito: **exit 1 / 9,525 s**, acepta `mimalloc 3.5.2 (MIT)` y falla en siguiente **Lua-5.4.4-rev1-windows**, SHA `8ac853288712267ec9763be152a9274ce87b54728b8add97e2ba73c0fd5a0345`, archivo faltante en root-package-cache y unpack en original-engine-deps/packages. Freeze 20/20, fuente mimalloc y cache 54 archivos / 307.886.048 bytes intactos; ninguna build. Capturas `original-engine-mimalloc-configure.*`.

Negativo wheelhouse corregido en build/deps dedicados: **exit 1 / 86,968 s** con `JE Python wheelhouse missing`, ensurepip local creó venv y no creó requirements stamps. Freeze 16/16 y cache compartido intactos; no descarga de paquetes externos. `original-engine-wheelhouse-negative-capture.*` conserva argv/log/exit/tiempo/result. El primer intento tuvo excepción de captura PowerShell por stderr: sus nativeexit/tiempo no se acreditan; se conserva `negative-capture-gap.log` y la repetición está justificada por esa corrección del harness. Tampoco se cuenta como ejecución CMake un guard de índices argv equivocados que abortó antes de lanzarlo.

El diff del padre detectó que la edición de Python_windows había retirado su BOM UTF-8 original; se restauró BOM/CRLF conservando sólo las dos líneas ID/hash cambiadas. Por ese input final se repitió configure una vez: **exit 1 / 9,528 s**, mimalloc local aceptado, primer fatal Lua idéntico, 20 inputs/fuente/cache intactos. `source-freeze-root-mimalloc-encoding.json`, `original-engine-mimalloc-encoding-configure.*` son evidencia de las entradas finales publicadas; 9,525 s corresponde al intento anterior.

Lua siguiente: receta O3DE rev1 identificada y fuente oficial tag v5.4.4/commit `5d708c3f9cae12820e415d4f89c9eacbe2ab964b` adquirida manualmente tras HEAD auditado, 455.973 bytes SHA `2ff89bbea22a7c8b910d6b006f5ba4559edcf468a25537cedf9988ed90f6b450`. Tar 115 miembros/110 archivos/1.650.217 bytes, sin links/collisions; source inmutable `F:\JericoEngine\Dependencies\lua-5.4.4`. Patch O3DE SHA `5a8921ff20845030ccdfc3bc67f7797fe2339eb0e54a93c1089e748ea698120d`/MIT y library estática onelua Release de receta deben preservarse. Aún no hay paquete JE Lua ni consumer/build: implementación siguiente, no PASS inferido de esta adquisición. `lua-audit-summary.json` y `lua-source-{acquisition,tar-audit,extraction}.json`; la primera extracción de texto legal escogió una sección incorrecta, conservada como incidente, y se corrigió leyendo el bloque final real antes de extraer source.

### CSG con UV y superficies: RED y GREEN actuales

Se añaden atributos opcionales al kernel productivo: dos UV finitos por esquina e identidad por triángulo `SurfaceKey{namespace,surface}` con dos enteros de 64 bits. Atributos completos en ambos operands o legacy geométrico vacío; mezcla/malformed/NaN/Inf/out-of-range rechaza y preserva los cuatro buffers. Manifold conserva propiedades UV y mapea IDs temporales a identidad íntegra; los IDs temporales no se serializan. Weld conserva costuras/atributos y vacío válido limpia los cuatro buffers.

RED real contra kernel previo: build 0, CTest 8, 61 PASS/52 FAIL más crash del caso aislado de índice inválido. Primer GREEN reparó índices pero dejó cuatro fallos UV. El diagnóstico mostró keys/planos/bounds correctos y cruces de chart: faceID compartido permitió simplificar caras coplanares de la misma SurfaceKey a través de la costura. Fix mínimo: faceID temporal distinto por triángulo, varios pueden resolver la misma SurfaceKey. Tests/oráculos/tolerancias no cambiaron después del RED.

GREEN final: Release build 0 / **4,585 s**, CTest 0 / **0,103 s**, **dos tests / 117 checks PASS**. Comprueba U/S/I, vértices creados, costuras bajo identidad compartida, colisiones en los bits bajos de IDs, operaciones encadenadas/alias y negativos atómicos, además del cuarto/corredor/aperturas. `source-freeze-csg-attributes-final.json` y `csg-attributes-green2.*` registran cuatro inputs idénticos antes/después; `csg-attributes-green2.LastTest.log` conserva checks completos. Este camino puede retener más triángulos: no hay mejora de rendimiento afirmada. Adapters `.om`/halfedges, material Atom por cara, Undo/save/reopen/render/collider siguen **NOT VERIFIED**; no se afirma escena visible por un test geométrico.

### Preservación y outputs fallidos

La invocación inicial defectuosa de harness dejó 45 archivos / 556.496 bytes de CMake en cinco nombres propios de la raíz; se inventariaron y eliminaron con paths verificados, sin tocar source/Git ajeno. Captura de argv e inventario conservados. Luego se inventariaron tres outputs propios obsoletos: antiguo CPython work (275.774.746 bytes), inspección Qt parcial (4.431.533 bytes) y output Python vacío. Sólo el vacío se eliminó; **la revisión automática rechazó por política el borrado recursivo de los poblados, sin razón más específica**. Se conservan 280.206.279 bytes y el inventario/resultados; no se intentó otro mecanismo para eludirlo. Los outputs útiles/repo histórico se preservaron.

## Compilación original: primer bloqueo real (histórico)

Se ejecutó sobre el código original del resolver, sin default remoto y sin instalar paquetes:

```powershell
cmake -S F:/JericoEngine/JericoEngine -B F:/JericoEngine/JericoEngine/build/n0-validation/original-engine -G "Visual Studio 17 2022" -A x64 -DLY_PACKAGE_SERVER_URLS:STRING= -DLY_3RDPARTY_PATH:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/original-engine-deps -DFETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON
```

Entorno del proceso: `LY_PACKAGE_SERVER_URLS` retirado; `PIP_NO_INDEX=1`, `PIP_DISABLE_PIP_VERSION_CHECK=1`. **Exit 1, 11,839 s**. Evidencia: `logs/original-engine-configure.*`, incluido hash del resolver original `5FA803274B35E4C52430C219F6D124D0E0CFB63219685ADC2DE1A87722AC9D51`.

Primer error: **`zlib-1.2.11-rev5-windows` ausente**, al inicializar `LYPython.cmake` → `3rdPartyPackages.cmake` → `BuiltInPackages.cmake:29`. Después aparece Freetype ausente y errores derivados de alias/FindPackage. Python/venv/pip todavía no se alcanzaron. No se lanzó una build del engine tras configure fallido.

Después del patch se repitió una sola vez el mismo configure protegido, con inputs congelados: **exit 1 / 0,160 s**, primer error preciso `JE package unavailable offline: zlib-1.2.11-rev5-windows`, SHA/cache/unpack y roots locales vacíos; detiene la cascada. Logs `logs/local-engine-configure.*`. El mensaje heredado «Downloading package into» aparece antes del fatal, sin efectuar una descarga. Este configure reusa detección de compiler y no es una comparación de rendimiento con el primero.

| Par obligatorio | Estado real |
|---|---|
| Windows DX12 | BLOCKED antes de generar targets; ningún Editor/launcher ni frame |
| Windows Vulkan | BLOCKED antes de generar targets; enumeración de driver sólo inventario |
| Linux Vulkan | NOT TESTED: plataforma Linux no disponible en esta ejecución Windows |

## Paquetes ausentes y obtención permitida

El catálogo del primer inventario registra nombres/targets/SHA originales, **no el estado posterior a la ampliación ni grafo efectivo/SBOM**: 29 asociaciones sin clasificación final, dos AWS EXCLUDED, Python privado requerido, lock de tools, herramienta host y Manifold fuente local. En ese inventario inicial no había paquetes en roots controlados. Estado actual de los cinco paquetes aprovisionados arriba; no se descargarán los AWS excluidos.

| Paquete | SHA256 de artefacto esperado | Situación/fuente permitida |
|---|---|---|
| zlib-1.2.11-rev5-windows | `8847112429744eb11d92c44026fc5fc53caa4a06709382b5f13978f3c26c4cbd` | Primer bloqueo. Receta pública O3DE rev5 y fuente `madler/zlib`, tag v1.2.11, commit `cacf7f1d4e3d44d871b605da3b647f07d718623f` |
| freetype-2.11.1-rev1-windows | `861d059a5542cb8f58a5157f411eee2e78f69ac72e45117227ebe400efe49f61` | Bloqueo siguiente. Receta pública O3DE; fuente oficial GitLab freetype, tag VER-2-11-1 |
| python-3.10.13-rev1-windows | `a6f1fd50552c8780852b75186a97469f77d55d06a01de73e5ca601e4a12be232` | Ausente; receta pública O3DE. Python local 3.14.6 no sustituye el pin 3.10.13 |
| qt-6.11.2-rev1-windows | `f09edc180df0ded92827279c475167be13cbb517de182a5b10ec1337e68dd834` | No se encontró receta coincidente en el snapshot público revisado; la receta disponible 6.10.2-rev5 no autoriza downgrade |
| pyside6-6.11.2-py3.10-rev1-windows | `cd0272eb5ce46a3437132344a4a6b7022e3aee152da888963cdd9d025cbdf506` | Ausente; necesita fuente/receta/artefacto coincidentes auditados, sin sustitución de ABI |

Fuentes de recetas: [o3de/3p-package-source](https://github.com/o3de/3p-package-source/tree/70f0b7b5257bf7cf935bf1cfd270e1b53ccead18), especialmente `package_build_list_windows.json`, `package-system/zlib/build_config.json`, `package-system/freetype/build_config.json`, `package-system/python/` y `package-system/qt/build_config.json`. Los scripts de packaging [o3de/3p-package-scripts](https://github.com/o3de/3p-package-scripts) no se ejecutaron: sus descargas, dependencias y operaciones de limpieza deben auditarse antes. [Python 3.10.13 oficial](https://www.python.org/downloads/release/python-31013/) es source-only; no se inventa un instalador Windows de esa versión.

Para **cada paquete del CSV** hay dos rutas de trabajo permitidas: aprovisionar manualmente el artefacto exacto ya auditado/licenciado desde un almacén local controlado, con `.tar.xz`, `.tar.xz.SHA256SUMS`, hashes de contenido y metadata; o reconstruir desde fuente/receta auditadas y revisar el nuevo artefacto/hash/FindPackage antes de integrarlo. Reconstruir no implica reproducir el SHA del binario heredado. Las recetas/fuentes de las demás asociaciones todavía no están auditadas y su obtención queda BLOCKED hasta esa revisión. Se empieza por zlib/Freetype/Python y el origen de Qt/PySide del fork, sin descargar todo el catálogo.

El resolver acepta roots explícitos como `file:///F:/JericoEngine/Dependencies/packages`; caché de archivo y unpack se configuran mediante `LY_PACKAGE_DOWNLOAD_CACHE_LOCATION` y `LY_PACKAGE_UNPACK_LOCATION`, siempre paths locales absolutos. Nunca se envían credenciales/PAT al endpoint LFS heredado.

## Implementación del resolver local

`cmake/3rdPartyPackages.cmake` conserva la asociación/verificación/extracción de paquetes existente, pero elimina el default CloudFront, la inclusión/dispatch S3 y el fallback Git de su wrapper FetchContent. Valida tanto cache como environment antes de I/O; admite sólo `file:///` sin authority/UNC, o source override local explícito. Un paquete ausente da nombre, SHA, cache/unpack/origins y acción de aprovisionamiento. Un caché de archivo debe coincidir con el hash fijado.

`.lfsconfig` retira la URL CloudFront y establece `fetchexclude=*`, preservando punteros/OIDs. No se ejecutaron `git lfs pull`/fetch ni materialización. El primer intento de smudge tuvo captura de stdout defectuosa y **no se acredita igualdad de salida para ese intento** (`lfs-smudge.json`). El padre repitió el check con captura raw única, URL temporal forzada a path local inexistente y sin `GIT_LFS_SKIP_SMUDGE`: **exit 0 / 0,246 s, 131 bytes entrada/salida, SHA256 idéntico** `468099A7EBA9FA49F02B08052EA3513E632FBFB716B55962600A324FD3772306`. Los hashes/timestamps de cuatro hooks, `.git/config` y `.lfsconfig` antes/después son iguales. Ver [lfs-exclusion.json](lfs-exclusion.json) y su trace. Acredita la exclusión de un puntero real; no cierre offline de todo engine.

Pruebas reales del resolver con fixtures nuevos en rutas con espacios; los intentos remotos se interceptan antes de libcurl/AWS CLI, nunca se prueba bloqueando con una descarga real:

- RED contra resolver original: `package-red-project`, exit 1 / 18,641 s. Demuestra default/fallback/env remotos y falta de error específico; local/cache/source override funcionan.
- Primer GREEN válido: `package-green-actual`, exit 1 / 17,250 s; 19 PASS, dos fallos reales del archivo FetchContent local en Windows. CMake ExternalProject convertía `file:///F:/...` en `/F:/...`.
- Corrección mínima: dar a ExternalProject el path local absoluto del archivo ya validado; se mantiene `file:///` para transferencias locales. `package-green-fixed`, exit 0 / 18,429 s, **21/21 PASS**, hashes antes/después iguales, ningún marcador de I/O remoto.
- La revisión del padre encontró que argumentos ExternalProject no reconocidos podían eludir la política: RED suplementario `package-options-red`, exit 1 / 36,463 s, **22/24 PASS**; PATCH_COMMAND ejecutó sólo un marcador local y DOWNLOAD_DIR fue aceptado indebidamente. Fix: allowlist de NO_ADD_SUBDIR/EXCLUDE_FROM_ALL/SYSTEM/SOURCE_SUBDIR, rechazo de argumentos no reconocidos/missing values antes de FetchContent/I/O; la ruta explícita PATCH_FILES existente se conserva. GREEN final `package-options-green`: **exit 0 / 27,638 s, 24/24 PASS**, sin marcadores de I/O remoto ni hook. Hash resolver `B5893A9216005EC9E186C09134996B586ADAECDC9CDC5A185D9553F114883AAC`; runner `72A63A7DA79FB1CC2D8CBAEB3622899CADE2F80241E77314307A474B58ECF4DD`; case `B77B0314103DA7F9560F5C0361EEC503817FB71B509BE17DDEDC61861554EECD`. Inputs antes/después iguales.

Esta prueba acredita el resolver y su wrapper, **no** todos los CMake upstream, pip, licencias, tránsito de red ni build offline completa. Los flags del configure protegido siguen siendo necesarios hasta cerrar esas rutas.

Límite concreto PATCH_FILES: sin callers actuales; script `apply_patch.py` leído y sin downloader, pero sus paths conservan validación heredada y requieren auditoría/validación local antes de activar esa ruta. SOURCE_SUBDIR/source override también deben pertenecer a fuentes upstream auditadas: aprovisionamiento explícito no certifica transitivas/licencia.

Reproducción de policy: `cmake -DJE_TEST_OUTPUT=F:/JericoEngine/JericoEngine/build/n0-validation/package-review -P F:/JericoEngine/JericoEngine/cmake/tests/JEPackageOfflineTests.cmake`. Output siempre absoluto/dedicado; cada run crea fixture nuevo, sin borrar paquetes/cache del usuario.

## Núcleo BSP/CSG productivo, sin segundo solver

Se adapta el núcleo existente `Gems/WhiteBox/Code/Source/Core/WhiteBoxCsgCore.{h,cpp}` a **`JE::BSP::Csg`**, sin alias a `WhiteBox::Csg`/AZ y sin dependencia AzCore/AZStd. Los adapters `WhiteBoxCsg.{h,cpp}` usan el nuevo owner real. PODs/enum/funciones libres: no hay UUID/reflection/serializer propio en el núcleo; no se cambian type IDs de componentes/assets White Box.

El mismo código productivo, enlazado a Manifold, se compila en `JE_BspCsgTests`, no una copia del algoritmo. Se corrige:

- A-A/disjoint intersection son resultados vacíos válidos; publican ambos buffers vacíos y devuelven éxito.
- Buffers con vertex/triangle incompletos y enum desconocido se rechazan.
- Fallo de input, operación o cleanup conserva la salida anterior; la salida candidata se publica sólo al éxito. Tolerancia de weld `1e-6` permanece.

Provenance Manifold: v3.5.1, commit `cc8a7f66d7d5a560da94346258c5b546af27811e`, Apache-2.0. Adquisición del primer lote: [archivo codeload GitHub directo](https://codeload.github.com/elalish/manifold/tar.gz/refs/tags/v3.5.1), sin redirects, **30.976.085 bytes**, SHA256 `1b42f28d7c1c6d07df7244ca22cb7d82de980899001969c9985c2662a77eb43c`. Entries tar revisadas contra paths absolutos/`..` antes de extraer. Archivo/source en `F:\JericoEngine\Dependencies`, fuera de Git. No se copiaron fuentes Cry ni avisos/licencias borrados.

El fixture 3D usa source local explícito, downloads/TBB/cross-section/Python/fuzz/Tracy desactivados únicamente para esta compilación focal. No elimina funcionalidades del engine. Manifold conserva warnings upstream; core/tests JE `/W4 /WX`.

RED: antes de cambiar comportamiento se quitaron sólo imports/logging AZ para compilar el núcleo (no se llama pristine-original completo). Primer build falló por un warning de header Manifold; se corrigió el scope SYSTEM de sus includes, sin bajar warnings propios. El CTest RED falló por vacíos/buffers incompletos; el RED suplementario añadió enum desconocido y fallo de cleanup submicrométrico. Logs y hashes conservados.

GREEN del primer lote: configure exit 0 / 0,453 s, build Debug exit 0 / 3,310 s, CTest exit 0 / 0,121 s: **1/1 CTest, 33 checks internos PASS**. Ejecutable SHA256 `81E6AD710EB36E3DC4592C71A0BF2B4843EE245816DF3A692D4167DAFE732EDF`. Inputs congelados antes/después, ver `logs/csg-green-fixed.inputs-*.sha256`. La ampliación actual añade atributos y 117 checks; no confundir estos exes/configs históricos.

El fixture construye con el kernel real habitación 10×8×4 m hueca, corredor, unión, puerta y ventana mediante CSG. Verifica volúmenes authored 70,625 → corredor 24,6875 → unión 92,3125 → puerta 91,375 → ventana **90,90625 m³**, aristas cerradas/orientadas, bounds `[-5,5]×[-10,4]×[0,4]`, aire en habitación/corredor/puerta/ventana y pared restante sólida. No es una captura del editor.

Reproducción focal:

```powershell
cmake -S F:/JericoEngine/JericoEngine/Gems/WhiteBox/Code/Tests/CSG -B F:/JericoEngine/JericoEngine/build/n0-validation/csg -G "Visual Studio 17 2022" -A x64 -DJE_MANIFOLD_SOURCE_DIR=F:/JericoEngine/Dependencies/manifold-3.5.1 -DFETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON
cmake --build F:/JericoEngine/JericoEngine/build/n0-validation/csg --config Debug --target JE_BspCsgTests
ctest --test-dir F:/JericoEngine/JericoEngine/build/n0-validation/csg -C Debug --output-on-failure
```

## Consumidores y deuda de integración

Ruta existente: `EditorWhiteBoxComponent::ApplyBoolean` → `Api::ApplyMeshBoolean` → serialización/asset Undo `.om` → `RebuildWhiteBox` → render/physics → `BuildGameEntity`. El adapter rechaza todavía aplicar un resultado vacío y conserva el mesh: `RebuildRenderMesh` no sustituye render cuando no hay caras; `EditorWhiteBoxColliderComponent::CreatePhysics` retorna antes de destruir collider con cero caras. TODO localizado con condición de retirada: teardown vacío, Undo/Save/reopen y runtime probados con consumidores compilados. No se activa un éxito de editor que deje render/colisión obsoletos.

`RebuildFromTriangleMesh` recalcula UV planar; no conserva UV/face material/region lineage. WhiteBoxRenderData/AtomRenderMesh tienen un material/slot. Sólo cambió un diagnóstico del Editor para describir correctamente que el resultado no se aplicó. Adapter/editor no se compilaron debido al bloqueo N0; su integración es **INTEGRATED NOT VERIFIED**, mientras el núcleo es VERIFIED con alcance focal.

Absorción pendiente: WhiteBox aún es Gem y conserva buses/componentes/asset UUIDs/registro/recursos/builders existentes; trasladarlos al owner engine sin cambiar identidades/Undo/cook requiere el consumidor real. Atom sigue único renderer/material backend. `engine.json` aún contiene multiplayer y CMake AzFramework→AzNetworking; la retirada completa AWS/multiplayer **no está hecha**. Se identificó uso local y creación de AzNetworkingModule/ScriptRemoteDebugging: desactivar un Gem no resolvería esas dependencias. No se borran consumidores por nombre.

Lectura de la ruta visible en `editor-route-source-file-audit.json`: sin LY_PROJECTS root declara Editor/AssetProcessor/AssetProcessorBatch y launcher O3DE genérico; éste omite procesamiento de assets y no acredita escena. `LY_PROJECTS=AutomatedTesting` añade launcher/assets específicos y WhiteBox/Atom/EditorPythonBindings/PhysX5/QtForPython, pero también multiplayer: composición debe corregirse antes de utilizarlo como producto permitido. Base.prefab real y los tests WhiteBox existentes usan APIs del Editor/CSG. Los tres TIF Atom referenciados por default_grid y tres TIF UV de WhiteBox aún son punteros LFS; materialtype/material/icon son reales. Backend `--rhi` puede ignorarse si no está registrado: verificar log/backend/frame real. Estas son rutas auditadas, ningún ejecutable existe ni se vio la escena.

## Próximo desbloqueo y aceptación

1. Aprovisionar Lua 5.4.4 requerido con origen/receta/ABI/licencia auditados; después la siguiente dependencia realmente requerida. Corregir metadatos Git de Python en la siguiente reconstrucción y revalidar artefacto. Repetir configure/build sólo al cambiar inputs relevantes; retener errores específicos.
2. Obtener Editor/Asset Processor/launcher original; probar Windows DX12 y Vulkan separadamente, escena y mediciones reales. Clasificar grafo/link/deploy y separar IPC Tools de transporte excluido antes de retirar código.
3. N1 en editor real: misma habitación/corredor/puerta/ventana, material por cara/UV/lineage, grid/snap, selección/Inspector, Undo/Redo, Save/reopen/cook/runtime; después Procedural Studio/PWAS/EPLS y geología según roadmap, con locks/overrides y budgets. Ningún requisito de producto se cierra por estos checks focales.
