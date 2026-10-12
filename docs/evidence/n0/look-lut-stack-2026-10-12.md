# Look Modification: acumulación de LUTs

La agregación reutilizaba el stack preparado de la vista y añadía de nuevo las LUTs en cada actualización. Una sola LUT acababa ocupando cuatro entradas y emitiendo el aviso de límite. Se elimina únicamente el subsetting transitorio LookModification antes de reagrupar la vista; se conservan los demás efectos, el límite de cuatro y sus diagnósticos.

## Verificación

- Windows Profile, checkout development, base a4667cf70fd5f81b8781b6d492c54a7b12ed969f.
- RED: cinco regresiones con PostProcessFeatureProcessor y View reales fallan antes del cambio: repetición, reemplazo, desactivación, retirada y cuatro capas con prioridad.
- GREEN: las cinco pasan; suite Atom del checkout de validación: 78/78, cero fallos/skips. Este total incluye trabajo local HDR separado de esta publicación.
- Evidencia local: `build/n0-validation/local-packages/look-lut-stack-regression-rev1/native-red-attempt1` y `native-green-attempt1/atom-only-suite`.
- Editor Vulkan: generación y activación de dos LUTs, Undo/Redo de activación Log2 por menú; log final sin aviso de exceso, timeout ni readback ya activo. Prefab de usuario conservado byteexacto FEE55849A032D29059D4369112047DC27E281D39631D0F0DF87D8BCF5EF517AA, sin Save. Evidencia: `build/n0-validation/hdr-shaper-editor-vulkan-attempt1/parent-ui-result.json` y capturas originales.

## Alcance

Esta publicación sólo corrige el stack por vista y añade sus cinco regresiones. La captura nativa, cancelación y metadata de shaper del editor pertenecen a integración local separada. No acredita Save/reopen, rango custom, múltiples capas HDR, equivalencia colorimétrica, rendimiento, DX12/VR ni cierre de N0/N1. La prueba visual usa el build local completo; no es una certificación de todas las funciones del commit aislado.
