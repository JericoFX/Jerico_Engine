# PWAS — contrato técnico de autoría y ensamblaje

**Fecha:** 2026-10-24  
**Estado:** SPECIFIED / ENGINE IMPLEMENTATION NOT VERIFIED  
**Owner previsto:** JE::World + JE::Procedural authoring, con un único World/Asset/Transaction/Cook.  
**Alcance:** Procedural World Assembly System (PWAS); NO es un generador de geometría, ni un sustituto del diseñador.

Este master amplía [JE_LEAN_ENGINE_AND_AUTHORING](JE_LEAN_ENGINE_AND_AUTHORING.md), [PROCEDURAL_PHYSICS_ENVIRONMENT_MASTER](PROCEDURAL_PHYSICS_ENVIRONMENT_MASTER.md), [REQUIREMENTS_MATRIX](REQUIREMENTS_MATRIX.md) (JER-053, JER-099, JER-101) y [PLAYGROUND_TO_JE_INTEGRATION_MASTER](PLAYGROUND_TO_JE_INTEGRATION_MASTER.md). En conflicto manda `GOAL.md`. El documento histórico de JericoEngine `docs/PROCEDURAL_PWAS_EPLS_MASTER.md`, blob `4f89d6ba268a78148d40450ae6cdb545d15c1e8f`, es fuente de requisitos y pruebas históricas, NO prueba de implementación O3DE.

## 1. Identidad, objetivo y límites

PWAS lee mundo y piezas existentes, **propone** disposición, distribución y ensamblaje; una transacción aceptada publica instancias/prefabs/relaciones en el World. El diseñador conserva la última palabra. El uso habitual es authoring/cook, nunca evaluar todo el graph cada frame en Shipping.

| Sistema | Es dueño de | PWAS puede |
|---|---|---|
| JericoBSP / JE::Geometry | Brush/CSG, estructura, caras/regiones, aperturas | Consultar superficies y espacio libre; NO mover una puerta por conveniencia |
| JE::Geology | Forma, strata/erosión, campos, malla derivada | Consultar campos semánticos, apoyar dressing |
| Procedural Studio / RMod | Fabricar piezas, perfiles, rutas, puertos/fittings y soporte | Elegir productos y solicitar fabricación explícita, sin duplicar solver |
| JE::World / Prefab / Scene | Identidad, entidades, overrides, lifetime | Proponer diffs; aplicar mediante transacción única |
| JE::Physics (Jolt) / Navigation | Collider, queries, shapes, constraints, nav | Solicitar clearance/host query, nunca crear un segundo mundo físico |
| EPLS / RBrush / Materials | Historia ambiental, pintura, surface layers | Consumir outputs o recibir solicitudes de instancias |
| Asset Pipeline / Cook | Persistencia, import/reimport, source-free packages | Publicar recetas de autoría y productos aceptados por los owners |

**No objetivos:** graph/VM duplicado, IA generativa obligatoria, servicios cloud, multiplayer, Gem instalable, motor de física paralelo, reescritura del editor ni reemplazar layout manual. No mezclar PWAS con EPLS: PWAS **coloca**; EPLS **aplica capas ambientales**.

## 2. Entradas y salidas contractuales

| Entrada | Contenido mínimo / regla |
|---|---|
| Región/scene | GUID del documento, origen y bounds, revisión de World, transform/unidades |
| Geometría | Surface/face/region ID estable, normal, pendiente, curvatura, cavidad, material/role, espesor cuando sea válido |
| Espacio | Habitación, portal, corredor, volúmenes, obstáculos, zona jugable y clearance; consultas delegadas a World/Jolt/Nav |
| Piezas | Prefab/asset versionado, bounds completos, pivot, orientación, sockets/ports, filtros, LOD y collider autorizado |
| Caminos | Spline con distancia de arco/frame estable; RMod es dueño de routing/fittings |
| Reglas | Tipo de distribución, densidad, presupuesto, seed, pesos, orientación, offsets, superficie/volumen elegible |
| Restricciones | Include/Exclude, `NoGenerate`, `NoWalk` si lo provee Navigation, locked/manual, zonas narrativas y exclusiones |
| Dependencias | Hash/version de source geométrico, prefab, reglas, máscaras y consultas espaciales |

La propuesta devuelve **candidatos aceptados/rechazados**, motivo concreto de cada rechazo (sin superficie, prefab faltante, colisión, clearance, máscara, budget, conflicto con lock), transform, familia/variante, identidad y dependencia. Un fallo de ruta o un espacio sin candidatos **es un resultado válido**; no se corrige deformando el BSP ocultamente.

Los datos de autoría y runtime son distintos. El runtime recibe únicamente instancias, prefabs, relaciones, colliders/materiales y datos cocinados necesarios. No necesita Knowledge Library, generador, UI ni reglas editables para dibujar una sala estática.

## 3. Esquema conceptual de receta y resultado

Los nombres siguientes son **campos contractuales**, no clases C++ ya existentes ni un nuevo serializer aprobado. Codex debe mapearlos a los DTO/reflection/versioning de `JE::World`/Assets ya disponibles; antes de fijar ABI o chunks nuevos, escribir el adaptador y sus pruebas.

~~~text
Recipe:
  recipeGuid, schemaVersion, generatorType, generatorVersion
  seed64, sourceDocumentGuid, scopeRegionGuid
  sourceRevisionsAndHashes, explicitDependencies
  rules[], includeMasks[], excludeMasks[], protectedRegions[]
  locks[], removals[], replacements[], manualOverrides[]
  acceptedGeneratedIds[], budgets, qualityPolicy

Candidate:
  stableCandidateKey, partOrPrefabRef, variantRef
  owningRecipeGuid, hostRegionOrSocketGuid, transform, bounds
  maskSamples, priorityKey, validationReason, dependencyStamp

PreviewResult:
  inputHash, proposalHash, generatedCandidates[]
  accepted[], rejected[], conflicts[], additions[], removals[], updates[]
  diagnostics[], statistics, cancellationRevision
~~~

- GUID de receta, source, región, surface, part, prefab e instancia no se deducen de coordenadas flotantes aproximadas.
- Una instancia generada conserva su ID mientras siga siendo el mismo objeto semántico; cambios de tessellation, LOD, worker count o mero orden de iteración no lo renumeran.
- `Lock` preserva identidad/transform/propiedad; `Remove` impide que reaparezca al regenerar; `Replace` enlaza sustituto estable; `Promote to Manual` transfiere ownership mediante transacción explícita.
- No absorber recetas distintas por compartir preset. El preset común no comparte overrides ni IDs de consumidores.
- Guardar versión/hashes/seeds completos y migración explícita; un esquema desconocido o una referencia rota se diagnostican, nunca se convierten silenciosamente en contenido diferente.

## 4. Generación y matemáticas reproducibles

Métodos candidatos para primera fase: Surface, Volume, Spline, Grid, Poisson Disk/Blue Noise, clusters y puntos pintados. Son estrategias de **placement**, no nuevos productores de mesh. Antes de optimizar a GPU, implementar referencia CPU reproducible con tests de orden, seeds y bounds.

**Secuencia contractual de evaluación:**

1. Capturar snapshot inmutable de World, inputs, reglas, revision/hash, locks y superficies host.
2. Resolver candidatos con soporte, orientación, variante/prefab y transformación estable. Para splines usar distancia de arco y frame (RMF) del owner; para sockets usar compatibilidad y umbrales/histéresis ya existentes.
3. Asignar clave/semilla derivada de receta + dominio semántico + candidato, sin depender del número de threads, reloj o hash aleatorio del proceso.
4. Ordenar por prioridad reproducible y desempate de clave estable. En Poisson: **resolver soporte/transform antes de ordenar y aplicar thinning por distancia; sólo después máscaras/SDF y filtros downstream**, preservando el contrato histórico M7F; los rechazos de proximidad también cuentan en el hash del resultado.
5. Evaluar Include/Exclude/`NoGenerate`, restricciones de roles/materiales, ocupación, bounds transformados completos, clearance de puertas/corredores y Navigation donde exista. Una muestra puntual no sustituye evaluar el volumen del prefab.
6. Aplicar precedencia de ownership y overrides; no desplazar piezas manuales, locked ni regiones narrativas para aumentar densidad.
7. Producir propuesta/diff, diagnósticos y estadísticas sin tocar World; presentar gaps si faltan datos o no hay solución.

La fórmula precisa de pesos/scoring y PRNG canónico para JE **requiere fixture y decisión de owner antes de convertirla en API estable**; no inventar constantes o una distribución distinta sin comparar el donante M7F. Densidad se interpreta en unidades explícitas (`por m²`, `por m³` o `por metro` según modo), con seed y budget; no «un objeto por voxel» implícito. Rotación/alineación/sink/scale tienen rangos, límites físicos y orden fijo.

## 5. Precedencia y transacciones

**Prioridad:** estructura World/BSP y zonas protegidas > elemento manual o promoted > lock/Remove/override de la receta > restricciones de masks/clearance > candidato procedural. Una orden de cambio estructural requiere otra transacción/owner.

- `Preview`: sólo staging; lista altas/bajas/updates/conflictos y motivos. `Cancel` no publica ni modifica Undo.
- `Apply`: comprueba que source revision y hashes aún correspondan; una transacción World publica sólo diff aprobado y guarda seed/IDs/overrides. Si el snapshot es stale, rechazar y volver a calcular, nunca publicar encima de una edición nueva.
- `Undo/Redo`: restauran receta e instancias/identidades manuales y generadas como una operación coherente.
- `Regenerate Region`: dirty dependencies acotadas; mantiene otros IDs/transforms/overrides intactos; no reevalúa el nivel completo por mover una repisa.
- `Bake/Cook`: reemplaza únicamente outputs owned por la receta. Un prefab/asset manual sin ownership de PWAS nunca es borrado; source-free produce el mismo contenido aceptado.

Desactivar una receta o eliminar un prefab faltante debe informar impacto y dependencias. Ninguna operación parcial puede dejar jerarquía de World, collider, nav o render con referencias colgantes.

## 6. UI JericoEditor

Un workspace del **mismo** editor Qt, no una segunda aplicación. Flujo mínimo:

1. Seleccionar habitación/superficie/volumen/spline o colección; ver roles, bounds, masks, locks y disponibilidad de inputs.
2. Elegir familia de piezas/regla/preset; seed explícita, densidad con unidades, orientación, distancia, límites y variantes.
3. `Generate Preview` muestra ghosts, colisiones, aperturas protegidas, rejects/gaps y cambios sobre scene actual.
4. Inspector enseña dependencia/razón/ID por candidato; permitir `Lock`, `Remove`, `Replace`, `Promote Manual` sin perderlos con otra seed.
5. `Diff / Apply / Cancel / Undo / Redo / Save / Reopen` mantienen cámara, selección, propiedad y decisiones del diseñador.
6. Debug de occupancy, spatial queries, source hashes, tiempo CPU, número de candidatos/rechazos, dirty regions y presupuesto consumido.

El graph avanzado es opcional; no exigir nodos o scripting para colocar una puerta, repisa o conjunto de tuberías.

## 7. Rendimiento, invalidación y Shipping

- Reutilizar spatial index/queries oficiales; no escanear mundo entero ni todos los prefabs por frame.
- Dirtiness por region + dependency graph; caché con hash de inputs y revisiones de source. Cancelación y `stale jobs` obligatorios.
- Presupuestos de candidatos, tiempo, memoria e instancias; al truncar, diagnóstico de cantidad omitida y regla responsable. No pérdida silenciosa.
- Variar quality tier reduce representación decorativa/LOD sin borrar objetos lógicos, colliders/triggers/identidades jugables ni los locks.
- Cook por target Windows-DX12, Windows-Vulkan y Linux-Vulkan; mismo contenido lógico de recetas aceptadas. Nada de runtime graph evaluator de autoría.
- Probar offline/source-free después de Cook; error explícito si una dependencia requerida falta.

## 8. Fases de integración desde Playground

Leer [PLAYGROUND_TO_JE_INTEGRATION_MASTER](PLAYGROUND_TO_JE_INTEGRATION_MASTER.md) antes de extraer código. El Playground no implementa PWAS entero. Sí contiene evidencia parcial reutilizable: `SplineConstruction` (ArcLength/RMF/sweep/UV), `ProceduralAssemblies` (14 familias, Stable Part IDs y damage) y consultas de surface; no hay evidencia de que sus rutas de Diligent/ImGui o su CMake antiguo funcionen dentro de O3DE. JE conserva `JE::Geometry`/BSP, World, Scene, RMod, Physics, UI y Asset Pipeline como propietarios.

Primero extraer **matemáticas y fixtures**, luego construir un solo consumidor real en JE. No integrar una app Playground, no copiar headers del engine antiguo a producción y no registrar Gems instalables.

## 9. Matriz de aceptación (sin declarar PASS)

| ID | Fixture mínimo | Condición de aprobación |
|---|---|---|
| PWAS-01 | Sala BSP con puerta, ventana y corredor | Ninguna propuesta invade aperturas ni ruta jugable |
| PWAS-02 | Misma receta/seed 1 y varios workers | IDs, transforms, rechazos, conteos y hashes idénticos |
| PWAS-03 | Poisson con prioridad/proximidad | Thinning determinista y `proximityRejected` incluido aun si vale cero |
| PWAS-04 | Dos recetas enlazan un preset | No se mezclan locks, Remove, IDs ni overrides |
| PWAS-05 | Cambiar seed con lock + manual + removal | Congelados iguales; removido no reaparece; manual no cambia |
| PWAS-06 | Prefab grande junto a portal/NoGenerate | Rechazo por bounds completos, no sólo origen |
| PWAS-07 | Cancel/Apply con edit concurrente | Cancel no publica; resultado stale no pisa World |
| PWAS-08 | Undo/Redo, Save/Reopen | Identidades/overrides/jerarquía idénticos |
| PWAS-09 | Regeneración de región sucia | Otra habitación intacta; coste medido |
| PWAS-10 | Bake/cook y ejecución source-free | Instancias, colisión/nav y IDs persistentes sin generador |
| PWAS-11 | Spline degenerado/socket incompatible | Diagnóstico; no NaN/rotación arbitraria/union fantasma |
| PWAS-12 | Falta material/collision/nav requerido | Diagnóstico y rechazo, nunca inventar un valor |

Primer vertical recomendado: **BSP cuarto/corredor → Procedural Studio pieza → PWAS preview/lock/regenerate → EPLS layers → Undo/Save/Reopen/Cook**. Registrar por separado source, test, editor visual y runtime. El documento no certifica implementación ni habilita una migración masiva.
