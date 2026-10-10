# N0 — plan de auditoría y baseline funcional

Para Codex: ejecutar tareas en orden mediante el flujo de planes disponible en su entorno, respetando AGENTS por encima de defaults de skills. No crear branches/worktrees ni subagentes fuera de los roles permitidos.

**Goal:** demostrar el O3DE base legalmente trazable y funcional sobre Vulkan antes de modificar sus sistemas.
**Arquitectura:** conservar source/build/registro de O3DE. Una rama, un owner de build, dos plataformas con estados independientes. Registrar evidencia sin inventar código producto para justificar la etapa.
**Stack:** C++/CMake/Qt6/Atom/Vulkan del snapshot; versiones reales de paquetes se verifican, no se actualizan a ciegas.
**Spec:** GOAL, arquitectura nativa y expediente legal; ROADMAP N0.

## Restricciones y atención especial

Sin transferir Cry; sin borrar árboles; sin RT/DX12/geología/flotabilidad; sin tokens enviados a LFS ajeno; sin afirmar runtime por lectura. No instalar FBX SDK/ATL/MFC nuevos. No usar comparación parcial GitHub como diff exhaustivo. El README upstream conserva instrucciones VS2019; el CMake real contiene referencias más recientes. Elegir generador/toolchain instalado y compatible, no exigir instalar VS2026 por su nombre.

Casos a vigilar: checkout con cambios ajenos; LFS incompleto; una dependencia en catálogo pero no enlazada; scene que luce bien pero usa RT o assets remotos; runtime que solo funciona con Asset Processor/checkout disponible.

## Tarea 1 — fijar identidad y preservar trabajo

Archivos de evidencia a crear al ejecutar: `docs/work/evidence/N0/BASELINE.md` y `COMMANDS.md`.

- [ ] Inspeccionar remotos, rama, status/HEAD; seguir CODEX_START para development sin reset/stash. Registrar cambios previos y no tocarlos.
- [ ] Fetch y pull ff-only seguros. Confirmar que el SHA O3DE base es ancestro y verificar sus objetos `git cat-file -t`/`git show`/`git rev-parse <sha>^{tree}`. No cambiar checkout al pasado: el paquete documental permanece en HEAD.
- [ ] Comparar metadata del origen O3DE y conservar LICENSE/SPDX/NOTICE. No importar historia JericoEngine2.
- [ ] Comprobar default de GitHub y ajustarlo a development con CLI autorizado cuando sea posible; verificar salida. Falta de permiso se registra sin bloquear trabajo con branch explícito.

Salida: identidad exacta y lista de cambios preservados. Resultado negativo: repo equivocado/cambios que impiden switch → stop de la operación peligrosa, no descarte.

## Tarea 2 — dependencias, LFS y toolchain

Evidencia: `DEPENDENCIES.md`, `GEM_GRAPH.md`, `PLATFORM_MATRIX.md` bajo el mismo directorio N0.

- [ ] Leer `.lfsconfig`, `git lfs env`, hooks/config efectivos, `.gitattributes` y package server config. Separar lectura pública histórica de subida de binarios privados. No enviar PAT a CloudFront por seguir un comentario. Registrar hosts efectivos y plan de mirror controlado.
- [ ] Inventariar `engine.json`, manifests Gem, CMake targets/aliases/services y dependencias de Editor/runtime/builders. Recorrer transitivas; `dependencies:[]` no prueba ausencia de 3P/CMake coupling.
- [ ] Confirmar CMake: mínimo source `3.25`, recomendación del snapshot `4.2.3`; solo VS18 exige la condición 4.2 indicada. Registrar generador/compiler instalados, Python, Qt/PySide, Vulkan loader/driver/layers y GPU disponibles. No concluir toolchain real desde README antiguo.
- [ ] Comprobar los paquetes Windows declarados Qt/PySide6 6.11.2, DXC y otros frente a los artefactos efectivos. Registrar hash/licencia/uso. No fusionar rama qt-6.12.0 por ser más nueva.
- [ ] Clasificar AWSNativeSDK/AwsIot/telemetry/cloud, servicios, build hosting y enlace real. Auditar OpenSSL efectivo y riesgo de soporte antes de Shipping; no aplicar update de ABI sin tests.
- [ ] Resolver materialización de assets/LFS estrictamente necesarios desde origen autorizado y verificar integridad, incluyendo `git lfs fsck` sobre lo obtenido. Pointer no descargado no es asset válido.
- [ ] Revisar soporte/interfaces OpenXR y multiview en el árbol/dependencias: qué existe, falta o requiere módulo externo licenciado. Resultado de auditoría, no instalar otra plataforma ni afirmar VR funcional.

Salida: dependencias directas/transitivas iniciales y bloqueos exactos. Inventario no equivale a aprobación jurídica completa.

## Tarea 3 — compilar el baseline sin reescritura

Archivos a leer: raíz CMakeLists/CMakePresets; `cmake/Platform` de la plataforma; `scripts` de bootstrap; `AutomatedTesting` o proyecto fixture admitido; los targets del build real. Outputs fuera de source y excluidos de commit salvo evidencias pequeñas.

- [ ] Elegir un solo directorio de build por plataforma/configuración y un solo owner; no builds paralelos sobre inputs mutables. Definir budgets de workers/memoria para no bloquear el escritorio.
- [ ] Descubrir comandos actuales de registro/creación mediante help de scripts presentes. El README documenta `scripts\o3de.bat register --this-engine` y `create-project --project-path`; verificar que el script de este pin los admite antes de ejecutarlos.
- [ ] Configurar usando CMake con source/build explícitos y caché 3P autorizada. Conservar comando entero y salida. Generador según toolchain comprobado, no ejemplo fijo VS2019 copiado.
- [ ] Inspeccionar targets generados y construir Editor, Asset Processor y GameLauncher del proyecto real. El README muestra el patrón `<Project>.GameLauncher Editor --config profile`; no adivinar nombre de proyecto/ejecutable.
- [ ] Si falla, reproducir y localizar primer error útil; buscar error book y código actual. Un fix mínimo admitido se prueba y registra separado del baseline sin modificar interfaces/familias ajenas.
- [ ] Registrar configure/build exit codes, logs y binarios/hash. No marcar editor/render PASS todavía.

Salida: ejecutables reales o blocker con target/log/comando exactos. No crear stubs de dependencias para lograr un build engañoso.

## Tarea 4 — escena Vulkan, editor y datos cocinados

Evidencia: `EDITOR_RUNTIME.md`, capturas/licencias del fixture y comandos de lanzamiento bajo N0. Usar captura nativa real, no mockups.

- [ ] Crear/usar fixture sintético o contenido licenciado: mesh PBR, cámara, luz, sombras y material. Identificar renderer seleccionado en el arranque y device/driver efectivos.
- [ ] Forzar selección Vulkan mediante configuración/argumentos realmente admitidos por el pin. Verificar que no se activan RT/DiffuseProbeGrid realtime ni fallback silencioso DX12. No borrar sus fuentes para desactivarlos.
- [ ] Abrir Editor, seleccionar/mover/editar, guardar, cerrar y reabrir. Esperar finalización real del Asset Processor y registrar errores/cancel; no medir una escena con shaders pendientes como steady state.
- [ ] Cocinar/bundlear con herramientas existentes; ejecutar GameLauncher con productos admitidos. Retirar acceso a source/Asset Processor de manera reversible en entorno de prueba sin borrar originales; comprobar dependencias reales.
- [ ] Si la ruta baseline solo prueba runtime cooked pero no source-free, registrar exactamente la dependencia restante. N0 parcial puede permitir trabajo focal, no se marca cierre total del gate.
- [ ] Cerrar, relanzar y comprobar ausencia de callbacks/recursos huérfanos de la prueba. Registrar crash/errors, no esconderlos cerrando diálogo.

Salida: Editor/Runtime reales, productos/escena/renderer y límites. Aceptar baseline O3DE no congela su formato como Jerico final.

## Tarea 5 — coste, diagnóstico y controles

Evidencia: `PERFORMANCE_BASELINE.md`, métricas originales y `VERIFICATION.md` bajo N0.

- [ ] Medir cold start, import/cook, respuesta del editor y steady CPU/GPU; resolución/cámara/settings fijos, GPU/driver/build/logs. Registrar p50/p95/p99 cuando haya muestras suficientes y cantidad de muestras.
- [ ] Registrar RAM pico/residente y VRAM observada, diferenciar cache/payload/transient copies. No usar solo el tamaño del archivo como budget.
- [ ] Usar profiler/Atom/RenderDoc o herramienta disponible con permisos actuales. Documentar counters no accesibles, sin cambiar políticas globales de driver.
- [ ] Probar resize/minimize/focus/unload/reload y asset inválido acotado; controles ON/OFF/ON solo cuando verifiquen una causa/coste real. No dumping de buffers por frame en modo normal.
- [ ] Repetir en otra plataforma cuando exista entorno. Marcar lo no ejecutado, sin transcribir medidas de Cry ni asumir equivalencia.

Salida: baseline medido, no una promesa genérica de ser más rápido.

## Tarea 6 — cierre y siguiente integración

- [ ] Completar evidence table: step/platform/inputs/command/result/hash/limits. Revisar checklist de legal/graph/compilation/editor/runtime/performance.
- [ ] Actualizar HANDOFF y ACTIVE_SLICE con estado verdadero. Si hay baseline funcional, preparar N1 con owner/archivo/consumidor exactos encontrados; no scaffold de once subsistemas.
- [ ] Revisar diff, `git diff --check`, staging explícito, commit honesto y sync/push development por el padre. Fetch posterior y SHA remoto igual al publicado.
- [ ] Continuar siguiente resultado no bloqueado dentro de GOAL; no pedir al propietario que vuelva a decidir sistemas ya fijados.

## Antes de implementar cualquier subsistema posterior

El plan de cada vertical concreta nombres/archivos/API tras inspección del source. Debe contener prueba de aceptación y negativa, productor/consumidor, licencia, owner, cambios de editor/cook/runtime y criterio de retirar transitorio. El roadmap no inventa interfaces de APIs que todavía no se leyeron. La implementación puede ser profunda, pero debe llegar al consumidor real en esa misma vertical.
