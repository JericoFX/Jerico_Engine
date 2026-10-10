# Auditoría de dependencias y puntos de absorción JE

**Fecha:** 2026-10-10. **Snapshot leído:** `a536fb78c6195c3edf10afa34bfdf65c6f841ee5` en development. **Estado:** inspección de fuentes/CMake, NO build/link/runtime/offline verificado. Esta auditoría prepara retirada de AWS/multiplayer, dos backends, integración nativa y BSP; no declara cumplido el aislamiento del motor.

La comparación Git entre O3DE `241c78b8064898a709cf1c5dd3535c6406763ef3` y el snapshot mostró dos commits, 25 archivos Markdown afectados y ningún cambio de source/CMake/assets. Eso permite reutilizar los blobs históricos fijados del baseline, no importar PASS anteriores.

## 1. Hallazgos con fuente exacta

Las rutas son nombres existentes para localizar código; NO nomenclatura final autorizada. Todos los enlaces se forman con `https://github.com/JericoFX/Jerico_Engine/blob/a536fb78c6195c3edf10afa34bfdf65c6f841ee5/` más la ruta indicada.

| ID | Fuente leída y blob | Observación comprobada | Acción/limitación |
|---|---|---|---|
| DEP-01 | `.lfsconfig`, fbd1bdb9ba00cba22e0231c35807aa34643bc543 | Endpoint LFS por defecto CloudFront; comentarios sugieren ruta de fork/autenticación | No usarlo para nuevos privados ni transmitir PAT. Resolver objetos necesarios en almacenamiento permitido antes de cambiar endpoint; no reescribir historia/borrar LFS |
| DEP-02 | `cmake/3rdPartyPackages.cmake`, 211b76ef5f3d00466ae50bd140f71d066b9493e7, líneas 1–120 | Default `https://d3t6xeg4fgfoum.cloudfront.net`; variable de entorno se antepone, no reemplaza ese default; se incluye downloader S3 y se admite file:// | Reemplazar lista efectiva y rutas de fallback, no solo añadir servidor alterno. Probar cache local completa y falta de paquete sin conexión. No leído entero el downloader |
| DEP-03 | `Code/Framework/AzFramework/CMakeLists.txt`, 55db750ea456898a7fd8e9c7183f4390febc3526 | AzFramework tiene dependencia PUBLIC a AZ::AzNetworking, además de zstd/lz4 | Desactivar Gem Multiplayer no separa esta dependencia de framework. Seguir usuarios reales de IPC/assets/debug/network; no se infiere que Multiplayer ya esté activo |
| DEP-04 | `Code/Framework/AzNetworking/CMakeLists.txt`, 9f65c6703c9d679d48fd525d680bb35f68174af5 | Target STATIC, PRIVATE AzCore/zstd/OpenSSL, PUBLIC RapidJSON/RapidXML; AutoPackets.xml genera headers/source | Separar mínima comunicación local de tools; retirar transporte de juego y codegen asociado donde no queden consumidores. Linker map requerido: CMake edge no prueba todos los objetos incluidos |
| DEP-05 | `Gems/WhiteBox/Code/CMakeLists.txt`, 73afe4f3f2aafd639f6c2b6ddebb42e848ea7389 | Runtime/API dependen de Atom/AzCore/CommonFeatures/Legacy::CryCommon; Editor de AzToolsFramework/Editor.Headers/OpenMesh/Manifold. Existen aliases Clients/Servers/Unified y módulo Editor; physics tests requieren PhysX5.Editor | Base de integración BSP es real, pero no autónoma. Migrar consumidores/servicios/builders y aliases al owner JE, adaptar test físico hacia Jolt. No importar CryEngine5.7 para satisfacer nombre Legacy: inspeccionar source O3DE licenciado |
| DEP-06 | `Gems/WhiteBox/Code/whitebox_editor_supported_files.cmake`, 1fb9e3b2a86d55a4ae0c3efb15cd8ecc2dda0eae | Lista source de CSG/CsgCore, component modes, manipuladores vertex/edge/polygon, shape builders, asset handler y undo command | No hace falta empezar con BSP vacío. Auditar implementación por operación, continuidad UV/IDs, persistencia y costo antes de reutilizar |
| DEP-07 | `Gems/WhiteBox/Code/Source/Core/WhiteBoxCsgCore.cpp`, 267ef2b818503de4aec5bf774e92849189269588, completo | Usa manifold::MeshGL64 con numProp=3, copia posiciones/índices, Union/Intersection/Subtraction, valida estados de Manifold; IsEmpty devuelve false; weld posterior con 1e-6 y cuantización por celda; índice uint64 convertido a uint32 | Geometry core existente, NO prueba de materiales/UV/GUID. Añadir casos resultado vacío válido, coplanares, escalas/límites, overflow y topología; no afirmar bug visual ya reproducido |
| DEP-08 | `Gems/WhiteBox/Code/Source/Core/WhiteBoxCsg.cpp`, 887b7041e5135782cee36bbf15831d6c5dbd9612, líneas 1–220 | ToTriangleMesh transforma caras a posiciones/índices y suelda; agrupación conectada por normales coplanares y comienzo de ear clipping | Lectura parcial. Rastrear reconstrucción/resto y pruebas antes de declarar pérdida general de UV/material. El núcleo no transporta esos atributos por sí solo |
| DEP-09 | `Gems/RecastNavigation/Code/CMakeLists.txt`, 3c07ba041f36a673992b4c442235756d9e0b0103 | API expone Recast/Detour/DetourCrowd; implementación requiere AzFramework/LmbrCentral/DebugDraw; tests PhysX.Mocks; aliases client/server/unified | Manifest vacío no significa cero deps. JE Navigation encapsula tercero, mantiene providers/tests útiles y elimina server producto. Tests PhysX no prueban dependencia del runtime directamente |
| DEP-10 | `Gems/RecastNavigation/3rdParty/FindRecastNavigation.cmake`, 424f3ec7909fd24d02933e141a6e9801d0bdd85f | v1.6.0, licencia declarada Zlib, Git hash 6dc1667f580357e8a2154c28b7867bea7e8ad3a7 y hash archive d48ca0121962fa0639502c0f56c4e3ae72f98e55d88727225444f500775c0074; o3de_fetch_content + FetchContent_MakeAvailable, static libs, DT_POLYREF64 ON | Hay pin, no dependencia sin versión. Todavía necesita fuente local/offline y comprobar artifact/licencia efectiva. Conservar ancho de PolyRef o versionar rnav; demos/tests upstream OFF no acreditan tests propios |
| DEP-11 | `Gems/Atom/Feature/Common/Code/Source/Platform/Windows/runtime_dependencies_clients.cmake`, 7638f14880221701836785bce8007dc87ea2fecd | Selecciona Vulkan.Private, DX12.Private y Null.Private | Las dos rutas requeridas ya están declaradas. Migrar propiedad a JE, probar ambas y limitar Null a tools/tests; no decir que ya están validadas |
| DEP-12 | `Gems/Atom/Feature/Common/Code/Source/Platform/Linux/runtime_dependencies_clients.cmake`, 69907bbdee8cfb432ac764ad78d58edecd6bf6cb | Selecciona Vulkan.Private | Linux Vulkan es el tercer par de prueba, no Linux DX12. Auditar también listas tools/builders antes de declarar cerrado el package |

Inventario previo aún válido por blobs/source sin cambios: engine.json enumera capacidades incluidas y Templates de Gems; BuiltInPackages_windows declara SDKs AWS, Qt/PySide y otros paquetes. Declaración en catálogo NO demuestra descarga/enlace. Revalidar usos transitivos antes de eliminar cada paquete. La auditoría anterior de 23 ramas sigue siendo histórica; ahora DX12 es obligatorio y geología activa.

## 2. Riesgos BSP derivados de fuente, no fallos ejecutados

CSG ya implementa operaciones con Manifold. No aceptar su salida como producto BSP final hasta probar:
- A−A e intersección disjunta: distinguir SuccessEmpty de InvalidInput/Failure según contrato del editor; no borrar fuente si falla una operación.
- Coplanares, tangentes, escalas extremas/negativas y world/local transform. Tolerancias de weld/normal son decisiones del source, no un teorema de watertightness.
- Límites de índices y tamaños antes del narrowing/reserva. Ver qué valida el caller antes de acusar un overflow explotable.
- UV/material por cara, región/GUID, nuevos interiores y bordes: seguir reconstrucción completa. numProp=3 solo prueba que el core mostrado transporta geometría.
- Undo/Redo, save/reopen, collider/nav/mesh cook, preview cancel y errores. Source y CMake no prueban calidad de runtime.

Éstos son gaps de aceptación y pruebas propuestas, no correcciones implementadas ni diagnósticos de bugs confirmados.

## 3. Auditoría local exigida antes de retirar dependencias

Generar para cada configuración y plataforma: targets directos/transitivos CMake, archivos/defines/includes, generated module lists, linker map/imports, libs/DLLs deployed, paquetes/URLs realmente resueltos. Recorrer create project, Editor startup, import, material/CSG, save/reopen, cook, launcher, cierre. Observar filesystem/network/endpoints, jobs y recursos que sobreviven.

Permitir dependencias internas licenciadas de Qt/Jolt/Luau/Manifold/Recast/codecs con pins y fuente/artefactos controlados. Excluir completamente AWS/multiplayer y backends no permitidos. No desactivar solo el UI del Gem mientras sigue un runtime module cargado.

Prueba offline consta de dos escenarios: (1) fuentes/paquetes completos en cache local auditada, clean configure/build/cook/run con red bloqueada; (2) quitar una dependencia del fixture de cache y comprobar error acotado, sin fallback de red. No borrar cache del usuario para simular el segundo caso.

Verificar LFS con inventario/objetos/hashes y materialización en destino autorizado; Git code tree no acredita payloads LFS. No copiar tokens o URLs temporales de API a informes. No afirmar SBOM completo hasta inventariar distribución efectiva y licencias/transitivas.

## 4. Prioridades operativas

P0: listas efectivas de packages/LFS sin AWS; cierre de cloud/multiplayer/extra renderer; baseline por pares y medición. P1: absorber WhiteBox/UI/Geometry con JE, preservar atributos y mostrar cuarto jugable. P2: material/cook/CSG+geología y reactividad de editor; profiling continuo. IPC/transporte se separa conforme sus consumidores se migran, sin conservar todo AzNetworking como excepción eterna.

Auditoría de nombres JE debe revisar source, generated headers, UUID/name-based IDs, serialization/asset metadata, CMake exports y binaries. Un grep no sustituye pruebas; un namespace alias no cumple objetivo.

## 5. Límites de lo realizado

No CMake configure/build, linker map, captura de red, engine runtime, benchmarks ni GPU/readers probados aquí. No eliminación de AWS/multiplayer, ningún namespace migrado en source ni Gem absorbido por este commit documental. Lectura focal en los archivos indicados, no auditoría exhaustiva de todos los archivos del motor. Codex debe registrar evidencia de esas operaciones antes de afirmar independencia o rendimiento.
