# Roadmap JE — BSP/UI primero, geología activa, DX12 y Vulkan

**Revisión del propietario 2026-10-10.** Los IDs N0–N12 se conservan para trazabilidad, pero ya no constituyen una cola numérica rígida. Estado general ACTIVE; documentación no acredita implementación.

## Camino visible prioritario

`N0 baseline local → N1 UI/BSP → N2 materiales/formatos básicos → N4 colisión/interacción → N8 geología/RMod/RBrush → N3/N5/N6/N7/N9/N10 según consumidor → N11/N12 cierre`.

N3 mínimo puede adelantarse cuando una interacción requiera Luau; lo mismo una dependencia mínima de física/material. No desarrollar todas las familias a la vez. Un ACTIVE_SLICE, un integrador y development. Optimización y migración JE acompañan cada tramo: no esperar al final. `docs/architecture/JE_IDENTITY_AND_NATIVE_MODULES.md` gobierna los nombres, la absorción de Gems y excepciones privadas.

## N0 — fuentes, dependencia local y escena real con los dos backends

Resultado: base O3DE auditable, toolchain/deps permitidas y escena/editor/runtime originales medidos. Sin AWS/multiplayer productivos ni uso de endpoints CloudFront/S3 para conseguir el baseline. Inventariar source/targets/builders/downloads/LFS, definir cache local completa y caso missing-package fail-closed. Excluir server/cloud/extra RHI mediante grafo real, no borrado por nombres.

Matriz de aceptación: Windows-DX12, Windows-Vulkan, Linux-Vulkan. Hardware ausente = NOT TESTED explícito. Una plataforma disponible puede avanzar trabajo visible sin declarar portabilidad completa. Null/headless nunca satisface fila gráfica. Medir PBR/luz/sombra/cámara/save/reopen/asset processor/launcher en fixture sintético/licenciado. No afirmar primer build ejecutado por esta documentación.

## N1 — JericoEditor y BSP útil inmediatamente

Resultado: construir y editar una habitación con corredor, puerta y ventana en el viewport central, dentro del editor real. Reusar WhiteBox CSG/Manifold y component modes tras auditarlos; skin/docking/Inspector/Outliner/paleta Jerico sin segundo editor. Migrar este owner y su API/targets a JE con puentes privados temporales inventariados.

Grid/snap, caras/aristas/vértices, extrude/inset/bevel/slice/hollow/booleans se admiten por operaciones completas; no una demo de cubo que proclama BSP completo. Primer checkpoint obligatorio: corte de puerta/ventana, UV/material por cara, Undo/Redo, Save/reopen y consumidor runtime de la geometría resultante. Prueba diferencia vacía, coplanares/escala, límites y conservación de material/region IDs. Capturas reales y coste de editar/reconstruir. Sin activar un Gem Jerico por proyecto.

## N2 — materiales, importación y formatos que ya se usan

Resultado: escena BSP/GLB PBR, nombres libres, ORM/separados, normal/color-space/UV/alpha y material instances/graph conectados al Asset Processor y Atom adaptados a JE. Ufbx y otros formatos como subverticales probados, no lista de extensiones supuestamente soportadas.

`.rmesh/.rmat/.rtext` deben tener writers/readers/validators y uso real; no posponerlos todos a N11 ni inventar formatos sin consumidor. Reimport/cancel/Undo/reopen/cook y ejecución aislada; semánticas correctas en Windows DX12/Vulkan y Linux Vulkan por gates separados. Coste de import, memoria pico/residente, mips/tangentes y retirement de recursos. Shader toolchain offline, variants y reflexión reutilizados; no otro material VM/device.

## N4 — Jolt y un objeto BSP/modular físico real

Resultado: puerta/cajón con collider/body/hinge/slider, pivot/gizmo/límites/motor/COM, Play/Pause/Step/Reset, queries/triggers y persistencia/cook. PhysX solo para consumers aún sin migrar, nunca el mismo body en ambos. WhiteBox physics tests y navegación tienen dependencias PhysX que hay que adaptar, no perder tests.

Generar colisión de BSP/geología por la autoridad JE, no una física oculta de tools. Activar destrucción simple prefracturada cuando el consumidor lo requiera; Jolt mueve fragmentos, no crea la topología.

## N8 — geología, RMod y detalle del entorno, adelantados

**Geología activa:** integrar masa/brush→capas→SDF/campos→Surface Nets/Hermite/QEF cuando corresponda→mesh/atributos→material/collision/nav/cook. No forzar toda arquitectura BSP a volumen ni borrar cortes planos analíticos. Reusar matemáticas originales con procedencia verificada, no copiar el engine del Playground.

Checkpoints visibles y separados: (a) roca/familia y cliff con estratos/cortes/overhang; (b) cueva sustraída en masa con interior/materiales/colisión y paso jugable; (c) erosión/fracturas/daño y actualización local, cancel/Undo/save/reopen; (d) source-free y calidad/coste en los tres pares gráficos. Selector de forma independiente de material; mismo campo geológico guía triplanar, strata, sedimento y FreshInterior. BSP→OpenVDB continúa cancelado, no toda geología.

RMod: library de piezas/ports/sockets/rutas, elbows/T/cross/reducers/flanges/intersecciones manuales, snap con histéresis, soportes por queries piso/pared/techo, rectas con UV y RMF. Cables/catenaria/cadenas 16/32/64 como pruebas discretas con Jolt. RBrush BRUSH/PAYLOAD/TARGET para paint/mask/decal/vertex/daño, PWAS/EPLS con locks/overrides/seed/GUID y layout manual respetado.

Terrain/vegetation/streaming/HLOD/environment se amplían por necesidad real. Viento físico separado de bending. Agua superior/local sigue alcance conservado; underwater/costa posterior. **Flotabilidad continúa DESIGN ONLY**, no se activa por geología ni vehículos.

## N3 — Luau, referencias, input, triggers y Store

Se adelanta solo el mínimo que un checkpoint visible precise. Luau strict, self válido, drag Outliner→self.references tipada Scene/PrefabInternal, Commands/Events, input semántico y triggers Once/cooldown con activador/filtro. Store opt-in captura cambio confirmado sin doble registro. Referencia destruida/reciclada, reload, save y source-free. Una VM de gameplay/UI y tipos públicos JE, no nueva tabla de entidades.

## N5 — Motion Studio y manipulación

EMotionFX integrado a JE, `.rskel/.rrig/.ranim/.rgraph`, huesos/pesos/FK/IK/retarget/clip correction. Pausar-corregir-guardar un clip con consumidor real. GripPoints/MatePoints/FitProfiles, una sesión bimanual, fitting por etapas/DOF/tolerancias/simetría/histéresis, safe drive y rollback; Jolt manda e IK contacta. Probar llave/batería y puerta sin solver particular por objeto. Save restaura o cancela coherentemente una sesión, no serializa punteros.

## N6 — JericoUI, inventario, secuencias, narrativa y Save

Reusar LyShine/Shine/Maestro por piezas verificadas, sin merge de rama entero ni nombres AZ finales. UI rui2d/rui3d→rui, Luau/tweens/grid/hover/drag/preview3D, localización rlang/rloc/rfont, input/focus/accesibilidad y World-space/RTT. Inventory/Items/Item Studio poseen estado, UI presenta. Sequencer rseq maneja cámaras/curvas/eventos por canal y devuelve control al stop/skip/cancel; no inmoviliza player por defecto.

Save por provider/version con restore candidato/corrupción/backup; Dialogue/Quest/Journal y replay QA local. Content packs/campañas existentes como datos offline cuando sean necesarios, **sin comercio, entitlements remotos, online accounts ni plataforma DLC**.

## N7 — audio y VFX integrados

Backend abierto privado, SoundEvents/mixer/voices/streaming/2D/3D/zonas/portales/oclusión y material físico→sound sets. Luau/contacto/sequence usan el mismo audio; device loss/unload seguros. OpenParticleSystem Preview se compara antes de adoptar; emitter/flipbooks/rpart/culling/overdraw visibles. Lighting atlas/Smolder después de medir un caso; sin gameplay dependiente de readback masivo.

## N9 — navegación e IA local

Recast/Detour privado fijado y disponible localmente, provider geometría/física compatible con Jolt, dirty tiles/links/agent shapes/repath. No confundir tests PhysX con runtime de navegación ni navmesh de suelo con caminar por techos.

HTN puro al planificar, sensores/memoria/relaciones/territorios/manadas/colonias, reservas y acciones cancelables/presupuestadas. Debug por entidad muestra decisiones/rechazos y aprendizaje individual/compartido desde resultados. Caza/transporte al nido/bait/contramedida conservados y guardables. Esto es IA single-player, NO multiplayer.

## N10 — perfiles adicionales y OpenXR

2D/2.5D/pixel/isométrico/híbrido/vehículos existentes comparten el motor. Sprites/tilemap/normales/luces/sorting/plane constraints/pixel snap, input/posesión y Jolt. No retrasan el núcleo BSP/geología. XR es contrato desde N0/N1 y aquí se prueba headset/actions/swapchains/haptics/pose/loss/resume/UI/agarre. Bindings DirectX12/Vulkan según soporte real de plataforma/runtime, sin inferir parity por un test desktop.

## N11 — cierre de integración, identidad y distribución

Completar migración JE iniciada en cada owner, composición nativa y retirada de rutas Gem productivas. Solo DX12/Vulkan en distribución; no AWS/multiplayer/servers/otros renderers. Excepciones de dependencia/notices explícitas y legales. No reset masivo ni eliminación de archivos desconocidos.

Catálogo `.r*` y `.epak` real, ABI independiente, chunks/deps/hash/bounds/version y source-free. Delta/overlay/rollback y protección local se incorporan donde se requieran; no criptografía propia ni servicios online. Algoritmos/keys/licencias auditados, hash no es firma. Bootstrap/build desde cache permitida offline, falta de paquete falla sin fallback a Amazon. Directorio de producto con ejecutable/paquetes/bibliotecas y recursos/notices de terceros necesarios.

## N12 — aceptación y rendimiento continuo

No es la primera etapa de optimizar. Cada N ya mide y muestra. Aquí cerrar matriz de pares gráficos, recorridos editor/runtime/cook, namespaces/source/generated/ABI, package/link graph, privacidad/offline, crash/hang/soak/fuzz/device/focus/session loss, incremental rebuild, accesibilidad y calidad.

SIGGRAPH desde N1/N2/N8: por escena BSP/geología medir bottleneck, conservar baseline, aplicar algoritmo mínimo, comparar ON/OFF/ON e imagen/timings/memoria. Rechazar mejoras sin beneficio o con deuda de calidad. No adoptar una arquitectura/paper completo ni importar tuning de consolas. Más checks no equivalen a un motor usable.

### Experimento gráfico AMD posterior a la integración de Gems (decisión 2026-10-29; diferido)

**Prioridad: último trabajo, después de absorber y verificar las Gems restantes en los propietarios nativos JE. Estado: BACKLOG / DESIGN ONLY.** El propietario eligió la **opción A**, comparación reversible **dentro de JericoEngine**, no un prototipo Cauldron externo ni sustitución anticipada. Esta decisión no cambia el ACTIVE_SLICE vigente, no habilita ejecución durante la integración de Gems y no acredita efectos adaptados o probados.

Una vez completado el gate de integración nativa de Gems, estudiar y adaptar desde AMD FidelityFX SDK con versión, procedencia y licencia auditadas, dentro de `JE::Render`, sin Gems activables por proyecto ni segundo renderer. Conservar las técnicas existentes para comparación y fallback:

- **Oclusión ambiental:** Disabled / SSAO / GTAO / **AMD CACAO**.
- **Reflejos:** Disabled / SSR / **AMD SSSR** / ray tracing existente.

Orden recomendado al reabrir el trabajo: **CACAO → SSSR**, cada uno como vertical con habilitación/deshabilitación real y sin mantener simultáneamente dos caminos costosos que no se utilicen. La interfaz de comparación selecciona un método por categoría para evitar doble oclusión o reflejos accidentales. Cualquier combinación se valida antes de exponerla en Runtime.

**Aceptación futura:** misma escena de mina húmeda/cueva, cámara, resolución, luces, materiales y hardware; capturas nativas comparables y artefactos, timings GPU y CPU p50/p95/p99, VRAM/RAM, calidad temporal, activación/desactivación y fallback. Windows/Vulkan y Linux/Vulkan son gates separados; DX12 se conserva inactivo hasta instrucción posterior. Sin resultados visuales ni benchmarking, el estado sigue NOT TESTED, no integrado. Preparar el diseño detallado y obtener aceptación antes de escribir el primer efecto.

### DDGI con cobertura automática tipo Flax (investigación condicionada; después de integrar Gems)

**Objetivo de autoría:** activar GI para una mina/cueva/corredores sin obligar al artista a colocar manualmente `DiffuseProbeGrid` o cubemaps. Los probes internos NO desaparecen: Flax utiliza DDGI con sondas automáticas que siguen la cámara y cascadas, y traza mediante Global SDF y Global Surface Atlas. Los cubemaps/reflection probes son un problema distinto: reflejos especulares. **No copiar el renderer, código o arquitectura de Flax.**

**Base actual:** `JE::Render::Features::DiffuseProbeGrid` deriva del DDGI de Atom y usa volúmenes con espaciado y modos Real-Time RT/Baked/AutoSelect. Conservar ese único propietario, datos, assets y fallback; no declarar que ya existe un sistema Flax-like ni prometer paridad en GPU sin RT.

- **Fase 0 — medir:** fixture con cuarto oscuro, puerta abierta/cerrada, corredor, cueva y cámara en movimiento; comparar DDGI manual vigente contra iluminación sin GI. Registrar GPU p50/p95/p99, memoria, fuga de luz, ghosting, cambios bruscos al mover cámara y latencia al encender/apagar luces.
- **Fase 1 — automatizar primero:** evaluar un modo `Automatic GI Coverage` que administre/reutilice grids alrededor del jugador/cámara dentro del owner nativo, con presupuesto de probes/rayos y actualización incremental; preservar `Manual/Advanced` para quien necesite control. Estudiar grid desplazable y luego cascadas solo si el renderer actual las admite a coste razonable. No crear entidades por frame, ni reconstruir texturas completas al desplazarse, ni exigir cubemaps de artista para GI difusa.
- **Fase 2 — compatibilidad:** probar interiores separados por muros/puertas, teletransporte, cambio de nivel y streaming, Editor/Runtime, save/reopen y pérdida de dispositivo; preservar baked/fallback en GPU sin RT. Software tracing con SDF + atlas es una alternativa **independiente, cara y NO aprobada**: solo estudiarla si un caso medido demuestra una ventaja que compensa duplicar geometría/cachés.
- **Gate de adopción:** aceptar el modo automático únicamente si reduce trabajo de autoría sin regresiones de calidad/rendimiento frente a DDGI manual a igual hardware/escena; si falla, mantener DDGI actual. No desplazar la absorción de Gems ni registrar esta propuesta como implementada o probada.

Referencias técnicas de lectura (no dependencias): [Flax Realtime GI](https://docs.flaxengine.com/manual/graphics/lighting/gi/realtime.html) y [O3DE Diffuse Probe Grid](https://www.docs.o3de.org/docs/user-guide/components/reference/atom/diffuse-probe-grid/).

### Sombras PCSS opcionales, después de integrar Gems (decisión 2026-10-29)

**Estado: BACKLOG / DESIGN ONLY.** Integrar en el futuro **Percentage-Closer Soft Shadows (PCSS)** como opción de calidad del filtrado existente de sombras en `JE::Render::Features`, no como otro motor de sombras, otro Gem ni dependencia binaria de NVIDIA. **Primero cerrar y verificar la absorción de las Gems restantes**; esta decisión no altera `ACTIVE_SLICE` ni acredita PCSS implementado, compilado o medido.

**Base a reutilizar:** Shadow Maps/atlas, sombras direccionales en cascadas, point/spot y filtros PCF, ESM y ESM+PCF con sus shaders, feature processors y controles actuales. Mantener **PCF como predeterminado y fallback**, incluyendo perfiles económicos y VR; no eliminar los demás filtros ni duplicar atlas/pases. El modo PCSS se habilita explícitamente por configuración/calidad y por tipo de luz si la ruta lo soporta.

- **Diseño mínimo:** blocker search en la shadow map existente, estimación de penumbra según tamaño angular/radio de la luz y diferencia de profundidad oclusor-receptor, seguida de PCF con radio variable; adaptar la convención de profundidad/bias y los límites de cada cascada/atlas. Permitir luz pequeña (linterna) con sombra definida y lámpara amplia con penumbra progresiva. Implementación propia del algoritmo publicado, sin incorporar código de Flax ni DLL/SDK propietario.
- **Presupuesto:** ajustar por calidad y por luz el radio de búsqueda, muestras máximas y distancia; limitar el coste en múltiples luces locales, vegetación/transparencia y movimiento. PCSS nunca se suma encima de PCF/ESM como doble filtrado ni fuerza procesamiento en luces que no proyectan sombras.
- **Comparación A/B:** misma escena, luz y cámara; `PCF` frente a `PCSS` en linterna, lámpara minera amplia, puerta, telaraña y criatura móvil. Medir suavidad y progresión de penumbra, acne, peter-panning, shimmering temporal, fugas entre cascadas y límites de atlas, GPU p50/p95/p99, VRAM, CPU y draw calls. Validar también activación/desactivación real y fallback seguro.
- **Gates:** Windows/Vulkan y Linux/Vulkan por separado; DX12 conservado pero inactivo hasta autorización; VR/OpenXR sin prometer paridad hasta medir. Si PCSS no aporta diferencia visual relevante o incumple el presupuesto por perfil, usar PCF. Integración en Editor/Runtime/asset shader solo después de aprobar el diseño específico y pruebas.

Referencias de lectura (no código a copiar): [Flax 1.13 PCSS](https://flaxengine.com/blog/flax-1-13-released/), [NVIDIA PCSS (algoritmo)](https://developer.download.nvidia.com/shaderlibrary/docs/shadow_PCSS.pdf) y [O3DE Directional Light](https://www.docs.o3de.org/docs/user-guide/components/reference/atom/directional-light/).
