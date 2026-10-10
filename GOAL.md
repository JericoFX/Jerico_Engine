# JericoEngine — objetivo persistente sobre O3DE

Fecha de decisión: 2026-10-10. Estado de producto: **ACTIVE / IMPLEMENTACIÓN O3DE NO VERIFICADA**.
Repositorio de producto: `JericoFX/Jerico_Engine`. Única rama de trabajo: **`development`**.

## 1. Decisión del propietario y precedencia

Joan decidió abandonar CryEngine 5.7 como base de desarrollo y transformar esta base O3DE en **JericoEngine**, priorizando la procedencia legal y la integración directa de capacidades. Esta decisión sustituye las instrucciones de volver a Cry, incorporar Diligent obligatoriamente, derivar navegación de MNM o portar SVOGI desde Cry. No convierte esas implementaciones en código permisivo.

«Sin Gems» significa que las capacidades Jerico son subsistemas nativos del producto: no se instalan ni se habilitan manualmente como una colección de extensiones por proyecto. **No significa borrar `Gems/`, eliminar sus implementaciones útiles ni reescribir el mecanismo de módulos antes de tener una base funcionando.** Se permiten bibliotecas privadas, targets separados y los mecanismos internos de registro que realmente necesite O3DE. La selección y la composición son responsabilidad del motor.

Orden de autoridad: última decisión explícita del propietario > este GOAL > AGENTS > decisiones de arquitectura y contratos de familia > ROADMAP > ACTIVE_SLICE. Los documentos de los repositorios anteriores son fuentes de requisitos e historia, no instrucciones ejecutables que puedan reinstalar decisiones sustituidas.

Esta publicación es documental. No borra repositorios, ramas, código, assets, cachés ni trabajo ajeno. No inicia automáticamente la implementación de todas las familias.

## 2. Base exacta y fuente histórica

- O3DE admitido para arrancar: `241c78b8064898a709cf1c5dd3535c6406763ef3`, árbol `db4b7d4a5d784d2283c95903f461e311d69704c8`. Ambos coinciden con el objeto del upstream oficial `o3de/o3de` inspeccionado.
- `development` declara versión interna `2.8.0`. No se la declara versión estable ni base compilada por esta auditoría.
- Fuente histórica Cry: `JericoFX/JericoEngine2` en `304d9d2e13704e4967e82a8d5a13a5a506be01d4`. No se importa su historial, fuente, shaders, binarios ni paquetes.
- Las otras 22 ramas del nuevo repositorio son referencias inactivas. No hay líneas de desarrollo Jerico paralelas ni fusiones masivas. Véase `docs/research/BRANCH_AUDIT_2026_10_10.md`.

## 3. Producto final

Motor generalista para una persona o equipo pequeño, con editor real y runtime real: importar, preparar materiales, construir arquitectura, configurar física, crear rigs/animación, programar, montar UI/sonido/VFX, simular, guardar, cocinar y empaquetar desde herramientas nativas.

Familia de producto: JericoEngine, JericoEditor, JericoRuntime, JericoLauncher, JericoCooker/AssetProcessor, JericoShaderCook, JericoPackageTool, JericoProfiler/Doctor/RuntimeQA, UI Studio, Motion Studio, Procedural Studio, Item/Narrative Studio, JericoAudio, JericoParticles y JericoXR. No crear un ejecutable ni una biblioteca por cada nombre sin justificar su ciclo de vida.

La identidad visible, APIs públicas propias, configuración de proyecto y contratos finales son Jerico. Los nombres y avisos de terceros se conservan donde correspondan. Un reemplazo global de `AZ`, `Atom`, `Cry` o `Amazon` no es una migración ni una prueba legal.

## 4. Decisiones tecnológicas vigentes

| Área | Destino y límite |
|---|---|
| Base | Transformación del O3DE existente; no segundo motor al lado |
| Gráficos | Atom RPI/Pass System/RHI como una única ruta, backend Vulkan |
| Plataformas activas | Windows y Linux; cada una exige pruebas propias |
| DX12/Metal/WebGPU/móvil | No son gates actuales; fuentes preservadas, trabajo diferido |
| Ray tracing | Sin RT de render en esta fase; no dependencia de BLAS/TLAS/ray queries para las funciones admitidas |
| Diligent | Referencia/playground o reutilización puntual legalmente auditada; no RHI obligatorio ni segunda capa bajo Atom |
| Editor | Qt 6 Widgets, aprovechando el editor y AzToolsFramework existentes; no reescritura QML por defecto |
| Física | Jolt como destino único; PhysX transitorio hasta migrar consumidores y probar equivalencia relevante |
| Scripts de juego y UI Jerico | Luau `--!strict`, API tipada y validación nativa de referencias |
| Scripts internos de herramientas | Python y Lua de herramientas/materiales pueden sobrevivir acotadamente; no constituyen una segunda VM de gameplay Jerico |
| Navegación | Auditar Recast/Detour y su integración O3DE; no copiar MNM de Cry |
| IA | HTN explicable, percepción/memoria/relaciones y acciones nativas; un único planificador de alto nivel |
| Animación | Auditar/reutilizar EMotionFX y sus herramientas; extender hacia Motion Studio, IK/FK y formatos Jerico |
| Audio | JericoAudio; backend abierto de bajo nivel privado, MiniAudio candidato; no Wwise/FMOD obligatorios |
| UI | JericoUI nativa, Luau, `.rui2d/.rui3d` de autoría y `.rui` cocinado; auditar LyShine y `shine` |
| XR | OpenXR como capacidad oficial integrada, no plugin externo que el usuario deba instalar |
| Assets | Una identidad persistente, un grafo de dependencias/cook, lectores cocinados; formatos Jerico versionados |
| AWS | Sin cuenta, servicio ni SDK AWS obligatorio en producto; independizar también LFS/paquetes de distribución cuando se cierre esa fase |

## 5. Requisitos que no se pueden perder

### Editor y mundo

Viewport compartido entre workspaces; jerarquía izquierda, Inspector contextual derecho y timelines/logs abajo. Conservar selección, gizmos, docking, prefabs/variantes, layers, Undo/Redo, guardado y recuperación. Edit/Simulate/Play usan los sistemas reales. No duplicar World, Scene, registro GUID, asset database, undo stack o reloj para introducir una feature.

Herramientas de geometría con workflow de brushes estilo Hammer 2, identidad propia y White Box como candidato, no como supuesto BSP/CSG completo. Modelado de caras/extrusión/inset/bevel/cortes con UV y colisión; producto runtime mallas/colliders modernos, no obligación de un árbol BSP histórico.

### Assets y materiales

GLB/glTF 2.0, FBX mediante `ufbx` de tools, OBJ/MTL; texturas PNG/JPEG/TGA/TIFF/HDR/EXR/DDS y KTX2/Basis; heightmaps PNG16/RAW16/32/EXR/TIFF; audio WAV/FLAC/OGG. Sin Autodesk FBX SDK ni ATL/MFC como nuevos requisitos impuestos al propietario. Los formatos se incorporan por verticales, no se declaran disponibles por listarlos.

Nombres libres de texturas; semánticas Base Color, Normal, Roughness, Metallic, AO, Emission y Opacity explícitas. Admitir ORM empaquetado y mapas separados, convenciones normal/color-space/mips editables. Reimport no pisa overrides ni fuentes. Una Canonical IR y un cook determinista conectado al Asset Processor existente.

### Gameplay, referencias y guardado

Drag desde Outliner al IDE crea o reutiliza referencia tipada en `self.references`, preservando GUID, capacidades, ámbito Scene/PrefabInternal y Undo/Redo. `self` se enlaza nativamente a una instancia válida; nunca se guarda un puntero. Commands piden, el subsistema decide, Events informan. Store rastrea cambios confirmados de propiedades opt-in; no obliga a repetir `Store.set` después de cada mutación nativa.

Triggers de volumen, proximidad, click, input, evento o condición; filtros, activador, receptor, Once y cooldown. Input semántico con perfiles, contextos, rebinding, gamepad/XR, desconexión y pérdida de foco. Inventory es autoridad de items/contenedores; UI solo presentación. Save/Load transaccional por proveedores/esquemas; corrupción, migraciones, guardado atómico y recuperación. Dialogue/Quest/Journal y replay de QA son capacidades distintas, no tablas arbitrarias de VM.

### Animación, interacción, UI, audio y XR

`.rskel` estructura, `.rrig` controles/roles/constraints, `.ranim` clips; edición de huesos, pesos, FK/IK, retargeting y corrección pausada de clips en el editor. Sockets, GripPoints/GrabPoints, MatePoints y FitProfiles; un InteractionSession bimanual y un solver de ajuste reutilizable para llaves, pilas, tapas e insertos. Jolt manda sobre cuerpos; IK muestra contacto, no teletransporta el objeto.

Sequencer `.rseq` con cámaras, keyframes, curvas, eventos y restauración al salir; no toma control total del movimiento del jugador por defecto. UI con grid/hover/drag/preview de inventario, tweens y timeline; UI 2D, world-space, render-target y contenido 3D. Localización `.rlang` de autoría y `.rloc` de runtime; fuentes `.rfont` con dependencias de atlas `.rtext`.

Sonido 2D/3D/ambient/attached, SoundEvents compartidos por editor y Luau, mixer, zonas/portales/oclusión/reverb; HRTF opcional. VFX económicos primero, flipbooks EmberGen, viento compartido cuando exista consumidor. OpenXR reutiliza input, interaction, UI y audio de desktop.

### Procedural, entorno, física y perfiles

PWAS/EPLS siguen siendo requisitos de autoría no destructiva: propuestas, máscaras, locks, overrides, seeds y publicación transaccional. No inventar un segundo sistema por la variante escrita ELSP. Priorizar assets modulares, puertas/cajones, tuberías, conductos, cables, soportes y barandas. `.rmod.toml` es autoría; `.rpart` queda reservado para partículas.

Una librería de splines/RMF/longitud de arco, ports orientados, sockets, snap con histéresis, fittings e intersecciones manuales, flanges/uniones, soportes mediante queries de suelo/pared/techo. Catenaria y cadenas con Jolt, daño/bake/proyección y edge chains reutilizables.

**Geología procedural integral no es un frente activo.** Conservar matemáticas y evidencia históricas como referencia; no resucitar el pipeline de rocas/cuevas ni BSP→VDB. Esto no prohíbe un importador VDB offline para VFX si se admite separadamente.

Physical Material es la autoridad física; destrucción prepara chunks/bonds/anchors, activa Jolt solo cuando corresponde, microescombros como VFX. No duplicar materiales ni solver físico. Un Environment compartido contiene tiempo, iluminación, viento y clima. Viento visual y participación física por cuerpo son cosas distintas.

Flotabilidad permanece **DESIGN ONLY** hasta autorización específica: conservar `Enable Buoyancy` por WaterVolume, proveedor físico consultable sin renderer, fuerzas/torques en Jolt y pruebas previstas. El cambio de base no activa esa implementación.

Perfiles 2D, 2.5D, Pixel2D, isométrico e híbrido/vehículos comparten el motor. Sprites con normales/iluminación, tilemaps, snapping y restricciones de plano; no resolverlos con un engine o VM alternativos.

## 6. Rendering y SIGGRAPH

Consultar `docs/RENDERING_RASTER_SIGGRAPH_MASTER.md` antes de sustituir un pass. Atom existente primero; fuente pública y contrato Jerico después; cambio mínimo con imagen/timing/memoria/fallback.

Preservar id Tech 6 completo: clustered lights/probes/decals, Stamp/RBrush, caché de sombras, rough glass, iluminación desacoplada de partículas, coherencia, resolución dinámica y profiling. RBrush separa BRUSH/PAYLOAD/TARGET; no es solo decals. Conservar PBR/Frostbite, Hillaire/Bruneton, volumétricos, Nubis, agua de Horizon, GPU-driven/HZB, SDSM, geometría/LOD y atlas histórico. Cada paper es candidato, no feature automáticamente comprometida.

No trasladar SVOGI/Cry shaders por nombre de archivo ni exigir reproducciones numéricas idénticas al renderer anterior. Mantener las lecciones de coste: medir escenas representativas, no solo fixtures pequeños, y no convertir contadores de checks en calidad o rendimiento.

## 7. Alcance de agua y restricciones heredadas vigentes

La aceptación de agua de Cry se cerró por decisión del propietario el 2026-10-09; quedaron límites y una repetición source-free sin ejecutar. Esa historia no acredita O3DE. La prioridad de producto es agua vista desde arriba; underwater y mejoras costeras son etapas posteriores, no motivos para reabrir la antigua reparación ni para bloquear N0.

## 8. Packaging y cierre

Distribución final en un directorio de producto con ejecutable, `.epak` y bibliotecas necesarias; los recursos/licencias de terceros pueden conservar subdirectorios técnicamente requeridos. Sin checkout de desarrollo, compiladores fuente, credenciales ni formatos originales como requisito de ejecución.

Los formatos finales son binarios validados, versionados y de ABI independiente. No crear XML propio; metadata de tools y formatos internos temporales se migran con sus consumidores, no mediante una purga global. El catálogo está en `docs/EDITOR_ASSET_WORLD_MASTER.md`.

Un vertical PASS exige implementación canónica, autoría/persistencia/cook/runtime relevantes y prueba negativa. Windows no acredita Linux; código presente no acredita integración; documentación no acredita runtime. Estados: PLANNED, DESIGN ONLY, IMPLEMENTED NOT INTEGRATED, INTEGRATED NOT VERIFIED, VERIFIED, DEFERRED, BLOCKED y REJECTED con causa.

El objetivo global no termina con N0, una demo o un cambio de nombres. La matriz de requisitos y `docs/DEFINITION_OF_DONE.md` impiden rebajar alcance por omisión. Continuar por verticales coherentes del roadmap sin detenerse tras microtareas, respetando exclusiones y permisos reales.
