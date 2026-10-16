# Materiales: reutilización de presets y persistencia, cortes89–91

El padre aceptó el corte focal: **Generar material reutiliza settings nativos compatibles sin reescribirlos; los settings incompatibles o malformados se conservan y requieren revisión.** La proyección de un override de plataforma vacío también se normaliza. La evidencia combina seis casos nativos y dos sesiones GUI independientes sobre el renderer Vulkan existente.

## Resultado verificado

- **Native91:** seis identidades completas, cero fallos, errores, skips o disabled. Build focal, inventario, test y hosts Editor/AP/AssetBuilder terminaron0. Los tests son byteidénticos entre RED90 y GREEN91. XML: `AD4E5660319E1027F181602307DD0A2C0B24267F09513503237A5EDD7D3C6395`.
- **GUI89, PID18824:** clic derecho sobre la textura → Generar material → mismo Workspace. Se creó y abrió el material con Shaderball; vertex color cambió su aspecto, Undo restauró gris y Redo rojo. Save persistió el valor; cerrar y reabrir el panel mantuvo el resultado. Hubo dos ciclos focales AP del material, el segundo después del guardado; los logs actuales del segundo tienen cero errores y avisos. Los relojes crudos AP/Editor/filesystem se conservan sin calcular latencia entre ellos.
- **GUI91, nuevo PID8712:** reabrió la fuente guardada sin editar ni guardar. La captura original1920×1032 muestra Shaderball rojo, NeutralUrban, vertex colorON, mapas Normal/Roughness, FlipYON y documento limpio con Undo/Redo deshabilitados. Los hashes de fuente y producto permanecieron iguales. No se observó un cook nuevo del material en esta sesión. Ambos Editor cerraron normalmente0, sin guard RAM.
- Los postflights verificaron14.034 inputs,40 archivos de autoría,26 del propietario,7 de fixture,9 binarios y escena sin deriva. En GUI91 también se cotejaron fuente y producto contra GUI89. Los nueve módulos cargados coinciden con sus pins, incluida la DLL de ImageProcessing que contiene la corrección: `5B66B3B0BEBC63DA7DBFF3E087B45ADAFE1FEB4F7100B1D04296ECCC978BD1AF`. La ausencia de backends DX12/Metal/Null y Python corresponde a capturas puntuales, no a toda la vida del proceso.

Fuente guardada:1818B, SHA256 `09B36BFE1B1BC48A8868B5A4008F3812100537DB14E99440BD412E1589D7CCA9`, StandardPBRv6 y `vertexColor.enable=true`. Producto focal:5850B, SHA256 `2F763F95948C317494D2DA1107EDFBEB972983E17BD5DF56DBD69ED2144B7571`. La fixture original incluye siete PNG/sidecars y suma346.829B.

## Historia y reproducción

El ZIP conserva RED88 de cinco casos/un fallo al repetir Ensure sobre settings compatibles; GREEN89 de esos mismos cinco; el control manual89 fallido por un puntero LFS en lugar del PNG; y RED90 de seis casos/un fallo Equals después de cargar el PNG real. El fix91 de dos líneas y los seis casos GREEN91 cierran ese segundo fallo. Cinco y seis no se suman como once casos distintos. UI90 sólo se preparó, nunca se ejecutó.

Se incluyen logs crudos, XML, freezes, postflights, acciones y capturas originales, módulos, fuente/producto focales, fixture7, preimágenes/candidatos, parches e inversas y recibos de aplicación. Los scripts privados exactos están en `reproduction/`; conservan rutas y requisitos históricos y no constituyen un replay portátil sobre HEAD. Los README provisionales se preservan bajo `history/readmes/`; sus pendientes antiguos son históricos. Este README y la aceptación del padre en `gui91/run/parent-closure-acceptance.json` describen el cierre vigente.

`manifest.json` externo identifica cada entrada por ruta relativa, bytes, SHA256, origen histórico y copia privada usada para sellar. El archivo se creó una sola vez y se reabrió verificando cada entrada. No contiene DLL/PDB/EXE, dumps, bases de datos ni el Cache completo. El censo crudo de Node con commandlines está excluido; sólo se incluye el resumen sanitizado revisado por el padre. El refresh Graft91 terminó0, pero el check standalone agotó35s: se conserva como timeout y no se declara PASS global.

## Límites

La integración conserva WIP ajeno: no hubo replay limpio HEAD ni se demuestra independencia de todos los cambios. Los347 fallos AP heredados no están aceptados globalmente; los avisos/errores existentes del Editor permanecen en los logs. Los picos de RAM son muestras globales, no benchmarks. La ausencia de procesos hijos no demuestra sus códigos de salida individuales; servicios compartidos y mspdb6496 de owner incierto se preservaron.

**Gems completas:11; incremento:0.** N0 y el objetivo persistente siguen abiertos. Este corte no cierra whole Atom, dependencias globales, Linux, VR, Shipping, formatos nativos ni rendimiento.
