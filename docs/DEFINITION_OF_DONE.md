# JE — Definition of Done

Vigente con GOAL/AGENTS del 2026-10-10. **Documentación, source inspection, implementación e integración/ejecución son estados distintos.** Un commit docs-only no necesita compilar engine para probar su alcance; tampoco acredita producto.

## Estados

CONTRACT ADOPTED/PLANNED, DESIGN ONLY, IMPLEMENTED NOT INTEGRATED, INTEGRATED NOT VERIFIED, VERIFIED con alcance, NOT TESTED/BLOCKED preciso, DEFERRED/EXCLUDED con autoridad. AWS/multiplayer son EXCLUDED; geología ACTIVE; DX12 y Vulkan requeridos. No esconder esas decisiones bajo un antiguo estado deferred.

## Gate de un vertical

Owner JE y único producer/consumer, integración canónica real, source/targets/data/initialization coherentes; entrada inválida/cancel/teardown probados. Editor y Undo/save/reopen/prefab/cook/runtime según feature, no sólo header/codec/demo externa. Evidencia reproducible con commit/commands/output/hardware/backend/scene/settings/imagen/timing/memoria/limitaciones. API o plataforma no probada sigue pendiente.

Pares gráficos: Windows-DX12, Windows-Vulkan, Linux-Vulkan. Un par VERIFIED permite progreso local, no PASS de los demás. Null/headless no prueba render; mock/stereo desktop no prueba visor. Rendering RT no se admite por usar DX12.

## Identidad y nativización

Nuevo código propio JE::/targets JE::/macros JE_. Source O3DE adoptado converge realmente a JE con consumidores/codegen/UUID/serializers/assets; aliases sólo bridges privados con retirada. No JE=AZ como cierre. Fuente/headers/generados/export symbols/config/package se auditan con excepciones explícitas por path para bibliotecas genuinas y avisos legales, no exención general de internals O3DE.

Proyecto limpio usa capacidades integradas sin instalar/activar Gems ni catálogo remoto. Startup/shutdown una vez, assets/builders/versioning intactos. Final owners dentro del engine y sin autoridades Gem paralelas. Mover carpetas o labels no basta.

## Dependencias

Grafo CMake efectivo + enlace/imports + deployment + ejecución/captura de red prueban ausencia AWS/multiplayer/servers y backends no permitidos. Sólo DX12/Vulkan como render producto. Dependencias privadas licenciadas/pinneadas/locales permitidas; no external service. IPC mínimo de tools no justifica shipping de stack multiplayer. Cache completa offline y caso faltante sin auto-download/fallback; no borrar cache del usuario para testear.

SBOM/notices/licencias/linking/redistribución y procedencia por incorporación. Código propietario ajeno no se vuelve permitido por renombrar/traducir. Ideas/papers se contrastan con caso útil y licencia de código separada.

## BSP, geología, materiales y formatos

BSP prueba operaciones estructurales reales, UV/material por cara/region lineage, resultados vacíos y errores, topología/colisión/cook/Undo. Geología prueba roca/familia/cliff/overhang/cueva, plano analítico donde corresponde, estratos/erosión/fracturas/materiales concordantes, espacio jugable/locks, cancel/invalidation local y coste. No basta ruido o screenshot de otro engine.

ORM/separados equivalentes bajo mismas semánticas, color-space/normal Y/UV/tangents/alpha/mips y reimport/overrides. Cada formato r* VERIFIED tiene reader/writer/validator/version/bounds/dependencies/corruption tests y consumidor real. Formatos no son renombres de assets O3DE. .epak source-free, chunk/closure/streaming/overlay/rollback y crypto solamente cuando implementados/testeados; digest no acredita firma.

## Progreso visible y aceptación global

Cada vertical UI/BSP/render/geología muestra resultado real antes/después y medidas proporcionadas. Optimización compara mismo contenido/input/preset, coste CPU/GPU/memoria y calidad, sin relajar tolerancias ni usar conteo de checks como calidad. Revisar p95/p99/latencia y cold/steady.

GOAL global cierra sólo con capacidades obligatorias/identidad/exclusiones/pairs/formatos/source-free/evidencia y sin owner duplicado, o un cambio explícito de alcance del propietario. Mantener deferred/design-only visibles; no reabrir flotabilidad ni RT/geología→VDB al implementar otras familias.
