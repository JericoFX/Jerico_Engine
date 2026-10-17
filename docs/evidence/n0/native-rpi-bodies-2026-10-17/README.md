# RPI99 — evidencia focal de integración nativa

Public y Reflect pasan al owner existente `Engine/Render/Integration`; Edit queda en `Editor/Render/Integration`, bajo `JE::Render::RPI`. No se añade otra DLL de producto. El ledger preparado contiene **2302 operaciones**; no equivale a 2302 componentes ni a Gems aceptadas.

El build **HOSTS5 de 14 targets terminó 0** y se reutiliza: R32 combina esos hosts inmutables con GREEN31 para RI y el test cambiado. No hubo otro build de 14 targets. El RED de `Parent.Reset → child.QueueForInitialization → Parent.Build` completó 1 caso y falló con attachments esperados 1 / obtenidos 0. Tras conservar Reset al encolar inicialización, el focal GREEN pasó 1 caso y la única CTest de 6 suites completó **707 casos únicos**, sin fallos, errores, skips ni disabled. Se incluyen 8 XML byteexactos; el focal no se suma otra vez a 707.

El grafo PE previo registra **93 nodos / 1616 aristas**. R32 comprobó 2 binarios cambiados y 91 idénticos; los conjuntos directos de RI (13) y del test (21) permanecen iguales. Es reuso explícito, no 93 inspecciones PE nuevas. El subcierre SDK medido conserva 30 DLLs.

El Editor Vulkan **PID 31748** mostró BSP/InspectorUV y MaterialWorkspace con mapas, shaderball y zoom en el mismo proceso; el runtime **PID 8616** mostró la habitación. Ambos cerraron normalmente con salida nativa 0. El manifest referencia 7 capturas originales por ruta, bytes y SHA256; no incorpora JPEG ni dumps. No es un benchmark ni confirma la abertura de ventana.

El control físico posterior encuentra **46 productos, 44 paths nativos y 3 passes**; los 3 passes coinciden con sus SHA previos. Las identidades DB son anteriores: 17 SourceGUID, 38 identidades preservadas y 8 productos añadidos de 4 PNG. No se infieren nuevos RunKeys ni igualdad de bytes donde faltaba un SHA previo. Dos productos `common/` se materializan en `Cache/Intermediate Assets`.

## SDK — verificado dentro de su alcance

La única ejecución SDK99 terminó con instalación0 (48,772s), configure0 (80,833s), build del consumidor0 (19,597s), imports PE0 y loader0. Se comprobaron seis targets nativos importados, 30 DLLs desplegadas byteexactas desde el SDK y cinco mensajes de ciclo de vida. Resultado final: 030E931E1142610DFEBE94FF09EDB30E206E877FD69DD6A55CB9FFB9838ACF54; inventario instalado: 6307F3AFD2F5EA90B1795F32E4F3DF83025A1CC7E653F18C8BBF2DBA56C8C21A. Sin timeout ni guard de RAM.

El consumidor se resuelve desde el prefix instalado; el checkout sigue accesible. No activa módulos, cook ni renderer: las pruebas gráficas pertenecen a Editor/runtime. No acredita aislamiento físico, Shipping ni empaquetado source-free global. El escaneo registra un directorio de instalación previamente inexistente: Gems/Atom/Tools/ShaderManagementConsole/Code/Include/. La instalación y los seis contratos importados pasaron; no se inventa un paquete para cubrir ese directorio.

La evidencia usa WIP preservado; no existe replay de un HEAD limpio. Siguen abiertos 336 fallos globales AP, Beauty, VR, Linux, Shipping y el goal global. RPI99 se acepta como absorción nativa Windows/Profile/Vulkan; no declara cierre global de una Gem ni aumenta su contador. Las 11 identidades nominales históricas no constituyen un inventario certificado de pendientes.
