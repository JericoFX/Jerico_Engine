# Contrato de agentes — JericoEngine sobre O3DE

Vigente desde 2026-10-10. Leer `GOAL.md`, `CODEX_START.md`, `docs/ROADMAP.md` y `docs/work/ACTIVE_SLICE.md` antes de editar. Este contrato rige también dentro de Gems y subdirectorios; las reglas locales de build/estilo se conservan mientras no contradigan seguridad, procedencia o el objetivo nuevo.

## Una rama, un integrador, un motor

Única rama de trabajo y publicación: **development** en **JericoFX/Jerico_Engine**. Prohibido crear ramas de feature/research, worktrees, forks de trabajo o una segunda línea Jerico. Las ramas existentes distintas de development son referencias de lectura. No fusionarlas todas ni borrarlas. La opción default de GitHub no decide el branch: usar siempre development explícitamente.

No modificar JericoFX/JericoEngine2, JericoEngine ni JericoGeologyPlayground; son fuentes de lectura/procedencia. No importar su historia con merge/cherry-pick ni copiar sus árboles. No reactivar la transformación Cry, el backend Diligent obligatorio, MNM ni el port de SVOGI.

## Coordinación conservada de la última directiva comprobada en Git

El AGENTS histórico, en `JericoEngine2@304d9d2e13704e4967e82a8d5a13a5a506be01d4`, registra el 2026-10-09: Sol padre con razonamiento alto, dos Sol hijos escritores con scopes disjuntos y una Luna de apoyo solo al padre. Se conserva esa política de roles; no se reinstala la directiva Astra-único-escritor que el mismo archivo marca histórica.

El padre posee arquitectura, asignación, integración, revisión, build compartido, aceptación y publicación. Hasta dos hijos escriben únicamente archivos asignados y reportan al padre; no delegan, cambian Git ni ejecutan builds sobre entradas mutando. Luna investiga y ejecuta compilaciones/pruebas expresamente asignadas, pudiendo escribir sus outputs/logs; no modifica fuente/docs/config, no publica ni decide PASS.

No afirmar que editar un documento cambia el modelo real de un agente. Si el entorno no ofrece esos modelos/subagentes, declararlo y mantener un único escritor efectivo. No instalar configuraciones globales ni inventar agentes para satisfacer un nombre. Preservar trabajo inactivo y cambios del usuario.

## Método de trabajo

Think Before Coding; Simplicity First; Surgical Changes; Goal-Driven Execution. Cada vertical empieza localizando productores, consumidores, lifetimes, identidad, editor, guardado, cook y runtime existentes. Reutilizar el owner real de O3DE antes de introducir una abstracción.

Un único World, asset graph, registro de identidad, timeline/clock por dominio, physics world, gameplay VM y scheduler gráfico. Jerico nativo no es un megabinario obligatorio: pueden existir bibliotecas y módulos privados. Tampoco es un plugin Jerico que oculta otro motor completo o un wrapper permanente que conserva dos autoridades.

No empezar cambiando miles de nombres. No reemplazar AZStd/reflection/EBus globalmente; evitar nuevos locators/broadcasts para peticiones normales cuando haya una dependencia tipada adecuada. Migrar junto con consumidores y medir.

## Integración vertical sin bloqueos artificiales

Trabajar sobre un resultado observable y persistente, no un header, Float32, registry o codec sin consumidor. El padre puede incluir las dependencias mínimas necesarias para cerrar ese resultado, documentando el ajuste en ACTIVE_SLICE. No detener toda la aplicación porque falte una abstracción sin consumidor. Un bloqueo verdadero debe nombrar archivo/API/permiso/insumo, reproducción y tareas independientes aún viables.

No activar implementaciones explícitamente DESIGN ONLY, especialmente flotabilidad. No abrir RT, DX12, geología procedural integral ni nuevas plataformas por iniciativa del agente. La prioridad actual es el N0 definido, no todas las ideas del catálogo.

Después de un vertical verificado: revisar diff, publicar, actualizar HANDOFF/ACTIVE_SLICE y continuar con el siguiente objetivo no bloqueado mientras el entorno permita trabajar. El objetivo global sigue ACTIVE. Si falta hardware, conservar el estado NOT TESTED; nunca fabricar captura, ejecución, subagente o resultado.

## Git obligatorio y seguro

1. Verificar directorio, `git remote -v`, `git branch --show-current`, `git status --short`, HEAD y cambios preexistentes. No descartar cambios ajenos ni auto-stash.
2. `git fetch origin` antes de trabajar y antes de publicar. Con árbol limpio, `git pull --ff-only origin development`. Si hay divergencia, detener la publicación y reconciliar explícitamente; no reset/force/rebase automático.
3. Asignar ownership de archivos. Un único owner por build/test compartido; congelar entradas durante la verificación. No crear copias de engine para eludir esta regla.
4. Ejecutar pruebas pertinentes, revisar archivos exactos y `git diff --check`; staging por rutas, no `git add .` indiscriminado.
5. Commit descriptivo. Publicar solo en `origin development`. Comprobar la ref remota después con fetch/ls-remote y registrar el SHA confirmado.
6. Cada push va acompañado de sincronización: fetch/pull seguro previo y comprobación posterior. Pull no significa sobrescribir trabajo concurrente; pull tras commit local divergente puede fallar y requiere reconciliación, nunca fuerza.

Prohibidos: `git reset --hard`, `git clean`, `push --force`, mirror push, borrar ramas/remotos, limpiar SDK/assets/cachés sin manifest y alcance aprobado, publicar código privado o cambiar visibilidad. No enviar credenciales a LFS heredado ni introducir secretos en logs/docs.

## Procedencia y AWS

Aplicar `docs/legal/PROVENANCE_AND_DEPENDENCIES.md`. O3DE Apache/MIT no relicencia CryEngine 5.7 ni todo Lumberyard. Archivos provenientes de O3DE con nombres Cry/Amazon pueden ser legítimos; clasificarlos por licencia, no por texto. No suprimir LICENSE/NOTICE/SPDX/créditos necesarios.

Antes de incorporar código/test/asset antiguo: origen exacto, licencia, autoría, derivados/transitivas, aprobaciones y destino. Si es incierto, conservar como requisito/observación y hacer implementación independiente desde fuentes permitidas. No llamar clean-room a un proceso que no lo demuestra.

Separar cuatro cuestiones AWS: servicios en runtime, SDK enlazado, herramientas de build y hosting de LFS/paquetes. Un `AWSNativeSDK` en un catálogo no prueba que esté enlazado; una URL CloudFront sí demuestra dependencia de distribución mientras se utilice. Auditar ambas sin inferir una de otra. No exigir cuenta AWS ni desplegar servicios.

## Rendering, errores y evidencia

Atom es la autoridad gráfica; no agregar otro RenderGraph, device ni exposure. Antes de corregir, buscar `docs/ENGINE_DEBUGGING_BOOK.md`, inspeccionar captura original y distinguir primer output defectuoso de efectos posteriores. Hipótesis no es causa demostrada.

Para regresiones escribibles: prueba RED que distingue el fallo, cambio mínimo y GREEN sin relajar tolerancias. Para visuales: escena/cámara/settings fijos, outputs reales, comparaciones ON/OFF/ON y restauración, además de revisión visual. Instrumentación desactivada no debe añadir readbacks por frame. No cambiar seguridad del driver ni configurar contadores globales sin permiso.

Pruebas completas incluyen configuración, comandos, código de salida, hash de entradas/productos, logs y límites. No repetir journaling inmenso en todos los masters: resultado actual en HANDOFF, evidencia en el directorio del vertical, lección reusable en el error book. No miles de checks redundantes como sustituto de un caso de uso real.

## Definición de publicación

Distinguir `docs:`, `test:`, `fix:` y `feat:`. Un commit documental puede estar verificado como documentación sin exigir compilar el engine; no debe decir producto PASS. Un commit de implementación parcial se marca explícitamente y no desplaza el baseline aceptado sin rollback. No copiar PASS anteriores a O3DE. Leer `docs/DEFINITION_OF_DONE.md`.
