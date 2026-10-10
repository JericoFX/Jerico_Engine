# Codex: empezar aquí, una sola rama

Trabajar exclusivamente en **JericoFX/Jerico_Engine / development**. Transformar la base O3DE hacia JericoEngine; no crear una rama, worktree, fork ni engine paralelo. No tocar los repositorios anteriores. No borrar Cry ni ramas desde este encargo.

## Lectura obligatoria

Leer, en orden: `AGENTS.md`, `GOAL.md`, `docs/README.md`, `docs/ROADMAP.md`, `docs/work/ACTIVE_SLICE.md`, `docs/work/HANDOFF.md` y `docs/DEFINITION_OF_DONE.md`. Para el primer trabajo seguir `docs/work/N0_FOUNDATION_PLAN.md`. Antes de un subsistema, leer su master y la entrada correspondiente de `docs/migration/LEGACY_REQUIREMENT_MAP.md`.

La instrucción del propietario de trabajar solo en una rama está resuelta: development. El valor default de GitHub puede seguir mostrando optimize_scriptcanvas_1 hasta corregirlo. No deducir la base desde ese valor.

## Checkout seguro en Windows

Ejecutar desde el checkout correcto. Estos comandos están documentados, no ejecutados por la auditoría remota. No usar reset ni borrar cambios para hacerlos pasar.

```powershell
$ErrorActionPreference = 'Stop'
$origin = git remote get-url origin
if ($LASTEXITCODE -ne 0) { throw 'No se pudo leer origin.' }
if ($origin -notmatch 'github\.com[:/]JericoFX/Jerico_Engine(\.git)?$') {
    throw 'Este checkout no apunta al repositorio autorizado. Revisar sin cambiar remotos.'
}
$dirty = @(git status --porcelain)
if ($LASTEXITCODE -ne 0) { throw 'No se pudo leer el estado Git.' }
if ($dirty.Count -gt 0) { throw 'Hay cambios locales: preservar y reconciliar antes de cambiar de rama.' }
git fetch origin
if ($LASTEXITCODE -ne 0) { throw 'Fetch falló; no continuar con una base supuesta.' }
git show-ref --verify --quiet refs/heads/development
$refStatus = $LASTEXITCODE
if ($refStatus -eq 0) {
    git switch development
} elseif ($refStatus -eq 1) {
    # Copia LOCAL de la única rama canónica que ya existe en origin.
    git switch --track -c development origin/development
} else {
    throw 'No se pudo inspeccionar la referencia local.'
}
if ($LASTEXITCODE -ne 0) { throw 'No se pudo seleccionar development.' }
git pull --ff-only origin development
if ($LASTEXITCODE -ne 0) { throw 'Sincronización no fast-forward; reconciliar sin force ni reset.' }
git branch --show-current
git rev-parse HEAD
git status --short
```

Si no hay checkout, obtener el repositorio privado mediante credenciales ya configuradas y seleccionar development explícitamente. Antes de materializar binarios leer `.lfsconfig` y el expediente legal: el endpoint heredado es CloudFront. No publicar assets privados ni enviar PAT a un host no autorizado. No hace falta descargar LFS para leer estos Markdown.

## Ajustar la rama predeterminada de GitHub

El objetivo administrativo es que GitHub muestre development por defecto. Con GitHub CLI instalado y sesión ya autorizada, Codex debe comprobar y, si hace falta, ejecutar:

```sh
gh repo view JericoFX/Jerico_Engine --json defaultBranchRef --jq .defaultBranchRef.name
gh repo edit JericoFX/Jerico_Engine --default-branch development
gh repo view JericoFX/Jerico_Engine --json defaultBranchRef --jq .defaultBranchRef.name
```

La última salida debe ser `development`. No cambiar visibilidad, permisos, protección o ramas. Esta acción no fue realizada por el conector de la auditoría documental; no declararla hecha sin salida real. Falta de CLI/permiso administrativo no impide trabajar con development explícito; registrar la limitación sin abrir otra rama. Sintaxis verificada en https://cli.github.com/manual/gh_repo_edit .

## Encargo de ejecución

Ejecutar N0 sobre fuentes reales. Primero herramienta/plataforma/LFS/licencias/grafo, después compilación original y escena Vulkan, Asset Processor y runtime cocinado. No borrar Gems ni hacer branding masivo antes del baseline. No copiar código/shaders de JericoEngine2. No activar RT, DX12, geología integral o flotabilidad DESIGN ONLY.

No quedarse en una lista de trabajos posibles ni bloquearse por una propiedad abstracta. Producir resultados verificables, registrar fallos concretos y avanzar en tareas independientes. Después de N0 funcional en una plataforma, continuar verticales que esa evidencia permita; mantener el otro sistema operativo NOT TESTED hasta probarlo.

Usar los roles del AGENTS vigente y un único integrador/publicador. Cada commit implementa un resultado coherente en la ruta canónica. Actualizar HANDOFF/ACTIVE_SLICE en vez de pedir que Joan vuelva a explicar las decisiones de este paquete.
