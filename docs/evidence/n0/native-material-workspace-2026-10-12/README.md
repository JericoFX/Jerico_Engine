# Material Workspace nativo — 2026-10-12

Estado: vertical Material Workspace verificado localmente; publicación focal pendiente. N0/GOAL permanecen abiertos.

MaterialDocument, su bus y sesión viven en JE::Editor, dentro de EditorMaterials. La acción Material Editor abre un panel del editor existente, comparte viewport y Undo/Redo global con las entidades. El panel conserva documentos e historial al cerrarse; cerrar la aplicación ofrece Save/Discard/Cancel. Se retiró la dependencia que obligaba a construir MaterialEditor.exe con Editor. El host standalone temporal comparte el documento nativo; MaterialCanvas todavía abre otro proceso. Este corte no acredita retirada completa de Gems ni ejecutables.

## Evidencia funcional

- Vulkan intento1: apertura sin proceso MaterialEditor, historial intercalado entidad/material, Save, Cancel y cierre/reapertura del panel. El selector de color abortó; ese intento no es un PASS completo.
- Color picker: pointer LFS embebido como PNG produjo imagen nula. Dos regresiones RED, recuperación del payload original207B con hash exacto desde GitHub permitido, regeneración puntual del qrc y GREEN. El comando qrc del paquete Qt no declara dependencias de imágenes; no se modificó el paquete. Procedencia en color-resource-provenance.json.
- Vulkan intento2: reapertura desde disco, selector funcionando, material rojo→verde guardado y visible en BSP tras AP, selección retenida y cierre Save. Captura original en vulkan-material-reload.png.
- DX12 intento1: mismo panel y flujo, verde→púrpura visible tras Save/AP, selección retenida y cierre Discard. Captura original en dx12-material-reload.png. Diferencia de brillo respecto a Vulkan pendiente de diagnóstico controlado; no se afirma equivalencia visual.
- Crear desde StandardPBR.materialtype: Cancel no crea archivo. Crear falló porque el selector devuelve alias y el consumidor exigía origen absoluto. materialtype-alias-red.xml reproduce el diálogo real. Corrección resuelve sólo el origen antes de validar/cargar/referenciar; destino absoluto y CREATE_NEW se conservan.

Las escenas originales y de prueba conservaron SHA256 FEE55849A032D29059D4369112047DC27E281D39631D0F0DF87D8BCF5EF517AA y 4C5E436524DD9EA5EB4FAE0B8BF61DF1024D5B7784D4C92EA242A28298C70040. Sólo los materiales de prueba propios se guardaron. No Reset UV ni cambios guardados en prefab.

## Validación y límites

La copia de JsonSerializerSettings por operación corrige el reporter retenido con referencias vencidas al cerrar Inspector. Regresión con llamadas repetidas/reentrantes y controles de Inspector. SettingsRegistry159/159 y EditorLib72/72 pasaron. Color GREEN Materiales24/24; alias añade una regresión. Recibos detallados locales en build/n0-validation/native-material-workspace-rev1; snapshots portables acompañan este documento.

Compilar y estas capturas no acreditan rendimiento, tamaño Shipping, Linux Vulkan, VR/OpenXR ni integración total. Sigue heredado el riesgo de cambio de schema/tipo durante Reopen y el avance del cursor Undo ante rechazo de una operación incompatible. No se añadieron runtime Python, renderer, cooker ni sistema de materiales paralelo. Los controles CMake ajenos preservados limitan la reproducibilidad desde un checkout limpio; no se declara clean-build PASS.
Cierre focal: alias GREEN build exit0/37.372s, Materiales25/25 y EditorLib72/72; 181inputs intactos. DX12 intento2 crea StandardPBR versión5, AP lo procesa y documento abre/reabre en panel, sin proceso externo; tipo original intacto. Materialtype-create-green.png y creation-ui-green.json conservan evidencia. El aviso de cambio externo tras creación propia queda como detalle UX pendiente. Total256 casos únicos incluyendo SettingsRegistry previo. El cambio posterior de .gitattributes sólo permite guardar las capturas originales en Git sin LFS; no afecta binarios.
