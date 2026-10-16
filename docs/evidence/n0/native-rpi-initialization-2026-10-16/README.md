# Inicialización RPI nativa — corte98, 16/10/2026

Estado: inicialización y registro absorbidos; Atom_RPI completo sigue PARTIAL. N0 y GOAL permanecen activos. Base Git development `72bbe49bf23552edb0fc8e9b7264d490884293e6`; pruebas sobre el WIP preservado, sin replay limpio de HEAD.

## Cambio y límites de ownership

Los nueve archivos de RPI.Private/RPI.Editor se integran en los owners existentes Engine/Render/Integration y Editor/Render/Integration, bajo JE::Render::RPI. Las factorías CreateRpiRuntimeModule/CreateRpiEditorModule conservan identidades, cuatro componentes requeridos, servicios, versión0 y campo RpiDescriptor. El editor conserva el registro de functors durante la vida del componente. Los cinco puntos de host usan la factoría nativa antes de Bootstrap; las once aplicaciones físicas se recompilaron durante el corte. No se crea otra DLL, renderer, ECS, cooker ni sistema de materiales.

Se retiran Atom_RPI.Private y Atom_RPI.Editor de los targets y planes de activación, más la configuración fija atom_rpi.setreg. Sus defaults se conservan en C++ y siguen aceptando overrides parciales. Se retiran dos DLL de módulos del despliegue; Atom_RPI.Public/Edit, sus API, assets, Registry y manifest permanecen para el corte siguiente. Incremento de Gems completas0; el recuento histórico nominal11 conserva sus límites anteriores.

## Pruebas nativas y diagnóstico causal

- RED factory: ocho pruebas anteriores pasan; dos nuevas fallan por export ausente. Después se preservan también el error de compilación de la fixture y el RED de cuatro casos con asserts.
- CDB demuestra que destruir el componente sin activarlo llega a ShaderVariantAsyncLoader::Shutdown y hace join sobre un hilo no iniciado. La corrección añade solamente joinable antes de join. La fixture JSON elimina su reflexión al cerrarse, siguiendo el patrón existente.
- GREEN final: build nativo0/66,354s y CTest0/19,243s; XML reales8runtime +4editor +576RPI.Public =588 casos únicos completados,0fallos/errores/skips/disabled. Los wrappers anteriores que salieron1 por VCTIP se conservan; no se presenta ese1 como un fallo de compilación nativa. R5 sólo agrega metadata de assets y reutiliza estos resultados sin repetirlos.
- PE: los7836 exports públicos de Atom_RPI.Public y sus13 dependencias conservan su tabla frente al baseline SDK96; factorías nativas y los seis CVar compartidos tienen el owner/export/import previsto.

Los XML, resultados, logs nativos y revisión PE están en green-tests, native-build, red-factories, red-lifecycle y pe-boundary. El manifest registra bytes y SHA256.

## Editor, Asset Processor y runtime Vulkan reales

El primer Editor98 falla al no encontrar SceneAndViewSrgs y después sufre AV. Quitar la activación de los módulos también había quitado el descubrimiento de Assets/Registry del RPI residual. AssetStatus1 significa Missing; no se atribuye a un compilador fallido. Los22 planes comparados y los logs de AP sostienen el diagnóstico.

Registry/rpi_assets.setreg conserva únicamente la entrada vacía Atom_RPI para ese descubrimiento. No registra Targets, Modules ni carga de DLL Gem. No se reemplaza por un ScanFolder de Assets incompleto: Registry y aliases también necesitan metadata durante esta transición. Los otros dos setreg de RPI quedan intactos.

Con R5, AP registra Processing/Processed de SceneAndViewSrgs a23:28:11–12Z y se conservan los nueve productos físicos Vulkan. No se recuperó una nueva RunKey enlazada a los hashes; la evidencia acredita esos eventos, productos y uso gráfico, sin inventar una trazabilidad de job completa ni PASS global de AP.

Editor98R5 muestra la habitación/corredor existente y White Box con material metal_iron por cara/UV2x2. Material Workspace abre je_phnative87_1k con shaderball rojo, NeutralUrban y mapas dentro del mismo Editor.exe. Sin editar ni guardar la escena/material. Cierre normal exit0/249,900s; las20 identidades propias de Editor/AP/builders quedan ausentes. La UI conserva0 pendientes y338 fallos globales heredados.

Launcher98R5 dibuja la habitación en Vulkan y cierra normalmente con exit0/22,191s, sin AP activo. Se conservan las listas reales de módulos: providers nativos y ausencia de los dos módulos retirados; runtime sin Editor/Qt, Python, DX12/Metal/Null en esa captura. Null en herramientas headless sigue permitido. No se afirma ausencia durante toda la vida de un proceso a partir de una sola captura.

Las capturas originales, sin recorte ni reducción, están retenidas localmente por ruta/bytes/SHA en localOriginalVisuals del manifest. No se incluyen PNG/ZIP con filtros LFS ni se realiza descarga/subida LFS no auditada. El dump original y los módulos retirados están retenidos en evidencia privada, sin borrarlos ni atribuir ahorro de disco.

## SDK: delta puntual, sin otra instalación completa

SDK96 permanece como baseline aceptado e inmutable. El único consumidor98 configuró0/28,269s y compiló0/19,422s. Su primera invocación errónea con WindowsPowerShell5.1 no lanzó fases nativas; se conserva el diagnóstico y se utiliza el PowerShell7.6.5 configurado.

Después del build, reglas transitivas antiguas de SDK96 sobrescribieron las dos DLL nativas nuevas del consumidor con las viejas. El guard detuvo la ejecución antes de imports/loader. Sólo esas dos copias regulares generadas se respaldaron y sustituyeron desde el overlay, comprobando un único enlace propio y hashes. No se tocaron hardlinks ni SDK96; Atom_RPI.Public ya era correcto y no se volvió a copiar.

La continuación ejecutó únicamente PE-imports0/0,044s y loader0/0,097s. Cinco mensajes reales verifican los seis targets públicos y las dos factorías RPI, identidades, cuatro descriptores, reflexión y dos ciclos de construcción sin activación. Las32 DLL finales coinciden con el cierre esperado,29 hardlinks inmutables y3 copias. Configure/build/install no se repitieron. Fuentes, headers, providers, evidencia cerrada, SDK96 y consumidor96 quedaron intactos.

Este delta acredita API/ABI/ciclo de vida; no acredita regeneración del empaquetado SDK completo, eliminación de metadata antigua de SDK96 ni activación gráfica dentro del consumidor. La activación gráfica se acredita por Editor/Launcher separados. Evidencia en sdk-delta.

## Tamaño y preservación

La cohorte acotada pasa de cuatro DLL/2053632B a dos/1822208B: -231424B (-11,269%). Para el editor, las tres DLL cargadas anteriores sumaban1814528B frente a1822208B actuales (+7680B); no se extrapola el ahorro de la cohorte al proceso, RAM o engine completo. footprint.json conserva los componentes.

El freezeR5 conserva14063 inputs y61 ausencias;86 archivos de autoría y73 ausencias, además de13 pins de hosts/providers, sin deriva. Todos los gates pertenecen al mismo WIP preservado. Los helpers propios de UI y procesos de las pruebas se cerraron; no se mataron Node/MCP compartidos desconocidos.

Pendientes: RPI.Public/Edit/assets y su metadata residual; empaquetado SDK completo actualizado cuando corresponda;338 fallos AP globales y DiffuseProbeGridUpdatePassTemplate heredado; VR/OpenXR, Linux, Shipping, Beauty y métricas comparables de CPU/GPU/RAM/VRAM/carga. No hay aceptación de todo Atom, de N0 ni del GOAL. No se descargaron dependencias ni se importó código upstream en este corte.

Upstream primario revisado como referencia de contratos y candidatos: [PRs actuales](https://github.com/o3de/o3de/pulls), [pipeline switching16171](https://github.com/o3de/o3de/pull/16171) y [VR16570](https://github.com/o3de/o3de/pull/16570). No se importa código ni se declara una auditoría exhaustiva; parent/upstream-scope98.json conserva el alcance.
