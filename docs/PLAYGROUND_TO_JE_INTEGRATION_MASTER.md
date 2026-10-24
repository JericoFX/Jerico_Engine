# Playground → JericoEngine: inventario, procedencia y gates de integración

**Fecha:** 2026-10-24  
**Estado:** DOCUMENTED HANDOFF / NO PORT OR RUNTIME INTEGRATION CLAIMED  
**Destino:** `JericoFX/Jerico_Engine`, rama única `development`, base O3DE y owners `JE::`.  
**Fuente de laboratorio fijada:** `JericoFX/JericoGeologyPlayground@7f7f09adaa25672ae2ac2af0d0033bab3f710fab` (`main`, 2026-10-05).  
**Fuente histórica de PWAS/EPLS:** `JericoFX/JericoEngine/docs/PROCEDURAL_PWAS_EPLS_MASTER.md`, blob `4f89d6ba268a78148d40450ae6cdb545d15c1e8f`, 1.003 líneas.

Enlaces: [Playground congelado](https://github.com/JericoFX/JericoGeologyPlayground/tree/7f7f09adaa25672ae2ac2af0d0033bab3f710fab), [GOAL Playground](https://github.com/JericoFX/JericoGeologyPlayground/blob/7f7f09adaa25672ae2ac2af0d0033bab3f710fab/GOAL.md), [contrato de matemática aislada](https://github.com/JericoFX/JericoGeologyPlayground/blob/7f7f09adaa25672ae2ac2af0d0033bab3f710fab/docs/PROCEDURAL_MATH_PLAYGROUND_2026_10_03.md), [PWAS](PWAS_TECHNICAL_MASTER.md), [EPLS](EPLS_TECHNICAL_MASTER.md).

## 1. Regla de uso: laboratorio, no segunda autoridad

El Playground valida **matemáticas, prototipos y fixtures** y sirve como suelo técnico para integración gradual. No se absorbe como aplicación, Gem, submotor, runtime o pipeline Diligent. Integrar significa seleccionar una capacidad con evidencia, auditar procedencia/licencia y contratos, adaptar la matemática al owner existente en JE, verificar de nuevo allí y retirar sólo los adapters de transición cuando los consumidores estén probados.

La instrucción del Playground del 2026-10-03 prohibía editar el **antiguo** JericoEngine2 desde ese laboratorio; este handoff no modifica el Playground ni su código. La decisión posterior del propietario admite planificar incorporación gradual al **nuevo O3DE JE**. No interpreta esto como autorización para copiar fuente antigua/tercera, cambiar contratos a ciegas ni declarar integración realizada.

**No importar:** ventana ImGui, backend Diligent/Vulkan local, CMake acoplado a `F:/JericoEngine`/`build/windows-vulkan-editor`, código/renderers del antiguo Cry, fallback de AWS, plugins/Gems independientes, segunda física o graph evaluado por frame.

**Conservar:** prueba matemática reproducible, benchmarks con parámetros, seeds/hashes, limitaciones conocidas, semántica de ID/region/material, UI como referencia de interacción, y datasets/evidencia de fixtures auditables.

## 2. Estado real comprobado de la fuente

| Área | Fuente concreta | Evidencia histórica y límite |
|---|---|---|
| Solid Boolean/CSG | `Source/SolidGeometry.cpp`, `Source/DamageGeometry.cpp` | Manifold 3.5.2 en Playground; host cerrado/boolean subtraction, topología fresca y budgets; no prueba engine JE |
| Surface FeatureField | `Source/SurfaceFeatures.hpp/.cpp` | Curvatura firmada, convexidad/concavidad, bordes, espesor, cavidad y cache; verificar paridad con mesh JE |
| Shared DamageField | `Source/DamageField.hpp/.cpp` | Manual/edge/cavity/impact/spline/age/noise, protect/tier, falloff y crack path; `impact` en campo NO equivale a Jolt ContactListener conectado |
| Geometric damage | `Source/DamageGeometry.hpp/.cpp` | Cavity, through-hole y path cutter; cierre/volumen/identidad FreshInterior; `Dent` de assemblies es sustracción booleana limitada, no lattice plástico de choques |
| Spline Construction | `Source/SplineConstruction.hpp/.cpp` | ArcLength, RMF, sweep, perfiles/UV, selected path damage; no sustituye ProceduralSpline/RMod de producción |
| 14 Procedural Assemblies | `Source/ProceduralAssemblies.hpp/.cpp`, `Source/AssemblyGeneration.cpp` | Pipe, rail, railing, pier, catwalk y bridge; stable Part IDs, estados, selección y algunas variantes geométricas; no es PWAS completo |
| Damage authoring/recipe | `Source/AssemblyLabUI.cpp`, `Source/AssemblyDamageIO.cpp` | Área movible, seed, filtros por part/role, save/load, Undo/Redo; UI ImGui no se porta a Qt como tal |
| Fragmentos físicos | `Source/AssemblyFragmentPhysics.hpp/.cpp` | Selección de un fragmento y simulación dinámica Jolt en laboratorio; no acredita runtime/scene/gameplay del motor nuevo |
| Baking/UV | `Source/AutoUv.*`, `Source/NormalBake.*`, `Source/DamageUvTransfer.*` | Regiones/caras frescas, UV e interior; adoptar sólo si el owner JE no tiene la capacidad |
| RBrush/fields | `Source/RBrushInterop.*`, `Source/RockAddonMath.*`, `Source/SurfaceGeometry.*` | Inputs de máscaras, operadores geológicos y math; nada autoriza duplicar RBrush, Geology o materiales |
| Web Surface | `Source/WebGraph.cpp`, `Source/WebPhysics.cpp`, `Source/WebBake.cpp` | Cards/textura y geometría de depuración/LOD, Jolt experimental; PWAS colocar, no crear web renderer alternativo |

**Verificación publicada, no ejecutada hoy:** Release + `26/26 CTest PASS` el 2026-09-28 en [Spline shared path](https://github.com/JericoFX/JericoGeologyPlayground/blob/7f7f09adaa25672ae2ac2af0d0033bab3f710fab/Evidence/2026-09-28-spline-shared-path/README.md) y [Fragment physics](https://github.com/JericoFX/JericoGeologyPlayground/blob/7f7f09adaa25672ae2ac2af0d0033bab3f710fab/Evidence/2026-09-28-fragment-physics/README.md); Vulkan RTX 3090 con capturas y hashes. [Universal damage](https://github.com/JericoFX/JericoGeologyPlayground/blob/7f7f09adaa25672ae2ac2af0d0033bab3f710fab/Evidence/2026-09-27-universal-damage-candidate/README.md) tuvo `25/25` y 14 familias. `GOAL.md` indica **PAUSED / PARTIAL** y Beauty/arrastre manual/paridad de destructores incompletos. Un commit documental el 2026-10-05 no actualiza aquellas pruebas.

El [handoff DOT de rocas](https://github.com/JericoFX/JericoGeologyPlayground/blob/7f7f09adaa25672ae2ac2af0d0033bab3f710fab/docs/DOT_ROCKS_PLAYGROUND_HANDOFF_2026_10_05.md) sólo prepara otro laboratorio separado; no prueba recepción ni resultados ni fusiona ambos.

## 3. Correspondencia con JE — un solo owner por cosa

| Matemática o resultado del Playground | Destino/consumidor JE | Qué requiere nueva aceptación |
|---|---|---|
| TriangleMesh, feature edges, region IDs, raycast | `JE::Geometry`/`JE::BSP` y World spatial queries | Conversiones, unidades, winding, face GUID, undo/prefabs, CSG/LOD/stream |
| Difference/Union/Cavity/Through | Kernel `JE::BSP::Csg` y Geometry/Destruction | Paridad booleanas, FreshInterior, collider/nav/cook y materiales; nunca dos solvers productivos |
| Spline/RMF/catenaria/fittings | `RMod / Procedural Studio / JE::Geometry` | Piezas/sockets/port compatibility, rutas, soporte, stable IDs, UV y save |
| SurfaceFeatureField/Shared DamageField | `JE::Geometry/Geology`, `EPLS`, RBrush y Smart Materials | Una interfaz canónica, masks PBR, dirty region y evolución de superficie |
| Procedural parts/assemblies | Procedural Studio produce assets; `PWAS` distribuye | Separación fabricar/colocar, full bounds, locks, manual overrides |
| WebGraph/card/bake | Procedural Web/VFX/Assets y `PWAS` placement | Coste de cards vs mesh/hilos, EdgeID, no centenas de objetos inútiles |
| Fragmentos Jolt | `JE::Physics` + Geometry/Destruction | Contact/constraints/collider/COM, dinámica local, staging/cook/save |
| AutoUV/normal bake/texture | `JE::Assets/Materials` y cooked `.rmesh/.rmat/.rtext` | Version/lineage, source-free, paridad de material y color-space |

**Diferencia de versión sensible:** `Playground README` enlaza Manifold **3.5.2** desde la dependencia del engine anterior. En `Jerico_Engine/development`, `Engine/Geometry/3rdParty/FindManifold.cmake` declara **v3.5.1** y `FindOpenMesh.cmake` OpenMesh 11.0.0. No cambiar pin por conveniencia ni asumir paridad de resultados: primero correr fixtures idénticos y comparar regiones, volumen, triángulos e interior.

No confundir que `JE::BSP::Csg` exista/compile con que daño dinámico, escena/cook/editor completos ya pasen. `OpenMesh` edita conectividad/topología; `Manifold` hace booleanas; ninguna de las dos crea por sí sola una deformación de impacto elástico/plástica. El diseño anterior de lattice/cage/Jolt debe recibir un gate separado.

### 3.1. Golpes y deformación de mallas: contrato distinto de fractura

El pedido de **pegarle a un mesh y dejar una abolladura** tiene tres caminos diferentes. No declararlos equivalentes:

| Efecto | Geometría y coste | Responsable de producción |
|---|---|---|
| Golpe visual leve | Normal/decal/mask, posible vertex offset de render; no muta collider | EPLS/Materials/Renderer con autorización del material |
| Abolladura real localizada | Vértices desplazados sobre influencia geodésica/superficie, preservando topología, espesor y UV hasta límites físicos | `JE::Geometry/Destruction` + collider recook/controlado de `JE::Physics` |
| Rotura/agujero/corte | Booleanas, nueva topología/FreshInterior, fragmentos/anchors, recálculo UV/normales/collision | `JE::Geometry/Destruction` (Manifold/OpenMesh auditados) y Jolt |

**Ruta técnica propuesta, pendiente de validar:** `Jolt Contact/Hit Event` normalizado (actor, mesh/region GUID, posición/norma en espacio de mundo, impulso/energía, radio, material físico, revisión) → conversión a local/consulta de `SurfaceFeatureField` y `DamageField` → escoger VisualDent, GeometricDent, Chip o Fracture por presupuesto/material → modificar únicamente región afectada → resolver normales/tangentes/UV/FreshInterior cuando aplique → actualizar bounds, collider y navegación mediante sus owners → transacción World/Undo/Save/Cook. No introducir un segundo solver de física ni deformar indefinidamente toda la malla en el render.

Para abolladura geométrica real, la función de desplazamiento debe respetar distancia superficial, borde/protección, curvatura, espesor mínimo y volumen permisible. **No hay todavía un modelo elastoplástico canónico** (plasticidad, retorno elástico, creep, lattice/cage) acreditado para JE: antes de elegirlo, fijar casos de impacto, materiales, tolerancias y bibliotecas autorizadas. `Dent` booleana en Assemblies es un ejemplo útil de daño con forma, no prueba de deformación física continua. Cracker/OCD/otros addons sólo son referencias: fuente/licencia/procedencia se auditan antes de cualquier port.

**Gate de aceptación IMD-01..08:** (01) impulso desde colisión Jolt trazable al evento; (02) daño visual sin cambio de collider; (03) dent real sin self-intersection/NaN y con espesor válido; (04) chip/agujero cerrado con FreshInterior, UV y normales; (05) materiales/NoDamage/locks preservados; (06) GUID/part/region estables, Save/Undo/Reopen; (07) collider/nav/COM coherentes tras fractura y sin física fantasma; (08) CPU/VRAM/cook/reimport y source-free medidos con fallback barato. **Todos pendientes en JE.**

## 4. Protocolo de extracción de capacidades

**P0 — Intake read-only.** Identificar source commit, archivo, autor, licencia, dependencias, fixture, riesgos, consumidores y owner JE; confirmar que la matemática es original/permitida. Revisar código JE equivalente antes de portar; evitar duplicación. Mantener manifest de componente/procedencia.

**P1 — Referencia CPU + tests.** Aislar cálculo necesario detrás de tipos propios de JE, sin dependencias de ImGui/Diligent/checkout anterior. Conservar seeds, unidades y casos límite; comparar contra el fixture del Playground con hashes/métricas cuando el algoritmo sea determinista. Un cambio intencional requiere explicación y nuevo oracle.

**P2 — Un consumidor real.** Conectar a `JE::World/Geometry/Assets`, agregar editor Qt en el workspace existente, fixture con Update/Undo/Save/Reopen y cook. Sin duplicar World owner, renderer, física, rutas o registry.

**P3 — Performance/cross-platform.** Medir CPU jobs, memoria, triángulos, tiempo de cook, Vulkan/DX12 y dirty regions. Verificar source-free/offline; comparar visual con el mismo material/cámara, no sólo compilar.

**P4 — Promoción.** Registrar estado `IMPLEMENTED`, `AUTOMATED VERIFIED`, `VISUALLY VERIFIED`, `RUNTIME VERIFIED` o `PENDING` por separado. Actualizar `ACTIVE_SLICE/HANDOFF` y tests; el Playground conserva evidencia y se puede seguir usando como laboratorio sin tener autoridad de producto.

No crear rama/worktree nueva por defecto; la rama de trabajo sigue siendo `development`. Asegurar que archivos pertenecientes a otros writers/agentes no se sobreescriban.

## 5. Verticales recomendados, secuenciales y comprobables

| Vertical | Rescate previo | Resultado visible JE | Gate de salida |
|---|---|---|---|
| V0 Contratos | Mapa de tipos/owners, source y fixture | Ningún runtime nuevo; baseline documentado | Dependencies/licencias/schemas y tests de source verificados |
| V1 Superficie compartida | `SurfaceFeatures` + `DamageField` | Inspector de región/curvatura/masks en escena BSP | IDs/lineage, signed curvature, cache/cancel/dirty, CPU ref |
| V2 Procedural Studio + PWAS | `SplineConstruction` + semantic `Assemblies` | Habitación, tubería/repisa y placement con locks | [PWAS-01..12](PWAS_TECHNICAL_MASTER.md), source-free, diff/undo |
| V3 EPLS + RBrush + Materials | Masks, UV/bakes, campos de Geology | Polvo/humedad/óxido en esa misma escena | [EPLS-01..14](EPLS_TECHNICAL_MASTER.md), no recook estructural |
| V4 Daño/Destrucción | Cutters/Edge/Path/fragments | Muesca/fractura con interior/collider/Jolt cuando corresponda | Geometría cerrada, fresh ID, Save/Cook, coste y Beauty |
| V5 Extensiones | Web, rocas/estratos, chain/catenaria | Consumidores propios conforme prioridades N8 | Evidencia por sistema, sin afirmar gate total |

V1/V2/V3 reutilizan `N1/N2/N4/N8` **sin obligar a terminar las otras familias**. V4/V5 no autorizan flotar objetos: `Enable Buoyancy` permanece DESIGN ONLY.

## 6. Demostración de extremo a extremo para PWAS + EPLS

1. En el JericoEditor verdadero, construir **cuarto BSP, corredor, puerta y ventana**; Save/Reopen valida World/CSG.
2. Procedural Studio crea una pieza con part/socket/UV/collider/material propios y la publica en catálogo.
3. PWAS coloca varias piezas/soportes y propone mobiliario sin bloquear corredor o aperturas; se bloquea una y se cambia seed. Diff prueba que manual/locked/Remove sobrevivieron.
4. EPLS usa materiales y campos explícitos para polvo en superficie apropiada, humedad en la filtración y óxido sólo en metal; RBrush pinta un override. Cambiar sólo polvo conserva IDs y colliders PWAS.
5. Preview/Cancel/Apply/Undo/Redo/Save/cerrar/reabrir/Cook/`.epak`/runtime source-free prueban cada ownership y dependencia. Jobs viejos/cancelados no publican después de revisiones nuevas.
6. Evidencia: inputs y SHA, builds reales por Windows DX12/Vulkan y Linux Vulkan, tests focales/headless, captura real, CPU/GPU/memoria/cook y defectos Beauty. Sin prueba, `NOT TESTED`.

**Criterio de bloqueo:** si falta una referencia, permiso, dependencia, tests de consumer o respuesta de owner, documentar fallo y no promover estado a PASS. Este master no autoriza cambios al Playground, sólo determina cómo examinar y adaptar resultados al nuevo JE.
