# Integración nativa de Camera / CameraFramework — 2026-10-12

Estado: integración Camera/CameraFramework verificada para publicación. Edición/persistencia y modo juego Vulkan comprobados; launchers independientes DX12 y Vulkan muestran la escena con los módulos nativos. No constituye cierre de N0 ni del GOAL.

## Implementación

- `Engine/World/Camera`, target `JE::WorldCamera`, contiene cámara, controlador, sistema, rig e interfaces; namespace `JE::World::Camera`.
- `Editor/Camera`, target `JE::EditorCamera`, contiene componentes y selector de autoría; namespace `JE::Editor::Camera`.
- Editor, AP/Batch, AssetBuilder, launcher y cuatro herramientas Atom componen los módulos directamente. SerializeContextTools registra Geometry/Camera de autoría y descriptores de materiales, sin exigir el sistema de preview. AssetBundler desactiva la carga de módulos y no recibió una dependencia de cámara innecesaria.
- Camera/CameraFramework dejan de ser Gems activables. StartingPointCamera consume la interfaz nativa y se compiló temporalmente sin añadirlo a la fixture. Los cinco seeds principales restantes de la fixture no representan el total de Gems del engine: Atom aún agrupa capacidades pendientes.
- UUID, nombres reflejados y campos persistentes conservados. El conversor legado `m_template` selecciona el componente editor cuando está reflejado, sin dependencia World→Editor. Los buses centrales `Camera::` de AzFramework/AzToolsFramework siguen como frontera temporal; no se declara migración completa de todo el núcleo a JE.
- Los ocho recursos de cámara conservan rutas relativas e identidades de scan folder. Tres PNG originales recuperados desde el pin O3DE auditado en `media.githubusercontent.com`, con tamaños/SHA iguales a sus OID y sin redirecciones; blobs Git normales. Véase `icon-provenance.json`.

## Verificación

Build Profile de productos y tests: exit 0, 263,054 s. Incluye Editor, AP, APBatch, AssetBuilder, launcher, MaterialEditor, MaterialCanvas, PassCanvas, ShaderManagementConsole y SerializeContextTools. Build final exclusivo de dos targets de pruebas: exit 0, 54,961 s. Configuración offline existente, sin descarga automática; StartingPointCamera ya había compilado y la configuración temporal se restituyó.

82 casos únicos pasan: WorldCamera 3, EditorCamera 6, EditorLib 72 y SerializeContextTools 1. Sin fallos, errores ni disabled; XML y resultados completos adjuntos. Los 100 inputs del freeze final permanecieron iguales durante la validación. Hashes de los productos nativos y de EditorLib.Tests/SCT conservados tras el build exclusivo de tests; se reutiliza su evidencia válida.

Se preservan los RED en `build/n0-validation/native-camera-integration-rev1`: dependencia directa AzTest ausente; selector probado con IDs ajenos al contexto editor; serializer JSON sin unreflection; cache de serialización liberada después del allocator. Correcciones en dependencias/fixtures, sin retirar oráculos ni suprimir errores. Los nueve casos de cámara pasan también al finalizar y descargar las DLL de pruebas.

Seis ejecuciones CLI reales de SerializeContextTools terminan con exit 0: dumptypes con especializaciones editor/Game, createtype de cámara/geometría/material de autoría y createuuid sin especialización. Ambos inventarios contienen los seis UUID runtime/editor; los tres JSON adjuntos demuestran creación y serialización. La especialización Game de esta herramienta no la convierte en runtime puro: compone deliberadamente tipos de autoría para conversión.

En el editor Vulkan, la cámara original aparece en el selector, `Be this camera` cambia la vista, el Inspector permite FOV 65→55, Undo restaura 65 y Redo por menú restaura 55. Save cambia sólo ese campo; una segunda sesión reabre 55 y entra en modo juego. Capturas originales y módulos cargados en `editor-vulkan/`. No se usó Reset All UVs. La escena original permanece `FEE55849A032D29059D4369112047DC27E281D39631D0F0DF87D8BCF5EF517AA`; la copia de prueba queda `4C5E436524DD9EA5EB4FAE0B8BF61DF1024D5B7784D4C92EA242A28298C70040`.

## Tamaño y límites

Las tres DLL originales de Camera/CameraFramework sumaban 1.755.136 bytes. WorldCamera + EditorCamera suman 1.360.896: ahorro de 394.240 bytes (22,46 %) en este conjunto. No acredita reducción global del engine, RAM, VRAM ni paquete Shipping. Los imports directos de WorldCamera no incluyen Qt/Python; no sustituye una auditoría transitiva de distribución.

Persisten errores conocidos de thumbnail y plantilla DiffuseProbeGrid, la restricción de edición geométrica con UV manuales y la diferencia visual histórica DX12/Vulkan. El primer cierre del editor fue normal; en la segunda sesión, las entradas simuladas no salieron del modo juego y se terminaron explícitamente los procesos propios. No se acredita salida normal de esa sesión.

El launcher arrancó y cargó escena en Vulkan y DX12 sin Editor/AP; los inventarios contienen WorldCamera, Geometry y RenderMaterials y no las DLL Camera heredadas ni módulos de autoría/Qt/Python. Cada backend obtuvo 20 muestras TCP/UDP vacías. Los primeros intentos conservaron un fallo de captura de la herramienta. Recuperada la captura, Vulkan attempt2 y DX12 attempt3 muestran realmente suelo, puerta y corredor; se adjuntan imágenes originales runtime.jpg y nuevos inventarios. Ambas repeticiones mantienen 20 muestras sin endpoints ni Editor/AP. AltF4 no terminó los launchers y se detuvieron explícitamente sus PID propios; salida normal pendiente. DX12 conserva iluminación más intensa que Vulkan. La observación acredita render de la escena, no paridad visual, Beauty ni rendimiento.

VR/headset, Linux, métricas CPU/GPU p50/p95/p99, interacción de juego, materiales completos, resto de Gems y paquete autónomo siguen abiertos. Ninguna captura está presentada como aceptación de Beauty o benchmark.

Comprobación final Material Editor Vulkan: arranque con Shaderball/Neutral Urban visible tras la nueva composición Camera; captura e inventario en material-editor-vulkan/. Sin editar/guardar materiales. AltF4 cierra el proceso. El ajuste final de una línea vacía al final de CameraModuleTests.cpp no cambia tokens C++; publication-whitespace.json conserva hashes antes/después y se reutilizan las pruebas del freeze original.

La validación corresponde a este checkout y sus recursos locales auditados. No acredita reproducción de un clon vacío ni paquete Shipping autosuficiente. Los seis registros residuales de Camera heredado pertenecen a variantes server/headless/unified fuera del producto actual; sus binarios/outputs históricos aún no se han eliminado.
