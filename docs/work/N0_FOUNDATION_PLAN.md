# N0 — plan de baseline local JE y dos backends

**Goal:** base trazable y escena real que permita transformar BSP/UI sin AWS/multiplayer ni dependencia de descarga opaca.
**Spec:** GOAL, AGENTS, JE_IDENTITY_AND_NATIVE_MODULES y JE_DEPENDENCY_AUDIT.
**Estado:** IN PROGRESS; baseline original BLOCKED por paquetes locales ausentes. Ejecución y estados parciales en [N0_EXECUTION_2026_10_10](../evidence/n0/N0_EXECUTION_2026_10_10.md), ACTIVE_SLICE y HANDOFF. Los checks focales del resolver/CSG no cierran baseline gráfico.

## Restricciones

Una rama development, un checkout y un owner de build. No code/stash/reset/force/borrado de trabajo ajeno. Dos renderers: DX12 y Vulkan; Windows ambos y Linux Vulkan. No red de juego/server/cloud/AWS. Bibliotecas privadas locales licenciadas permitidas, no librerías inventadas para eludir O3DE. Namespace JE desde código nuevo; bridges privados O3DE sólo migración documentada. Geología activa, BSP primero, sin RT de render ni flotabilidad autorizada.

## Task 1 — estado real y cierre de dependencias permitido

- [x] Confirmar repo/rama/HEAD/status/remotos; fetch y pull ff-only seguro; registrar qué cambios ya existen. Base 35e4137, development, checkout inicial limpio, sin cambio remoto ni repos históricos afectados.
- [ ] Inventariar toolchain/SDK/driver/plataforma y packages/materialización LFS, con versiones/hashes/licencias y uso tool/runtime. No auto-instalar componentes privilegiados.
- [x] Usar DEP-01/02 para revisar `.lfsconfig`, package server defaults, variables de entorno/fallback y downloader S3. Resolver local implementado/probado con rutas con espacios, rechazos antes de I/O; LFS URL retirada y materialización excluida. No acredita cierre transitivo de todo engine.
- [ ] Rastrear targets cloud/multiplayer/server/extra RHI, sus aliases/auto-generated registries/package references. No eliminar carpetas mientras haya datos/users no clasificados.
- [ ] Seguir AzFramework→AzNetworking y consumidores locales para separar IPC necesario de transporte de juego. No inferir que un PUBLIC CMake edge implica todas las funciones presentes en binary; comprobar link/map después.
- [ ] Generar expediente de dependencia por producto/perfil, con EXPECTED ABSENT/NECESSARY PRIVATE/TOOLS ONLY/TEMPORARY MIGRATION/BLOCKED. Falta de paquete local debe dar error específico sin fallback de descarga.

Verificación: fuente/configuración/outputs del grafo, sin build declarado aún. No bloquear el análisis independiente por un paquete faltante. No «arreglar» falta de fuente enviando PAT al LFS heredado.

## Task 2 — configurar/compilar baseline admitido

- [x] Localizar presets/targets/flags del SHA real, no repetir comandos antiguos del README ni suponer APIs/nombres de launcher. Configure Windows VS17/2022 x64 protegido ejecutado; targets no generados por paquete ausente.
- [ ] Congelar entradas y ejecutar configure/build con logs/códigos de salida, lista efectiva de deps/paths/defines y despliegue. No reducir errores a warnings para afirmar éxito.
- [ ] Separar matriz Windows-DX12 / Windows-Vulkan / Linux-Vulkan. El sistema operativo ausente queda NOT TESTED. La misma configuración de engine puede elegir backend, pero cada selección se prueba realmente.
- [ ] Recoger linker map/imports y output files, consultar que no se seleccionó Null/otra API ni servidor de juego. Un shader o RHI source compilado no prueba presentación.

No cambiar todas las bibliotecas/Qt de rama sin fallo reproductible. Si se necesita corrección mínima de baseline, registrar RED/fix/GREEN y mantener source origin. No comenzar un rename general del engine para resolver un error local.

Resultado parcial Task 2: configure original exit 1 / 11,839 s, primer zlib-1.2.11-rev5-windows ausente; no build del engine. Inventarios existen, pero roles/licencias/grafo efectivos siguen pendientes. Sólo núcleo CSG std+Manifold local compilado/probado; no confundirlo con original engine.

## Task 3 — escena/editor/cook/runtime visibles

- [ ] Fixture sintético/licenciado con mesh PBR, luz/sombra/cámara, draw y frame final. Abrir editor, seleccionar/mover, Save/reopen y terminar procesos limpiamente.
- [ ] Verificar Asset Processor y cook de contenido, luego launcher por la ruta baseline real; registrar fuentes que todavía requiera. No llamar formato final Jerico a un producto nativo O3DE.
- [ ] Capturar imagen real con backend/version/settings y timings CPU/GPU/peak/resident memory, cold versus steady. P50/p95/p99 si el muestreo lo permite; no FPS de presentación inferidos de timers.
- [ ] Probar paquete/fuente inválidos y ruta de error; entrada desconocida no borra proyecto ni sobrescribe artefactos aceptados.
- [ ] Recorrer editor/cook/run con red bloqueada y cache fuente completa; documentar cualquier conexión intentada. Prueba cache incompleta en copia de fixture, no borrando cache del usuario: error sin AWS/fallback.

Este baseline no acredita que todo AZ ya migró ni que todas las capabilities Gem ya se absorbieron. Marcar deuda concreta, no PRODUCT PASS.

## Task 4 — preparar N1 con source audit limitado a BSP

- [ ] Leer WhiteBox tool API/CSG/component modes/asset handlers/undo y sus dependencias. DEP-05..08 ofrece puntos de partida reales, no copia automática.
- [ ] Especificar operación completa de cuarto+apertura puerta/ventana con UV/material/region IDs, transform/tolerances, resultado vacío válido y failure rollback.
- [ ] Localizar funciones existentes de viewport/transactions/cook; nombres reales y tests. Plan de JE::BSP/Geometry dentro del engine, targets/generated files/resources y UUIDs afectados.
- [ ] Definir captura antes/después y coste de edición/reconstrucción, test Undo/save/reopen/runtime y progreso visible. No primer slice de clase vacía.
- [ ] Handoff actual con evidencia lograda y límites; pasar a N1 en el par disponible sin inventar PASS en los otros.

## Review focus

Paquete faltante debe fallar offline; entrada corrompida no cambia asset activo; scene/reference identities sobreviven rename; teardown limpia listeners/jobs/resources; backend selector no cae a Null/otro. Cada uno se verifica en el task que lo produce. Docs/source inspection no cuentan como ejecución de esas verificaciones.

Preparación Task 4 ejecutada con alcance focal: revisión productores/consumidores CSG y migración real de núcleo a JE::BSP; regressions RED/GREEN y fixture habitación/corredor/puerta/ventana. Sigue pendiente aceptación de editor, UV/material/region IDs, teardown vacío/Undo/save/reopen/cook/runtime, absorción completa de Gem y evidencia visual/costes. No marcar esos checks completos por geometría aislada.
