# Material Canvas nativo: replay real y liberación de memoria

Estado: **validación nativa aprobada; aceptación visual del corte pendiente**. Checkout único `F:\JericoEngine\JericoEngine`, rama `development`, HEAD `b8ba2effe175d4c37e922aa80c3014a3d3b783d7`. Fuentes nuevas sin commit/push. El propietario pidió continuar con código sin controlar ventanas; no se utilizó UI ni se modificaron sus escenas.

La fixture usa GraphDocument, GraphCanvas.Editor y GraphModel.Editor reales, con QApplication offscreen y el Undo global de JE. Comprueba nodos, slots, conexiones, referencias lógicas, posiciones, Undo/Redo intercalado con una entidad, Save/reopen y rechazo atómico de snapshots inválidos. Sólo se sustituye el transporte de consulta de scan folders del Asset Processor; no hay renderer, compilación de shaders ni proceso AP en estas pruebas.

La primera ejecución (`native-attempt8`) encontró escrituras de preparación dentro de `@products@` y una fuga de SystemAllocator de 81.808 B/458 registros. La fixture ahora separa fuentes y productos privados. El ciclo nodo→slot→caché del nodo quedó demostrado por una regresión independiente: el nodo seguía vivo y el slot devolvía su propietario después de liberar nodo y grafo. El control de eliminación explícita pasó. Se cambió únicamente el caché privado a weak_ptr, conservando API, mutex, invalidación y serialización. También se liberan los tres iconos propietarios de StyleManager y sus pixmaps; sus paletas aparecían en el snapshot de fugas.

## Evidencia RED/GREEN

| Ejecución | Resultado real |
| --- | --- |
| `slot-parent-cache-red-native-attempt1` | Build 0; CTest 8; 13 casos, único fallo `ParentNodeCacheDoesNotKeepTheOwnerAlive`; control `RemovingTheParentNodeInvalidatesTheCachedLookup` aprobado. |
| `native-attempt9` | Build focal 0/240.105 s; build hosts 0/98.181 s; las cuatro suites CTest terminaron con exit 0. |
| XML GREEN | GraphModel 13 + EditorMaterials 42 + AtomToolsFramework 10 + EditorLib 72 = **137 casos únicos**, cero fallos/errores/disabled/skips. |
| Inputs | **1.055/1.055** intactos en postflight y rehash final del padre; freeze SHA `4768B0FA477C422326A05E72F1801D2DA09503E36F4E8AD7C6270F9BD046FC10`. |

Los dos casos GraphDocument reales terminaron sin fallos, incluidas las comprobaciones del allocator. La AV al iniciar la segunda fixture ya no se reprodujo en la suite completa; no se capturó su pila original y no se le asigna una causa exacta. Esta evidencia no mide RAM/VRAM del producto ni acredita render o experiencia de usuario.

Se reutilizó la configuración aprobada de `native-attempt7`: CMake/filelists permanecieron iguales; cache offline y destinos auditados se verificaron. No hubo descargas, instalación de dependencias ni reconfiguración innecesaria. Los hosts de compatibilidad se compilaron como consumidores del código compartido; no se abrieron aplicaciones externas.

Los XML y recibos están junto a este archivo. Los logs completos, preimágenes, diffs y resultados de fase permanecen en `build/n0-validation/native-materialcanvas-workspace-rev1/{native-attempt8,native-attempt9,slot-parent-cache-red-native-attempt1,parent}`. Recibo final del padre SHA `CC1CCA62671B898D9ECB0436DC421B6070927CB25E7C12B0A5222C10A21922FA`.

## Pendiente de aceptación

Conexiones e Inspector por UI; Undo mixto entidad/material; Save/reopen, cierre de panel y decisiones al cerrar la aplicación; compile→AP→material generado aplicado al mismo BSP; equivalencia visual DX12/Vulkan. N0, métricas de distribución, Linux Vulkan y VR siguen abiertos. No se declara eliminada toda la arquitectura Gem ni se publica el corte antes de esos gates.
