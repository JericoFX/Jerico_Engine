# Diagnósticos integrados en JericoEngine — 2026-10-12

Se absorben Profiler, ImguiAtom y AtomImGuiTools en `Engine/Debug/Profiler` y `Engine/Render/Debug`, con namespaces propios, factories nativas y consumidores migrados. Se retiran 44 archivos de implementación/metadata legacy. No se añade otro renderer ni se cierra N0/GOAL.

## Composición y alcance

- `JE::ProfilerCapture`: captura CPU y serialización. `JE::ProfilerView`: interfaz/heap/treemap sobre Capture y DebugImGui, sin dependencia de Atom. Los hosts gráficos seleccionan la factory visual que incluye captura; AP/Builder seleccionan captura. Launcher headless tiene guard explícito, pero sus targets deshabilitados no se compilaron.
- `JE::RenderDebug`: los dos adaptadores originales sobre los pases y contexto Atom existentes. Request bus y callback de selección de mesh tienen un único dueño de exports. AtomUtils consume ProfilerView sin ciclo inverso.
- Composición no equivale a aislamiento de DLL: AP y sus diez AssetBuilder observados cargaron Capture, View y RenderDebug transitivamente. La reducción de esas dependencias queda pendiente. El snapshot de recook sólo enumeró estos módulos; no acredita ausencia de Qt/Python en AP.
- Los dos fuentes ImGuiAtom se trasladan byte idénticos, con instalación y scan folder nativos. El pase de viewport real sigue usando `Shaders/imgui/imgui.azshader` de FeatureCommon; no se atribuye su render al shader legacy trasladado.

## Verificación

Freeze integrado rev2: 78 entradas, SHA `5FAFEBABACF9041B7B8789C24BBA2A945DD7E9A77212A2C95386542F2E0134D6`. Tras las pruebas sólo se retiraron dos líneas vacías finales de módulos, sin cambiar tokens ni números de línea del programa; hashes originales y finales en `parent/publication-whitespace.json`, reutilizando la evidencia semánticamente válida. Incluye cinco controles CMake locales ajenos al commit: esta evidencia no certifica reproducción de un clon limpio ni distribución autosuficiente. `parent/source-review.json` es revisión previa, con estado histórico pendiente; los resultados posteriores de este directorio la complementan.

VS2022 x64 Profile, árbol incremental existente, configuración offline sin servidores de paquetes ni nuevas descargas. Configure real exit 0 / 113,827 s. Primer build focal falló por acceso indexado a AZStd::list en tests; se corrigieron sólo siete accesos con front/back. Build focal posterior exit 0 / 137,625 s; once targets afectados exit 0 / 1148,354 s. XML: Profiler 6 + RenderDebug 4 + DebugImGui 6 + EditorLib 72 = **88 casos, cero fallos, errores o skips**. Los intentos previos de runner no se cuentan como builds/configuraciones válidos.

Regresión real: el profiler antiguo intentaba formatear una firma OpenMesh de 638 caracteres en fixed_string de 512 al salir del juego. El RED previo está conservado en `build/n0-validation/native-imgui-rev1/editor-vulkan-attempt1/Editor.log`. La reparación conserva el camino deshabilitado, evita formatear literales sin `%`, dimensiona formatos largos y cierra ambos va_list. No cambia la representación AZ::Name preexistente. Tests cubren dos literales de 639 caracteres, formato largo, anidamiento y JSON. En Editor Vulkan se activó CPU y se salió de juego con Ctrl+G: sin la aserción en el log completo, con captura original de la UI. Los nombres ScopeProducer ilegibles siguen pendientes.

Editor Vulkan mostró timestamps, timeline y treemap de memoria GPU. Material Editor Vulkan abrió normalmente y renderizó Shaderball/Neutral Urban. Las capturas prueban funcionamiento de esta integración; no acreditan una nueva prueba de edición de materiales ni calidad visual final.

Standalone DX12 y Vulkan renderizaron la habitación y mostraron CPU/timestamps/timeline. Inventarios de 126 módulos por backend: Capture/View/RenderDebug presentes; ausentes las cuatro DLL retiradas, Qt, Python/PySide y los módulos de autoría comprobados. Veinte muestras por backend sin endpoints TCP/UDP ni Editor/AP. Vulkan attempt1 fue rechazado por el guard mientras terminaba el DX12 anterior; no llegó a lanzar. Vulkan attempt2 es la prueba válida. Ambos launchers se cerraron por terminación explícita del PID propio: salida normal sigue sin acreditar.

Muestras de arranque, no benchmark: máximo working set DX12 842.211.328 B y Vulkan 602.411.008 B; máximo private bytes 2.191.585.280 B y 2.045.267.968 B. Son procesos/backends distintos, sin comparación de mejora ni percentiles CPU/GPU. Los overlays/capturas estáticos tampoco prueban suavidad o 60 FPS sostenidos.

## Assets, tamaño y preservación

Recook real: RunKeys 31870/31872 para ambas prefabs, cero errores/warnings; shader 31874, cero errores y tres warnings de encoding C.UTF-8. Folder 72 y portable key heredada conservados, ahora bajo Engine/Render/Debug/Assets; IDs/GUIDs estables y 27 productos presentes. Spawnables byte idénticos. De 25 productos de shader, nueve iguales y dieciséis diferentes: no se conservaron preimágenes para demostrar la causa de esas diferencias. `postflight-corrected.json` acredita 78/78 entradas y 27/27 productos; sustituye el conteo falso 0/78 del recibo original local por un bug de composición de rutas. `InspectCook.py` es un inspector externo de evidencia, no una dependencia del engine/editor/juego.

Audit de 129 binarios, cero imports de las cuatro DLL antiguas. Cohorte DLL: 1.441.792 → 1.354.240 B, reducción 87.552 B. Símbolos PDB: 59.949.056 → 39.759.872 B. Se eliminaron exactamente 19 DLL/PDB/LIB/EXP obsoletos, 71.063.007 B del build local, con hashes y límites de ruta comprobados. No equivale a ahorro del engine completo ni Shipping.

Prefabs original `FEE55849A032D29059D4369112047DC27E281D39631D0F0DF87D8BCF5EF517AA` y copia `4C5E436524DD9EA5EB4FAE0B8BF61DF1024D5B7784D4C92EA242A28298C70040` preservadas. No se usó Reset All UVs. La geometría que reproyecta UV manual sigue bloqueada: la protección evita perder autoría, no implementa todavía esa operación.

Pendientes globales: resto de Gems, límites de herramientas, nombres/registro heredados, 347 assets fallidos previos, DiffuseProbeGridUpdatePassTemplate/thumbnail faltantes, paridad de iluminación DX12/Vulkan, cierre normal del launcher, paquete independiente, rendimiento representativo, Linux y VR real. El cierre de este corte es integración funcional de los tres dueños indicados, no engine completamente libre de Gems.
