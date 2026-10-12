# Importación nativa sin iniciar Python innecesariamente

ScriptProcessorRuleBehavior llamaba a LoadPython antes de descubrir si la escena necesitaba scripts. Shaderball.fbx.assetinfo no contiene ScriptProcessorRule: su procesamiento nativo generaba productos pero emitía un error y una advertencia de Python.

Se omite la inicialización cuando no hay script por defecto, regla declarada ni script heredado pendiente. Se conserva el orden de ejecución default→descubrimiento del manifest, junto con el comportamiento y los diagnósticos para scripts solicitados. No se eliminan reglas ni metadata, no se incorpora Python ni se añaden dependencias.

## Evidencia revisada

- RED: SceneData.Tests compila; la nueva regresión falla por IsPythonActive y StopPython pese no haber scripts. Usa UpdateManifest y PreExport del owner real, verifica manifiesto vacío y conservación de una regla nativa.
- GREEN: SceneData compilado con referencias desactivadas, exit0/15.0s; suite completa 43/43, exit0/0.48s. El test DLL no cambió entre RED y GREEN. Fuente del comportamiento SHA A8C8C99FA15B85290B3338824C0B03DC3C554D93A7585E01D32EB94EDBE195D5; test SHA F7A58A01D855215136E35B86FAF4B9A42711796560BBE538672AD04EAE1BF262.
- Evidencia local: `build/n0-validation/local-packages/scene-script-optional-repair-rev1/native-red-attempt2` y `native-green-attempt1`. LastTest.log conserva las 43 pruebas; el JUnit de CTest agrupa esa suite en un caso.
- Asset Processor oculto, PID182500, receta existente con `--reprocessFileList=<lista de una ruta absoluta>`. Reproceso real completado el 2026-10-12 a03:05:50Z: `Shaderball.fbx-2283785088-16336.log`, 0 errores, 1 advertencia, sin diagnóstico de Python. Once productos del cache regenerados. Recibos/log/cierre en `build/n0-validation/material-preview-original-asset-rev1/assetprocessor-attempt3`; log SHA50E2FF2245E5DB81D60668648BAF3F07D90D0B5FC71B94068B4593B1DD597885.
- La advertencia restante describe Inside/MainOuterSphere con vertex streams distintos: no se fusionan, conservan un material slot. No se modificó el modelo para ocultarla.
- FBX original permanece SHA AD3B5915D503FAA0F2EBEDB80D6C2D6999F31E108834CC3702153D06C1B2D0CF. El intento previo de actualizar mtime sólo produjo CreateJobs0/0 sin procesado nuevo; la aceptación procede del reprocess explícito, no de ese intento ni de borrar cache.

## Límites

No elimina todavía todos los consumidores o interfaces Python ni acredita absorción completa de Gems, cierre N0/N1, reducción de tamaño o integración del editor. Los assets originales recuperados y el resto del baseline local no forman parte de esta publicación. Ningún paquete descargado o cache eliminado para esta reparación.
