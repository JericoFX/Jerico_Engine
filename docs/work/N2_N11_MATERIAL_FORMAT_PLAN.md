# N2/N11 — materiales y formatos JE con DX12/Vulkan

**Revisión 2026-10-10.** Spec: GOAL, contrato JE, EDITOR_ASSET_WORLD_MASTER y ASSET_MATERIALS_FORMATS_EPAK_MASTER. Sin AWS/multiplayer ni plataforma de distribución online. Este plan reemplaza la versión Vulkan-only. N2 avanza temprano con BSP; N11 cierra capacidades de distribución, no posterga todos los formatos propios hasta el final.

## 1. Inventario del único pipeline

Leer consumers/source/targets de Asset Processor, SceneProcessing, AssetBundler, Atom RPI/MaterialEditor/Canvas/Shader. Fijar identity mapping, registry, recipes, cache, reflection, SRGs, variants y GPU lifetimes. Reutilizar la autoridad real migrada a JE, no crear cooker/registry paralelo.

Probar baseline material/import/cook/runtime y casos texture missing/material corrupt/shader invalid/source move. Tabla por requisito EXISTE/PARCIAL/AUSENTE/NO PROBADO con source exacto; un Gem instalado no equivale a implementación Jerico.

## 2. Materiales visibles en BSP y modelos

1. Tests RED donde falte contrato: nombres libres, ORM/separados, semánticas color/normal/UV/scalars, alpha, material slots, transform/handedness.
2. Adaptar metadata/Inspector/builder existentes y tipo/instance Atom a la API JE; no un nuevo shader PBR completo por capricho de nomenclatura.
3. Mostrar cuarto BSP con material por cara y GLB multi-slot. ORM R/AO G/roughness B/metal frente a mapas separados equivalentes. Normal Y/escala negativa/UV degenerada y source color-space documentados.
4. Undo/Redo/reimport de una textura/Save/cerrar/reopen; cancel/error conserva revision previa y overrides. Source files intactos.
5. Probar Windows DX12 y Vulkan, Linux Vulkan por separado; source-free cooked path y captura/timings/memoria por par. No aceptar uno como prueba de los tres.

## 3. Primeros formatos propios con consumidor

Congelar ADR de `.rmesh/.rmat/.rtext` cuando cada consumidor se integra, no todo catálogo sin usuarios. Magic/version/endian/chunks/counts/offsets/bounds/hash/typed dependencies, finite values, unsupported-required rejection y validator/dumper compartido. Writers/readers tests de roundtrip/determinismo/corrupción/overflow/migración y scene real.

Material Graph sobre tooling Atom adaptado a JE, sin otra VM/scheduler y sin compiler fuente en Shipping. Shader products/backend profiles guardan identidad correcta de DXIL/SPIR-V; no reutilizar binario de una API en la otra por compartir material GUID. Error de cook no activa material medio construido. Herramientas/functors privados se auditan sin crear segunda API gameplay.

Mips/tangent frames/normal convention/revisions correctos; .rtext significa textura, no .rtex adicional. Invalidation local de texture/material no recocina geometry/physics sin cambio dependiente. GPU fence/deferred release, audio/physics readers cuando corresponda, sin liberar una revision en uso.

Geología usa los mismos material/mesh/texture readers y campos semánticos; no variantes de formato incompatibles sólo para roca/cueva. Probar una estratificación que coincide con geometría y material, y FreshInterior tras corte, además del material PBR básico.

## 4. N11 paquetes locales y source-free

Auditar AssetBundler/VFS actuales, adaptarlos al contrato JE y `.epak` real, no cambiar extensión de .pak. Directorio final sin source/tools/compilers/checkout/AWS account, con recursos/notices necesarios. Source-free fixture con source inaccesible y red bloqueada.

Index/chunks/deps/platform/cooker/schema IDs/limits e integridad; streaming/random access y limites de descompresión. Políticas de Zstd/BLAKE3/firma/cifrado del master son decisiones de diseño a auditar/pinnear, no nuevas dependencias cloud ni crypto ya verificada. Bibliotecas locales permitidas, hash no sustituye autenticación y claves privadas no entran al cliente. No construir identidad online/entitlements/comercio para cargar contenido local.

Base+delta .epak con precedence/removals/compatibility, staged validation/atomic activation/rollback/interrupción/disk full/signature corruption. ContentPack/Campaign local conserva namespaces/GUID/save scope sin duplicar ejecutable ni añadir server/networking. No .epatch por comodidad. Compaction/rebase cuando esté justificado y preserve identidades.

## 5. Gates

Cada tarea termina con test/build/consumer/evidence y commit revisado en development. Tres pares gráficos diferenciados. Producto source-free no se acredita con running editor; material real no se acredita con codec solo; paquete firmado no se acredita con digest; patch rollback no se acredita por log que diga applied.

Review focus: semánticas ORM/normal, UV/material tras CSG, source path traversal/corruptos, chunk overflow/decompression limits, revision closure/hot reload GPU-safe, falta de cache sin red y ausencia AWS/multiplayer. Las rutas fuente actuales AZ/Gem son referencias de migración, no nombres finales del producto.
