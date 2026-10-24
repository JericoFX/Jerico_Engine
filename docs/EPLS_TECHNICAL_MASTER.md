# EPLS — contrato técnico de capas ambientales

**Fecha:** 2026-10-24  
**Nombre canónico:** Environmental Procedural Layer System (EPLS). `ELSP`/`ELPS` no crean otro módulo.  
**Estado:** SPECIFIED / ENGINE IMPLEMENTATION NOT VERIFIED  
**Owner previsto:** JE::Surface/Materials + World authoring, usando el único Asset/Transaction/Cook y consumidores runtime de JE.

Extiende [JE_LEAN_ENGINE_AND_AUTHORING](JE_LEAN_ENGINE_AND_AUTHORING.md), [PROCEDURAL_PHYSICS_ENVIRONMENT_MASTER](PROCEDURAL_PHYSICS_ENVIRONMENT_MASTER.md), [PWAS_TECHNICAL_MASTER](PWAS_TECHNICAL_MASTER.md) y [PLAYGROUND_TO_JE_INTEGRATION_MASTER](PLAYGROUND_TO_JE_INTEGRATION_MASTER.md). `GOAL.md` conserva precedencia. La fuente histórica es `JericoFX/JericoEngine/docs/PROCEDURAL_PWAS_EPLS_MASTER.md` (blob `4f89d6ba268a78148d40450ae6cdb545d15c1e8f`, 1.003 líneas); no copiar su código ni atribuir sus verificaciones a O3DE.

## 1. Propósito y fronteras

EPLS describe **historia ambiental observable** y estado físico/material de superficies: humedad, polvo, suciedad, óxido, musgo, hongos, nieve, hielo, hollín, filtraciones, grietas de aspecto superficial, desgaste, telarañas y crecimiento. La forma del nivel la decide BSP/Geometry/Geology; el placement de objetos y conjuntos, PWAS; físicas, Jolt; partículas aéreas, VFX; representación, Materials/Renderer. EPLS orquesta señales y capas, **no recrea sus owners**.

- Un preset puede mezclar mask/material/normal/decal con una solicitud **explícita** de bevel, cavity, chip o fractura al owner Geometry/Destruction. EPLS **no corta topología** ni cambia collider implícitamente.
- Un parche de musgo con volumen, telaraña física o escombro es petición a PWAS/Procedural Studio y eventualmente Jolt; no un scatter oculto dentro del shader EPLS.
- El estado ambiental canónico (wetness/snow/rust/coverage) se mantiene separado de la respuesta por calidad de shader/cámara. No hacer que suciedad «desaparezca» lógicamente al bajar gráficos.
- Los materiales físicos, audio tags y colisión se publican por contratos de sus owners; la máscara PBR no inventa masa, fricción ni un rigid body.
- Authoring es offline/preview/cook; dinámica genuina (agua/temperatura/tiempo/daño) sólo por componente runtime mínimo autorizado, nunca recompilar el grafo entero por frame.

## 2. Fuentes compartidas y elegibilidad

| Fuente | Responsable / semántica |
|---|---|
| Surface ID, material role, transform, normal, tangent, UV/triplanar | JE::Geometry/World; referencias estables o invalidación explícita |
| Signed curvature, convex/concave, edge distance, cavity/contact, exposure, thickness | SurfaceFeatureField compartido o equivalente JE, **sin campo paralelo por capa** |
| Estratos, fallas, erosión, sedimento y FreshInterior | JE::Geology/Geometry; mismo campo para forma, material y daño |
| Altura, gravedad/world-up, pendiente, orientación | World/Geometry; unidades y espacio de coordenadas explícitos |
| Agua, leak volume/path, humedad, temperatura, viento, sol/exposición | Environment; input ausente se diagnostica, no se simula encubiertamente |
| Contactos, clearance y geometría vecina | World/Jolt query aprobado; coste acotado/caché |
| Máscaras `NoGenerate`/exclusiones/locks, RBrush strokes | World/Mask/RBrush; las decisiones pintadas son autoritativas |
| Tags del material físico/visual, absorción/elegibilidad | Smart Materials y Physical Material; sin asumir que metal, madera y roca envejecen igual |

No usar una normal de sombreado como pendiente estructural sin indicar la fuente. Proyección world-space/triplanar debe conservar semánticas/geometría estables tras LOD/reimport; geometría nueva tiene `FreshInterior` y se evalúa con reglas propias, no hereda automáticamente el desgaste de una cara eliminada.

## 3. Tipos de capas y salidas permitidas

| Capa | Factores de elegibilidad que deben declararse | Salida habitual / owner |
|---|---|---|
| Wetness / Leaks | Volumen/flujo, pendiente, cavidad, contacto, absorción | Mask/material roughness/darkening/decals; Environment como fuente |
| Rust / Corrosion | Material metálico, humedad, exposición, juntas, drenajes | Material mask/color/roughness/normal; Geometry sólo por daño explícito |
| Dust / Dirt / Soot | Orientación, oclusión/cavidad, exposición, fuente y pintura | Máscara, blending, decal; polvo aéreo pertenece a VFX |
| Moss / Fungus | Humedad, sombra, material, temperatura, contacto, superficie | Material o decal; volumen físico/asset por PWAS |
| Snow / Ice / Frost | Temperatura, orientación, shelter, altura, precipitación | Masks/materials/decals; geometría/collision explícita si procede |
| Cracks / Edge wear | Curvatura/edge, DamageField, intensidad, material | Normal/decal barato; chip/through-hole se delega a Geometry/Destruction |
| Webs / Biological growth | Anchors, zonas protegidas, humedad/abandono cuando se definan | Card/texture o instancia por PWAS; solver/tearing propios del owner |
| Fresh damage aging | FreshInterior + tiempo/agua/exposición según regla | Material layer; conservar lineage y dominio de superficie |

Esta tabla define familias, **no coeficientes universales** ni aprobaciones artísticas. Las reglas deben permitir arte manual; un valor no provisto no se rellena con ruido aleatorio.

## 4. Evaluación matemática mínima

La base técnica a validar antes de optimizar:

1. Capturar snapshot de superficies/World/Environment con revision/hash y coordenadas consistentes.
2. Construir o consultar campos reutilizables de geometría: curvatura firmada, convexidad/concavidad, distancia a borde, cavidad, espesor, exposición y campos de geología/daño.
3. Para cada capa, computar máscara procedural normalizada `P(x) ∈ [0,1]` desde una función documentada `rule(inputs, parameters, seed)`. Determinar explícitamente qué inputs faltantes desactivan la regla.
4. Aplicar include/exclude, roles de superficie, locks y pintura autoritativa. `ManualOverride` no es sólo un factor multiplicativo: puede reemplazar la propuesta dentro de una región; `ProtectedMask` anula operaciones procedurales sin borrar el stroke manual.
5. Componer capas en orden explícito y por canal (base color, roughness, metallic, normal, AO, opacity, emissive, etc.) respetando color-space, PBR y el material fuente. Las máscaras no siempre son conmutativas: guardar el orden y la función de mezcla.
6. Producir mask/layer/decal/bake y, cuando se soliciten efectos físicos o geometría, una solicitud tipada al owner. Distinguir preview de resultado aplicado; no modificar World durante el cálculo.

Modelo matemático **orientativo de interfaz, no una curva artística ya aprobada**:

~~~text
P_i(x) = saturate( rule_i(GeometryFields(x), Environment(x),
                         MaterialTags(x), Parameters_i, Seed_i) )
M_i(x) = ResolveAuthority( P_i(x), Include/Exclude(x),
                           Protected(x), ManualOverride_i(x) )
Material_{i+1}(x) = BlendByChannel(Material_i(x), Layer_i, M_i(x))
~~~

`ResolveAuthority` debe especificar precedencia en una tabla y fixtures antes de codificar. Para cada familia es obligatorio un caso límite (superficie sin agua, material incompatible, normal invertida, cavidad protegida) y comparar CPU reference con GPU/bake dentro de tolerancias publicadas. No copiar fórmulas de addons privados/GPL ni usar ruido de cámara. No asumir que un SDF que sólo define una isosuperficie es distancia exacta para wetness/contacto.

**Tiempo/historia:** distinguir `StaticBaked` de `DynamicRuntime`. Si una regla es temporal, registrar reloj/epoch/input temporal explícito, pasos y persistencia; definir si el estado depende de la historia (acumulación/secado) o sólo de fuentes actuales. Cambios de quality tier no reescriben esa historia. Flotabilidad/WaterVolume permanece DESIGN ONLY y no se activa por una capa de humedad.

## 5. Receta, superficies e identidad

Los siguientes campos son contrato **conceptual** a mapear al serializer/reflection de JE, no nombres de clases que ya existan:

~~~text
EPLSRecipe:
  recipeGuid, schemaVersion, seed64
  worldDocumentGuid, targetSurfaceRefs[], targetRegionRefs[]
  orderedLayers[] { layerGuid, type, enabled, blendMode, channelMasks,
                    sourceRefs, parameters, seed, includeExclude,
                    protectedRefs, manualOverrides, outputPolicy }
  dependencyVersionsAndHashes, qualityAndBudgetPolicy

EPLSResult:
  inputHash, outputHash, layerSurfaceMasks[]
  materialParameterPatches[], decalInstances[], bakeRequests[]
  requestsToPWASOrGeometry[], diagnostics[], dirtySurfaceRefs[]
~~~

- `SurfaceRef` enlaza región/cara/mesh derivada y versión; jamás reproyectar a la pared «más cercana» tras CSG o cambio de LOD si se perdió el owner. Resolver por lineage declarado o informar referencia invalidada; el dato se puede preservar para recuperación manual.
- Las máscaras pintadas y capas generadas se conservan separadas de la textura base, con unidades, resolución/proyección, color-space/packing y hash. Fuente reimportada no se modifica destructivamente.
- Guardar orden y enable de capas, seed, nombres de canal, reglas de compatibilidad de material y overrides. Evitar una nueva codificación de materiales paralela a `.rmat/.rtext`.
- Capas EPLS pueden solicitar instancias con ID/owner PWAS; el mismo objeto no queda simultáneamente owned por ambos sistemas.

## 6. Autoría, Undo, invalidación y runtime

- `Preview` enseña capa individual, mezcla final, fuentes que la causaron, cobertura, máscaras y coste; `Diff` muestra cambios respecto al último Apply.
- `Apply` publica una transacción con revisions verificados; `Cancel` no publica. `Undo/Redo/Save/Reopen` conservan máscara, orden de capas, pintura y refs.
- Cambiar sólo una máscara/material EPLS **no** regenera rock mesh, BSP, collider, nav ni placement si no existe una dependencia explícita; es el gate de rendimiento primordial.
- Cambiar la geometría mediante bevel/fracture invalida únicamente superficies y masks afectadas; remapear por stable lineage; missing support implica diagnóstico antes de cook.
- Jobs pesados cancelables, publicación de la revisión más nueva únicamente, dirty region/surface explícita, presupuesto de memoria y registro de tiempo CPU/GPU/readback.
- Shipping carga productos cocinados (mask/material/decal/instancia), no el grafo autoral. Cambios dinámicos admitidos usan runtime mínimo; un efecto visual de baja calidad no elimina el estado lógico de wetness/snow/rust.

## 7. UI JericoEditor

Workspace acoplable en el **mismo** Qt/JericoEditor, con selección compartida de World, Geometry, Materials y RBrush. Inspector mínimo:

- Stack de layers: nombre/tipo, enable, orden, blend por canal, intensidad, preview individual/final.
- Source panel: volumen, spline, material/tag, field sampling, exposición, temperatura/humedad, errores de input.
- Target panel: regiones/caras/objetos, `NoGenerate`/exclusiones, locks, FreshInterior, refs inválidas.
- Painting: máscara y stroke por RBrush, reemplazo manual y protección explícitos; no borrar overrides con otra seed.
- Preview/Diff/Apply/Cancel/Undo/Redo y costes/budgets visibles; botón «crear geometría» hace solicitud con owner identificado, nunca un corte silencioso.
- Debug: mostrar field, mask, coverage, material-channel deltas y fallos de lineaje; cámara/LOD no alteran el estado canónico.

No construir un Material Editor nuevo ni un scatter EPLS alternativo; los controles comparten el viewport y el graph frontend de dominio ya existentes.

## 8. Fuentes reutilizables desde Playground

[PLAYGROUND_TO_JE_INTEGRATION_MASTER](PLAYGROUND_TO_JE_INTEGRATION_MASTER.md) fija SHA y evidencias. Candidatos: `SurfaceFeatures.cpp/.hpp` para edge/curvature/cavity/espesor, `DamageField.cpp/.hpp` para pesos/masks, `RBrushInterop` para máscara pintada y `NormalBake/AutoUv` para derivados. Son **experimentos**, no un consumidor EPLS completo. No copiar Diligent/ImGui ni lógica geológica antigua como runtime. Primero validar semántica sobre el mismo mesh con source ↔ JE; el material/cook lo posee JE.

Cracker, Damage Control, Edge Decal, Infinite Fracture y otros addons históricos fueron referencias matemáticas: revisar procedencia/licencia antes de portar cualquier fragmento; no transportar scripts/graphs licenciados por analogía.

## 9. Pruebas de aceptación (pendientes)

| ID | Escenario | Gate |
|---|---|---|
| EPLS-01 | Sala BSP con metal/madera/roca | Óxido sólo elegible en metal; otras reglas diagnostican material no válido |
| EPLS-02 | Filtración en junta y zona seca control | Máscara húmeda trazable al volumen; sin humedad mágica |
| EPLS-03 | Piso/techo/pared, cavidad convexa y cóncava | Orientación y curvatura correctas con winding/transform |
| EPLS-04 | NoGenerate/Protected + pintura manual | Procedural excluido; override manual y lock sobreviven a seed |
| EPLS-05 | Strata/cueva/FreshInterior | Mismos campos geológicos en superficie/material; interior no adopta máscara de cara eliminada |
| EPLS-06 | Edge wear visual vs chip geométrico | Visual no modifica collider; geometría sólo vía owner explícito |
| EPLS-07 | Cambiar polvo sin mover repisa/puerta | GUIDs PWAS/BSP y collider/nav iguales; dirty work sólo de material |
| EPLS-08 | Preview/Cancel/Apply/Undo/Redo | Cancel no publica; replay y orden de capas idénticos |
| EPLS-09 | Surface ref rota por CSG/reimport | Diagnóstico/lineage o rechazo, nunca proyección a pared vecina |
| EPLS-10 | Worker count/seed y CPU/GPU | Masks deterministas según tolerancia; no diferencia lógica por cámara/LOD |
| EPLS-11 | Bake/Save/Reopen/source-free | Render/materiales y refs válidos sin graph de autoría |
| EPLS-12 | Dynamic wetness permitido, quality bajo/alto | Estado persistente igual; cambia representación, no historia |
| EPLS-13 | Job lento obsoleto y edit nuevo | Revisión antigua no publica |
| EPLS-14 | Solicitud de musgo físico/telaraña | Placement generado por PWAS; sin doble owner |

Primer vertical: una escena BSP con metal, madera y roca; EPLS aplica polvo/humedad/óxido mediante campos explícitos, RBrush corrige una zona, PWAS permanece intacto, Undo/Save/Reopen/Cook reproducen el resultado. Medir CPU/GPU/memoria y backend; distinguir lo probado de lo visualmente aceptado. **Este documento no es un PASS de implementación.**
