# N2 / N11 — ejecución de materiales, formatos y empaquetado Jerico

**Estado:** IMPLEMENTATION PLAN / NO PRODUCT CODE YET. **Rama:** development. **Spec:** docs/ASSET_MATERIALS_FORMATS_EPAK_MASTER.md, GOAL.md, docs/EDITOR_ASSET_WORLD_MASTER.md, docs/legal/PROVENANCE_AND_DEPENDENCIES.md. **Owners:** uno para Asset Processor/IR/cook, uno para Atom/material/graph y un integrador central según AGENTS.

## Global constraints

- No iniciar N2 antes de obtener baseline N0 funcional en el sistema operativo probado. Mantener Linux/Windows con PASS independientes.
- Jerico no crea otro engine ni otro scheduler, registry, VFS, cooker, Asset Processor, Material Graph VM o RenderGraph. Trabajar en rutas de O3DE existentes.
- Integración nativa implica configuración por engine; proyectos no instalan Gems Jerico manualmente. No borrar los Gems fuente al principio.
- El target de Shipping nunca usa fuentes glTF/FBX/KTX2/Shader, Editor/Tools, Remote Shader Compiler ni paquetes de desarrollo.
- Lo escrito en plan y tests no acredita que la versión .rmat/.rtext/.epak ya exista. Sin commit probado del reader+consumidor, estado PLANNED.
- Pinning, procedencia y licencias antes de nuevos deps; no importar CryEngine 5.7. No activar flotabilidad ni otros DESIGN ONLY.

## Etapa N2-A — inventario del pipeline real

1. Leer y diagramar los consumidores reales de Code/Tools/AssetProcessor, Asset Bundler, Gems/SceneProcessing, Atom RPI, Atom Feature/Common/Material, Atom Asset/Shader, Atom Tools/MaterialEditor y MaterialCanvas. Obtener targets CMake, init/shutdown, builders, asset IDs, hashes, cache y lifetime GPU.
2. Probar con un proyecto O3DE limpio creación/edición/guardado/cook de un material PBR original. Registrar archivo fuente, product ID y bytes/formatos cocinados, shader product/variant y ruta real hasta Vulkan.
3. Probar casos negativos baseline: textura faltante, material corrupto, shader inválido y source move/rename; registrar qué protege O3DE realmente y dónde falla el contrato final.
4. Publicar una tabla por requisito JER-011..016/JER-084..093: EXISTE, PARCIAL, AUSENTE, BLOQUEADO y OWNER; coste estimado basado en consumidor/código real. No crear nombres de nuevas clases sin encontrar interfaces reales.

**Entregable:** informe de grafo/consumer + reproducción/editor screenshot + build/cook/launcher logs con HEAD/toolchain/versions. Sin refactor todavía.

## Etapa N2-B — semántica de materiales y primer consumidor

1. Añadir tests de import metadata: nombres arbitrarios, entradas ORM AO/Roughness/Metallic, entradas separadas, conversión color-space, normal Y, scalar, UV set, tangent/handedness y material slots. Comenzar con control que falle en el punto realmente ausente.
2. Adaptar el menor tramo real en SceneProcessing/AssetBuilder/import metadata/Material Editor para que el autor pueda seleccionar mapa/semántica/convención de forma explícita; test unitario de persistencia.
3. Conectar la revisión al tipo/instance actual de Atom con material/texture resources ya existentes. No implementar un shader PBR alternativo por convenio de nombres.
4. Probar GLB con dos materiales, normal y ORM; luego misma semántica por mapas separados. Guardar, cerrar editor, reabrir, reimportar una textura y comparar resultado/rutas. Ensayar UV degenerada, escala negativa, alpha mask/blend y missing map.
5. Medir coste real de edición/reimport en CPU, RAM y VRAM; no prometer hot reload sin medir retiro de descriptores/recursos.

**Entregable:** una escena Jerico de materiales editables que corre por Atom/Vulkan y conserva metadata/overrides. Commit singular con test RED/GREEN, chef/cook y prueba runtime del consumidor. Mantener compatibilidad de editor original.

## Etapa N2-C — shader graph y productos rtext/rmat/rmesh

1. Congelar por ADR los wire layouts de los primeros formatos que vayan a tener un consumidor real (NO todos los formatos a la vez): IDs/versiones/chunks/endianness/flags/hashes, límites y relaciones source/product.
2. Implementar el reader+writer+validator+dump mínimo como adaptación del pipeline de O3DE y conectar en una única autoridad de asset IDs. Probar corrupción/overflow/missing dependency/datos NaN y reconstrucción determinista.
3. Prioridad de formato: .rtext y .rmat para material, .rmesh para una escena que lo consuma. No cambiar extensión de un asset original O3DE sin reader real.
4. Material Graph usa Atom MaterialCanvas/MaterialEditor y Shader Builder existentes; cocinar a material/producto Jerico y variantes del shader pipeline original. Controlar explosión de permutaciones y fallos visible en panel, sin Shader Compiler en runtime.
5. Hot reload incremental: reimport solo una textura/mat y verificar que escena/prefab/collider no recocinan innecesariamente; cancel/errores revierte selector y garantiza GPU-safe retirement. Construir benchmarks con muchas texturas y un mesh pesado.

**Entregable:** reader/validator en Editor y Runtime, fuente retirada en prueba aislada, material visible. No declarar N2 PASS si solo funcionan codecs y no import/reimport/Undo/cook/run.

## Etapa N11-A — diseño firmado y licencias de formato/paquete

1. Auditar O3DE Asset Bundler y VFS con sus índices/chunks/streaming/compression/signature support. Registrar qué se puede extender sin segunda autoridad.
2. Comparar Zstd/BLAKE3/libsodium con alternativas ya incluidas y paquetes de 3P. Seleccionar versión/hash/licencia, políticas de seguridad y costos; si una licencia o soporte no sirve, documentar alternativa con gate.
3. Congelar .epak por ADR: IDs, chunk index/order, deps, hashes, compression IDs, límites de read/decompression, compatibilidad/migrations, claves/signature/crypto policy y threat model. No cifrar datos arbitrarios ni introducir claves secretas permanentes en el cliente sin estrategia.
4. Preparar vectores independientes de verificación: firma válida, manipulación de un chunk, firma falsa, firma de otro proyecto, key rotation/revocation, paquete truncado/bomba de descompresión, rutas inválidas.
5. Generar un plan de rollback y de migración del packaging O3DE: no renombrar simplemente .pak→.epak.

**Entregable:** ADR aprobado y suite negativa reproducible. No publicar paquetes supuestamente protegidos si no existe la validación efectiva.

## Etapa N11-B — runtime y actualizaciones transaccionales

1. Construir el mismo contenido bajo uno o varios .epak usando Asset Bundler/VFS existente; source-free launcher desde directorio aislado sin el checkout.
2. Indexar y validar chunk hashes, dependencias, orden de overlays y firmas antes de que un producto activo cambie. Runtime no puede montar paquete corrupto en modo parcial.
3. Generar base.epak + delta.epak que cambie solo .rtext/.rmat, agregue y elimine assets con política documentada. Ensayar desconexión, disco lleno y cierre del proceso entre escribir datos y activar índice; rollback mantiene versión anterior.
4. Instalar un ContentPack/DLC con identidades/deps propias y recursos compartidos; CampaignRegistry selecciona experiencia sin nueva DLL ni duplicar ejecutable. Saves por CampaignId, con rechazo o migración de incompatibles.
5. Verificar compaction/rebase de deltas, paths/layout final Jerico y símbolos/legal notices de terceros; documentar firma real y cifrado configurable separado. Prueba offline sin AWS, sin compiladores ni assets de autoría.
6. Medir throughput, latencia de mount/read/update, memoria descomprimida pico y costos de contenido largo; un tamaño de archive pequeño no acredita streaming.

**Entregable:** paquete final reproducible, validación negativa, actualizaciones con rollback, runtime source-free y reporte legal/SBOM. N11 puede admitir subetapas, pero ningún STATUS puede saltar dependencias.

## Gates de review comunes

Un revisor debe comprobar antes de integrar:
- Input falso/manipulado (path traversal, file size, chunk overlap, hash/signature mismatch) no llega a consumer.
- Un cambio de .rtext no corrompe .rmat/.rmesh ni descarta la revisión anterior durante reload.
- Valores ORM/separados no se mezclan bajo sRGB accidental ni se intercambian G/B de roughness/metallic.
- Material/alpha normal maps funcionan con UV, escala negativa, tangentes y perfiles de plataforma.
- No hay segunda autoridad, compatibilidad de scene/prefab, ni runtime cargando fuente o ejecutando tool Lua.
- Invalidación/cancel/cold load/p95 perf, compilación/render y continuidad UI cuentan con evidencia.
- Windows y Linux, y modo Release/Shipping, mantienen estados separados si un sistema no se probó.

## Estado de transferencia desde Cry

La implementación del anterior JericoEngine2 y sus test gates pertenecen a su commit histórico, no a esta rama. Usar requisitos y oráculos observables, no portar shaders/modelos/algoritmos protegidos por su nombre. Probar versiones nuevas de r* y .epak contra tests independientes, sin conservar ABI Cry por accidente.

## Rutas de referencia y puntos de decisión

Antes de editar, Codex debe localizar interfaces reales del SHA actual dentro de:
- Code/Tools/AssetProcessor; Code/Tools/AssetBundler;
- Gems/SceneProcessing;
- Gems/Atom/Asset/Shader, Gems/Atom/RPI, Gems/Atom/Feature/Common;
- Gems/Atom/Tools/MaterialEditor, Gems/Atom/Tools/MaterialCanvas;
- Code/Framework/AzCore, AzToolsFramework y VFS/launcher usados por el producto.

Las rutas concretas pueden cambiar en upstream; registrar symbol/target actual antes de fijar firmas. No inventar C++ APIs por similitud con Cry. Cada tarea relevante empieza con test que falla y termina con build/test/consumer real, diff revisado, commit y fast-forward seguro en development.
