# Motion Studio, interacción, Sequencer, UI, audio, VFX y XR

Estado: contrato de destino N5-N7/N10. No acredita EMotionFX adaptado, Jolt ni Luau integrados. Fuente histórica principal: AUDIO_VFX_ANIMATION_UI_MASTER de JericoEngine2; completar la lectura del tramo relevante antes de implementar.

## 1. Reutilización antes de reescritura

Auditar EMotionFX y su integración Atom para skeleton/clip/graph/skinning, Maestro/TrackView para timelines, LyShine y rama shine para UI, MiniAudio/AudioSystem para audio y OpenParticleSystem para VFX. Conservar comportamiento útil con procedencia permitida; no traer CryAnimation, Scaleform, Mannequin o middleware de CryEngine2.

Poder reproducir una animación no prueba herramientas de rig, pesos o IK. Un editor UI existente no prueba Luau ni los formatos Jerico. OpenParticleSystem sigue marcado Preview en el manifest leído; todo candidato necesita prueba de producción.

## 2. Datos de animación

`.rmesh`: geometría, skin weights/indices. `.rskel`: jerarquía/rest pose/unidades/bind data. `.rrig`: roles, controles, límites, solvers y contacto. `.ranim`: curvas/clips/eventos. `.rgraph`: resultado del graph de animación. `.rseq`: orquestación temporal reusable. No guardar tipos internos de EMotionFX ni confundir rig con skeleton.

Motion Studio debe crear/corregir huesos, pivots, jerarquía y pesos; normalizar y mostrar influencias, espejar cuando sea compatible, editar FK/IK, retargetear, trabajar layers/curvas/keyframes y corregir clips importados. Auto-rig/auto-weight propone y permite revisión, no promete resolver toda anatomía. Cascadeur/Blender son herramientas externas opcionales, no requisito para reparar un hueso o una pose.

Flujo exigido: importar personaje/manos → comprobar skeleton/weights → reproducir clip → pausar → ajustar IK/pose → guardar corrección como datos de animación → reopen/cook/runtime conserva el cambio. Distinguir modificación del clip, offset del rig y preview temporal. Undo/Redo no corrompe el source.

## 3. Objetos interactivos y verdad física

Jolt posee cuerpos, colisiones, masa/COM/inercia y constraints. IK representa dónde contactan manos/patas; no mueve el body por una vía paralela. Un dynamic body no tiene dos writers de transform.

GripPoint/GrabPoint describe agarre; SceneSocket ubicación de accesorio; MatePoint define relación de encaje; InsertionSocket y FitProfile definen inserción permitida. No reutilizar un port de tubería o socket genérico como flotador o grip sin rol explícito.

Door = body + collider + hinge + grip + latch opcional. Drawer = body + slider + grip. Valve = body/constraint y puntos de interacción. No crear DoorPhysics/DrawerPhysics/BatterySolver por objeto. Un Fit framework resuelve frame objetivo, tolerancias, ejes permitidos, secuencia de etapas, residual, velocidad/fuerza admitida, confirmación y salida/cancel. Cualquier asistencia artificial de snap se muestra y limita.

Un InteractionSession posee participantes, objeto, agarres, reservas y etapa. Bimanual no son dos sistemas de mano peleando. Una mano principal/secundaria o pesos de contacto se configuran y se resuelven consistentemente. Dos actores que quieren el mismo objeto requieren reserva/permiso; al desaparecer uno el sistema libera recursos de forma segura.

Casos de destino: puerta/manivela, cajón, batería/fusible, llave, tapa y pieza insertada; manos FPS con contact correction, reposicionamiento y animaciones de coger/soltar. Prueba con Jolt paused/step/reset, no animación decorativa.

## 4. Herramientas físicas y fitting

Seleccionar mesh → fit collider → masa/material/COM → Add Hinge/Slider → colocar pivot/eje con gizmo → límites/fricción/motor/spring/break threshold → grip/socket → Play/Pause/Step/Reset → aplicar o deshacer.

Viewport muestra colliders/contactos, COM/inercia, frames/ejes de constraints, límites, target IK, residual de fit, fuerza/torque y owner de reserva. Cambios incompatibles con step en ejecución se aplican en un límite seguro. Save/reopen y `.rcoll/.rpref/.rrig` preservan significado.

## 5. Sequencer

Un modelo temporal con tracks tipados, keyframes, curvas, eventos, cámara, rail/spline, audio, animación y parámetros ambientales/UI. Maestro es candidato de implementación, no ABI final. Reutilizar evaluación/curvas donde sirva; no crear un evaluador distinto para UI, cine y Motion sin necesidad.

Al reproducir, la secuencia adquiere control explícito por canal. Una cámara fija no toma por defecto el movimiento del jugador. Stop/cancel/skip/destroy restaura políticas de control, overrides y callbacks; eventos tienen semántica definida al hacer seek/reverse/loop. Preview del editor no ejecuta daño/gameplay destructivo salvo modo admitido.

UI microanimación puede ser local al `.rui`; timelines reutilizables/cross-system van a `.rseq`. No obligar una asset sequence independiente para cada hover. Curvas y orden de evaluación deterministas bajo los mismos inputs; registrar divergencias de clocks, no esconderlas con conversiones float.

## 6. JericoUI, localización y accesibilidad

`.rui2d/.rui3d` de autoría → `.rui` runtime. UI ScreenSpace, world-space y render-target; nodos con contenido/modelos 3D cuando se admita. Layout/anchors/pivots/focus/clipping/input/z-order/scale; controles, texto, imágenes, scroll/grid, drag/drop y preview 3D de inventario. Luau tweens/handlers usan la misma API Commands/Events, sin VM propia de UI.

Inventory conserva datos; slots/hover/tooltip/item rotation son presentación. Grid no debe duplicar los objetos al hacer drag ni perder una transacción si se cierra el menú. Debug de hit targets, foco y layout.

`.rlang` fuentes/textos y `.rloc` cocinado; fallback, pluralización/formato, UTF-8, dirección/shaping según soporte y fuentes `.rfont`. Qt translations del editor se gestionan por separado; `i18nsupport` no implementa automáticamente el sistema del juego.

Settings: remapping, audio, subtítulos/captions, escalado, Reduce Motion, photosensitivity, camera comfort y haptics. Confirm/revert para un display mode arriesgado. Indicadores no dependen solo de color. Perfiles visuales alternativos no bifurcan gameplay.

## 7. JericoAudio

Una autoridad SoundEvent/mixer/voz/espacialización, compartida entre editor, Luau, física, animación y sequences. Audio 2D, 3D, ambiental, attached, zonas y portales; streaming, voice limits/prioridad/virtualización, buses, ducking y preview con presupuestos. Low-level backend privado: auditar MiniAudio y el trabajo original de Jerico antes de elegir código. Sin Wwise/FMOD obligatorios.

Oclusión/obstrucción consultan geometría/física con budget, reverb y portales tienen estado explícito; no un raycast por voz cada frame sin medida. HRTF opcional según licencia/coste y configuración; XR no crea un audio engine distinto. Physical Material enruta impacto/fricción/pasos mediante `.rsoundsurface`; no reemplaza el material físico.

Stop/destrucción de entidad, cambio de dispositivo, pausa y scene unload cancelan o trasladan voces conforme a política. Tiempo de audio no se fuerza al framerate gráfico. Streams inválidos/EOF/corruptos fallan de forma acotada.

## 8. JericoParticles

Costo primero: polvo, vapor, chispas, humo y flipbooks (incluido material generado con EmberGen cuando sus derechos lo permitan). `.rpart` es producto VFX; import de VDB/volúmenes solo en tools y con datos runtime propios. No revive geology→VDB.

Auditar OpenParticleSystem: sim CPU/GPU, curvas/graph, spawning, bounds/culling, overdraw, transparencias, collision approximation, iluminación, eventos y cook. Vector fields/viento solo con uso real y budgets; un campo común de Environment.

Proyectiles/daño/trigger son gameplay/physics, no lectura obligatoria de miles de partículas GPU. Eventos visuales pueden degradarse por LOD sin cambiar el juego. Técnicas idTech de lighting atlas y Smolder son etapas comparativas posteriores, no dependencias del primer emitter.

## 9. OpenXR integrado

XR oficial comparte World, Luau, Jolt, input, interacción, UI, audio y render Atom/Vulkan. Localizar e inventariar el soporte OpenXR disponible o externo, licencia y pin; no afirmar que `engine.json` ya lo activa.

Session lifecycle, views/projections, swapchains, reference spaces, frame timing, actions, controller/hands según soporte, haptics, composición y estados de pérdida/reentrada. No convertir poses de tracking en simulación física sin política. Separar camera simulation de head pose; late update/reprojection solo con contrato probado.

Validación en headset real cuando haya hardware. Desktop stereo/mock no acredita session, input ni confort. Windows/Linux tienen estado separado y drivers/runtime registrados. Interfaces limpias permiten futuros backends; no abrir DX12 para cerrar XR actual.

## 10. Criterios por vertical

N5: esqueleto/rig/clip y mano que agarra puerta Jolt con límite; pausar/corregir/guardar/cook; bimanual sin doble writer; llave o batería prueba el Fit reusable.

N6: inventario visual Luau con hover/drag/tween/preview y save; sequence cámara+audio devuelve control tras stop/cancel/skip; localización cambia sin romper referencias.

N7: SoundEvent desde Luau y contacto físico con misma autoridad; scene unload sin voz huérfana. Emitter/flipbook authorable, culling y tiempos/memoria observados en runtime cocinado.

N10: headset, input/pose/haptics/UI world-space y agarre comparten mismo juego; sesión perdida/reanudada no deja body/cámara/input colgados. No se afirma un test de hardware no ejecutado.

Diferidos explícitos: facial/performance capture completo, motion matching obligatorio, neural animation, per-finger física obligatoria y multiplayer de sesiones de interacción. No bloquear el caso de uso principal con ellos.
