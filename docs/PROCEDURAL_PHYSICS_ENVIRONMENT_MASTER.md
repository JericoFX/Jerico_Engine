# JE — BSP, geología, RMod, RBrush, Jolt y entorno

**Geología REACTIVADA por el propietario el 2026-10-10.** Prioridad visible N1/N2/N4/N8 según ROADMAP. Flotabilidad sigue DESIGN ONLY; AWS/multiplayer excluidos. Todo owner propio y adaptación converge a JE::; dependencias auténticas privadas conservan nombres/licencias.

## 1. Geometry Core compartido

BSP, Geology y Procedural Studio comparten geometría, regiones/material lineage, perfiles, CSG, splines, queries y cook. PWAS orquesta propuestas de escena, no reemplaza Geometry ni ownership manual. Un renderer JE adaptado de Atom, dos backends DX12/Vulkan; no importar shell/runtime/Diligent/World del Playground.

Rescatar código/math/tests originales solamente después de auditar procedencia/licencia, fixtures y adapters. Conservar SDF/Surface Nets/Hermite/QEF QR/SVD, cortes analíticos, edge chains/RMF/catenaria/daño/bake. **BSP→OpenVDB/NanoVDB como arquitectura general sigue cancelado**: geología activa no significa backend descartado activo. VDB Tool-only de VFX es decisión distinta.

## 2. BSP y edición estructural primero

WhiteBox ya tiene CSG con Manifold, componente/editor modes/asset undo handlers. Auditar `WhiteBoxCsg.cpp`, `WhiteBoxCsgCore.cpp`, tool API y consumers antes de escribir otro CSG. No declarar BSP completo por source existente. Controles: crear brush, grid/snap, caras/aristas/vértices, extrude/inset/bevel/hollow/slice/union/subtract/intersect, align/UV/tiling/materials por cara. Runtime produce mesh/collision/nav, no obliga a árbol BSP de render histórico.

Preservar SurfaceRole/EdgeRole, FaceLocalFrame y material/region GUID donde el modelo los admite. Floors/walls/ceilings, puertas/ventanas/columnas/escaleras/arcos/beams son semántica de autoría reusable, no hardcode del juego. Usar StyleSet/VariantFamily y reglas de tiling/snap donde ayuden a módulos.

Pruebas discriminantes: cuarto+corredor con aperturas, diferencia vacía válida frente a error, coplanares/tangentes/overlap, escalas negativas/extremas, nonmanifold/degenerados, winding/transform, UV/material por cara y FreshInterior después de cortar. `numProp=3` del CSG inspeccionado no transporta atributos completos; revisar reconstrucción y no asumir preservación. Límites de uint64→uint32 y tolerancias weld deben verificarse con inputs reales.

Todo: preview/diff/Apply/Cancel, una transacción, Undo/Redo, Save/reopen, collider/nav/cook/runtime y coste de reconstrucción. Datos no soportados se preservan o se rechaza el guardado; nunca borrar componentes al abrir sin módulo.

## 3. Geología nativa, no ruido genérico

### Modelo de autoría

Separar forma/espacio de material:
- Base: Rock/Boulder, Rock Family, Cliff/Massif, Overhang; Cave/Tunnel/Chamber como host sólido con espacio negativo explícito.
- Operation stack no destructivo/reordenable/enable/masks: macroshape, ruido FBM/ridged, warp, strata, juntas/fallas/fracturas, erosión térmica/hidráulica admitida, sedimento/talus/deposición, desgaste/colluvium/rockfall presupuestados.
- Seed/generator version/input dependencies/locks/overrides; selector floor/wall/ceiling es rol/material, no sustituto del generador de forma.

BSP/layout manual define volumen y espacio libre; geología lo respeta. Regiones Carve/Wrap/Blend/Exclude/Touch/Respect sólo cuando el contrato de authoring las soporte, con límites explícitos. No rellenar puerta, estrechar corredor ni alterar recorrido por cambiar seed del detalle. Cave no son rocas dispersas alrededor de un agujero: host/negativo y superficies interior/exterior bien definidos.

### Campo, mesher y cortes

Pipeline de destino: primitive/brush/working mesh→recipe de campo→evaluación dispersa CPU/GPU según tramo→Surface Nets con información Hermite→QEF acotado/QR/SVD cuando necesario→mesh/atributos→material/collision/nav/LOD/cook. Los solvers se eligen por pruebas, no por nombre.

Mantener sharp/native PlaneCut e intersecciones/normales analíticas cuando el sampling suavice planos. No confundir normal geométrica con shading normal. Validar seams, dominios locales [0,1]^3, tolerancias, ambigüedad/degeneracy y fallback. Topología cerrada, material lineage y colliders tienen gates propios; menor error de normales no acredita todo.

Ni todo brush debe convertirse a SDF ni todo ruido vivir en GPU. Mantener ruta hard-surface exacta y usar campos sólo donde aporten forma/daño. Evitar nuevo full-world volume o readbacks por operación pequeña.

### Material geológico

El mismo campo/atributo que formó la roca alimenta triplanar/world-space sin UV manual cuando corresponda: estrato/unidad/material domain, joints/cracks, slope/curvature, altura/profundidad, cavidad/erosión, depósitos y FreshInterior. Reutilizar SurfaceFeatureField/DamageField y mapping canónico a `.rmat/.rtext`; no generar otro ruido material que despegue estratos de la geometría.

Layers/masks editables; superficie cortada expone interior con material/normal correctos, wetness/dirt/moss y desgaste separado de forma. Texturas ORM/separadas, normals, PBR y color-space del master material también rigen geología. Bake a normal/decal/texture/card cuando no afecte silueta/collision/gameplay, preservando opción explícita y distancias/LOD.

### Coste y pruebas visibles

Invalidación por dirty leaves/voxels/regions, buffers reutilizados, jobs cancelables y versionados; candidate-first y una publicación. Presupuestar compute/readback/meshing/collision generation/RAM/VRAM, cache de capas e incremental rebuild. No declarar local meshing si aún se reconstruye malla completa; medir y registrar deuda real.

GEO-01 roca/familia y cliff de estratos casi horizontales con variación, undercut/ledge/overhang. GEO-02 cueva con masa/exterior/interior, paso libre y collision/navigation. GEO-03 PlaneCut oblicuo y seams, fractura/erosión/depósito con material concordante. GEO-04 mover una máscara/layer/seed conservando locks/IDs, cancel/Undo/reopen/cook/source-free. GEO-05 costes y snapshots comparables en Windows DX12/Vulkan y Linux Vulkan. Capturas de Playground son referencia histórica, no prueba de integración.

## 4. RMod y redes

`.rmod.toml` de autoría: part identity/mesh/recipe/collision/materials/tags/ports/sockets/host/rotation/stretch/variants/prefab. Produce `.rmesh/.rcoll/.rpref`, no un formato runtime nuevo sin lifecycle. `.rpart` reservado a partículas.

Port es frontera de red con frame/perfil/dimensiones/clase/compatibilidad/twist; socket es attachment/support/grip/insertion, no arista. Forward hacia fuera. Conexión: posición coincide dentro de tolerancia y forward opuesto, up/roll coherentes para sección no circular. Escala singular/handedness/perfil no compatible se valida explícitamente.

Path es intención; solver elige piezas rectas/codos/T/cruces/reductores/bridas/válvulas/tapas/roturas y auxiliares. Rectas largas pueden generarse con UV por longitud; no estirar válvulas/fittings. U/T y piezas con muchos ports no se reducen a tubo de dos extremos. Lista de modelos de carpeta configurable, preview y edición de ports/sockets nativos.

Intersección manual se confirma semánticamente, no por cruce en pantalla. Markers `==0==`/bridas deben corresponder a conexión, no decoración. Snap con histéresis y empate estable, overrides y mandatory parts. HostQuery por World/Jolt con ray/shapecasts clasifica piso/pared/techo/esquina/obstáculos, orienta soporte y clearance; no mundo físico paralelo ni soportes al aire.

Tests: recta/90/oblicuo/U/T/cruz, extremos cortos/cero, ports opuestos/rotados/escalados/incompatibles, locked parts, dos redes sin unión, techo/piso/obstáculo, undo/cook e IDs de regiones no afectadas.

## 5. Cables y edge chains

Una librería Path con arco/RMF/roll y fallback de tangente degenerada; edge chains world-space conservan continuación recta en junctions, convexidad/concavidad y roles. Mismo soporte para decals/damage/assemblies.

Catenaria ideal libre como referencia de reposo, no solver de contacto. Resolver longitud/endpoints/gravedad/tolerancia y condiciones imposibles. Cadenas 16/32/64 con endpoints fijados: comparar caso sin suelo con referencia, error/coste; no igualdad matemática exacta con joints discretos. Contacto/caída/interacción pertenecen a Jolt. Visual interpola estado, no una U fija que ignora colisiones.

## 6. PWAS/EPLS y RBrush

PWAS produce propuestas de layout/dressing/módulos; EPLS capas/señales de superficie/entorno. No crear segunda implementación por la variante escrita ELSP. Recipe GUID/type/version/seed/dependencies/parameters/masks/locks/manual overrides/generated IDs, preview/diff/Apply transaccional. El diseñador decide: cambios de seed no pisan BSP/aperturas/espacio libre aceptados.

Implementación y aceptación se especifican separadamente en [PWAS_TECHNICAL_MASTER](PWAS_TECHNICAL_MASTER.md) y [EPLS_TECHNICAL_MASTER](EPLS_TECHNICAL_MASTER.md). [PLAYGROUND_TO_JE_INTEGRATION_MASTER](PLAYGROUND_TO_JE_INTEGRATION_MASTER.md) fija qué código matemático puede auditarse como referencia, qué pruebas históricas existen y qué gates nuevos debe pasar en JE. No considerar el éxito de un prototipo Diligent/ImGui como una prueba de Editor, Cook o Runtime O3DE.

RBrush: BRUSH=cobertura, PAYLOAD=dato, TARGET=destino. Vertex paint, surface/terrain masks, decals, sculpt o daño usan outputs distintos y owners comunes donde procede. `.rbrush` authoring, imágenes/ABR externos import-only con licencia. Strokes editables guardan seed/spacing/jitter/transform/depth/angle/channel masks/receiver/layer/order/locks/references. No modificar fuentes destructivamente.

Decals JE sobre implementación Atom auditada: OBB/projection, correct gradients/mips, blend por canal de color/normal/roughness/metal/emissive, orden estable/fade/receiver/LOD/rigid/bone attachment cuando exista. Probar metal puro, ángulos rasantes, overlaps/arrays overflow y movimiento. Shared clustered registry sólo si beneficia sin duplicar render graph. 3D damage niveles mask/indentation/cavity/through-hole/physical break, FreshInterior y geometry/collider update sólo cuando sean necesarios.

## 7. Jolt, destrucción y viento

Jolt único owner de body/shape/query/constraint/character/ragdoll/vehicle y fixed step. Inspector con masa/COM/inertia, material físico, joint pivot/limits/friction/spring/motor/break, contactos y debug; cambios en safe point. Door=body+hinge+grip; drawer=body+slider+grip. PhysX sólo transición con consumers y removal tests, nunca dos writers para body.

Fracture authoring: prefracture/chunks/bonds/anchors/interiores/material lineage/collision; activar fragmentos locales Jolt y microdebris como VFX. Dynamic fracture opt-in/bounded, no simulación general carísima por cada impacto. Material físico separado del visual, sin DestructionMaterial paralelo.

Environment único con tiempo/sol/luna/sky/fog/clima/viento/agua. Physical wind participation por body y respuesta cuantitativa distinta de mesh bending; fan es emitter del mismo campo. Velocidad relativa/fuerza/torque en fixed boundary, disabled/removed deja cero fuerza extra pero no borra velocidad previa. Tests incluido/excluido, save/reopen, substep y streaming.

## 8. Flotabilidad: preservada, DESIGN ONLY

No implementar sin autorización específica. WaterVolume `Enable Buoyancy`, provider de ocupación/superficie/densidad/velocidad consultable sin renderer; Jolt aplica fuerzas. `F_b = -normalize(g)*rho*|g|*V_submerged` en centro de empuje, torque respecto a COM. Normal de ola no sustituye gravedad. Muestras ponderan volumen total, no lo multiplican. Drag relativo, límites/substeps, vacío/solapamiento/casco hueco/sleep/provider missing con política.

No desplazar transforms cosméticamente para simular flotación ni readback GPU por body obligatorio. Gate futuro: plano headless, densidad/masa descentrada/ola/corriente/seco-húmedo/joints/múltiples fluidos y save/load. Esta sección no bloquea BSP/geología ni se activa por reactivar N8.
