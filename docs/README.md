# Documentación vigente — JericoEngine / JE

**Vigente 2026-10-15: una rama development; JE::; sin AWS/multiplayer; sólo Vulkan activo por ahora, DX12 preservado e inactivo; BSP/UI primero, geología y VR/OpenXR activos.** [N0 ejecutado](evidence/n0/N0_EXECUTION_2026_10_10.md) incluye resolver offline, paquetes locales/consumers y núcleo BSP con UV/superficies verificados. Baseline completo, editor visible y runtime siguen pendientes.

## Nombres públicos de herramientas

En JericoEditor y explicaciones públicas usamos nombres fáciles de recordar: **Jerico Assembly** (PWAS: coloca y ensambla), **Jerico Surface & Damage** (EPLS: envejece y solicita daño geométrico), **Jerico Brush** (RBrush: pinta materiales/máscaras/daño) y **Jerico Modular** (RMod: fabrica piezas y conexiones modulares). Las siglas y nombres de archivos actuales son alias técnicos de compatibilidad; no se duplican los módulos ni se renombran UUID, recursos o formatos sin migración probada.

| Documento | Uso |
|---|---|
| [GOAL](../GOAL.md) | Última decisión del propietario, capacidades y exclusiones |
| [AGENTS](../AGENTS.md) | Roles, Git, método y evidencia |
| [CODEX_START](../CODEX_START.md) | Inicio seguro y prioridad visible |
| [Motor ligero y Jerico Assembly/Surface & Damage](JE_LEAN_ENGINE_AND_AUTHORING.md) | Reducir peso sin recortar herramientas; Studio, PWAS/EPLS, cook y demos integradas |
| [Jerico Assembly (PWAS)](PWAS_TECHNICAL_MASTER.md) | Colocación y ensamblado de objetos, reglas deterministas, máscaras/locks/overrides, IDs, UI y cook |
| [Jerico Surface & Damage (EPLS)](EPLS_TECHNICAL_MASTER.md) | Envejecimiento, materiales, máscaras, solicitud de astillas/agujeros/fracturas y pruebas EPLS |
| [Playground → JE](PLAYGROUND_TO_JE_INTEGRATION_MASTER.md) | Inventario fijado, evidencia real, ownership y migración por verticales sin duplicar engine |
| [Contrato JE](architecture/JE_IDENTITY_AND_NATIVE_MODULES.md) | Namespace real, CMake/codegen/UUID, Gems dentro del engine |
| [Arquitectura](architecture/O3DE_NATIVE_TRANSFORMATION.md) | Owners y disposición de capacidades |
| [Auditoría de dependencias](research/JE_DEPENDENCY_AUDIT_2026_10_10.md) | Hallazgos DEP-01..12 con source/blobs y límites |
| [Licencia Jerico GPLv3](../LICENSE_POLICY.md) | Alcance GPL-3.0-only y conservación de licencias originales de O3DE y terceros |
| [Legal/dependencias](legal/PROVENANCE_AND_DEPENDENCIES.md) | Licencias, bibliotecas privadas, offline y exclusiones |
| [Checklist legal O3DE/terceros](legal/O3DE_LICENSE_COMPLIANCE_CHECKLIST_2026_10_13.md) | LIC-01…07: atribuciones, Gems, LGPL/Qt, assets, avisos y bloqueo de release |
| [Roadmap](ROADMAP.md) | N1/N2/N4/N8 adelantados, resto de capacidades conservadas |
| [Matriz](REQUIREMENTS_MATRIX.md) | IDs JER, gates y estados, sin progreso ficticio |
| [Definition of Done](DEFINITION_OF_DONE.md) | Qué cuenta como integrado/visible/seguro |
| [Editor/assets/world](EDITOR_ASSET_WORLD_MASTER.md) | UX/transacciones/identidad/asset pipeline |
| [Materiales/formatos/epak](ASSET_MATERIALS_FORMATS_EPAK_MASTER.md) | PBR/ORM/r*/cooked packages locales |
| [BSP/geología/procedural/física](PROCEDURAL_PHYSICS_ENVIRONMENT_MASTER.md) | Geometry/Geology/RMod/RBrush/PWAS/Jolt |
| [Renderer/SIGGRAPH](RENDERING_RASTER_SIGGRAPH_MASTER.md) | Vulkan activo, fuente de ideas y mejoras medibles |
| [Scripting/AI/gameplay](SCRIPTING_AI_GAMEPLAY_MASTER.md) | Luau/Store/HTN/ref/input/save locales |
| [Motion/UI/audio/XR](ANIMATION_UI_AUDIO_XR_MASTER.md) | Rig/IK/Fit/sequences/UI/audio/VFX/OpenXR |
| [Perfiles](PRESENTATION_PROFILES_MASTER.md) | 2D/2.5D/isométrico/vehículos sin engines paralelos |
| [Error book](ENGINE_DEBUGGING_BOOK.md) | Lecciones históricas, no fallos actuales inferidos |
| [Auditoría de ramas](research/BRANCH_AUDIT_2026_10_10.md) | Snapshot histórico de 23 ramas; no orden de fusionarlas |
| [Mapa histórico](migration/LEGACY_REQUIREMENT_MAP.md) | Procedencia y reglas sustituidas |
| [N0](work/N0_FOUNDATION_PLAN.md) | Ejecución inicial con dependencias permitidas |
| [N2/N11](work/N2_N11_MATERIAL_FORMAT_PLAN.md) | Material/format/package por consumers |
| [ACTIVE_SLICE](work/ACTIVE_SLICE.md) | Único vertical actual |
| [HANDOFF](work/HANDOFF.md) | Hecho real, límites y siguiente acción |
| [Verificación](work/VERIFICATION_REPORT.md) | Alcance comprobado de publicación |

GOAL define destino, masters contratos, ROADMAP orden y ACTIVE_SLICE trabajo actual. No reconstruir decisiones desde chats ni dejar dos masters de ejecución contradictorios. La orden posterior de 2026-10-15 deja Vulkan como único backend activo y conserva DX12 inactivo; sustituye la exigencia gráfica anterior. El historial previo conserva detalles útiles, pero no puede reactivar cancelación total de geología, multiplayer optional o AZ como nomenclatura propia final. Fichas de papers/ramas anteriores son evidencia de lectura histórica, no órdenes de implementación.
