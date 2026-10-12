# Retiro de consumidores Python del editor y herramientas

El editor deja de iniciar Python, registrar su terminal, ejecutar scripts de
arranque o macros Python y exponer RunFile/RunFileParameters. Se conservan las
acciones nativas y los valores serializados de macros antiguas; estas últimas
se muestran como no compatibles, sin reinterpretar su texto como consola.
Abrir un proyecto existente usa el ejecutable del editor y valida project.json.
El Project Manager dependiente de Python deja de formar parte de su despliegue.

AtomToolsFramework retira su inicialización, automatización y menús Python.
ShaderManagementConsole utiliza el generador C++ de variantes existente. Los
hosts dejan de depender de QtForPython; la composición del engine retira las
entradas Python y multiplayer. Las plataformas Windows/Linux dejan de generar
launchers server/unified. No se eliminan los repositorios históricos.

Se incluye el reemplazo HDR nativo ya implementado porque la versión anterior
llamaba al runner Python: retirar sólo ese runner habría perdido Generate y
Activate. Este cambio publica ese prerrequisito, sin ampliar su funcionalidad.

## Verificación y límites

Los cuatro JUnit adjuntos se ejecutaron el 2026-10-12 en Profile: EditorLib 72,
generador de variantes 13, Atom Feature 82 y CommonFeatures Editor 30. Total:
197 casos, cero fallos, errores, deshabilitados u omisiones. Los result.json
conservan comandos y códigos de salida. Las pruebas usan C++, no Python.

La compilación incremental posterior de seis targets terminó con exit 0 en
99,454 segundos (`build-result.json`); sus 82 inputs permanecieron idénticos
(`postflight-inputs.json`). Cambiaron los DLL de EditorLib.Tests y del generador:
se repitieron sus 72+13 casos con exit 0. Los archivos `postbuild-*` conservan
esa comprobación final. Los otros dos DLL no cambiaron y mantienen la evidencia
de 82+30 casos; no se suman las repeticiones como cobertura adicional.

El build productivo y las pruebas de edición/runtime previas están documentados
en `../native-geometry-materials-2026-10-12`. La prueba del generador no acredita
el flujo completo de su ventana, Undo/Redo, Save/reopen o cancelación desde UI.
La validación de HDR efectiva no demuestra equivalencia visual de todos los
shapers/rangos ni VR.

Esta publicación no certifica un paquete distribuible sin Python: todavía deben
cerrarse el grafo transitivo, los archivos desplegados y la reproducción offline
con dependencias auditadas. Se conservan nombres de buses/tipos nativos heredados
para mantener compatibilidad; su nombre no implica un intérprete embebido.
La creación nativa de proyectos/componentes y la migración de macros a Luau
siguen pendientes. N0, N1 y el GOAL completo permanecen abiertos.
