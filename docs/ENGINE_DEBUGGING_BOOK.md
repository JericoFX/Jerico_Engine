# Libro de errores y aprendizaje de JericoEngine

Estado inicial O3DE: todavía sin ejecución local acreditada por este paquete. Las entradas heredadas son **lecciones**, no diagnósticos del nuevo Atom. Fuente histórica fijada: JericoEngine2@304d9d2e13704e4967e82a8d5a13a5a506be01d4, `docs/ENGINE_DEBUGGING_BOOK.md` y `docs/work/ACTIVE_SLICE.md`.

## Método obligatorio

Buscar síntoma/subsistema antes de tocar código. Reproducir en escena/inputs reales, capturar productores y consumidores, encontrar el primer output defectuoso y distinguir causa probada de hipótesis. Formular control negativo/RED, corregir mínimo, GREEN sin relajar tolerancia, medir coste y restaurar estado. Visual semejante no prueba causa igual.

Cada entrada nueva contiene: fecha/commit, síntoma, reproducción, inputs/entorno, causa demostrada o hipótesis explícita, intentos rechazados y por qué, corrección, evidencia, límites y prevención. No llenar el libro de teoría para aparentar bugs resueltos.

## H-01 — compilación y contadores no acreditan integración/calidad

En la base anterior quedaron capabilities implementadas fuera del consumidor y etapas con numerosos checks pero coste/visual sin cerrar. Prevención: identificar qué ejecutable/ruta usa el cambio y el comportamiento visible esperado. La prueba necesita ese consumidor; un Float32 o codec aislado no bloquea unrelated work ni demuestra el producto.

## H-02 — medir la escena real, no generalizar un fixture pequeño

ACTIVE_SLICE antiguo registra coste sky ~24.8 ms en una escena fija a 1236x320, aun con validación numérica extensa. No se ha medido aquí. Prevención: diferenciar cold load, steady frame, tiempos GPU, CPU waits y presentación. No llamar optimización a instrumentación sin cambio de coste demostrado ni decir leak por una captura de memoria.

## H-03 — estado/bytes de un recurso no prueban ecuación correcta

La investigación histórica del agua separó máscara de cualquier fragmento oculto, superficie visible, refracción y geometría FFT. La máscara por sí sola no acreditó causa del defecto. Prevención: rastrear semántica real, bindings/constants/textures y primera diferencia de pass; no cambiar winding/barriers por hipótesis visual no reproducida.

## H-04 — cambio de base no hereda PASS ni errores

Los hashes/logs/Cry shaders no son pruebas de O3DE. Reusar escenarios y observaciones permitidos, con fuente y unidades actuales. No exigir igualdad byte a byte entre modelos físicos distintos ni atribuir problemas al backend por nombre.

## H-05 — decisiones históricas contradictorias

El AGENTS viejo acumulaba topologías y el GOAL incluía obligaciones Cry ahora sustituidas. Prevención: una autoridad actual, fechas y precedence explícitos; no ejecutar el primer párrafo encontrado ni copiar instrucciones de reset. Rama default de GitHub tampoco define la rama activa Jerico.

## H-06 — coste y dependencia oculta

Catalogar un Gem no muestra todas las dependencias CMake/3P. Catálogo AWS no prueba linkage, pero LFS CloudFront es una dependencia de distribución real mientras se use. Qt6 no se acredita con referencia legal de Qt5. Prevención: separar source/manifest/build/link/deploy/runtime/licencia y verificar el pin efectivo.

## H-07 — estado transitorio y clocks

Snapshots, cache generations y times deben representar al productor real. Un flag de caché no prueba que un recurso nunca fue escrito; una conversión float de ticks puede perder precisión. Prevención: contracts de init/reset/write/reproject/unload; inspect real immutable submissions; no reloj paralelo para esconder la discrepancia.

## O-01 — bootstrap y rama única (auditoría documental)

Observado: 23 ramas; default inicial optimize_scriptcanvas_1, 453 commits de development ausentes y un cambio propio. Decisión: development único; alternativas lectura selectiva. Esto resuelve el criterio de trabajo en documentos, no acredita cambio administrativo de default. Verificarlo al arrancar conforme CODEX_START.

## Límites de seguridad y recursos

Capturas y logs contienen solo lo necesario; no credenciales/datos privados innecesarios. Instrumentación apagada no añade readbacks normales. No modificar configuración global del driver/contadores/seguridad sin permiso. Mantener un owner de builds y congelar entradas. Error sin reproducción permanece abierto, incluso si una explicación parece convincente.
