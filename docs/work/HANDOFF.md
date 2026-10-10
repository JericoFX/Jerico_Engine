# Handoff — última directiva JE/BSP/geología

Fecha 2026-10-10. Rama única development. Nueva implementación **NOT VERIFIED**.

## Cambio del propietario incorporado

Sin AWS ni multiplayer en producto, no optional/deferred. Código propio/adaptado JE:: real, no AZ final ni alias decorativo. Solo DX12/Vulkan (Windows ambos, Linux Vulkan). Gems necesarios absorbidos dentro del engine. BSP/UI/materiales visibles primero, geología reactivada y optimización SIGGRAPH continua; ideas de otros motores permitidas conforme a licencia.

Conservar materiales PBR/ORM/separados, formatos r*/epak, gameplay/animation/UI/physics/AI/XR locales existentes. No expandir a plataformas externas. Buoyancy sigue DESIGN ONLY; RT/underwater y BSP→OpenVDB no se activan por la nueva orden.

## Hecho en esta revisión

Lectura real de dependencias CMake/CSG/paquetes: AzFramework→AzNetworking→OpenSSL/RapidXML/codegen; WhiteBox→Atom/editor/OpenMesh/Manifold y tests PhysX; CSG existente con geometría/validación y gaps de aceptación; Recast deps/pin v1.6.0/FetchContent; selección DX12/Vulkan Windows, Vulkan Linux; CloudFront/S3 default de packages y CloudFront LFS. Ver DEP-01..12 con blobs y alcance de lectura.

Actualizados los objetivos, identidad/composición, exclusiones, planes de ejecución y orden visible. Ningún namespace/source/build/backend/Gem ha sido convertido por escribir estos documentos. Ninguna prueba GPU/offline/linker ha sido ejecutada. Los commits históricos 013e887/a536fb7 contienen la planificación anterior y no son autoridad cuando contradicen GOAL actual.

## Próximo trabajo de Codex

N0 por el plan nuevo: repo/status/sync, dependencias permitidas locales, graph efectivo y baseline por pares disponibles. No consumir AWS para resolver build. Luego N1 cuarto/corredor/puerta/ventana con editor real, material/UV/Undo/save/cook/runtime y owner JE; N2/N4/N8 según necesidades, geología antes de completar todo gameplay.

No redescubrir cada vez el roadmap ni bloquear por un tipo abstracto. Publicar resultados visibles con evidence y seguir el siguiente vertical no bloqueado. Cuando falte hardware/paquete, conservar NOT TESTED/BLOCKED específico y avanzar tareas independientes.

## Pendientes que no se ocultan

Build/Editor/Asset Processor/runtime y métricas reales, cierre completo offline/transitivo/licencias/SBOM, refactor IPC/red, identidad JE y migración UUID/codegen/cache, absorción de Gem resources/builders, conservación UV/material de CSG, Jolt/physics/nav y geología GPU/mesher/cook. Default branch administrativo no se afirma cambiado por este conector; development explícito es la rama de trabajo. Ningún otro repositorio o rama fue borrado.
