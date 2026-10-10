# Perfiles de presentación y juego: 2D, 2.5D, pixel, isométrico, vehículos y XR

Contrato N10, con decisiones de coordenadas/input/render preparadas desde N1-N3. Fuente: JERICO_2D_2_5D_PIXEL_ISOMETRIC_VERTEX_VEHICLE_SPEC de JericoEngine2. No es una nueva familia de motores.

## Un motor

Full3D, SideScroller2_5D, TopDown2_5D, Pure2D, Pixel2D, Isometric2D, Isometric2_5D, Hybrid2D3D y VehicleGame son perfiles de defaults. Comparten World, GUID/ref, Luau, Save, Jolt, input, UI/audio, assets/cook, Atom y editor. Se pueden mezclar sprites y geometría 3D. XR usa esos mismos servicios, no otro gameplay.

## Coordenadas y cámara

No hardcodear 2D=XY. Un PlaneConstraint define origen, ejes horizontal/vertical/depth y banda de profundidad. Movimiento free3D/locked plane/depth band/billboard/screenspace es explícito. Physics calcula en su espacio canónico; constraints no se aplican desde un script que pelea con Jolt.

Cámaras ortográficas/perspectivas y proyección isométrica con métricas/unidades. Isométrico es cámara/presentación y reglas de sorting/picking, no necesariamente una cuadrícula ni física falsa. Conversión puntero→plano/world para colocar/seleccionar coherente con viewport/DPI/resolución. Tests de tile picking y bounds en esquinas/cambios de zoom.

## Sprite renderer y tilemap

World sprites no son widgets de JericoUI. SpriteRenderer/AnimatedSprite usan quads/instancias, atlas/arrays, pivot/ground anchor, tamaño/pixels-per-unit, UV, color/opacity, layer/order/depth/material y flags. Batching compatible evita un draw por sprite, sin cambiar el orden requerido por blending.

Normal map, roughness, emissive y light/shadow masks permiten iluminación 2D con normales; espacio tangente y flipX/Y actualizan las normales correctamente. Normal-lit no implica sombras geométricas correctas: definir representación y fallback. TileMap/TileChunk tienen palette/layers/paint/collision/nav/streaming donde corresponda; Undo/cook produce datos runtime, no recipe editada cada frame.

Pixel snap/pixel-perfect: resolución de diseño, escala entera cuando corresponda, filtro nearest/mips/policy, cámara subpixel y snapping definidos. No imponer TAA que borre pixel art. Test de viewport resize/DPI, movimiento lento y sprite animation sin shimmer inesperado. Perfil Pixel puede optar por post distinto sin segundo renderer.

## Vertex paint y material layers

RBrush scalar/vector payload sobre target vertex; slots/canales explícitos, selección/falloff, mask/locks, Undo y persistencia. Topology/reimport necesita remap o invalidación diagnosticada, no mezclar atributos de otra malla. Material graph consume esos canales, no una textura invisible paralela. Pintura de vértices y decals/material masks comparten gesto, no autoridad de datos.

## VehicleGame

Jolt VehicleConstraint u otra capacidad real de su pin se integra con physical materials, wheels/suspensión/drive/brake/steering, contacts y debug. Input semántico y posesión permiten entrar/salir, cámara y UI; no crear un PlayerVehicle que duplique el World. Presets de coche/otros vehículos no equivalen a soporte completo sin pruebas.

Probar suspensión/carga/material friction, pérdida de suelo, entrada/salida/cancel, save/load, controller disconnect y un vehículo en escena 3D/isométrica. Vehículos acuáticos permanecen bloqueados por autorización separada de flotabilidad; no activarla por introducir VehicleGame.

## Perfiles de calidad y accesibilidad

Performance, Balanced, Quality, Cinematic/Editor, VR y Pixel2D expresan presupuestos/defaults, no forks de código. Medir costes del contenido real y cambiar LOD/shadows/post/particles con fallback. No prometer 60/90 FPS universales. VR impone su frame pacing y confort medidos en dispositivo real, no marketing.

Subtítulos/captions, remapping, Reduce Motion, photosensitivity, escala, foco/input y haptics persisten en User Settings. Cambios cosméticos de presentación por entidad conservan gameplay; cualquier cambio de collider es explícito y validado. Ningún perfil convierte una capacidad diferida en obligatoria sin decisión.
