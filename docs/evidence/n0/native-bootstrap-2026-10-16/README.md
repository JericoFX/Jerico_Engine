# Bootstrap nativo y correcciones de arranque/cierre — corte97

Evidencia delimitada de la integración en el árbol de trabajo preservado. Los resultados históricos conservan su estado original; el cierre vigente se resume aquí y en el manifest. SDK96 terminó y fue revisado por el padre; se conserva como baseline para evitar rondas completas por cambios locales.

Bootstrap pasa al owner `Engine/Render/Integration`, con namespace `JE::Render::Bootstrap`, sus buses, PAL, recursos y factory nativa. El ledger de integración abarca86 rutas (61 presentes/25 retiradas); no equivale a aceptar una Gem adicional. El seed original se conserva y su entrada se integra en la lista nativa del engine. Este paquete breve no reproduce el ledger completo ni acredita un replay limpio de HEAD.

La carga real del room expuso una tabla material vacía pero dirty: registrar y liberar el último tipo antes del primer Compile acababa solicitando un buffer de0 bytes y dereferenciando un resultado null. El RED95 causal inició un caso y terminó con AV,0 casos completados y sin XML; no es un GTest RED completado. La corrección mínima de MaterialSystem conserva una entrada centinela−1 y capacidad física mínima1, sin cambiar el número lógico de tipos. El test verifica release-before-first-compile y reregistro posterior. Los XML GREEN96 registran1 caso focal y576 casos de RPI completados, sin fallos/errores/skip/disabled; el focal pertenece a la suite, no debe sumarse como caso distinto.

Durante el cierre del launcher96, CDB capturó una AV de segunda oportunidad al liberar `shared_ptr<AuxGeomDraw>` desde los destructores de viewport/mapa/AtomBridge durante ModuleManager::UnloadModules. El mapa conservaba las colas hasta el destructor del componente; la traza no registra el instante exacto de descarga del proveedor. Las3 líneas de Bridge liberan el mapa al final de Deactivate, después de desconectar sus buses. CDB informó error al obtener `.ecxr`; `.exr` y la pila sí documentan la AV. El exit0 del debugger no es exit0 del launcher.

Resultados cerrados y revisados por el padre:

- **Hosts97:** once targets incrementales, exit0/156.649s (aprox.157s), máximo RAM48.21%;14064 inputs y44 ausencias sin deriva. No se ejecutó configure en este gate.
- **Factory8:** seis casos RenderIntegration y dos EditorRenderIntegration, CTest0/0.948s. No sustituyen el proveedor RPI/Scene real del cierre Vulkan.
- **Launcher97:** room JEMaterialCookNative5 visible con Vulkan; el mismo ejecutable/argumentos y la misma solicitud OS sin force pasaron de AV−1073741819 a cierre nativo0. Se conserva el warning de DynamicDrawContext presente también en RED.
- **Editor97:** habitación, Inspector UV y MaterialWorkspace con shaderball interno en el mismo proceso; cierre por AltF4 nativo0. AP y sus diez builders ausentes al cierre; postflight14064 fuentes/86 protegidos/56 ausencias sin diferencias. Máximo RAM60.72% es una observación de esta ejecución, no un benchmark.
- **SDK96:** instalación 0/51,173s, configure 0/78,858s, build del consumidor 0/18,295s, imports 0/0,044s y loader 0/0,112s. Seis targets públicos,32 copias DLL y cuatro mensajes de ciclo de vida;11193 archivos finales. Acredita imports/ABI/reflexión; el dibujo y cierre real se acreditan por Editor/Launcher97. Configure conserva sus warnings, incluidas variables FETCHCONTENT_* sin uso; no se infiere aislamiento de red de esas variables.

Las capturas JPEG originales97 se referencian con rutas, bytes y SHA256 en `manifest.json`; no se copian, convierten ni reinterpretan. Los logs raw y recibos copiados son byteexactos. Los recibos históricos pueden decir PENDING para gates que se cerraron después; su fecha y alcance se conservan.

Límites: Profile Windows Vulkan sobre WIP preservado;338 fallos AP globales siguen abiertos. El Editor registra el error heredado de la plantilla DiffuseProbeGridUpdatePassTemplate, warnings de assets faltantes y consola; no se declaran resueltos ni se ocultan. No hay replay limpio de HEAD, ni aceptación de VR, Linux, Shipping, Beauty, pacing/rendimiento o cierre global de dependencias. Incremento de Gems completas:0. Este paquete tampoco prueba nuevas ediciones/Save de materiales en97.

Política de reutilización: SDK96 es la base vigente. Una corrección de cuerpo de función requiere la prueba focal/target/runtime afectado. Otra ronda completa exige un cambio demostrado de API/exports, dependencias o layout instalado. En Bootstrap hubo tres rondas completas; la última podía sustituirse por validación puntual.

Contraste upstream: [AtomBridge en el commit revisado](https://github.com/o3de/o3de/blob/74006f539d581ea2ae8d21d4ef629d7a610a3e01/Gems/AtomLyIntegration/AtomBridge/Code/Source/AtomBridgeSystemComponent.cpp#L108-L130) conserva el mapa al desactivar. Dos consultas oficiales por palabras clave no encontraron PRs; ese resultado no prueba ausencia global. El contraste de MaterialSystem y las respuestas/hash/fecha están en upstream/.
