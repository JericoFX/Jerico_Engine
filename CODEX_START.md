# Codex — una rama, JE, BSP visible

Repositorio `JericoFX/Jerico_Engine`, **development** exclusivamente. Leer `GOAL.md`, `AGENTS.md`, `docs/architecture/JE_IDENTITY_AND_NATIVE_MODULES.md`, `docs/ROADMAP.md`, `docs/work/ACTIVE_SLICE.md` y `docs/work/HANDOFF.md`. Para dependencias: `docs/research/JE_DEPENDENCY_AUDIT_2026_10_10.md`. Para material/cook: los dos masters de assets/materials y el plan N2/N11.

## Última orden, no interpretar desde documentos antiguos

JE:: real en código propio, sin AWS ni multiplayer, solo DX12/Vulkan, Gem capabilities dentro del engine. Geología reactivada. BSP/UI/materiales primero, optimizaciones SIGGRAPH visibles y medibles. Ideas de otros engines permitidas según licencia. No RT de render ni reactivar flotabilidad DESIGN ONLY. No implementar plataforma de cuentas/red/cloud/DLC commerce.

No repetir la fase antigua Vulkan-only ni retrasar BSP hasta N8. Conservar capacidades técnicas del resto de masters bajo la precedencia de GOAL y el mapa de overrides de REQUIREMENTS_MATRIX. No tratar la presencia de una clase AZ o un Gem como destino final autorizado.

## Git seguro

En el checkout autorizado: `git remote -v`, `git status --short`, `git branch --show-current`, `git fetch origin`. Preservar cambios ajenos. Con árbol limpio seleccionar la rama local development existente o crear solo su tracking local de origin/development; `git pull --ff-only origin development`. Si diverge, reconciliar sin reset/force/auto-stash. No crear feature branches/worktrees. Antes/después de publicar verificar la ref remota; nunca push mirror.

El default administrativo puede seguir siendo optimize_scriptcanvas_1. No lo confundir con la rama de trabajo. El ajuste `gh repo edit JericoFX/Jerico_Engine --default-branch development` requiere la sesión/permiso reales y verificación posterior; no cambiar visibilidad ni protecciones. El conector documental no afirma haber realizado ese ajuste.

## Primer trabajo

Ejecutar N0 con dependencias permitidas y locales. No usar endpoints de .lfsconfig o CloudFront/S3 por defecto ni enviarles credenciales. Identificar paquete que falte en vez de auto-descargar. Registrar target/dependencia bloqueante y continuar auditoría/otros pasos viables. Mostrar pronto escena Vulkan/DX12 de base en los pares disponibles, con baseline de coste.

Luego N1: UI Jerico con BSP real dentro del editor existente; construir habitación/corredor/apertura de puerta/ventana, UV/material, Undo/Save/reopen. Migración de ese owner a JE y composición nativa. No empezar por un panel sin consumidor ni por renombrar todo el engine. La nueva geología se integra como productor del mismo Geometry/Asset/Material pipeline, con checkpoints de roca/cliff/cueva, no un engine/playground separado.

Todo resultado visible debe incluir build/backend/escena/settings y evidencia real. Lo no ejecutado se marca NOT TESTED; pruebas de Cry o una captura inventada no acreditan esta base.

## Ampliación obligatoria — reducir peso e integrar PWAS/EPLS

Leer `docs/JE_LEAN_ENGINE_AND_AUTHORING.md`. N0 registra source/SDK/runtime/cache/símbolos/build/idle/CPU/GPU/RAM/VRAM por separado; no dice cuánto ahorra hasta medir. N1/N2/N4/N8 incorporan una primera escena conjunta BSP + Procedural Studio + PWAS + EPLS con locks/Undo/save/cook y rendimiento visible. EPLS/ELSP es un solo sistema; Studio fabrica, PWAS coloca, EPLS aplica historia ambiental.

Achicar significa retirar dependencias/costes no necesarios, no nuestras herramientas. Separar lo que el editor/cooker necesita de lo que se distribuye/carga en el juego. No evaluator de recipes PWAS/EPLS por frame, no previews de panel oculto continuos, no recook de todo por cambiar una capa; conservar tipos/serializers y gameplay. No nuevo engine/profile manager/asset graph. Las demos/gates LEAN y PWE complementan ROADMAP y JER existentes sin sustituirlos.
