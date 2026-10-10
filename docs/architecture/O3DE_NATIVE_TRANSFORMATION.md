# Arquitectura nativa — transformar O3DE sin duplicar el motor

Estado: diseño de destino; source inventory inicial, integración no verificada. Autoridad superior: `GOAL.md`.

## 1. Qué significa integrar

Jerico selecciona, compila, inicializa y ofrece sus sistemas como capacidades permanentes del motor. El usuario crea un proyecto y usa Physics, Script, Animation, Audio, UI o Geometry sin instalar Gems Jerico. Un perfil puede omitir trabajo/coste de funciones no usadas, pero no se convierte en un marketplace ni en una cadena manual de extensiones.

Primero se puede usar la maquinaria interna de O3DE para registrar módulos y componentes. No se exige eliminar `gem.json`, `AZ::Module` o toda la infraestructura antes del primer resultado. Una conversión física a `Code/...` solo se justifica cuando reduce complejidad y migra CMake, builders, reflection, assets y runtime conjuntamente. Cambiar de carpeta no prueba integración nativa.

El orden es: base O3DE funcionando → contrato/owner Jerico → consumidor real → editor/persistencia/cook/runtime → retirar ruta redundante. No crear todas las clases Jerico vacías por adelantado.

## 2. Owners y rutas existentes para iniciar cada auditoría

| Owner destino | Punto de partida en O3DE | Qué conserva / qué cambia |
|---|---|---|
| Core/Platform/Jobs | `Code/Framework/AzCore`, `AzFramework` | Lifetimes, tareas, IO, reflexión y entidades existentes; contratos públicos propios solo donde haya consumidor |
| Editor/Transactions | `Code/Editor`, `Code/Framework/AzToolsFramework`, `AzQtComponents` | Selección, property editor, docking, undo, prefabs; evolucionar UX y comandos, no segundo editor |
| Assets/Cooker | `Code/Tools/AssetProcessor`, `AssetBundler`, `Gems/SceneProcessing` | Builders y dependencia/invalidation; conectar IR y productos Jerico en esa ruta |
| Render | `Gems/Atom/RHI`, `RPI`, `Feature/Common`, `Asset/Shader` | Un device/FrameGraph/Pass System; extender material/shaders/feature processors |
| Physics | `Code/Framework/AzFramework/AzFramework/Physics`, `Gems/PhysX` | Auditar interfaces/consumidores; implantar Jolt sin duplicar cuerpos productivos |
| Script | AzCore/AzFramework Script y `Code/Tools/LuaIDE` como rutas a localizar | Luau en gameplay, bindings y ownership seguros; herramientas heredadas se clasifican aparte |
| Animation | `Gems/EMotionFX`, `Gems/AtomLyIntegration/EMotionFXAtom` | Reutilizar runtime/skinning/editor, ampliar rigs/IK/edición y productos |
| UI/Sequencer | `Gems/LyShine`, `Gems/Maestro`, `Code/Editor/TrackView`; rama shine como lectura | No dos runtimes UI/timelines; migrar formato/binding/consumidores |
| Navigation/AI | `Gems/RecastNavigation` + owner de World | Navigation no es HTN; integrar queries/providers de colisión adecuados a Jolt |
| Audio | `Gems/MiniAudio`, `Gems/AudioSystem` | Backend bajo nivel y toolset auditados; Jerico posee SoundEvent/mixer/espacialización |
| Procedural/World authoring | `Gems/WhiteBox`, Terrain, SurfaceData, GradientSignal, LandscapeCanvas | Herramientas/queries de base, no un segundo World o generador geológico obligatorio |

Las rutas describen familias inspeccionadas por manifest/documentación, no una auditoría de todas sus funciones. Congelar interfaces exactas después de leer el target de la versión real.

## 3. Gem disposition inicial

`CONSERVAR` no significa sin cambios para siempre; `AUDITAR/ADAPTAR` no significa ya aceptado; ningún manifest sustituye revisar CMake/transitivas y datos.

| Familia/Gem | Destino | Obligación de auditoría |
|---|---|---|
| Atom / CommonFeaturesAtom / AtomLyIntegration | CONSERVAR e integrar | Aliases/runtime/editor builders, bootstrap, passes, dependencias AZ y ownership |
| LmbrCentral | CONSERVAR lo requerido | Identificar shapes, spline, componentes/assets reales; no borrar por nombre Lumberyard |
| Camera / CameraFramework | CONSERVAR | Cámara única, editor/play/sequence/XR y poses |
| PrefabBuilder / SceneProcessing | CONSERVAR | GUID, overrides, productos y reimport; evitar doble prefab graph |
| GraphCanvas / GraphModel | ADAPTAR a GraphCore | UI compartida; compiladores/evaluadores específicos, no imponer un VM común |
| MaterialCanvas / MaterialEditor / PassCanvas | ADAPTAR | Inspeccionar Atom Tools reales antes de escribir editores nuevos; controlar variantes |
| WhiteBox | AUDITAR/ADAPTAR alta prioridad | Manifest depende de Atom_RPI, Atom_Feature_Common, CommonFeaturesAtom y EditorPythonBindings; no asumir CSG robusto, UV o topología final |
| EMotionFX | AUDITAR/ADAPTAR alta prioridad | Skeleton/clip/graph, editor, skinning, retargeting; comprobar licencia y gaps de rigging |
| MotionMatching | DIFERIR | No bloquear animación básica ni asumir base de datos/retargeting resueltos |
| Maestro | AUDITAR/ADAPTAR | Tracks/keyframes/curvas, cámara, eventos y formato; no control automático del jugador |
| LyShine / ScriptedEntityTweener | TRANSITORIO / ADAPTAR | Tools, XML/slices, binding Lua, UI animation; destino rui/Luau |
| Shine (solo otra rama) | REFERENCIA CANDIDATA | Prefabs/conversión; ningún merge autorizado por este listado |
| MiniAudio | CANDIDATO alto | Manifest 0.1.0 y sin dependencias Gem declaradas; comprobar biblioteca efectiva, codec/streaming/thread/lifecycle y licencia aparte |
| OpenParticleSystem | CANDIDATO con cautela | Manifest dice Preview y formatos pueden cambiar; probar GPU/CPU, flipbooks, profiling, authoring y cook |
| RecastNavigation | CANDIDATO alto | Manifest declara Recast/Detour por FetchContent y licencia zlib de biblioteca; rastrear CMake y posible acoplamiento físico, no confiar solo en dependencies vacío |
| Terrain / Vegetation / SurfaceData / GradientSignal / LandscapeCanvas / FastNoise | ADAPTAR selectivo | Un entorno, masks/seed/streaming/instancing; no convertirlos en geology obligatoria |
| SkyAtmosphere / Stars | CONSERVAR/AUDITAR | Estado/clock/sol/IBL/exposure únicos; LUTs y coste medido |
| DiffuseProbeGrid | RESTRINGIR | Modo realtime usa ray tracing. Baked puede ejecutarse sin RT pero su generación tiene otro requisito; no activarlo automáticamente en fase sin RT |
| PhysX / ScriptCanvasPhysics | TRANSITORIO → SUSTITUIR | Migrar todos los consumidores (character, triggers, nav, editor, cook) hacia Jolt antes de retirar |
| ScriptCanvas / ScriptEvents | TRANSITORIO/RETIRAR de gameplay | Rescatar infraestructura útil sin segundo VM; no destruir GraphCanvas por asociación |
| SaveData | CANDIDATO de almacenamiento | No es por sí solo Save/Load de World/Store/physics/VM |
| DebugDraw / DevTextures / Profiler / ImGui / RemoteTools | CONSERVAR/ADAPTAR | Debug reusable, coste apagado, privacidad/puertos locales |
| EditorPythonBindings / QtForPython / PythonAssetBuilder | TOOLS privados | No despliegue Shipping implícito ni segunda API de mutación |
| NvCloth / AtomTressFX | OPCIONAL DIFERIDO | Licencias, platform support y coste; no arrastrar middleware por defecto |
| Multiplayer / Achievements / Commerce / Presence / ejemplos | NO BASELINE | Conservar fuente, excluir configuración productiva cuando el grafo lo permita |
| AWS Gems/SDKs/herramientas | NO PRODUCTO | Inventariar uso real, retirar integración de servicio y resolver hosting de paquetes/LFS por separado |

## 4. Cómo migrar un subsistema completo

Para cada decisión crear una ficha dentro de la evidencia del vertical con: source SHA/rutas, licencias, targets, init/shutdown, servicios, datos persistentes, builders, consumidores, coste actual y tests. Elegir KEEP/ADAPT/REPLACE/DEFER/REMOVE con razón.

Ejemplo físico: un cubo aislado con Jolt no autoriza eliminar PhysX. Primero mover queries y cuerpos de una escena productiva, colliders/cook, propiedades, debug/undo y runtime empaquetado. Luego ampliar joints/characters/nav/terrain según el roadmap. Nunca simular el mismo cuerpo en PhysX y Jolt simultáneamente. Fixtures de comparación independientes pueden utilizar ambos motores sin crear doble autoridad de juego.

Ejemplo scripting: sustituir Lua 5.4 por Luau no es cambiar el include o biblioteca. Separar binding, bytecode, sandbox, editor/typecheck, subscriptions y material functors. El material pipeline original puede ejecutar Lua interno; inventariarlo y mantenerlo solo como tool privado acotado hasta migrarlo. No dejar dos VMs de gameplay públicas.

Ejemplo Atom: un pass nuevo debe consumir attachments/SRG/scene/settings ya gestionados por Atom. No crear JericoRenderGraph al costado, ni device Vulkan directo. Reusar shader builder/AZSL/DXC y variants. Un render wrapper con nombre Jerico no da acceso válido a features no probadas.

## 5. Composición nativa verificable

La prueba mínima crea un proyecto limpio mediante la ruta admitida y muestra un panel/componente Jerico real registrado por la distribución del motor. No tiene pasos de instalar un Gem Jerico ni activar cadenas manuales. Autoría → undo/redo → save/reopen → builder/cook → runtime observa el mismo dato. El registro se destruye una vez al cerrar.

Durante N0 no renombrar `engine_name` ni el engine ID: la identidad técnica es consumida por registro/proyectos/CMake. En la fase de identidad cambiar juntos product names, targets, paths, descubrimiento, instalaciones y paquete. Preservar aliases de migración limitados con fecha/owner/criterio de retiro; no duplicar fuentes de verdad.

## 6. Opciones descartadas

1. Borrar todos los Gems y reconstruir lo esencial: coste y riesgo innecesarios.
2. Conservar O3DE entero y agregar otro JericoEngine independiente: doble estado, herramientas y diagnóstico.
3. Cambiar automáticamente de base a main por ser main: no refleja la decisión ni el historial observado.
4. Reescribir todo AZ/EBus/CMake al inicio: no hay evidencia que justifique ese alcance.
5. Agregar Diligent bajo Atom para tener Vulkan: Atom ya es el punto de partida; exigir ventaja medida para otra decisión.

## Fuentes

Pinned fork `241c78b8064898a709cf1c5dd3535c6406763ef3`: `engine.json`, `LICENSE.txt`, `Gems/{WhiteBox,RecastNavigation,MiniAudio,OpenParticleSystem}/gem.json`, `cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake`. Documentación oficial: https://www.docs.o3de.org/docs/user-guide/gems/reference/ y https://docs.o3de.org/docs/atom-guide/ . Los manifests no constituyen pruebas de ejecución.
