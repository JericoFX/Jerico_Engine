# EditorModeFeedback — evidencia de integración nativa

Estado: integración focal Windows/Profile/Vulkan revisada y aceptada por el padre para publicación selectiva. No declara terminada la Gem global, N0 ni el GOAL. El [plan](../../../work/gems/EditorModeFeedback.md) conserva los requisitos y la revisión de upstream.

Destino: `JE::EditorRenderFeatures` STATIC, namespace `JE::Editor::Render::Features`. La capacidad conserva selección y efectos de foco de prefab dentro del editor, sin DLL adicional ni cambio del algoritmo de los shaders. El componente de autoría no se incorpora a la factory runtime.

## Resultados preservados

| Comprobación | Evidencia y alcance |
|---|---|
| Grafo y archive nativo | [Configure incremental](configure-result.json) y [compilación del owner](archive-build-result.json): exit 0, sin timeout ni guard. Estos resultados prueban configuración/compilación, no interacción. |
| Identidad de assets | [Comparación AP](asset-identity-review.json): 14 fuentes y 62 productos mantienen GUID, SubID, tipo y ruta; los 62 productos físicos existen. El watch cambia a la raíz nativa conservando la portable key. Se mantienen los avisos y dos errores de desconexión al cierre del cook; no se declara log limpio. |
| Consumidor instalado | [Salida original](sdk-loader.stdout.log), [resultado](sdk-loader-result.json), [postflight](sdk-postflight.json) y [revisión](sdk-review.json): dos ciclos, 739 comprobaciones, cero fallos; factory editorial 14 descriptores/9 requeridos y runtime 9/7. Las seis filas de proveedores corresponden a RenderFeatures.dll, no a una DLL EditorModeFeedback. El owner editorial es estático. No prueba activación GPU del consumidor. |
| Guardar y reabrir | [Captura original](reopen-whitebox-material-uv-original.jpg): prefab privado con WhiteBox, material metal_iron y escala UV 2,2 persistidos. |
| Play/Stop | [Log completo del editor](gui-r5-product.log) registra Entered game, una ejecución de `quit` durante Play y Exited game. [Captura de retorno](play-stop-restored-original.jpg) y [contorno posterior](post-stop-selection-outline-original.jpg) muestran la restauración de edición y selección. Se utilizó la consola existente, liberando antes el cursor mediante ImGui. |
| Foco de prefab | Capturas originales [antes](outside-focus-before-original.jpg), [Monochromatic](outside-focus-monochrome-original.jpg) y [restaurado](outside-focus-restored-original.jpg): geometría fuera del prefab enfocado oscurecida/desenfocada; al volver al nivel raíz recupera su presentación. Se restauró Normal. |
| Deselección y segunda preview | [Revisión R3](gui-r3-review.json), [deselección](deselection-original.jpg) y [segunda preview](second-preview-original.jpg): el contorno desaparece al deseleccionar y Material Workspace abre y cierra su preview en el mismo editor. El contador de errores de ese recibo corresponde al log capturado de R3, no garantiza ausencia global de errores ni sustituye el log completo R5. |
| Cierre y cohorte | [Resultado R5](gui-r5-result.json), [postflight](gui-r5-postflight.json) y [revisión visual](gui-r5-review.json): exit 0, 309,975 s, sin timeout ni guard de RAM, cero deriva de entradas congeladas y censo nativo vacío. El tiempo incluye interacción humana/automatizada; no es un benchmark. |

## Límites y procedencia

R5 reproduce el aviso Vulkan VUID09600 del mip 8 de LuminanceMinAvgMax en la pipeline de preview. Se conserva en el log completo; no está reparado ni se atribuye a esta absorción sin demostrarlo. Las capturas prueban las interacciones descritas, no calidad Beauty, rendimiento ni iluminación global.

La revisión SDK es histórica y enumera como pendientes gates GUI que R5 verificó después; no se reescribe ese recibo. Los resultados se obtuvieron en el árbol de trabajo mixto, no en un checkout limpio del futuro commit selectivo.

La revisión final W1/padre reconcilió 143 rutas de producto: 141 del mapa inicial más Code/Editor/CMakeLists.txt y Gems/AtomLyIntegration/gem.json corregidos tras el configure RED. Se comprobaron 45 operaciones C++ contra sus hashes y los hunks compartidos frente a HEAD. No se identificó una dependencia nueva del WIP ajeno. La integración incorpora explícitamente los siete guards de creators y cinco comprobaciones de templates heredados de la reparación UI33/35/36, conservados y regresados con la segunda preview. No se atribuye a este corte la autoría de esa reparación previa.

[provenance.json](provenance.json) registra las 22 copias byteexactas, tamaños, SHA-256 y fuentes locales. Las ocho capturas R5 y el log completo se contrastaron contra sus hashes registrados antes de copiar. No se usa el stdout truncado del runner como sustituto del log del editor.

Linux, VR/OpenXR, Shipping, SDK global sin Python y métricas globales siguen abiertos. No hay incremento del contador de Gems ni SHA de publicación de este corte todavía.
