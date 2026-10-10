# Roadmap JericoEngine — O3DE nativo, rama única

Estado global ACTIVE. N0 preparado; ningún milestone de implementación acreditado por este commit. J0/J3A/M10.5 antiguos son contexto, no progreso de O3DE.

## Orden y dependencias

Ruta propuesta: N0 → N1 → N2 → N3 → N4 → N5 → N6 → N7 → N8 → N9 → N10 → N11 → N12. Cada N admite resultados coherentes publicados individualmente: no megacommit ni micro-slice por tipo. Una rama development, un ACTIVE_SLICE y un integrador. El padre puede ajustar el orden por consumidores demostrados, nunca reactivar exclusiones.

Windows y Linux tienen estado separado. Una plataforma verificada puede avanzar mientras la otra tiene deuda explícita; no declarar ambas ni paralizar todo por hardware inexistente. XR es contrato nativo desde N0/N1: vistas, input e interacción no deben fijarse a desktop/mono. N10 cierra la experiencia y prueba de hardware; no es el momento de descubrir que las interfaces impiden VR.

## N0 — origen, dependencias y baseline O3DE

**Resultado:** fuente admitida, toolchain reproducible y Editor/Asset Processor/runtime originales con escena Vulkan y métricas.

Seguir `work/N0_FOUNDATION_PLAN.md`. Verificar SHA/tree/LFS/3P/licencias, CMake/toolchain y requisitos excluidos. Seleccionar development aunque el default aún difiera. Auditar AWS por servicio, enlace, build y hosting. Compilar sin modificación cosmética ni importar updates de otras ramas anticipadamente. Mapear interfaces XR/multiview y sus riesgos, sin inventar soporte ya integrado.

Aceptación: fixture sintético/licenciado con mesh PBR/luz/cámara/sombra, abrir/editar/guardar/reabrir, procesar assets y ejecutar launcher cocinado. Registrar qué packaging original todavía no permite; no llamarlo formato Jerico final. Medir CPU/GPU/RAM/VRAM/frame pacing y respuesta desde el inicio. Evidencia N0-W/N0-L separada. Dependencia o GPU ausente = estado del paso preciso, no PASS. No borrar Gems.

## N1 — composición nativa y primer panel/componente Jerico

**Depende:** baseline funcional en plataforma objetivo. **Resultado:** proyecto limpio recibe una capacidad Jerico del motor sin instalación/habilitación manual de Gem Jerico.

Extender los registros/editor/componentes existentes con una propiedad y consumidor real de escena. Panel contextual usa comandos/transacciones. No scaffold de todos los sistemas ni segundo World. Mantener diseño de múltiples vistas, semantic input y física compartida para XR.

Probar init/use/shutdown una vez, selección, Undo/Redo, save/reopen, prefab instanciado, cook/runtime y limpieza. Título cambiado o biblioteca no utilizada no satisfacen el hito.

## N2 — assets, materiales y observabilidad gráfica

**Depende:** N1/builders originales. **Resultado:** GLB/materiales → modelo canónico → cook → revisión correcta en Atom/runtime.

Asset Processor único; ORM/separados, normal/color-space, nombres libres, mips/tangentes, slots/instancias y reimport con overrides. Ufbx/otros formatos entran como subverticales probados. No dar todos por soportados tras un GLB simple.

Observabilidad dentro de Atom: pass inputs/outputs, timings CPU/GPU y memoria; diagnóstico apagado sin readbacks. Primer pass propio solo con consumidor real. Controles de PBR/shadows/reflections/fallback y material replacement. Diseñar cámaras/motion vectors/histories por vista, no un buffer global que impida XR.

Aceptación: import/reimport/cancel/Undo/reopen/cook/runtime y entradas corruptas. Los productos O3DE transitorios se etiquetan; N11 cierra los finales.

## N3 — Luau, referencias tipadas, input, triggers y Store inicial

**Depende:** identidad/transacciones/cook N1-N2. **Resultado:** entidad con script strict y referencia a luz/objeto, interacción y propiedad guardable.

VM/binding/typecheck/cook y tipos desde reflexión. Drag Outliner→IDE crea/reutiliza self.references; self con lifetime nativo. Input semántico compatible con XR, trigger activador/filtro/Once/cooldown y Commands/Events. Primer proveedor Store/Save opt-in sin duplicar cambios nativos.

Aceptación: activación cambia luz, estado persiste, referencia rota/ID reciclado/destroy/reload sin acceso inválido. Material/tool Lua se inventaría aparte; no segundo gameplay VM.

## N4 — Jolt y edición de un objeto físico real

**Depende:** N2/N3. **Resultado:** puerta/cajón genérico en editor con collider/body/hinge o slider y runtime Jolt.

Auditar AzFramework Physics/PhysX; migrar consumidor real con queries/events/cook/debug. PhysX queda temporalmente solo para consumidores clasificados, nunca sobre el mismo cuerpo Jolt. WhiteBox/geometry y primer RBrush/decal se incorporan con pruebas propias; no exigir CSG completo para una puerta.

Aceptación: pivot/eje/límites/motor/COM, push/pull/raycast/trigger, Play/Pause/Step/Reset, save/reopen/Undo/cook/runtime. RBrush separa brush/payload/target, resultado editable y source intacto. Ampliar joints/character/terrain/nav según consumidores, sin una segunda física del editor.

## N5 — Motion Studio, IK e interacción bimanual

**Depende:** N4 e importación de skeleton/clip. **Resultado:** manos/personaje manipula objetos Jolt con contacto veraz y corrección de animación persistente.

EMotionFX/skinning/graph primero. Separar rskel/rrig/ranim; bones/weights/FK/IK/retarget/clip editing; pausar-corregir-guardar. GripPoints, sockets, InteractionSession y Fit reusable para llave o batería. No solver distinto por objeto.

Aceptación: IK no teletransporta body; bimanual sin dos writers; cancel/destrucción libera reservas; corrección sobrevive cook/runtime. Fit usa tolerancias anisotrópicas, simetrías y etapas/DOF, con hysteresis/rollback y safe-drive en Jolt. Fijar explícitamente handoff al estado seated y política Save/Restore de sesiones activas: restaurar segura o cancelar con estado coherente, nunca guardar punteros de solver. Desktop y XR comparten contrato.

## N6 — UI, inventario, narrativa, secuencias y persistencia

**Depende:** N3/N5. **Resultado:** UI Luau con inventario real y una secuencia cámara/animación/eventos, save/load y localización.

Auditar LyShine/shine/Maestro sin fusionar ramas enteras. rui2d/rui3d→rui, rlang→rloc, rfont; grid/hover/drag/drop/preview3D/tweens/timelines. Item Studio e Inventory son distintos; UI no escribe estado a escondidas. Narrative Dialogue/Quest/Journal por subverticales.

Aceptación: UI/input/focus/texto/localización, inventario consistente, save corrupto rechazado, restore transaccional, sequence stop/skip/cancel devuelve controles. No forzar movimiento total del player por una cámara. Audio/VFX provisional no acredita N7.

## N7 — JericoAudio y JericoParticles

**Depende:** N2/N3/N6. **Resultado:** SoundEvents y VFX authorables/cocinados en editor/runtime.

MiniAudio/backend abierto privado, mixer/voice/streaming/2D/3D/zonas/portales/oclusión con budget, Physical Material→sound sets. OpenParticleSystem Preview se evalúa contra emitter/flipbook/curves/culling/lighting/authoring requeridos, no se adopta ciegamente.

Aceptación: un evento desde Luau/contacto/sequence converge en un solo audio; device loss/unload sin voces huérfanas. Partículas de polvo/chispas/flipbooks con presupuesto de overdraw/CPU/GPU; gameplay no depende de readback masivo. Smolder y atlas de iluminación se evalúan después del sistema base, no son bloqueantes.

## N8 — RMod, PWAS/EPLS, RBrush avanzado y mundo/entorno

**Depende:** N2/N4/N7. **Resultado:** construir un entorno modular editable, con redes/soportes/variantes/cables y detalle no destructivo.

Part library configurable, ports/sockets, rutas y fittings (incluidas intersecciones manuales), snap con histéresis, flanges y soportes suelo/pared/techo mediante World/Jolt. Rectas generadas con UV; joints/piezas no se estiran arbitrariamente. Edge chains, damage/bake, catenaria y cadenas 16/32/64. Doors/ventanas/muebles como presets, no hardcode del core.

PWAS propone respetando locks/overrides/GUID y layout manual; EPLS capas/señales sin segundo Environment. RBrush paint/stamp/scatter/material/vertex/damage con outputs distintos. Terrain/vegetation/streaming/HLOD y ambiente se amplían por necesidades reales; coste medido. Destrucción prefracture/bonds/fragmentos con Jolt y microdebris VFX.

Agua superior/local, atmósfera/fog/clima/viento físico opt-in tienen subverticales con su aceptación; no se acreditan por el trabajo viejo. Underwater y breaking waves diferidos, flotabilidad DESIGN ONLY y geología integral no activa. GI raster se analiza sin portar Cry ni introducir RT escondido.

## N9 — Recast/Navigation, HTN, relaciones y memoria

**Depende:** World/physics/animation/script/save. **Resultado:** criaturas ejecutan planes explicables con navegación real, reservas y memoria persistente.

Recast provider revisado al reemplazar PhysX, dirty tiles/links/agent sizes/repath y debug. HTN alto nivel con condiciones puras, queries asíncronas, acciones cancelables y budgets. GraphCanvas/GraphModel son frontend, no segundo planner. Visor por entidad muestra decisión, rechazo, percepción, target y memoria.

Aceptación: araña transporta presa al nido; cuadrúpedos coordinan bait contra araña ciega; colonia/individuo aprenden de resultados confirmados y sobreviven save/load. Destruir target/unload/cancel no bloquea reservas. Nav de suelo no acredita trepar techos; modos extra requieren tests específicos.

## N10 — perfiles 2D/2.5D/isométrico/vehículos y XR completo

**Depende:** sistemas de juego compartidos. **Resultado:** perfiles adicionales sin engines paralelos; OpenXR verificable con visor real.

Sprites/atlas/tilemaps, plane/depth constraints, sorting/pixel snap/cámara ortográfica, normales y luces 2D; isométrico conserva métricas/world/picking. Vehicle usa Jolt y semantic input/posesión, no física aparte. Consultar PRESENTATION_PROFILES_MASTER.

OpenXR presente como contrato desde N0; aquí cerrar sessions/views/swapchains/actions/haptics/tracking loss, UI world-space y agarre compartido. Desktop stereo/mock no es validación de hardware. Vulkan Windows/Linux por separado, sin activar DX12 para pasar XR.

## N11 — identidad, módulos, formatos finales y Shipping

**Depende:** suficientes consumidores reales N1-N10. **Resultado:** distribución Jerico source-free y reproducible con cierre de dependencias.

Migrar nombres/targets/paths y consumidores juntos; no simple CryAction→JericoAction ni replace global AZ. Consolidar por ownership, conservar bibliotecas privadas útiles y avisos legales. Retirar archivos/rutas redundantes solo tras prueba y manifest. Formatos r* y epak versionados, bounded, deterministic, ABI independiente; no renombrar archivos O3DE para simularlos.

Packaging en directorio final; no checkout/compiladores/source/XML propio/AWS account/servicios. Resolver LFS/3P hosting controlado, licencias Qt/transitivas y SBOM; offline real. Retirar PhysX y Lua/ScriptCanvas gameplay solo cuando no queden consumidores. Herramientas privadas y third-party notices se clasifican, no se borran por nombres.

## N12 — endurecimiento y aceptación global

**Depende:** matriz cubierta salvo diferidos autorizados. **Resultado:** recorridos completos en plataformas admitidas, rendimiento y UX sin afirmaciones ficticias.

Crash/hang QA, fuzz de parsers/import/saves/packages, profiling/soak, device/focus/session loss, corruption/recovery, incremental cook, reproducible build y source-free. Performance/Balanced/Quality/Cinematic-Editor/VR/Pixel2D con presupuestos basados en mediciones, no números históricos de consolas.

Verificar cada requirement ID, README/GOAL coherentes, deuda explícita, licencias/avisos y ausencia de dobles autoridades. El GOAL global solo cierra con todos los requisitos obligatorios o una decisión explícita que cambie alcance; no sustituir una familia por una demo.

## Reglas permanentes

No perseguir todos los papers. Investigación dirigida por problema/consumidor; resultado KEEP/ADAPT/REJECT/DEFER. Lección reusable en error book, evidencia actual en HANDOFF y directorio del vertical. Los proyectos anteriores permanecen intactos como historia/procedencia; no importan PASS ni código automáticamente.
