# Matriz JE de requisitos y exclusiones

**Actualización del propietario 2026-10-10, posterior a a536fb7.** IDs existentes conservados; semánticas que cambiaron se corrigen aquí. Estado global de filas activas: CONTRACT ADOPTED / IMPLEMENTATION NOT VERIFIED; evidencia parcial ejecutada al final. Ninguna fila completa se cierra por un test focal. EXCLUDED no significa DEFERRED.

| ID | Contrato | Owner/etapa | Evidencia mínima |
|---|---|---|---|
| JER-001 | Base O3DE, no importación automática de Cry | Legal/Core N0 | SHA/tree/procedencia por archivo incorporado |
| JER-002 | Rama única development, un integrador | Continuo | Ref/publicación, sin branches/worktrees |
| JER-003 | Funciones nativas dentro del engine, sin activar Gems por proyecto | Core/Editor N1→N11 | Proyecto limpio sin Gem install/catalog |
| JER-004 | Un World/identity/asset graph/undo/physics/render owner | Arquitectura | Producers/consumers/lifetimes únicos |
| JER-005 | DX12 y Vulkan; Windows ambos, Linux Vulkan | Render N0/N2/N12 | Tres pares probados separadamente |
| JER-006 | Atom adaptado a JE::Render, sin Diligent debajo | Render | Un graph/device/submit, identidad migrada |
| JER-007 | Qt6 Widgets Jerico, viewport compartido/docking | Editor N1 | Tabs conservan scene/selection/undo |
| JER-008 | Diálogos/DPI/layout/save/close/recovery | Editor N1/N12 | Recorridos reales y corrupt/error |
| JER-009 | GUID/typed refs/Scene/PrefabInternal | World N1/N3 | Duplicate/remap/instance/destroy/reload |
| JER-010 | Drag Outliner→self.references, self seguro | Script N3 | Binding reutilizado, generación reciclada rechazada |
| JER-011 | glTF2/GLB/ufbx FBX/OBJ-MTL tool-only | Assets N2 | Jerarquía/slots/skin por admisión, fuente ausente runtime |
| JER-012 | Texturas/heightmaps del master assets | Assets N2/N8 | Tests por formato/bounds/paths |
| JER-013 | Nombres libres, ORM/separados y metadata | Assets/Render N2 | Equivalencia/override/reimport |
| JER-014 | Mips/tangents/negative scale/handedness | Assets N2 | Odd dimensions/UV degenerate/normal flip |
| JER-015 | Reimport/cancel/overrides/candidate-first | Assets N2 | Revision anterior sigue válida tras fallo |
| JER-016 | Canonical IR/DDC/cook/deps/hot reload únicos | Assets N2/N11 | Invalidation local y retiro seguro |
| JER-017 | Hierarchy/prefab/variants/layers/cells/HLOD/residency | World N1/N8 | Save/instance/unload/reload/ref |
| JER-018 | Commands/Events/tipos para UI/Luau/MCP | Core N1/N3 | Misma transacción y permisos |
| JER-019 | Luau strict gameplay/UI VM único | Script N3/N6 | Typecheck/cook/reload y límites |
| JER-020 | Triggers shape/proximity/click/input/event/state/Once | Gameplay N3 | Activator/filtro/reentrancia/destrucción/save |
| JER-021 | Store opt-in de cambios confirmados | Save N3/N6 | Sin doble Store.set/registro |
| JER-022 | Input contexts/rebind/devices/XR/haptics | Input N3/N10 | Focus/disconnect sin estado atascado |
| JER-023 | Save providers/schema/corrupt/transaction/recovery | Save N3/N6/N11 | Invalid restore no publica parcialmente |
| JER-024 | Inventory vs Items/ItemStudio vs UI | Gameplay N6 | Pickup/drag/full/destroy sin duplicar items |
| JER-025 | Dialogue/Quest/Journal/Narrative/localización | Gameplay N6 | Recorrido cook/save con refs válidas |
| JER-026 | Replay QA local, actions/seeds/build identity | QA N6/N12 | Divergence observable, no promesa cross-platform |
| JER-027 | GraphCore UI común, evaluadores por dominio | Editor N3/N6/N9 | No segunda VM/planner |
| JER-028 | Jolt body/shapes/queries/triggers/joints/character | Physics N4 | Authoring/cook/runtime en scene real |
| JER-029 | PhysicalMaterial/COM/inertia/motor/break/debug | Physics N4/N5 | Pause/step/limits/contact reales |
| JER-030 | Jolt único, PhysX transitorio sin body duplicado | Physics N4/N11 | Consumer/removal manifest y single writer |
| JER-031 | Skeleton/rig/clip/graph rskel/rrig/ranim | Animation N5 | Formatos sin ABI de tercero |
| JER-032 | Bones/weights/FK/IK/retarget/clip correction | Motion N5 | Pausar/corregir/guardar/reopen/cook |
| JER-033 | Grip/Grab/SceneSocket/Mate/Insertion/Fit | Interaction N5 | Roles claros, mismo framework llave/batería |
| JER-034 | Un InteractionSession bimanual, Jolt manda | Interaction N5 | Contacto sin teleport/doble transform writer |
| JER-035 | Fit DOF/tolerances/stages/symmetry/hysteresis/rollback | Interaction N5 | Seated/cancel/safe-drive/invalid refs |
| JER-036 | Save sesión activa con restore/cancel seguro | Interaction N5/N6 | Sin pointers de solver |
| JER-037 | rseq tracks/curves/events/control por canal | Sequencer N5/N6 | Stop/seek/skip devuelve control |
| JER-038 | rui2d/rui3d→rui, grid/hover/drag/3D/tweens | UI N6 | Inventory autoridad única, cooked UI |
| JER-039 | Screen/world-space/RTT/3D UI | UI/XR N6/N10 | Hit/focus/scale por espacio |
| JER-040 | rlang/rloc/rfont/fallback/shaping/accessibility | UI N6/N12 | Texto/idioma/DPI, no solo color |
| JER-041 | Audio privado/SoundEvents/mixer/voices/stream | Audio N7 | Lua/contact/sequence convergen; device loss/unload |
| JER-042 | Spatial/zone/portal/occlusion/reverb/HRTF opcional | Audio N7/N10 | Budget/listener compartido XR |
| JER-043 | VFX authorable/flipbooks/rpart/culling | VFX N7 | Cook/runtime/overdraw, gameplay no readback masivo |
| JER-044 | VDB privado Tool-only de VFX si admitido | VFX N7 | No source VDB en Shipping ni backend geology→VDB |
| JER-045 | WhiteBox/CSG→JE BSP/Geometry temprano | Geometry N1/N4 | Cuarto/apertura/UV/material/Undo/collision |
| JER-046 | RBrush BRUSH/PAYLOAD/TARGET | Surface N1/N8 | Paint/mask/decal/damage salidas separadas |
| JER-047 | Decals OBB/gradients/channels/order/receiver/LOD | Surface/Render N2/N8 | Metal/grazing/overlap/moving receivers |
| JER-048 | Vertex paint/material masks/remap | Geometry N8/N10 | Topology/reimport preserva o invalida explícito |
| JER-049 | RMod library/ports/sockets/profile/mating | Procedural N8 | Frame/roll/compatibilidad y saved IDs |
| JER-050 | Routes/fittings/manual intersections/flanges/hysteresis | Procedural N8 | T/U/cruz/empates/locked parts |
| JER-051 | Soportes floor/wall/ceiling/obstacles por queries | Procedural N8 | Sin soportes flotantes/world paralelo |
| JER-052 | UV/arc-length/RMF/catenaria/cadenas/edge chains | Procedural N8 | Curva degenerada y 16/32/64 links |
| JER-053 | PWAS proposals/masks/locks/seeds/overrides/GUID | Procedural N8 | Layout manual conservado al cambiar seed |
| JER-054 | EPLS señales/capas, sin segundo sistema ELSP | Environment N8 | Owners/datos canónicos |
| JER-055 | Prefracture/bonds/anchors/interiores/fragmentos | Physics/Geometry N8 | Activación local Jolt y debris VFX |
| JER-056 | Environment/clock/sun/sky/weather/wind único | Environment N2/N8 | No duplicar clocks/exposure/campos |
| JER-057 | Physical wind opt-in y fan local | Physics N8 | Incluido/excluido; emitter retirado cero fuerza extra |
| JER-058 | Agua superior/local real | Water N8 | No PASS de Cry trasladado |
| JER-059 | Recast/Detour local, tiles/links/queries/provider | Navigation N9 | Repath/obstáculo/Jolt/unload |
| JER-060 | HTN puro/explicable/budget/cancel | AI N9 | Planning no muta World, rechazos visibles |
| JER-061 | Roles/species/relationships/groups/territory | AI N9 | Targets/reservas coherentes, no multiplayer |
| JER-062 | Aprendizaje individuo/colonia/bait guardable | AI N9 | Outcomes confirmados y comunicación limitada |
| JER-063 | OpenXR nativo compartido desde foundation | XR N0/N1→N10 | Interfaces no fijadas a mono/desktop |
| JER-064 | XR hardware/actions/haptics/pose/loss/UI | XR N10 | Headset y pares/backend binding propios |
| JER-065 | 2D/2.5D/pixel/isométrico/híbrido/normals | Presentation N10 | Picking/sorting/pixel/shared physics |
| JER-066 | VehicleGame Jolt/input/posesión | Gameplay N10 | Suspension/carga/entry/exit; buoyancy no implícita |
| JER-067 | Calidad Performance/Balanced/Quality/Cinematic/VR/Pixel | Settings/Render continuo | Presupuestos/fallback medidos |
| JER-068 | r*/epak versioned/bounded/ABI independent | Assets N2→N11 | Readers/writers/consumers negativos/migraciones |
| JER-069 | Distribución final executable/epak/libs/notices | Package N11 | Source-free aislado sin checkout/tools |
| JER-070 | Sin XML propio final; transición con consumidores | Assets N11 | Readers reales, no purge de archivos legales/tools |
| JER-071 | AWS excluido en runtime/build/hosting/fallback | Legal/Build N0→N11 | Offline/cache/falta de paquete sin descarga AWS |
| JER-072 | SBOM/licencias/Qt/linkage/avisos de terceros | Legal N0/N11 | Dependencias efectivas, no README-certificado |
| JER-073 | Sin Autodesk FBX SDK/ATL-MFC impuestos | Build N0/N2 | Target real y sustitución permitida |
| JER-074 | Crash/hang/Doctor/fuzz/security/trust | QA continuo | Failure/recovery/privacy y parsers bounded |
| JER-075 | AutomationBridge/MCP local tipado | Tools N1/N3 | UI/API misma transacción/alcance |
| JER-076 | JE:: identidad real, owners propios y módulos | Core continuo/N11 | Source/generated/export/serialización sin aliases finales |
| JER-077 | SIG-01..22 por problema/consumer/imagen/coste | Research continuo | KEEP/ADAPT/REJECT/DEFER demostrado |
| JER-078 | Error book y evidencia histórica separada | QA continuo | Hipótesis no causa; no PASS de otro engine |
| JER-079 | Buoyancy/Enable Buoyancy por WaterVolume | DESIGN ONLY | Permiso propio requerido |
| JER-080 | Underwater/costa Horizon | DEFERRED | No bloquean BSP ni reabren pruebas Cry |
| JER-081 | Geología integral REACTIVADA: roca/cliff/cueva/stack | Geology N8 adelantado | Showcases/topología/material/collision/cook; sólo BSP→VDB sigue cancelado |
| JER-082 | Metal/WebGPU/DX11/OpenGL/móvil excluidos; render RT fuera de fase | Render | Build/selector/package, DX12 NO está excluido |
| JER-083 | Facial/neural/motion matching/per-finger no foundation | DEFERRED | Multiplayer sessions EXCLUDED, no una extensión futura |
| JER-084 | JE Material Graph/instances sobre implementation Atom | Materials N2 | rmat cooked/variant sin source compiler |
| JER-085 | ORM R=AO/G=roughness/B=metal y normal/color-space | Materials N2 | Equivalencia mapas separados/reimport/scalars |
| JER-086 | rtext/mips/BC por semántica y budget | Assets N2 | Odd dimensions/normal vector/retirement seguro |
| JER-087 | Codecs r* magic/version/chunks/endian/bounds/validator | Formats N2→N11 | Roundtrip/corrupt/migración y consumidor |
| JER-088 | EPAK indexed streaming/compression/hash por política | Package N11 | Random access/bounds/pin/licencia; no platform online |
| JER-089 | Firma/integridad/cifrado separados y política local | Security N11 | Reject corrupción/firma inválida cuando se implemente; no crypto propia |
| JER-090 | Base/delta overlays y rollback transaccional | Package N11 | Interrupción preserva contenido válido |
| JER-091 | ContentPack/campañas/saves locales y assets compartidos | Gameplay/Package N6/N11 | Sin commerce/entitlement remoto/online accounts |
| JER-092 | Hot reload/dependency revisions exactas | Assets/Render N2 | No recook inútil, cancel/unload/fences |
| JER-093 | Multi-slot/skin/UV/alpha/corrupt import fixtures | Import N2 | Cobertura real por variante, no extensión reconocida |
| JER-094 | JE:: también en internals adaptados, no JE=AZ | Identity continuo | Namespace/codegen/type-ID/export migration real |
| JER-095 | Absorción source/assets/builders/registro de Gems en engine | Native N1→N11 | Lifecycle/consumer/removal tests y proyecto sin Gem manager |
| JER-096 | Multiplayer/servers/session/replication/cloud excluidos | Build/Core N0→N11 | No linked/deployed/runtime authorities ni optional flags para ellos |
| JER-097 | Windows DX12/Vulkan + Linux Vulkan, Null solo test | Render N0/N2/N12 | Selector negativos y evidencia por par |
| JER-098 | Fuentes/paquetes controlados y build offline fail-closed | Dependencies N0/N11 | Cache completa/incompleta, sin fallback externo silencioso |
| JER-099 | BSP/UI producen primer progreso visible | Editor N1 | Cuarto/corredor/aperturas/materials/Undo/Save/runtime |
| JER-100 | Geología forma/material comparten campos semánticos | Geometry/Material N8 | Strata/erosión/depósito/interior concuerdan visualmente |
| JER-101 | Optimización continua de editing/meshing/render | Performance N1/N2/N8 | Imágenes/CPU/GPU/RAM/VRAM/p95/p99 comparables |
| JER-102 | Ideas de otros engines con procedencia y experimento | Research | Referencia exacta/qué no copiar/fallback/resultado |
| JER-103 | Seguimiento de hallazgos DEP-01..12 y aliases/UUID | Core/Build/Geometry | Source audit llevado a grafo/link/runtime sin falsos PASS |

Al cerrar una fila, enlazar commit y evidencia de todos sus casos relevantes. No dar por completada una familia por un fixture; no recuperar exclusiones de documentos antiguos. Números N son owners/etapas, no secuencia numérica obligatoria: manda ROADMAP revisado. Las políticas materiales/cripto restantes son diseños y se implementan con necesidad real, sin añadir plataforma externa.

## Evidencia parcial de ejecución N0

Ver [expediente 2026-10-10](evidence/n0/N0_EXECUTION_2026_10_10.md), source/log hashes y publicación development. **No son cierres de filas.**

| IDs | Estado ejecutado | Aceptación pendiente |
|---|---|---|
| JER-002 | Clone/rama/sync y checkout único comprobados | Ref de publicación verificada en cierre Git |
| JER-071/098 | Resolver local y rechazos/fallback CMake focal verificados; LFS excluido, sin payload fetch | Grafo/link/deploy/runtime, otras rutas CMake/pip/PATCH_FILES y offline completo |
| JER-076/094 | Núcleo productivo real JE::BSP::Csg std+Manifold compilado, sin alias; adapters migrados por fuente | Resto del engine, UUID/codegen/export/serialización y consumers reales |
| JER-099 | Habitación/corredor/puerta/ventana CSG: 33 checks de kernel GREEN | Editor real, UV/material por cara, Undo/save/reopen/cook/runtime e imagen |
| JER-005/097 | Configure Windows protegido BLOCKED por zlib; GPU/Vulkan enumerados | Windows-DX12, Windows-Vulkan y Linux-Vulkan render reales |
| JER-101 | Bytes source/LFS y tiempos focales medidos | Métricas/coste/calidad comparables del producto, sin ahorro afirmado |
