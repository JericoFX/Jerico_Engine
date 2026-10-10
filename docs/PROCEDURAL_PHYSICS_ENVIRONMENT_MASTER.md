# Geometría modular, PWAS/EPLS, RBrush, Jolt y entorno

Estado: contrato de destino N4/N5/N8/N9. Geología integral fuera del frente activo; flotabilidad DESIGN ONLY. Las ecuaciones y datos siguientes describen requisitos, no APIs ya presentes.

## 1. Librerías reutilizables, no otro playground dentro del motor

Rescatar solo matemáticas/código cuya procedencia se audite: edge chains, longitud de arco, RMF, catenaria, snap/histéresis, routing/fittings/supports, projection/bake y campos de daño. No traer shell ImGui, renderer Diligent ni Scene/asset registry propios del Playground.

Geología/SDF/Surface Nets/Hermite/QEF/QR/SVD/erosión quedan catalogados como investigación y matemáticas históricas, no roadmap activo de cuevas/rocas. Mantener la cancelación BSP→VDB. Una herramienta local de daño puede usar SDF si la representación es necesaria y está acotada; no obliga a convertir todo el mundo en volumen.

## 2. Geometría/BSP del editor

White Box se audita para authoring de caras/aristas/vértices, extrude/inset/bevel/cuts, selección, UV/material slots y collision. No es evidencia automática de boolean robusto ni BSP completo. El workflow JericoBSP construye arquitectura/graybox; runtime consume mallas/colliders/nav, no brush tree de autoría.

Booleans/remesh/reducción: tolerancias/unidades, no-manifold, degenerados, winding y conservación de material/GUID. Candidato Manifold u otra biblioteca requiere licencia/pin y pruebas, no un placeholder. Damage y cortes pueden reutilizar infraestructura; prohibido duplicar solver por puerta o tubo.

## 3. RMod: piezas, ports y sockets

`.rmod.toml` describe una pieza reusable: identidad, mesh/recipe, collision, tags, ports, sockets, reglas de host, giros, estirado/repetición, variantes y prefab/gameplay. Produce `.rmesh/.rcoll/.rpref` y dependencias; no nuevo runtime RMod salvo ciclo de vida demostrado. `.rpart` sigue siendo VFX.

Port = frontera conectable de red. Socket = accesorio, support, grip o inserción, no arista de red. Un port tiene ID local estable, frame, perfil circular/rectangular/oval, dimensiones, clase, compatibilidad, twist y adapter policy. Eje forward mira hacia afuera.

Conexión A-B válida: posiciones coinciden dentro de tolerancia y `forward(A) ~= -forward(B)`; perfil/reglas compatibles. Mantener roll/up para secciones no circulares. Escala no uniforme, inversión de handedness y transform singular se rechazan o procesan explícitamente. No rotar únicamente por la dirección de la curva olvidando el frame del modelo.

Route es intención y nodos semánticos; solver elige Parts. Rectas largas se pueden generar con UV/longitud física y repetición adecuada; no escalar arbitrariamente válvulas o junctions. Codos/tees/cruces/reductores/bridas/caps/roturas y accesorios provienen de piezas cuando existan. Bend procedural solo para intervalos no resueltos por piezas.

Un creador/lista de assets permite seleccionar modelos de una carpeta configurada, preview, asignar ports/sockets y validar orientación. Aceptar T/U/valve/malla rota y más de dos ports; no imponer un modelo de tubo de dos extremos a todo.

## 4. Topología, fittings y soportes

Intersección manual modifica topología semántica solo mediante operación explícita. Detectar tee/cross/elbow/reducer con tolerancias, tipos y ángulos; un cruce en pantalla no es conexión. El solver inserta el fitting correcto y mantiene markers de conexión/bridas (`==0==` como intención visual histórica) y debug de unión; no basta una línea naranja decorativa.

Snap con radios de entrada/salida distintos y orden estable para evitar oscilación. En empate, regla determinista que no depende del orden de iteración. Orientación/compatibilidad bloquean un snap inválido aunque esté cerca. Overrides/mandatory parts sobreviven resolver otra región.

HostQuery consulta suelo/pared/techo/esquina/obstáculos mediante el owner físico/world existente. Raycasts/shapecasts, normal y clasificación guían soportes, clearances y orientación. No crear collider world propio para procedural. Espaciado por distancia física/longitud de arco; no unir soporte al aire ni atravesar host. Fallback visible si falta soporte compatible.

Tests: segmentos cero/mínimos, recta/90°/oblicuo/U, T/cruce manual, port invertido/rotado/escalado, incompatibilidad, dos fittings juntos, obstáculos, normal techo/piso, multi-network sin unión, edición local preserva GUID de regiones no afectadas, cancel/undo/cook. Los screenshots de Babylon/Playground no prueban integración.

## 5. Cables, cadenas y edge chains

Una representación común de Path y frame transportado (RMF) controla roll y UV por longitud de arco; fallback en tangente casi cero/inversión. Extracción de edge chains en espacio world conserva continuación recta en junctions cuando corresponde y mantiene clasificación convexa/cóncava.

Catenaria libre ideal es referencia de reposo, no solver de colisión. Con endpoints, longitud admisible y gravedad se resuelve la curva con tolerancias; extremos coincidentes/tensión extrema o cuerda demasiado corta requieren política. Contacto con suelo y objetos pertenece a Jolt, no a dibujar siempre una U.

Probar cadenas de 16/32/64 eslabones con extremos fijados y comparar reposo con la referencia catenaria en el caso sin contacto. Registrar error y coste; no exigir igualdad exacta a un sistema discreto con joints. Cables/cadenas físicas usan fixed step, límites y colisión; la representación visual interpola el estado autoritativo.

Edge damage puede alimentar SurfaceFeatureField/DamageField. Ruido seed/versioned no cambia por frame; una costura visual barata puede cocinarse a normal/decal/cards cuando silueta/colisión/gameplay no cambian.

## 6. PWAS y EPLS

PWAS propone composición/dressing/variación respetando layout manual, máscara, locks, seeds, clearances y overrides. EPLS organiza capas/estado ambiental y de superficie conectado al mismo Environment/material/world; no planner físico ni segundo sistema bajo la variante ELSP.

Recipe = GUID + generator/version + seed + inputs/dependencies + parámetros + include/exclude + locked regions + overrides + identidades de resultados. PreviewResult muestra diff y coste. Apply es transacción. No mover puertas/ventanas/topología/recorridos manuales por cambiar seed de dressing. Promover a manual congela lo acordado.

Targets: piezas de minería, laboratorio y estructuras, muebles/estantes/camas/cajones, pipes/cables/ducts, scatter, suciedad/desgaste y materiales. Son herramientas generalistas; presets Penumbra no se convierten en clases hardcoded del core. Helpers de Blender/MCP son authoring externo opcional con validación.

## 7. RBrush no es solamente decals

BRUSH define cobertura; PAYLOAD define dato; TARGET define destino. Una misma UI/paleta puede pintar scalar en vértices, mask/material, colocar instancias decal, sculpt height o aplicar daño geométrico. Cada output tiene owner y cook distintos. `.rbrush` es autoría por defecto; imágenes/ABR externos son import-only con permisos y conversión.

Stroke guarda preset/ref, seed, spacing/jitter, presión cuando exista, orientación/depth/angle, channel mask, receiver filter, layer/order, locks y referencias. Preview/erase/cancel/Undo/reopen conservan esas reglas. No escribir destructivamente sobre textura fuente para dar un efecto de pintura.

Decals: projector OBB, world→local validado, clipping, UV, gradients/mips correctos, blend de color/normal/roughness/metal/emissive, orden estable, depth/angle fade, max distance, attachment rígido/bone/socket cuando esté soportado. No atribuir a idTech soporte deformable no demostrado. Runtime comparte Atom y lista espacial donde tenga valor; overflow de arrays/clusters visible, no desaparición silenciosa. Probar sobre metal puro y dieléctrico, pendientes, borde de receptor, solapamientos, movimiento y distancia.

## 8. Jolt y destrucción

Auditar bodies/shapes/queries/triggers/joints/character/ragdoll/vehicle, sleep/wake, masks/materials, eventos, debug y scene persistence. Una API Jolt existente no acredita editor/cook/Shipping. Integración native first con adapters mínimos al World existente; no doble PhysX/Jolt para un body.

Physical Material contiene fricción/restitución/densidad/respuesta y vínculos auditados; visual Material no es autoridad de masa. Fracture authoring crea chunks/bond graph/anchors/interiores/material lineage y colisión. Jolt mueve fragmentos: no genera la geometría de fractura por sí solo.

Predeterminado prefracture/cook, budget de fragmentos, activación local al romper bonds y piezas dormidas/intactas baratas. Microescombros como instancias/VFX; runtime dynamic fracture solo opt-in con límites. Daño superficial primero mask/decal, volumen/colisión/silueta/gameplay requieren geometry/collider update seguro. No nuevo DestructionMaterial ni solver por objeto.

## 9. Environment y participación física

Una publicación de estado para tiempo, sol/luna/estrellas, atmósfera/fog/clouds, viento, lluvia/nieve, wetness y agua. No duplicar clocks, exposición ni fuerzas en visual code. Cambios de hora/version del estado invalidan datos derivados según dependencia real.

Viento full3D compartido; fans son emisores locales de ese mismo campo. Cada body declara participación física y respuesta cuantitativa separadas de mesh bending. Evaluar velocidad relativa, área/respuesta autorizada, fuerza/torque en physics safe point. Apagar viento deja cero fuerza adicional, no detiene mágicamente velocidad previa del cuerpo. Tests: item incluido/excluido en mismo campo, emitter desactivado/destruido, save/reopen y substep.

## 10. Flotabilidad preservada, sin implementación autorizada

Se conserva el diseño 2026-10-08: WaterVolume con **Enable Buoyancy**, BuoyancyComponent por body, proveedor de ocupación/superficie/densidad/velocidad del fluido y Jolt aplicando fuerza/torque. Fluido sin renderer debe funcionar. No inventar un body que sigue la altura visual del océano.

`F_b = -normalize(g) * rho * |g| * V_submerged`; aplicar en centro de empuje y calcular torque respecto al COM. Normal de ola no reemplaza la dirección de gravedad. Muestras representan fracciones de volumen; no multiplicar el volumen total por cantidad de puntos. Drag usa velocidad relativa en el punto, con substeps y límites explícitos.

Modos primitiva/hull/muestras, casco hueco, corrientes, fluidos superpuestos, habitación sumergida, sueño/wake, streaming y provider ausente tienen pruebas separadas. Consultar versión/API real de Jolt, no dar `ApplyBuoyancyImpulse` por solución completa. Ningún readback GPU por body y frame obligatorio.

Futura aceptación, solo tras autorización: plano headless, densidades baja/igual/alta, masa descentrada, corriente, seco/mojado, prioridades sin doble fuerza, colisiones/joints y save/load; escala 10/100/1000 como medición, no presupuesto inventado. Esta etapa no bloquea N0 ni reactiva reparaciones underwater.
