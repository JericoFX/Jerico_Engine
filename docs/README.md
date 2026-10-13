# Documentación vigente — JericoEngine / JE

**2026-10-10: una rama development; JE::; sin AWS/multiplayer; DX12 y Vulkan; BSP/UI primero y geología activa.** [N0 ejecutado](evidence/n0/N0_EXECUTION_2026_10_10.md) incluye resolver offline, paquetes locales/consumers y núcleo BSP con UV/superficies verificados. Baseline completo, editor visible y runtime siguen pendientes.

| Documento | Uso |
|---|---|
| [GOAL](../GOAL.md) | Última decisión del propietario, capacidades y exclusiones |
| [AGENTS](../AGENTS.md) | Roles, Git, método y evidencia |
| [CODEX_START](../CODEX_START.md) | Inicio seguro y prioridad visible |
| [Motor ligero y PWAS/EPLS](JE_LEAN_ENGINE_AND_AUTHORING.md) | Reducir peso sin recortar herramientas; Studio/PWAS/EPLS, cook y demos integradas |
| [Contrato JE](architecture/JE_IDENTITY_AND_NATIVE_MODULES.md) | Namespace real, CMake/codegen/UUID, Gems dentro del engine |
| [Arquitectura](architecture/O3DE_NATIVE_TRANSFORMATION.md) | Owners y disposición de capacidades |
| [Auditoría de dependencias](research/JE_DEPENDENCY_AUDIT_2026_10_10.md) | Hallazgos DEP-01..12 con source/blobs y límites |
| [Legal/dependencias](legal/PROVENANCE_AND_DEPENDENCIES.md) | Licencias, bibliotecas privadas, offline y exclusiones |
| [Checklist legal O3DE/terceros](legal/O3DE_LICENSE_COMPLIANCE_CHECKLIST_2026_10_13.md) | LIC-01…07: atribuciones, Gems, LGPL/Qt, assets, avisos y bloqueo de release |
| [Roadmap](ROADMAP.md) | N1/N2/N4/N8 adelantados, resto de capacidades conservadas |
| [Matriz](REQUIREMENTS_MATRIX.md) | IDs JER, gates y estados, sin progreso ficticio |
| [Definition of Done](DEFINITION_OF_DONE.md) | Qué cuenta como integrado/visible/seguro |
| [Editor/assets/world](EDITOR_ASSET_WORLD_MASTER.md) | UX/transacciones/identidad/asset pipeline |
| [Materiales/formatos/epak](ASSET_MATERIALS_FORMATS_EPAK_MASTER.md) | PBR/ORM/r*/cooked packages locales |
| [BSP/geología/procedural/física](PROCEDURAL_PHYSICS_ENVIRONMENT_MASTER.md) | Geometry/Geology/RMod/RBrush/PWAS/Jolt |
| [Renderer/SIGGRAPH](RENDERING_RASTER_SIGGRAPH_MASTER.md) | Dos backends, fuente de ideas y mejoras medibles |
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

GOAL define destino, masters contratos, ROADMAP orden y ACTIVE_SLICE trabajo actual. No reconstruir decisiones desde chats ni dejar dos masters de ejecución contradictorios. El historial previo conserva detalles útiles, pero no puede reactivar Vulkan-only/DX12-deferred, cancelación total de geología, multiplayer optional o AZ como nomenclatura propia final. Fichas de papers/ramas anteriores son evidencia de lectura histórica, no órdenes de implementación.
