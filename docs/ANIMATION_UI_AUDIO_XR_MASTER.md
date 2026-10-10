# JE Motion Studio, interacción, UI, audio, VFX y XR

**Contrato vigente 2026-10-10, no implementación acreditada.** GOAL/contrato JE superiores. N5/N6/N7/N10 preservan capacidades existentes sin postergar BSP/geología. Nomenclatura propia/adaptada JE::, bibliotecas privadas con licencias. Solo DX12/Vulkan y single-player local; multiplayer/red/servicios AWS excluidos, no deferred.

## Reutilización y datos

Auditar EMotionFX/EMotionFXAtom, Maestro/TrackView, LyShine y rama Shine como fuente focal, MiniAudio/AudioSystem, OpenParticleSystem Preview. Integrar implementación útil con consumers/builders/resources/registro bajo owner JE dentro del engine. No convertir todo Jerico en un Gem ni traer CryAnimation/Scaleform/Mannequin de CryEngine2.

.rmesh=geometry/skin, .rskel=hierarchy/rest/bind/unidades, .rrig=roles/control/constraints/IK/contact, .ranim=curves/clips/events, .rgraph=anim evaluation, .rseq=orquestación reusable. No serializar tipos privados EMotionFX/Jolt/AZ ni confundir skeleton con rig. Migrar identidad de type/UUID/codegen con el formato.

## Motion Studio y edición

Crear/corregir bones/pivot/hierarchy/weights, mostrar/normalizar/mirror influencias, FK/IK/Control Rig/retarget/layers/curves/keyframes. Autoría de rigs de anatomía distinta por controles generales; auto-rig/weight propone para revisión, no magia para toda anatomía. Blender/Cascadeur opcionales para casos especializados, no requisito para reparar bone/pose/clip dentro del editor.

Workflow: importar→inspeccionar skeleton/pesos→reproducir→pausar→ajustar IK/pose→guardar corrección→reopen/cook/runtime. Distinguir edición de clip, offset de rig y preview efímero. Undo no corrompe fuente ni hace permanente simulación por accidente.

## Física, grips y bimanual

Jolt posee body/constraint/masa/COM/inertia/contactos, IK muestra contacto. Ningún body dinámico recibe transform de dos writers. Door=collider/body/hinge/grip/latch opcional; drawer=slider; valve=constraint/interaction. No DoorPhysics/DrawerPhysics/BatterySolver especiales.

GripPoint/GrabPoint=agarre, SceneSocket=accesorio, MatePoint=encaje, InsertionSocket/FitProfile=inserción. Ports de redes no se vuelven grips/flotadores sin rol explícito. Un Fit framework con target frames, tolerancias anisotrópicas/simetrías/DOF/etapas, residual, histéresis y force/velocity budgets; asistencia de snap explícita, safe drive por Jolt, rollback/cancel y seated handoff.

Una InteractionSession gestiona participantes/objeto/grips/reservas/etapa. Manos principal/secundaria/pesos de contacto sin pelear transforms, segundo actor necesita permiso/reserva. Destroy/unload libera/cancela seguro; save de sesión tiene política explícita restaurar coherente o cancelar, sin pointers de solver.

Inspector y viewport editan colliders/COM/inertia/joints/pivot/axes/limits/motor/spring/friction/break, Grip/Mate/Socket/fit residual/force/debug. Play/Pause/Step/Reset usa Jolt real, cambios al safe point. Probar puerta/manivela/cajón/llave/pila/fusible/tapa/insertos como escenarios del mismo framework, authoring→Undo→save→cook→runtime.

## Sequencer

Modelo común de tracks tipados/curvas/keyframes/eventos/cámara/rails/audio/animation/environment/UI. Reusar evaluación Maestro y migrar formato/binding/owner a JE, no timeline engine paralelo por panel. Microanimación local puede quedar en rui; reusable/cross-system rseq.

Control por canal explícito. Una cámara fija no toma locomoción completa del player. Stop/cancel/skip/destroy restaura control/overrides/callbacks; event behavior para seek/reverse/loop definido. Preview no ejecuta daño/gameplay destructivo sin modo autorizado. Probar clocks/pausa/deltas sin esconder diferencias mediante roundtrip float.

## JericoUI y localización

rui2d/rui3d authoring→rui cooked. Screen-space/world-space/render-target y content3D admitido, layout/anchors/pivots/focus/clipping/z-order/DPI; controls/text/images/scroll/grid/drag/drop/3D item preview. Luau tweens/event handlers usan mismo Commands/Events/identity, no ActionScript ni VM UI adicional.

Inventory posee transacciones, slots/hover/tooltip/rotation son presentación. Drag/full/close menu no duplican ni pierden items. Debug layout/hit/focus. rlang→rloc, fallback/plural/format/UTF8/shaping/direction según soporte y rfont/atlas; traducciones Qt del editor separadas, i18nsupport branch no equivale a localización completa del juego.

User settings locales: input remap/subtitles/captions/scale/reduce motion/photosensitivity/camera comfort/haptics y display confirm/revert. Texto/iconos además de colores. Entity presentation profiles cambian aspecto sin duplicar gameplay; colisión sólo cambia explícitamente bajo Physics.

## Audio

Una autoridad JE Audio/SoundEvent/mixer/voices/streaming, low-level backend abierto privado. MiniAudio candidato a auditar frente al trabajo Jerico existente: source pin/license/codecs/lifecycle/performance. Nada Wwise/FMOD/cloud obligatorio.

2D/3D/ambient/attached, zones/portals/occlusion/obstruction/reverb, voice priority/virtualization/buses/ducking/preview; HRTF opcional con licencia/coste. Consultas físicas presupuestadas y listener compartido XR. PhysicalMaterial→rsoundsurface pasos/impactos/fricción, no material físico paralelo. Stop/unload/device switch/EOF/corrupt streaming/pausa se prueban; audio clock no se ata a graphics framerate.

## VFX

Polvo/vapor/chispas/humo/EmberGen flipbooks de datos licenciados, coste primero. OpenParticleSystem sigue candidato Preview, revisar sim CPU/GPU/curvas/graph/spawn/bounds/culling/overdraw/transparent/light/cook; no adoptarlo sin pruebas. rpart=VFX, vector fields/wind cuando haya consumidor y un Environment común.

VDB import tool-only puede admitirse de forma independiente; no backend global de geología→VDB. Gameplay proyectiles/daño/triggers no dependen de miles de GPU particle readbacks. Técnicas idTech lighting atlas/Smolder se miden después del sistema base, no nuevo framework renderer.

## OpenXR

Capacidad nativa comparte World/JE/Luau/Jolt/input/interaction/UI/audio. Diseñar vistas/poses/histories no mono-only desde foundation, verificar runtime/plugin fuente/licencia/pin antes de asumir soporte. Session/reference spaces/actions/swapchains/composition/haptics/timing/loss/reentry y sensores según dispositivo.

Backend binding DX12/Vulkan conforme a plataforma/runtime real; no otras APIs. Desktop stereo/mock no acredita headset/comfort/haptics. Pose tracking no mueve cuerpo sin política; simulation camera y head pose separadas, late update sólo probado. Tests por par sistema/API/headset independientes; no DX12 Linux gate.

## Aceptación y diferidos

N5 contacto/clip correction/fitting llave o pila/bimanual con Jolt y datos guardables. N6 UI/inventory/localización+sequence cancel devuelve control. N7 sonido común desde scripts/contacto/secuencia y emitter flipbook profiling/cook. N10 headset/loss/resume/world UI/interaction reales. Cada familia demuestra authoring/persistencia/cook/runtime y coste, no mock.

Facial/performance capture completo, motion matching/neural obligatorio, per-finger física y full offline editorial permanecen diferidos. Multiplayer de sesiones de interacción NO es futuro de este plan: está excluido. Flotabilidad sigue DESIGN ONLY aunque exista VehicleGame o water.
