# Vertical activo — N0: base O3DE reproducible

Fecha: 2026-10-10. Estado: **READY TO EXECUTE / ENGINE NOT TESTED**.
Repo único `JericoFX/Jerico_Engine`; rama única **development**. El paquete documental prepara N0; no lo declara ejecutado.

## Resultado observable

Conservar la base O3DE que ya existe y demostrar, en la plataforma local disponible, Editor con una escena Vulkan real, Asset Processor y runtime cocinado, con procedencia/dependencias y métricas iniciales. Identificar por separado el estado Windows/Linux. No usar el runtime Cry como prueba.

## Entradas y autoridad

Leer CODEX_START, AGENTS, GOAL, ROADMAP y `N0_FOUNDATION_PLAN.md`. Origen técnico: O3DE `241c78b8064898a709cf1c5dd3535c6406763ef3`, tree `db4b7d4a5d784d2283c95903f461e311d69704c8`. HEAD de trabajo incluye luego el commit documental; registrar su SHA real antes de empezar.

## Incluido

Checkout correcto/sync; default branch check; inventario LFS/3P/toolchain/licencias; mapa de dependencias y AWS; builds originales con correcciones mínimas si una reproducción lo exige; prueba Editor/asset processing/runtime; CPU/GPU/memoria/responsiveness; ruta futura de composición nativa; interfaces XR desde foundation; reporte de bloqueos concretos y siguiente vertical.

## Excluido

Borrado de Cry/repo/ramas; crear otra rama/worktree; merges masivos; port de código Cry o antiguos engines; renombrado global; reescribir AZ/Core/Gems; integrar Jolt+Luau+UI+todo el roadmap al mismo tiempo; RT, DX12, nuevos targets móviles/WebGPU, geología integral, underwater y flotabilidad DESIGN ONLY.

## Gates

- Snapshot/licencias/toolchain y materialización requerida verificados.
- Editor/scene Vulkan real y assets procesados; runtime cocinado probado o limitación exacta registrada.
- Origen de assets y librerías permitido; sin dependencia nueva de cuentas AWS/SDK restringidos impuesta.
- Medidas reproducibles, no solo compilación o screenshot.
- Estado separado por plataforma y para hosting/download transitorio.
- HANDOFF con comando/log/hash/resultado/limitación y siguiente acción N1; no cerrar el GOAL global.

Si una plataforma no está disponible, N0 de esa plataforma sigue NOT TESTED. Se puede continuar N1 en la plataforma demostrada sin inventar Linux/VR PASS. Cambios de baseline por fix indispensable permanecen en development y se registran; las otras ramas son referencias, no nuevas bases automáticas.
