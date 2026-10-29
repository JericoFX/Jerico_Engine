# Vegetation → Jerico Assembly (PWAS): plan focal de integración nativa

**Fecha:** 2026-10-28. **Repositorio:** `JericoFX/Jerico_Engine`, rama única `development`.

**Estado:** GEM SELECCIONADA / INVENTARIO DE FUENTE PRELIMINAR / T1 **PENDIENTE DE GATE ANALIZADO** / IMPLEMENTACIÓN **NO INICIADA** / BUILD, EDITOR, RUNTIME, COOK y PERF **NO VERIFICADOS**. Esta publicación es documental; no afirma integración, benchmark ni aceptación.

**Decisión:** aprovechar la distribución que ya existe en `Vegetation` para **Jerico Assembly** (PWAS), ampliar la consulta de superficies existente en `SurfaceData` y reutilizar `GradientSignal` para máscaras. `LandscapeCanvas` aporta patrones de autoría visual, **no** el algoritmo de distribución ni un editor/runtime adicional. El owner de autoría es Procedural Studio/Jerico Assembly, no una nueva Gem o scatter paralelo. El master y sus gates siguen en [PWAS_TECHNICAL_MASTER](../../PWAS_TECHNICAL_MASTER.md) §2.1 y [JE_LEAN_ENGINE_AND_AUTHORING](../../JE_LEAN_ENGINE_AND_AUTHORING.md).

## Resultado de producto (primer corte verificable)

Escena reproducible de la mina: una habitación BSP con puerta protegida, rocas/escombros y soportes colocados por una receta; una extensión de cueva con suelo, repisa, pared, techo y dos niveles en la misma X/Y. El diseñador ve la propuesta, bloquea o mueve piezas, aplica/cancela, hace Undo/Redo y guarda. Un cook limpio reproduce lo aceptado sin ejecutar el generador en cada frame. Materiales, wetness y desgaste los resuelve EPLS/Material/Physics según sus contratos; Assembly **coloca** los prefabs y no rompe geometría ni simula física por su cuenta.

## Inventario observado en `development` (lectura de fuente, no ejecución)

| Donante / destino propuesto | Evidencia leída | Estado y límite |
|---|---|---|
| `Gems/Vegetation/Code/Source/AreaSystemComponent.cpp` → placement de `JE::Procedural` | `VegetationThreadTasks::UpdateSectorPoints` (aprox. 1092–1139) genera una cuadrícula XY, llama a `SurfaceDataSystem::GetSurfacePointsFromRegion` y crea claims; `UpdateContext` procesa sectores y prioridades | Código disponible; el muestreo orientado y múltiples soportes en la misma XY **no** están acreditados. Conservar camino XY barato/compatible; no reutilizarlo sin adaptación para paredes/techos/cuevas. |
| `InstanceSystemComponent.cpp`, `PrefabInstanceSpawner.cpp`, `Descriptor.cpp` | Requests de create/destroy, tickets de spawnables, descriptors y recursos de instanciación ya presentes | Reutilizar criterios/contratos y evaluar coste y ciclo de vida; no presumir identidad persistente, idempotencia o cook de PWAS. |
| `Components/SpawnerComponent.cpp`, `SurfaceMaskFilterComponent.cpp`, `DistanceBetweenFilterComponent.cpp`, `MeshBlockerComponent.cpp` | Spawners, inclusión/exclusión por Surface Tags/pesos, separación y blockers presentes | Inventariar variantes de slope, altitude, distribution, descriptor selector y align antes de decidir qué conservar. Filtrar **bounds del prefab completo** y portales/NoGenerate, no sólo el punto de origen. |
| `Gems/SurfaceData/Code/CMakeLists.txt` y `Gems/GradientSignal/Code/CMakeLists.txt` | Permanecen `Gem::SurfaceData.*` y `Gem::GradientSignal.*`; los static ya referencian owners JE de render en partes | No declararlos integrados ni retirar dependencias/UUID sin inspección de consumidores, lifecycle y seeds. Resolver query 3D/etiquetas con el owner Surface Data del corte correspondiente; evitar doble proveedor. |
| `Gems/LandscapeCanvas/Code/CMakeLists.txt`, `Source/Editor/MainWindow.cpp` | Es `Editor.Static` + `Editor` y usa GraphCanvas, Qt, y `JE::Editor::Tools::GraphModel`; no target runtime propio | Aporta UX de nodos y composición; no copiar otra ventana ni exigir Landscape Canvas para que el juego distribuya prefabs. Su eventual absorción requiere plan propio si se elige. |
| `Gems/Vegetation/Code/CMakeLists.txt` | `Gem::Vegetation.Static`, módulo dinámico legacy `Gem::Vegetation`, `Gem::Vegetation.Editor`, pruebas; dependencia en `Gem::SurfaceData.Static` y `JE::RenderScene` | Ownership nativo **pendiente**: ningún target nuevo `JE::` de scatter ni supresión de la carga de Gem verificados en esta revisión. |

El inventario anterior es selectivo: no satisface aún el gate exhaustivo de `AGENTS.md` y `docs/work/gems/README.md` (reflection/UUID, servicios, consumidores, assets, packaging, preimágenes y regresión congelada). **No tocar el WIP SurfaceData ni iniciar la absorción hasta obtener y registrar ese gate.**

## Revisión de upstream O3DE (fuentes primarias)

- [PR #19340](https://github.com/o3de/o3de/pull/19340), **merged**, corrigió la activación de `SurfaceDataColliderComponent` al entrar en Game Mode. La implementación de este repo **ya llama** a `OnCompositionChanged()` tras `UpdateColliderData()` en `Gems/SurfaceData/Code/Source/Components/SurfaceDataColliderComponent.cpp`; **no volver a aplicar el parche**. El componente aún se presenta como PhysX Collider; evaluar fuente de colisión Jolt real, no equiparar su nombre a soporte Jolt.
- [PR #9108](https://github.com/o3de/o3de/pull/9108), **merged**, corrigió rayos/triángulos del Mesh Surface Tag Emitter y sustituyó la intersección por un algoritmo watertight. Inspeccionar si el proveedor actual usa ese camino antes de añadir raycast/multi-hit; benchmarking del PR **no** mide Jerico.
- [PR #12291](https://github.com/o3de/o3de/pull/12291), **merged**, adaptó duplicación de nodos de Landscape Canvas a prefabs con propagación y Undo/Redo. Usarlo sólo como caso de prueba si Procedural Studio hereda autoría por grafo, no como motivo para crear un segundo editor.
- [PR #5834](https://github.com/o3de/o3de/pull/5834), **merged**, corrigió invalidación de regiones de Surface Data al cambiar límites del terreno y advierte de coste de profiling. Revalidar dirty region en los proveedores actuales.

**Procedencia y licencia:** los archivos donantes mantienen cabeceras O3DE `SPDX-License-Identifier: Apache-2.0 OR MIT`; preservar avisos y comprobar licencia/atribuciones de código y assets concretos en la extracción. Decisión: **adaptar selectivamente**, nunca copiar toda la Gem ni afirmar optimización de una fusión sin métricas.

## Plan de implementación (máximo tres tareas y gates)

### T1 — Cerrar análisis / preimagen / RED: PENDIENTE

- **Interfaz:** `SurfaceData` entrega posiciones/normales/tags/ID con semántica clara; Vegetation aporta candidatos, filtros, descriptors, blockers y spawns; `JE::World` conserva ownership de entidades; `JE::Procedural` de propuestas/transacciones. Congelar API real después de trazar consumidores.
- **Archivos a inspeccionar:** `Gems/Vegetation/Code/{Include,Source,Tests}/*`, `Gems/SurfaceData/Code/{Include,Source,Tests}/*`, `Gems/GradientSignal/Code/{Include,Source,Tests}/*`, composición en CMake/`gem.json`/Registry/seed lists, `Gems/LandscapeCanvas/Code/Source/Editor` sólo donde conecte a authoring; owners JE existentes. Registrar UUID serializados, aliases, servicios, prefabs, factories, transacciones, consumos activos/inactivos y preimágenes.
- **Gate:** baseline RED discriminante en escena con **dos soportes a distinta Z compartiendo XY y una pared**: el camino XY legacy no puede resolver todas las orientaciones mediante un solo muestreo de la cuadrícula. Medir conteos, transform, seed, invalidaciones, frame cost, memoria; documentar licencia y dependencias. El padre marca `ANALIZADO` únicamente con expediente completo, sin pisar ownership del corte SurfaceData en curso.

### T2 — Unidad nativa mínima aprobada: PENDIENTE

- **Archivos/owners destino propuestos (confirmar en T1):** `Engine/World` para queries/identidades de superficies y publicación; `JE::Procedural`/Procedural Studio para receta, candidatos, filtros, veto, preview/Apply/Undo; adaptaciones puntuales en `Gems/Vegetation` y `Gems/SurfaceData` sólo durante la migración hasta retirar la carga legacy. No introducir `Gem::JericoScatter`, VM propia, dependencias AWS ni otro runtime graph.
- **Ejecución:** mantener el sampler XY como fallback barato, añadir consulta 3D orientada con multi-hit y selección de soporte para interiores; reutilizar tags/inclusión/exclusión, densidad/gradient, descriptor/spacing/blocker/align y prefab spawn. Validar clearance por bounds, colisión, NoGenerate/locked/manual, seeds/IDs deterministas y dirty region. El preview no publica entidades hasta Apply; Cancel no altera mundo. Guardar/cocinar sólo outputs aceptados y ownership rastreable.
- **Editor:** priorizar UI existente de Assembly (inspector/presets/brush) y `JE::EditorTools`/GraphModel; usar ideas de `LandscapeCanvas` sólo si reducen trabajo. Absorción propia de Landscape Canvas se decide como otro corte si el graph fuese necesario. EPLS consume tags/estados o solicita placement a Assembly; las rupturas físicas quedan en Geometry/Destruction.

### T3 — Verificación y cierre: PENDIENTE / NO EJECUTADA

| Gate | Escena / aserción |
|---|---|
| VEG-JE-01 / `PWAS-SD-01/02` | Dos pisos misma XY, cueva de una sola mesh, pared y techo: hit/normal/ID/tags correctos; consulta XY heredada no cambia sin aprobación. |
| VEG-JE-02 / `PWAS-01/06` | BSP + puerta/portal/NoGenerate + prefab grande; no invadir área por considerar sólo pivot, ni colocar sobre superficie excluida. |
| VEG-JE-03 / `PWAS-02/03/04/05` | Receta/seed con distintos workers, spacing, prioridad, locks/manual/removals: mismos IDs/transform/hashes y rechazo estable. |
| VEG-JE-04 / `PWAS-07/08/10` | Preview/Apply/Cancel/Undo/Redo, Save/Reopen, recompilación y cook source-free equivalentes; no runtime generador por frame. |
| VEG-JE-05 / `PWAS-09/SD-06` | Mutar tag/superficie en una habitación: sólo dirty region regenerada; capturar CPU p50/p95/p99, memoria, instancias y tiempos de batch sin prometer umbral no medido. |
| VEG-JE-06 / `PWAS-SD-05` | Piedra/metal/madera + estado húmedo: Surface Tag no confunde material visual, material físico Jolt, audio ni layers EPLS. |

**Evidencia requerida:** configure/build focal Windows/Profile/Vulkan, tests RED/GREEN, Editor real, Asset Processor/cook, Runtime de escena guardada, logs frescos/exit, hashes de outputs, inventario PE/loader y mediciones de antes/después. Linux, VR, Shipping, DX12 y escena grande quedan `NOT TESTED` hasta prueba específica. No incrementar contador de Gems completadas por la creación de este plan.

## Continuidad / límites

- Fuente de verdad global: [GOAL](../../../GOAL.md), [ACTIVE_SLICE](../ACTIVE_SLICE.md), [HANDOFF](../HANDOFF.md); el corte activo manda. No tomar ownership paralelo de SurfaceData ni publicar código sin lease, preimágenes y comparación.
- Jerico Assembly puede esparcir vegetación, piedra, escombros, tuberías y props; **no** tiene permiso implícito para destruir/borrar assets manuales o modificar meshes. EPLS aplica historia ambiental; consulta geometría y pide operaciones estructurales a sus dueños.
- Próximo paso ejecutable: completar T1 con el checkout único Windows y su Graft/consumidores, obtener `ANALIZADO` y primer RED; recién entonces codificar T2. Documentado **no equivale** a integrado.
