# Definition of Done — evidencia proporcionada al alcance

Documento ACTIVE junto a GOAL/AGENTS. No convertir documentación en runtime PASS ni exigir compilar el motor para un commit exclusivamente Markdown.

## Estados separados

PLANNED/CONTRACT ADOPTED: diseño; DESIGN ONLY: diseño sin permiso de implementar; IMPLEMENTED NOT INTEGRATED: código fuera de ruta; INTEGRATED NOT VERIFIED: consumidor real aún no probado; VERIFIED: evidencia del alcance concreto; BLOCKED/NOT TESTED: causa/paso concreto; DEFERRED/REJECTED: razón y autoridad.

La etiqueta general de un Gem no prueba el contrato Jerico; manifest de licencia no sustituye todas sus transitivas. No declarar toda la familia VERIFIED cuando solo se ejercitó una variante.

## Publicación de documentación

Lecturas reales del repo/HEAD, fuentes y límites; referencias internas resueltas; autoridad/branch/nombres consistentes; decisiones antiguas sustituidas explícitamente; requisitos presentes en matriz y owners/etapas; diff revisado, solo rutas permitidas, notices intactos. Commit con padre/tree correctos, ref publicada sin fuerza y lectura de confirmación. Pruebas de engine NOT RUN.

## Vertical de producto

1. Observable: un usuario consigue el resultado previsto en el editor y runtime canónicos, no en harness alternativo.
2. Owner: productores/consumidores/lifetimes claros, init/use/shutdown y ausencia de doble autoridad sobre ese dato.
3. Autoría: Inspector/comandos, preview/apply/cancel, Undo/Redo y guardado/reapertura cuando corresponda.
4. Persistencia/cook: identidad/revisiones/dependencies, producto válido, invalidación/hot reload y source-free según la fase; deuda temporal explícita.
5. Pruebas: unidad/contrato/regresión y negativas pertinentes. Reproducción RED si se corrige un fallo, GREEN sin rebajar tolerancias.
6. Visual/physics: evidencia real a resolución original y inputs fijados. Un assert o screenshot bonito no acredita comportamiento/performance.
7. Coste: CPU/GPU/memory/load/frame pacing medidos al nivel relevante; warmup separado, fixture representativo y reversión ON/OFF/ON cuando sea útil.
8. Robustez: inputs inválidos, cancel, destroy/unload/reload, resize/device/focus/session loss según consumidor. No extrapolar un test a sistemas no cubiertos.
9. Legal: procedencia/version/licencia de incorporaciones, notices y sin material de Cry aprobado por nombre.
10. Git: único publicador/branch, fetch/pull seguro, diff, commit, push y comprobación remota. Concurrency no se resuelve con force/reset.

## Evidencia reproducible

Guardar en `docs/work/evidence/Nx-<resultado>/` al crearse el vertical: commit/build/platform/driver/settings/scene/camera/content hashes, comandos/códigos de salida, logs, capturas originales o referencias con derechos, métricas y límites. Assets LFS privados requieren destino autorizado; no enviarlos al endpoint upstream sin análisis.

No crear contadores de millones de checks repetitivos para reemplazar coverage. No guardar dumps gigantes por defecto ni duplicar cada log en masters. Los resultados se resumen en HANDOFF; error book guarda causa y prevención.

## Plataformas, XR y cierre final

Windows Vulkan no demuestra Linux Vulkan. Mock/stereo desktop no demuestra OpenXR hardware. Una configuración editada no cambia modelo de agentes ni demuestra ejecución. Un build source-free de O3DE temprano no acredita `.epak`/ABI final Jerico.

GOAL final: matriz obligatoria cerrada, formatos/packaging reales, distribución reproducible/source-free, notices/licencias/SBOM revisados, independencia de servicios/build-hosting según contrato, UX y rendimiento admitidos, cero doble owner sobre sistemas migrados y ningún subsystem parcial oculto. Los diferidos explícitos siguen visibles; solo el propietario puede cambiar su alcance.
