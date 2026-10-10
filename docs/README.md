# Índice canónico de JericoEngine

Nueva base: O3DE. Rama única: **development**. Publicación de 2026-10-10: documentación y diseño, no engine integrado.

| Documento | Función |
|---|---|
| [GOAL](../GOAL.md) | Objetivo persistente y decisiones superiores |
| [AGENTS](../AGENTS.md) | Roles, una rama, Git, seguridad y método |
| [CODEX_START](../CODEX_START.md) | Checkout, default branch y arranque |
| [ROADMAP](ROADMAP.md) | N0-N12 con resultados y dependencias |
| [REQUIREMENTS_MATRIX](REQUIREMENTS_MATRIX.md) | Requisitos concretos, owner, etapa y estado |
| [DEFINITION_OF_DONE](DEFINITION_OF_DONE.md) | Qué acredita un vertical y qué no |
| [Arquitectura nativa](architecture/O3DE_NATIVE_TRANSFORMATION.md) | Owners, Gems y mecanismos de integración |
| [Auditoría de ramas](research/BRANCH_AUDIT_2026_10_10.md) | Las 23 refs y selección sin merges masivos |
| [Rescate histórico](migration/LEGACY_REQUIREMENT_MAP.md) | Qué conservar/sustituir y qué sigue como historia |
| [Legal/dependencias](legal/PROVENANCE_AND_DEPENDENCIES.md) | Código permitido, transitivas, Qt, AWS/LFS |
| [Editor/assets/world](EDITOR_ASSET_WORLD_MASTER.md) | UX, identidad, import/cook, formatos y packaging |
| [Luau/gameplay/AI](SCRIPTING_AI_GAMEPLAY_MASTER.md) | Referencias, Store, save, input, inventario y HTN |
| [Motion/UI/audio/XR](ANIMATION_UI_AUDIO_XR_MASTER.md) | Rigs/IK/fitting, UI, secuencias, sonido y XR |
| [Procedural/physics/environment](PROCEDURAL_PHYSICS_ENVIRONMENT_MASTER.md) | RMod, RBrush, PWAS/EPLS, Jolt y entorno |
| [Perfiles 2D e isométrico](PRESENTATION_PROFILES_MASTER.md) | Sprites, normales, plano, vehículos y perfiles de calidad |
| [Renderer/SIGGRAPH](RENDERING_RASTER_SIGGRAPH_MASTER.md) | Atom/Vulkan, papers y criterios de adopción |
| [Error book](ENGINE_DEBUGGING_BOOK.md) | Lecciones reutilizables, no causas Atom supuestas |
| [ACTIVE_SLICE](work/ACTIVE_SLICE.md) | Único vertical actual |
| [N0 plan](work/N0_FOUNDATION_PLAN.md) | Pasos iniciales de Codex |
| [HANDOFF](work/HANDOFF.md) | Situación actual y siguiente acción |
| [VERIFICATION_REPORT](work/VERIFICATION_REPORT.md) | Cobertura real de la auditoría y límites |

GOAL define destino; masters, comportamiento; ROADMAP, orden; ACTIVE_SLICE, trabajo actual; HANDOFF, estado. La evidencia detallada vive por vertical: no copiar el mismo diario en cinco masters.

Los antiguos documentos siguen disponibles en sus repositorios fijados por SHA, pero no reactivan órdenes Cry/Diligent/MNM/SVOGI, topologías históricas de agentes o geología cancelada. La matriz de rescate explica sustituciones. No añadir otro GOAL persistente duplicado ni otra autoridad para una familia ya cubierta. Una ampliación actualiza su master y matriz de requisitos.
