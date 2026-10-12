# Agentes — JericoEngine / JE

**Directiva vigente 2026-10-10, posterior a a536fb7.** Leer GOAL, CODEX_START, contrato JE, ROADMAP y ACTIVE_SLICE antes de editar. Todo scope local, incluidos Gems, respeta este contrato. Los requisitos concretos del propietario prevalecen sobre skills y defaults de agentes.

## Un checkout, una rama, un integrador

Única rama de escritura/publicación: `development`, repo `JericoFX/Jerico_Engine`. No nuevos branches/worktrees/forks/engines; otras ramas solo lectura, sin merge masivo ni borrado. No modificar JericoEngine2/JericoEngine/Playground históricos. Un writer por archivo y un owner por build compartido; congelar entradas durante pruebas.

Última directiva aportada por el propietario (2026-10-11), que sustituye Astra-only: padre GPT-6.1 Sol con razonamiento high; dos escritores GPT-6.1 Sol high con scopes disjuntos asignados por el padre. Conservar agentes inactivos y trabajo existente; no tercer escritor, ramas, worktrees, forks ni padre sustituto. El padre coordina arquitectura, integración, revisión, verificación final, aceptación y publicación exclusivamente en development. Un único owner de builds/tests compartidos, con inputs relevantes congelados; sus cambios invalidan la evidencia afectada. Luna high sólo ayuda con investigación/revisión y compilación/validación expresamente asignada; puede escribir únicamente outputs/logs/evidencia de esa asignación, nunca fuente/docs/config/fixes/Git ni aceptación, y no delega. No afirmar cambios de modelo/rol efectivos sin evidencia del entorno.

## Reglas de producto que sustituyen las anteriores

- JE es la nomenclatura propia: C++ `JE::` con subnamespaces, CMake `JE::`, macros `JE_`, rutas/targets/diagnósticos propios coherentes. Migrar también internals adaptados de O3DE; no dejar `AZ` para siempre detrás de alias/fachada. Preservar nombres genuinos de SDKs/bibliotecas privadas y avisos. Detalles en contrato JE.
- Solo DX12 y Vulkan: Windows ambos, Linux Vulkan. No llamar DX12 diferido. Rendering RT, Metal/WebGPU/DX11/OpenGL/móvil siguen fuera. Null tests/headless no es renderer permitido de producto.
- AWS y multiplayer EXCLUIDOS, no opcionales ni deferred. No server/matchmaking/replication/cloud identity/telemetry/services. Auditar bootstrap/cook/deployment y no solo lista de Gems.
- Geología REACTIVADA. No obedecer cancelación integral anterior. BSP→OpenVDB sigue cancelado; no confundir reactivar herramientas con reactivar backend descartado.
- BSP/UI/materiales visibles son el primer camino. No postergar geometría hasta terminar audio/IA/packaging avanzado. Resto de capacidades existentes se conserva sin añadir plataforma online.
- Integración de Gems dentro del engine con implementación real, fuentes/builders/resources/registro y todos los consumidores; no renombre de directorios ni mega-Gem que oculta O3DE. Registro heredado solo temporal y con retiro medible.
- Ideas de otros motores permitidas. Fuente/código/asset/licencia por separado; prohibido trasladar Cry/UE propietario sin permiso o llamarlo clean-room sin evidencia. Conservar atribuciones.

## Método

Think Before Coding, Simplicity First, Surgical Changes, Goal-Driven Execution, Integration Before Abstraction. Leer implementación/pins/producers/consumers primero. Un World/identity/dependency graph/transaction stack/physics owner/VM de gameplay/render scheduler; no duplicar para satisfacer un diseño nominal.

Hacer verticales con resultado observable: editar BSP → aplicar material → Undo/Save/reopen → cook → runtime, por ejemplo. Los ajustes mínimos de dependencias necesarios se incluyen en el mismo vertical, con explicación; no bloquear por Float32/registry/header sin consumidor. No convertir cada private struct en milestone. Tras un vertical verificado, publicar y continuar con el siguiente no bloqueado.

Namespace migration es semántica, no replace global: inventario de namespace/type collisions/macros/CMake/codegen/UUID/reflection/serialización/ABI, migración conjunta de escritores/lectores y bridge temporal con owner/removal test. Una fachada `namespace JE = AZ` no es trabajo terminado. No renombrar Vulkan/DirectX/Qt/Jolt/Luau para ocultar dependencias legítimas. APIs del juego/formatos no exponen tipos internos de esas bibliotecas.

Geología debe respetar espacio jugable, planos analíticos, material lineage y datos de geometría compartidos con shading. No aceptar una textura de roca como prueba de topología, ni un campo SDF como prueba de collision/nav/cook.

## Dependencias y operación offline

Inventario con uso source/tool/build/runtime, SHA/hash/license, transitive graph, locations/URLs, downloads y owner. Usar código/paquetes locales controlados; no descargar desde AWS/S3/CloudFront ni catálogos Gem. Configuración faltante falla explícitamente. No mandar tokens o contenido privado a endpoints heredados. Sin secretos en logs/docs.

Comunicación IPC local de herramientas no equivale a multiplayer: seguir consumidores de AzNetworking, extraer el mínimo a JE Tools/IPC o reutilizar IPC adecuado existente. No conservar OpenSSL/RapidXML/network stack productivo por comodidad; tampoco eliminarlos sin rastrear otros usuarios legítimos. Shipping no lleva listeners/remote asset IO salvo necesidad local expresamente documentada; no abrir servicios públicos.

Qt y otras bibliotecas requieren licencia/linking/redistribución propios. Integrado no exige static link de todo. Sin Autodesk FBX SDK/ATL/MFC impuestos. No auto-instalar software privilegiado ni cambiar seguridad de driver. No certificar dependencia ausente por grep de nombres ni legalidad por README.

## Git y publicación

Antes de trabajar y antes de publicar: confirmar repo/remotos/rama/HEAD, `git status --short`, `git fetch origin`. En árbol limpio usar `git pull --ff-only origin development`; preservar trabajo ajeno y reconciliar movimiento remoto, sin auto-stash/reset/force. Staging por rutas propias y diff revisado. Cada push acompañado de sincronización previa y comprobación remota posterior. Nunca `push --force`, mirror, `git clean` ni borrados indiscriminados.

Al publicar mediante API Git, leer HEAD nuevo, crear commit con parent correcto y mover solo development con `force=false` y expected SHA; rechazar/reconciliar carrera, no sobrescribir. No afirmar que se ejecutó git pull local al usar API.

## Evidencia útil, continua y proporcionada

Antes de diagnóstico consultar ENGINE_DEBUGGING_BOOK y volver al primer output defectuoso con inputs reales. Captura original del viewport/render; UI mock no prueba producto. RED/GREEN discriminante, sin tolerancias relajadas para salvar fallo. No miles de checks triviales en lugar de pruebas de material, BSP, interacción y runtime.

En cada checkpoint visible: escena/cámara/settings/build/backend, captura antes/después, tiempos CPU/GPU p50/p95/p99, memoria y alcance. Instrumentación apagada no añade readback por frame. Windows DX12, Windows Vulkan y Linux Vulkan son estados separados. Headless no acredita render y desktop stereo no acredita headset. No reclamar velocidad a partir de menos código o de un paper.

Después: actualizar HANDOFF/ACTIVE_SLICE y el requisito probado. Evidencia detallada por vertical, lecciones en error book; no diarios repetidos en todos los masters. Docs-only puede verificarse como documentación sin compilar el engine; jamás PRODUCT PASS. Bloqueo nombra fallo/permiso/paquete y próximos trabajos independientes.

Buoyancy continúa DESIGN ONLY. Rendering RT/underwater y capacidades históricas diferidas no se activan por reordenar el roadmap. Multiplayer es excluido incluso si un master anterior decía deferred. No borrar evidencia anterior ni acreditar O3DE con PASS de Cry.
