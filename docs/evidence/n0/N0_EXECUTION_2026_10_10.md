# N0 — ejecución local y preparación del BSP

Fecha: 2026-10-10. **N0 IN PROGRESS; baseline original BLOCKED por dependencias locales ausentes. N1/editor/runtime NOT TESTED.** Este expediente distingue las pruebas ejecutadas del trabajo de producto pendiente.

`logs/copy-manifest.csv` registra archivos/orígenes/tamaños/hashes de las 75 copias de evidencia; `evidence-files.sha256` verifica 88 outputs/inventarios. `.gitattributes` local conserva sus bytes sin conversión EOL al versionarlos, reconoce CRLF en datos y mantiene el espaciado original de transcripts `.log` bajo control de hashes. Source/Markdown conservan el chequeo de whitespace; no se reformatean outputs para hacerlos parecer correctos. Los logs son evidencia de esta plataforma/configuración, no aceptación portable.

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

## Compilación original: primer bloqueo real

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

El catálogo registra todos los nombres, targets y SHA fijados por los manifests Windows/Python. **Es catálogo candidato, no grafo efectivo ni SBOM**: 29 asociaciones sin clasificación final, dos AWS EXCLUDED, Python privado requerido, lock de tools, herramienta host y Manifold fuente local. Los paquetes binarios del catálogo no se hallaron en los roots controlados inspeccionados. No se descargarán los AWS excluidos.

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

Provenance Manifold: v3.5.1, commit `cc8a7f66d7d5a560da94346258c5b546af27811e`, Apache-2.0. Única adquisición nueva: [archivo codeload GitHub directo](https://codeload.github.com/elalish/manifold/tar.gz/refs/tags/v3.5.1), sin redirects, **30.976.085 bytes**, SHA256 `1b42f28d7c1c6d07df7244ca22cb7d82de980899001969c9985c2662a77eb43c`. Entries tar revisadas contra paths absolutos/`..` antes de extraer. Archivo/source en `F:\JericoEngine\Dependencies`, fuera de Git. No se copiaron fuentes Cry ni avisos/licencias borrados.

El fixture 3D usa source local explícito, downloads/TBB/cross-section/Python/fuzz/Tracy desactivados únicamente para esta compilación focal. No elimina funcionalidades del engine. Manifold conserva warnings upstream; core/tests JE `/W4 /WX`.

RED: antes de cambiar comportamiento se quitaron sólo imports/logging AZ para compilar el núcleo (no se llama pristine-original completo). Primer build falló por un warning de header Manifold; se corrigió el scope SYSTEM de sus includes, sin bajar warnings propios. El CTest RED falló por vacíos/buffers incompletos; el RED suplementario añadió enum desconocido y fallo de cleanup submicrométrico. Logs y hashes conservados.

GREEN: configure exit 0 / 0,453 s, build Debug exit 0 / 3,310 s, CTest exit 0 / 0,121 s: **1/1 CTest, 33 checks internos PASS**. Ejecutable SHA256 `81E6AD710EB36E3DC4592C71A0BF2B4843EE245816DF3A692D4167DAFE732EDF`. Inputs congelados antes/después, ver `logs/csg-green-fixed.inputs-*.sha256` y log completo de checks.

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

## Próximo desbloqueo y aceptación

1. Aprovisionar zlib/Freetype/Python exactos permitidos y resolver receta/origen Qt/PySide; después auditar las dependencias efectivas que realmente pide configure. Repetir configure/build sólo cuando cambien inputs relevantes; retener errores específicos.
2. Obtener Editor/Asset Processor/launcher original; probar Windows DX12 y Vulkan separadamente, escena y mediciones reales. Clasificar grafo/link/deploy y separar IPC Tools de transporte excluido antes de retirar código.
3. N1 en editor real: misma habitación/corredor/puerta/ventana, material por cara/UV/lineage, grid/snap, selección/Inspector, Undo/Redo, Save/reopen/cook/runtime; después Procedural Studio/PWAS/EPLS y geología según roadmap, con locks/overrides y budgets. Ningún requisito de producto se cierra por estos checks focales.
