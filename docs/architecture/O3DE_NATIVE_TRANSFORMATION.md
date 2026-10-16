# Transformación O3DE → JE nativo

Vigente 2026-10-10. Diseño de destino, no integración ejecutada. Leer `JE_IDENTITY_AND_NATIVE_MODULES.md` en este directorio y GOAL. Sustituye la opción de conservar AZ/Gem como nomenclatura propia final.

## Un engine, no fachada

Reutilizar implementación madura de O3DE y absorber las capacidades necesarias dentro de los owners JE. Los nombres actuales se usan para auditar, nunca para definir la API final. Una sola autoridad de entidades, identidad/revisiones, prefabs, transacciones, physics, script gameplay y render graph. No segundo engine/cooker/registry porque haya código histórico.

El usuario crea un proyecto y dispone de BSP/materiales/scripts/física sin instalar Gems. Se permiten bibliotecas internas y módulos linkados, no un marketplace de features ni un mega-Gem Jerico. Registro O3DE, gem.json y aliases sólo transitorios, con mapa de consumidores/retirada. Mover source a Engine exige actualizar CMake, reflection, codegen, assets, builders, registry y runtime juntos.

## Owners destino y fuentes iniciales

| Owner JE | Base a inspeccionar/adaptar | Resultado |
|---|---|---|
| Core/Platform/World | AzCore/AzFramework | Entities/reflection/jobs/IO y lifetimes migrados a JE; separar red/IPC |
| Assets/Cook | Asset Processor, SceneProcessing, AssetBundler | Una Canonical IR/ID/revisión/dependency pipeline, formatos propios |
| Render | Atom RPI/RHI/Feature/Common/shader pipeline | Un renderer JE con Vulkan activo y VR/OpenXR sobre Vulkan; DX12/Metal inactivos, sin Diligent debajo |
| Editor | Code/Editor, AzToolsFramework/AzQtComponents | Qt6 Widgets Jerico, viewport compartido y transacciones/gizmos |
| Geometry/BSP | WhiteBox/CSG/OpenMesh/Manifold | Herramientas brush/caras y semántica/UV/material/collision, no nuevo editor aislado |
| Geology | Matemáticas propias Jerico/Playground auditadas | Operation stack, SDF/mesher cuando corresponde, materiales derivados del mismo campo |
| Physics | Interfaces físicas/consumidores O3DE + Jolt | Migración real de bodies/queries/cook/editor; PhysX sólo puente privado |
| Script | Binding/reflection/IDE existentes + Luau | JE API tipada, self/references seguros; ninguna API pública AZ/Lua heredada |
| Animation/Sequencer | EMotionFX, integración Atom, Maestro/TrackView | Rig/clip/graph/timeline y corrección native en JE |
| UI/Audio/VFX | LyShine/Shine candidato, MiniAudio, OPS | Reutilización selectiva bajo JericoUI/Audio/Particles con formatos propios |
| Navigation/AI | Recast + World/physics + HTN | Recast privado offline y planner explicable sin red de juego |

## Disposición de capacidades

INTEGRAR: Atom con Vulkan activo y VR/OpenXR sobre Vulkan, framework/world/assets/transactions necesarios; WhiteBox prioritario; GraphCanvas/GraphModel y MaterialEditor/Canvas; DebugDraw/profiler/dev textures; camera/prefab/scene processing. LmbrCentral contiene capacidades necesarias: extraerlas/migrarlas por función, no conservar un owner Lumberyard permanente ni borrar todas por nombre.

AUDITAR/ADAPTAR: EMotionFX/Maestro, Luau bindings, MiniAudio, OPS (Preview), Recast, terrain/vegetation/surface/gradients/landscape, SkyAtmosphere/Stars, UI tools. Graph framework común no es evaluador común ni VM nueva. Manifest vacío no garantiza ausencia de deps: Recast CMake demuestra deps a framework/shapes/debug y tests PhysX.

TRANSITORIO: PhysX mientras se migran sus consumidores; Lua/functors internos de materiales/tools hasta una decisión localizada que conserve comportamiento; Gem registration hasta composición JE probada. No son permisos para exponer doble runtime ni conservar AZ final. Auditar emisor/productor/consumidor, fase y removal test en cada puente.

EXCLUIR DE PRODUCTO, NO DIFERIR: AWS/IoT/GameLift/cloud integration, Multiplayer/MultiplayerCompression/server/session/matchmaking/replication, commerce/online accounts, Metal/WebGPU/DX11/OpenGL/móvil. No traer backends de ramas antiguas. Los aliases Servers de un Gem no prueban que ya haya un servidor activo, pero se retiran de la composición final.

TEST-ONLY: Null/headless y mocks necesarios; no como render tercer candidato. IPC local de Editor/AssetProcessor/MCP/debug se limita a tools. Nunca usar networking de juego entero para ese fin cuando pueda extraerse la parte requerida.

DIFERIDOS QUE SIGUEN ASÍ: rendering RT, facial/neural/motion matching obligatorio, underwater/costa según roadmap; buoyancy DESIGN ONLY. DiffuseProbeGrid realtime depende de RT y no se activa; baked tampoco autoriza automáticamente un bake RT en esta fase. Geología no está en esta lista: está activa.

## Primer ejemplo de absorción

WhiteBox: source/CSG→JE Geometry/BSP; componentes y asset/undo handlers a los owners existentes migrados; Manifold/OpenMesh privados; interacciones de viewport en Editor. Material/UV/region lineage y resultados vacíos se verifican, no se deducen de un boolean que devuelve true. El editor prueba construcción/Undo/save/cook/runtime y los pares gráficos. Luego retirar registration/aliases/carpeta original sólo cuando todos esos consumidores se hayan movido.

No fake empty modules: CMake de WhiteBox advierte que abrir/guardar sin un componente soportado puede perder sus datos. Mantener lectura preservadora o rechazar guardado de datos no soportados durante transición; un DLL vacío no es compatibilidad.

## Política de mantenimiento

Registrar base upstream y cambios propios por área; elegir fixes mínimos de otras ramas sólo cuando falta comportamiento y diff/license/test justifican. No actualizar Qt/RHI globalmente por nombre de rama. Mantener orden de init/shutdown, jobs/callbacks/serialización/cache/version y deployment coherentes al renombrar. No performance claim por dividir DLLs/renombrar.

La prueba de integración combina identidad JE real, comportamiento preservado, proyecto sin Gem manager, dependencias locales/privadas auditadas y ausencia de AWS/multiplayer. Ningún componente está aceptado sólo por esta tabla.
