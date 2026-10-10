# Contrato JE: identidad real y módulos nativos

**Vigente 2026-10-10. Diseño obligatorio; no migración ejecutada.** GOAL/AGENTS son superiores. Este contrato resuelve las ambigüedades de nomenclatura, Gems, dependencias y dos backends.

## 1. Namespace y nomenclatura final

| Superficie | Convención |
|---|---|
| C++ propio/adaptado | `JE::Core`, `JE::World`, `JE::Assets`, `JE::Render`, `JE::Editor`, `JE::BSP`, `JE::Geometry`, `JE::Geology`, `JE::Physics`, `JE::Script`, `JE::Animation`, `JE::Audio`, `JE::UI`, `JE::AI` y subdominios necesarios |
| Targets propios CMake | `JE::Core`, `JE::Render`, `JE::RenderDX12`, `JE::RenderVulkan`, `JE::Geometry`, `JE::Editor`, etc. Un target no exige un DLL por nombre |
| Macros/export/codegen/identificadores planos | Prefijo `JE_`; nombres válidos del lenguaje, no `::` en macros o paths |
| Rutas propias destino | `Engine/Core`, `Engine/World`, `Engine/Assets`, `Engine/Render/{DX12,Vulkan}`, `Engine/Geometry`, `Engine/Geology`, `Engine/Physics`, `Editor`, `Tools` según ownership |
| Producto/UX | JericoEngine/JericoEditor, nombres legibles; APIs y herramientas propias identificadas JE |
| Formatos | `.rmesh/.rmat/.rtext/.rskel/.rrig/.ranim/.rui/.epak` y catálogo existente, NO renombrarlos por adoptar JE:: |

Nombres orientativos de subdominios/targets se ajustan al ownership real para evitar colisiones; `JE::` raíz y eliminación de identidad propia AZ son obligatorias. No aplanar AZ/AzFramework/RPI/RHI/AZStd a un único scope con conflictos.

El código O3DE que pasa a ser implementación propia se migra realmente: headers/includes, namespaces, targets, exports, macros, forward declarations, plantillas, generated code, reflection strings, diagnóstico, paths y package discovery. No considerar terminado `namespace JE = AZ`, `using AZ::Entity` en headers públicos ni `JE::Physics` llamando eternamente a un segundo owner antiguo.

Excepciones limitadas: APIs oficiales de DirectX/Vulkan/Win32/Qt/Jolt/Luau y otras bibliotecas privadas; `std::`; source de terceros conservado sin modificar; LICENSE/NOTICE/SPDX/créditos, historial y metadatos de procedencia; import/conversores legacy delimitados. No existe excepción global «todo O3DE es tercero»: el código del engine que adoptamos debe converger a JE. No falsificar autoría renombrando dependencias ni borrar atribuciones.

## 2. Migración segura, no cosmética

Por subsistema mantener un manifiesto de nombres: símbolo viejo→nuevo, archivo/target dueño, tipo/UUID, consumidores, generadores, persisted keys y motivo de compatibilidad. Identificar explicit UUIDs, IDs generados del nombre y serializers; no asumir que renombrar una clase conserva identidad si codegen usa el string. No cambiar UUID arbitrariamente para que desaparezca AZ. Si cambia semántica, versionar y convertir datos.

Orden: pruebas de referencia y serialize/cook → declarations/definitions/generadores → todos los consumidores → init/shutdown y editor/runtime → retirar aliases. Bridges temporales privados, inventariados y no exportados al juego. Cada bridge tiene owner, consumidores restantes y prueba de retirada; no un registro perpetuo sin trabajo.

Probar build completo no-unity para descubrir includes indirectos cuando se toquen headers centrales; configs/exports/ABI, Qt moc/uic y bindings/scripts, native/serialized type resolution, prefab/asset references, source-free package y save migrations. Rehacer caches afectados con recipe/version, no borrar caché indiscriminadamente ni declarar compatibilidad por compilar.

No posponer toda identidad a N11: todo código nuevo usa JE desde N1; cada sistema absorbido migra sus nombres con sus consumidores. N11 cierra remanentes transversales. Tampoco bloquear el primer viewport por intentar un rename de millones de líneas de golpe.

## 3. Absorber Gems

Registro de destino por Gem: nombre/origen/commit/licencia, source path, targets API/runtime/editor/builder/tests, recursos/registry/seed lists, servicios e inicialización, auto-generated module lists, downstream references, tipo de linkage y ruta nueva dentro del engine.

Transición: conservar implementation→linkar como biblioteca interna del owner JE→registrar componentes/builders una vez desde composición del engine→migrar assets/settings y usuarios→quitar project gem activation/aliases/catalog→retirar carpeta/ruta Gem vieja cuando no tenga consumidores.

Final: crear proyecto no requiere instalar/activar Gems; código y assets de capacidades requeridas viven bajo owners del engine; no Gem Manager, marketplace ni descarga al abrir proyecto. Bibliotecas internas y DLLs útiles sí; plugins necesarios de Qt o loader gráfico no son Gems de funcionalidades Jerico. No confundir integración nativa con static link obligatorio o desordenar licencias Qt.

White Box se integra en Geometry/BSP, no un `JEWhiteBoxGem`; Atom en Render con dos backends, no un segundo renderer; GraphCanvas/GraphModel en framework editor, no otra VM; Recast en Navigation, sin conservar interfaces Gem públicas; EMotionFX/Maestro/Audio/UI se absorben con iguales reglas. Nunca mover carpetas antes de saber sus builders/resources y serialización.

## 4. Exclusiones comprobables

**AWS:** excluir SDKs/servicios/downloader S3, cuentas/tokens/telemetría AWS, endpoints runtime y fallback de paquetes/LFS. Fuente de .lfsconfig/3rdPartyPackages está auditada por separado. Datos/paquetes necesarios deben estar materializados en almacenamiento controlado y permitido; no mandar privados al endpoint heredado.

**Multiplayer:** no Gem Multiplayer/MultiplayerCompression, dedicated/unified game-server targets, Session/matchmaking/replication/backend/online account system ni dependencias de shipping para ellos. Conservar símbolos necesarios para datos históricos solo en import/test acotado, no producto. Renombrar Multiplayer a JE no lo admite.

**Renders:** build productivo permite DX12/Vulkan únicamente. Probar selección soportada, error claro para no soportada y ausencia de fallback silencioso a Metal/WebGPU/DX11/OpenGL. Null es test/headless y no se ofrece en UI productiva; no empaquetar código headless de test como renderer. Windows debe probar ambos; Linux solo Vulkan, no exigir DX12 en Linux.

IPC local para Asset Processor/editor/debug/MCP se separa de red de juego. Preferir la ruta local mínima, sólo en tools; capturar listeners y destino. Un vínculo CMake a AzNetworking requiere seguir consumidores y enlazado real; no demuestra que el Gem Multiplayer esté activo, pero impide declarar el build desacoplado sin revisión.

## 5. Gates

IDENTITY: scan de source/generated/public headers/targets/exported symbols/settings/deployment bajo own-code scope no tiene prefijos prohibidos sin excepción por ruta revisada. Un scan textual no demuestra funcionalidad; se acompaña de builds/serialización/editor/runtime.

NATIVE: proyecto limpio usa BSP/material/script/physics presentes sin Gem install/enable; un lifecycle y dependency graph real, assets/builders disponibles y offline. No alcanza cambiar display_name.

DEPENDENCY: grafo CMake efectivo + linker map/imports + deployment manifest + captura de filesystem/network en startup/edit/cook/play/package, con conexión ausente y caché/fuentes locales controladas. Build cold con cache completa y red bloqueada, más caso paquete faltante que falla sin descarga. No hay AWS/multiplayer/renderer no admitido en el cierre productivo.

VISUAL: cada tramo de migración conserva prueba del caso BSP/material/geología relevante en al menos su par backend/plataforma de trabajo y mantiene los otros gates pendientes explícitos. No aprobar la portabilidad completa ni el final JE por un único viewport.
