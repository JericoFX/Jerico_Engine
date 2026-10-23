# JericoEditor — Guía visual y atlas de herramientas (referencia de diseño)

**Estado:** DESIGN REFERENCE ONLY / NO IMPLEMENTATION PROOF.  
**Fecha:** 2026-10-23. **Owner:** JericoFX.  
**Destino:** JericoFX/Jerico_Engine, rama `development`.

> Esta guía complementa, pero no reemplaza, `GOAL.md`, `docs/EDITOR_ASSET_WORLD_MASTER.md`, `docs/ANIMATION_UI_AUDIO_XR_MASTER.md`, `docs/JE_LEAN_ENGINE_AND_AUTHORING.md`, `docs/ROADMAP.md` y `docs/REQUIREMENTS_MATRIX.md`. Si difiere, manda el contrato vigente.

## 1. Principio visual

Un **único JericoEditor Qt6**, no miniprogramas separados: World, selección, escena, viewport, historial, Asset Processor y Cook compartidos. Los workspaces cambian paletas, herramientas, overlays, Inspector y docks inferiores, sin duplicar autoridad. Estilo de las referencias 16:9 aprobadas: grafito/navy, fondo blueprint técnico **solo para el póster** (no es chrome real), logotipo J modular naranja/blanco, acento naranja, cian secundario, widgets compactos, barras claramente separadas, viewport realista. Inspiración de productividad tipo Hammer 2 sin clonarlo.

Layout: menú/toolbar/workspaces arriba; Outliner, Asset Browser o paleta a izquierda; viewport/canvas/grafo al centro; Inspector de entidad/componentes a derecha; logs, Problems, Timeline, Curves, Graph, Simulation o Profiler en docks inferiores. Paneles redimensionables, acoplables, persistentes, con DPI y multi-monitor. Edit / Simulate / Play reales. Undo/Redo compartido.

## 2. Design tokens **exactos**

| Token | HEX | Uso |
|---|---|---|
| Shell | `#081420` | Chrome global |
| Surface | `#0D1A26` | Superficie |
| Panel | `#112334` | Docks |
| Raised | `#162C40` | Superficies elevadas |
| Border | `#24465D` | Delimitadores |
| Primary cyan | `#00C8E8` | Operaciones, líneas, conexiones |
| Active orange | `#FF9418` | Tab/acción activa |
| Selection | `#FFB347` | Selección |
| Primary text | `#E8EEF4` | Texto e iconos |
| Secondary text | `#AEBBC6` | Metadatos |
| Muted text | `#748897` | Inactivo/secundario |
| Grid minor | `#173347` | Rejilla |
| Grid major | `#29536B` | Rejilla mayor |
| Success | `#2BC76F` | Correcto |
| Warning | `#F3B43F` | Advertencia |
| Error / X axis | `#EF5350` | Error / eje X |
| Y axis | `#42D37A` | Eje Y |
| Z axis | `#42A5F5` | Eje Z |

**Los HEX en imágenes generadas pueden tener erratas: esta tabla es autoridad.** No depender exclusivamente del color para estados.

Densidad de referencia al 100%: toolbar/control 28–30 px, filas compactas 22–24 px, listas 26–30 px, tabs 28–30 px, gaps entre paneles 6–8 px y entre grupos 8–12 px. Adaptar DPI y accesibilidad. Biblioteca única de tabs, árboles, Inspector/reflection, sliders+numeric, asset pickers, progress, layout, botones, dropdown, tooltip, estado vacío, búsqueda, timeline, graph, iconos. Estados Idle / Hover / Pressed / Focused / Disabled / Busy / Warning / Error / Success. Foco teclado visible.

## 3. Iconos

La marca J modular es separada de los iconos funcionales. **Propuesta de iconografía**, pendiente de aprobación como asset final: SVG monoline alineados a cuadrícula 24×24, aproximadamente 1.75–2 px trazo, legibles en 16/20/24. Neutro blanco/gris, naranja para activo, cian para manipulación. No emojis ni marcas de otros motores. Asociaciones: BSP cubo cortado/cara/arista; Procedural perfil/patrón/ensamble; PWAS objetos distribuidos; EPLS capas/gota; RBrush pincel/radio; Motion huesos/IK; Sequencer pistas/diamantes; VFX emisor/partículas; Audio onda; AI ruta/sensores; Luau código/breakpoint; XR visor/manos; Profiler gráfica/reloj; Cook paquete. Texto o tooltip para acciones ambiguas.

## 4. Atlas: workspaces y responsabilidades

| Workspace | Contrato visible / salida |
|---|---|
| World / Scene / Prefab | Outliner, componentes, Layers, overrides, GUID, snap, Play/Simulate |
| JericoBSP + Hard Surface | Brush/CSG source tree; stack reversible de operaciones derivadas; UV/material por cara; validación/mesh/collision/nav |
| Geology | Rock/Cliff/Cave, estratos/erosión, campos, Surface Nets donde corresponde, bake incremental |
| Procedural Studio | **Fabrica** piezas paramétricas, puertas, tuberías, escaleras, sockets/pivotes; biblioteca semántica |
| PWAS | **Propone, distribuye y ensambla** instancias; seeds, exclusiones, locks, diff, preserva layout manual |
| EPLS | **Aplica capas ambientales** (humedad, óxido, polvo, musgo, telarañas, nieve); nombre canónico EPLS, no módulo ELSP |
| RBrush | **Brush/Payload/Target**: material, máscara, decal, paint, damage; no segundo motor de materiales |
| RMod / Splines / Sockets | Rutas, T/U, bridas, soportes, cables/catenaria, rieles, snap con histéresis |
| Smart Importer + Asset Browser | Fuente inmutable, GUID, unidades, LOD, colisión, mapeo PBR explícito con **nombres de textura libres**, reimport y Cook |
| Materials / Smart Materials | Propiedades y Nodos de un mismo material, instancias, textura ORM/separadas, color-space, variantes y preview |
| Physics / Interaction | Jolt, body/collider/hinge/slider/COM/constraints, Grip/Mate/Fit, simulate/step |
| Damage / Destruction | Fracture/bonds/anchors/FreshInterior, Jolt mueve los fragmentos, debris como VFX |
| Motion Studio | Rig Stage gris, Motion Stage gris, Scene Viewport; bones/skin/Control Rig/FK/IK/contacts/curvas; `.rskel/.rrig/.ranim/.rgraph` |
| Jerico Sequencer | Timeline global reutilizable `.rseq`: camera/cuts, actores, animación, propiedades, luces, audio, VFX, UI, Luau/events |
| JericoVFX / JericoParticles | Emisores, bursts, flipbooks EmberGen, curva lifetime, GPU/CPU, bounds/culling/overdraw; `.rpart` |
| JericoAudio | SoundEvents, buses/mixer, fuentes, zonas, oclusión, reverb, surfaces y voice budgets |
| AI / Nav | NavMesh/Recast/links, HTN, percepción, debug de arañas ciegas y perros, decisiones explicables |
| JericoUI | Canvas 2D/3D, inventario/presentación, bindings Luau, tweens, foco, localización; `.rui` |
| Luau IDE | `--!strict`, reflection, autocompletion, referencias tipadas por drag Outliner, debugger, problemas |
| Items / Narrative | Definiciones de items, diálogo, quests/journal, inventario con owner único |
| Lighting / Environment / Weather | Luz, cámaras, un clock, sol/cielo, lluvia/nieve/viento, exposición; RT/DDGI conservados según GOAL actual |
| XR | OpenXR nativo, vistas/manos/actions, frame timing y preview en hardware cuando esté disponible |
| Profiler / Runtime QA / Cook | CPU/GPU/RAM/VRAM/overdraw, dependencias, reimport, errores, `.r*` y `.epak`, pruebas aisladas |
| Localization / Input / Save | `.rlang/.rloc/.rfont`, remap/foco/haptics, restore/replay QA local |

## 5. Una gramática de Timeline, no un sistema por herramienta

Jerico Sequencer posee la **orquestación multi-entidad** a través de `.rseq`. Motion tiene Dope Sheet/Curve Editor con datos `.ranim`; VFX usa curvas local-life y bursts `.rpart`; UI tiene tweens locales `.rui`; Audio envelopes y cues; todos reutilizan widgets temporales sin crear otro timeline/runtime global. Los eventos cross-system se representan en `.rseq`.

Controles coherentes: timecode en segundos/frames, playhead naranja, Play/Pause/Stop/Step, Record/Auto Key cuando aplica, zoom, scrub, snap, loop, range, mute/solo/lock, keyframes diamante, curvas/tangentes Bezier, track binding por GUID y diagnostics. Tracks Sequencer: Camera Cuts, Camera/Transform, Animation, Door/Physics-trigger permitido, VFX, Audio, Light/Environment, UI/Subtitles, Luau Events.

**Stop/Skip/Cancel/Destroy restauran control y overrides**; cámara no confisca movimiento de player por defecto; preview no ejecuta gameplay irreversible. Semántica seek/reverse/loop/crossing markers es tipada y probada.

## 6. JericoVFX en detalle

Láminas objetivo: fuga de vapor en cañería, electricidad/chispas en bunker, nieve exterior, cenizas/fuego, debris tras rotura, secuencia sincronizada de luz/Audio/VFX. Inspector: Spawn, Shape (point/box/sphere/mesh/spline si admitido), seed, lifetime, burst, color/size/opacity curves, velocity, force/turbulence/wind, collision/bounds, flipbook atlas, material, GPU/CPU quality, LOD, culling, overdraw, cache/cook. Panel inferior: lifetime timeline/curves, graph de dominio, profiler. Una autoridad Environment para viento; daño/decal bajo sus owners. No lógica gameplay dependiente de miles de GPU readbacks. Auditar OpenParticleSystem Preview antes de adoptar; VDB tool-only opcional no backend de geología.

## 7. Referencias visuales / backlog de láminas individuales

Láminas conceptuales preparadas en conversación: PWAS, EPLS, RBrush, Luau IDE, JericoBSP, Procedural Studio, Smart Importer, Motion Studio, Sequencer, VFX, **Editor Design System**, **Timeline Anatomy**. Los PNG no están incluidos en este Markdown: almacenar imágenes binarias por separado conservando nombre, hash, prompt/brief y fecha; nunca usar el screenshot como prueba de código implementado.

Pendientes útiles para nuevos pósters: JericoAudio; AI + Nav; JericoUI; Physics + Interaction; Profiler; Build/Cook; World/Prefabs; Geology; RMod/Splines; Destruction; Environment/Weather; Terrain; Lighting/Camera; Animation Graph/Retarget; Item Studio; Narrative; Localization; Input/Accessibility; XR; Save/Replay QA; Project Settings. Cada nueva lámina debe enseñar viewport realista del juego (Primera Guerra → mina → laboratorio), controles auténticos del dominio, el mismo shell, Inspector y docks, sin duplicar owner.

## 8. Handoff a Codex

1. Releer HEAD de GOAL/AGENTS/ROADMAP/Requirements y master relevante. No tratar este diseño como orden de implementar 25 workspaces en paralelo.
2. Marcar por feature: **existente en O3DE / candidato auditado / diseño Jerico / integrado / verificado**, nunca declarar PASS por una imagen.
3. Reutilizar Qt/AzQtComponents/Action Manager, WhiteBox/Manifold, GraphCore, Maestro, EMotionFX, OPS, Asset Processor, Jolt, etc. únicamente donde su código/contrato/licencia/consumidores permitan adaptación con ownership `JE::`.
4. Una identidad World, Asset Registry, transactions, renderer, física, graphs y Cook. UI y MCP invocan comandos tipados y comparten Undo/Redo. Sin AWS, multiplayer ni Python embebido; solo DX12/Vulkan.
5. Cada herramienta completa: Preview/Diff → Validate → Apply/Cancel → Undo/Redo → Save/Reopen → Cook → Shipping source-free con pruebas y coste medido. Resultados deterministas, locks/overrides conservados, procesos cancelados no publican versiones obsoletas.
6. La guía es un **atlas de diseño**; el orden real sigue ACTIVE_SLICE/ROADMAP con BSP/editor/materiales/geología y consumidores prioritarios.

**Fuentes:** [GOAL](../../GOAL.md), [Editor Master](../EDITOR_ASSET_WORLD_MASTER.md), [Motion/UI/Audio/VFX](../ANIMATION_UI_AUDIO_XR_MASTER.md), [Lean Authoring](../JE_LEAN_ENGINE_AND_AUTHORING.md), [Requirements](../REQUIREMENTS_MATRIX.md), [Roadmap](../ROADMAP.md). Documento ampliado en [ChatGPT](https://chatgpt.com/space/page_16d7d8450de48191963a7e0b31344029).
