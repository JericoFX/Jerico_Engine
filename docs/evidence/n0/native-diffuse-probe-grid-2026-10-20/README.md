# DiffuseProbeGrid — integración nativa

Estado: gates focales y revisión final de integración aceptados por el padre; publicación en preparación. Esta evidencia no declara completados N0, el GOAL ni otras plataformas. El [plan de la Gem](../../../work/gems/DiffuseProbeGrid.md) mantiene los requisitos y la trazabilidad completa.

La implementación runtime se integra en `JE::RenderFeatures`, bajo `JE::Render::Features`; la autoría se integra en `JE::EditorRenderFeatures`, bajo `JE::Editor::Render::Features`. Se conservan DDGI, ray tracing, Baked, RealTime, AutoSelect y los contratos serializados. No se añade otro renderer ni una DLL para esta capacidad.

## Evidencia conservada

| Comprobación | Resultado y límite |
|---|---|
| CPU | [Revisión](cpu-review.json): 82 casos Features y 587 RPI, 669/0. XML originales adjuntos. Son fixtures CPU, no prueba visual. |
| SDK instalado | [Salida R33](sdk-loader.stdout.log) y [resultado](sdk-loader-result.json): dos ciclos, 692 comprobaciones, cero fallos; seis proveedores resuelven a RenderFeatures.dll del consumidor. No prueba activación GPU ni un SDK global sin Python. |
| Hoja SDK posterior | [Revisión R39](sdk-leaf-review.json): sólo el shader Luminance authored se alineó con su fuente; no se repitió el SDK completo ni se afirma un recook instalado. |
| Contribución Baked | [Revisión R30](baked-contribution-review.json), [comparación](baked-floor-comparison.json) y tres PNG originales: cambio reversible 0→1→0 en una región del suelo. No certifica GI física, Beauty, rendimiento ni igualdad del cuadro completo. |
| Vulkan nativo | [Log R43](vulkan-native.cdb.log), [resultado](vulkan-native-result.json), [postflight](vulkan-native-postflight.json), [revisión del padre](vulkan-native-parent-review.json) y [revisión independiente](vulkan-native-independent-review.json): una grabación primaria por RealTime y AutoSelect, 288×180×1, llamada de rayos desde DPG, End y QueueSubmit correctos, y QueueWaitIdle existente posterior con resultado correcto en la misma cola. |

R43 terminó con exit 0 en 35,0937565 segundos, sin timeout, disparo del límite de RAM ni cambios en los inputs congelados. El proceso de prueba terminó. El tiempo incluye el depurador: **no es una medida de rendimiento del engine**.

Los monitores de reinicio de buffer/pool estuvieron armados durante cada muestra hasta el envío aceptado. Los estados finales fueron 63/63, sin avisos de generación inválida ni errores del evaluador en esa captura. No se extrapola a todos los frames.

El registro muestra dos entradas End idénticas para AutoSelect y un único retorno correcto; no se atribuye una causa no observada. La retirada del breakpoint 9 dio error 998 después de QueueWaitIdle correcto y del cierre; es un aviso del depurador, no un resultado Vulkan. Se conservan los avisos de formatos storage y DynamicDraw: no se afirma una ejecución global sin avisos.

## Procedencia

[provenance.json](provenance.json) registra fuente local, tamaño y SHA-256 de cada copia byteexacta. Las revisiones históricas conservan su fecha, cohorte y límites; los pendientes históricos se interpretan con el plan vigente. El freeze utilizado en R43 es histórico: una modificación posterior del plan exige actualizar el freeze antes de una nueva ejecución, sin invalidar retrospectivamente esa captura.

Las pruebas de Windows/Vulkan no acreditan Linux, VR/OpenXR, Shipping, calidad visual global ni presupuestos de RAM/VRAM/CPU/GPU. La publicación final debe preservar cambios ajenos y verificar el SHA remoto de `development`.

## Revisión de publicación

La [revisión independiente](publication-review.json) identifica los cambios propios y conserva las licencias. No encontró dependencia nueva de compilación o enlace respecto al WIP excluido. Los binarios y R43 se probaron en el árbol mixto, incluyendo cambios locales de Profiler y MaterialShaderParameterLayout: **no se afirma una compilación ni ejecución del árbol selectivo limpio**. Esos cambios ajenos no se incluyen en esta publicación. La aceptación es focal Windows/Profile/Vulkan; no certifica el cierre global de la Gem ni aumenta un contador de cierres globales.
