# Matriz de requisitos conservados

Fecha 2026-10-10. Todos los requisitos activos están **CONTRACT ADOPTED / IMPLEMENTATION NOT VERIFIED** en esta publicación. Existencia de source O3DE no acredita el contrato Jerico. D = diferido/no autorizado; H = historia/referencia. Ninguna fila puede desaparecer por resumir una familia.

| ID | Requisito y alcance mínimo | Owner / etapa | Prueba que lo distingue |
|---|---|---|---|
| JER-001 | O3DE como base, sin código Cry importado automáticamente | Legal/Core N0 | SHA/tree/procedencia por incorporación |
| JER-002 | Única rama development, un integrador | Governance continuo | Ref de cada publicación, sin branches/worktrees nuevas |
| JER-003 | Sistemas nativos, no instalación manual de Gems Jerico | Core/Editor N1 | Proyecto limpio usa capacidad sin pasos de instalación |
| JER-004 | Un World/identity/asset graph/undo/physics/render authority | Arquitectura continuo | Productores/consumidores y lifetime únicos |
| JER-005 | Vulkan Windows y Linux | Render N0/N2/N12 | Build y runtime real separados por plataforma |
| JER-006 | Atom RPI/RHI/Pass/FrameGraph, sin Diligent debajo | Render N0/N2 | Ruta de submission/resources y consumo observada |
| JER-007 | Qt6 Widgets, viewport compartido, docking/workspaces | Editor N1 | Cambio tab conserva escena/selección/historial |
| JER-008 | Diálogos, DPI, restore layout, close/save/recovery | Editor N1/N12 | Recorridos reales, no solo screenshot |
| JER-009 | GUID y typed refs, Scene/PrefabInternal | World/Script N1/N3 | Duplicate/instance/remap/destroy/reload |
| JER-010 | Outliner→IDE reutiliza self.references, self nativo seguro | Script N3 | Drag dos veces no duplica binding; handle reciclado rechaza |
| JER-011 | glTF2/GLB, ufbx FBX, OBJ/MTL por tools | Assets N2 | Jerarquía/slots/skin cuando se admita; casos por formato |
| JER-012 | Texturas y heightmaps según master assets | Assets N2/N8 | Format-specific tests, bounds y paths seguros |
| JER-013 | Nombres libres, ORM/separados, semánticas/normal/color-space | Assets/Render N2 | Inputs equivalentes dan material equivalente tras reimport |
| JER-014 | Mips/tangentes/handedness/negative scale | Assets N2 | UV degenerada, normal flip y odd dimensions |
| JER-015 | Reimport incremental, overrides, candidate-first/cancel | Assets N2 | Cambio fallido conserva revision activa |
| JER-016 | Canonical IR/DDC/cook/dependencies/hot reload únicos | Assets N2/N11 | Solo invalidación afectada, lifetime GPU seguro |
| JER-017 | World hierarchy/prefabs/variants/layers/cells/HLOD/residency | World N1/N8 | Save/instances/unload/reload sin refs inválidas |
| JER-018 | Commands/Events y tipos generados para Inspector/Luau/MCP | Core/Script N1/N3 | Misma mutación por UI/API y permisos comunes |
| JER-019 | Luau strict como gameplay/UI VM | Script N3/N6 | Typecheck/cook/reload, sin Lua gameplay paralelo |
| JER-020 | Triggers shape/proximity/click/input/event/state; Once/cooldown | Gameplay N3 | Activator/filtro, reentrancia, destroy y persistencia |
| JER-021 | Store opt-in captura cambios confirmados sin doble Store.set | Persistence N3/N6 | Native change queda guardado una vez |
| JER-022 | Input semantic/context/rebind/deadzone/gamepad/XR | Input N3/N10 | Focus/disconnect limpia estado/haptics |
| JER-023 | Save provider/schema/version/corrupt/transaction/backup | Persistence N3/N6/N11 | Restore inválido no publica estado parcial |
| JER-024 | Inventory distinto de ItemDefinition/Item Studio y UI | Gameplay N6 | Pickup/drag/full container/destroy sin duplicación |
| JER-025 | Dialogue/Quest/Journal/Narrative Studio y localización | Gameplay N6 | Referencias/cook/save de un recorrido narrativo |
| JER-026 | Replay QA/actions/seeds/build IDs/divergence | RuntimeQA N6/N12 | Reproducción acotada; no prometer cross-platform bit exact |
| JER-027 | GraphCore visual común, evaluadores por dominio | Editor N3/N6/N9 | Graph/texto de HTN convergen; no segundo VM |
| JER-028 | Jolt bodies/shapes/queries/triggers/joints/character | Physics N4 | Scene productiva/authoring/cook/runtime, no cubo aislado |
| JER-029 | Physical Material, COM/inertia, motors/break limits/debug | Physics N4/N5 | Editor Play/Pause/Step y límites reales |
| JER-030 | Jolt único; PhysX transitorio clasificado | Physics N4/N11 | Un body no tiene dos simulaciones/writers |
| JER-031 | Skeleton/rig/clip/graph separados rskel/rrig/ranim | Animation N5 | Import, edit y runtime sin ABI tercero serializado |
| JER-032 | Bones/weights/FK/IK/retarget/clip correction local | Motion N5 | Pausar, corregir, guardar, reopen/cook/reproducir |
| JER-033 | GripPoints/SceneSockets/Mate/Insertion/FitProfiles | Interaction N5 | Roles claros y mismo framework para llave/batería |
| JER-034 | Un InteractionSession bimanual, Jolt manda/IK representa | Interaction N5 | Dos manos sin disputa de transform; cancel libera |
| JER-035 | Fit tolerancias/DOF/stages/symmetry/hysteresis/rollback | Interaction N5 | Handoff seated, refs perdidas y safe-drive limitado |
| JER-036 | Save/restore de sesión activa con política segura | Interaction/Save N5/N6 | Recupera coherencia o cancela explícitamente, sin punteros |
| JER-037 | rseq cámaras/tracks/curvas/eventos, control por canal | Sequencer N5/N6 | Stop/skip/seek/cancel devuelve control; sin mover player por defecto |
| JER-038 | rui2d/rui3d→rui; grid/hover/drag/preview3D/tweens | UI N6 | Datos inventory únicos y UI cooked |
| JER-039 | Screen/world-space/render-target/3D UI | UI/XR N6/N10 | Hit testing/focus/scale correcto en cada espacio |
| JER-040 | rlang/rloc/rfont, fallback/shaping/format/accessibility | UI N6/N12 | Cambio idioma/fuente y labels no solo color |
| JER-041 | Audio bajo nivel privado, SoundEvents/mixer/voices/stream | Audio N7 | Luau/contact/sequence mismo evento, unload/device loss |
| JER-042 | Audio espacial/zonas/portales/occlusion/reverb/HRTF opcional | Audio N7/N10 | Consulta presupuestada y listener compartido XR |
| JER-043 | VFX CPU/GPU authorable, flipbooks/EmberGen, rpart | VFX N7 | Cook/runtime/culling/overdraw, sin gameplay por readback |
| JER-044 | VDB tool-only opcional, no geología→VDB | VFX N7 | Fuente ausente de Shipping y licencia/input validado |
| JER-045 | WhiteBox→geometry/BSP authoring, no BSP runtime requerido | Geometry N4/N8 | Edición/UV/material/collision/Undo, boolean probado aparte |
| JER-046 | RBrush BRUSH/PAYLOAD/TARGET | Surface N4/N8 | Paint/mask/decal/damage no confunden outputs |
| JER-047 | Decals OBB/gradients/channels/sort/receiver/LOD | Render/Surface N4/N8 | Metal/dieléctrico/grazing/overlap/moving receiver |
| JER-048 | Vertex paint/material masks y remap topology | Geometry N8/N10 | Reimport conserva o invalida explícitamente |
| JER-049 | RMod library/ports/sockets/profile/mating/variants | Procedural N8 | Ports opuestos, roll/perfil/negative scale controlados |
| JER-050 | Rutas/fittings/manual intersections/flanges/snap hysteresis | Procedural N8 | T/cruz/U/empates/locked parts sin topología accidental |
| JER-051 | Soportes piso/pared/techo/obstacles por World/Jolt | Procedural N8 | No soportes flotantes ni host atravesado |
| JER-052 | Rectas UV/arc-length/RMF/cables/catenaria/edge chains | Procedural/Physics N8 | Curva degenerada, 16/32/64 links y reposo libre |
| JER-053 | PWAS proposals/locks/overrides/seeds/GUID/manual layout | Procedural N8 | Cambiar seed no destruye layout/locked identities |
| JER-054 | EPLS capas/señales compartidas, no duplicado ELSP | Environment N8 | Una autoridad de estado y aplicación por targets |
| JER-055 | Physical destruction prefracture/bonds/anchors/interiors | Physics/Geometry N8 | Fragmentos activos Jolt y debris VFX presupuestados |
| JER-056 | Un Environment/clock/sun/sky/weather/wind | Environment N2/N8 | Sin exposición/clock/campo duplicado |
| JER-057 | Wind físico opt-in separado de mesh bending, fan local | Physics/Environment N8 | Incluido/excluido, zero force al retirar emitter |
| JER-058 | Agua superior/local con reflection/transparency/contact | Water N8 | Atom runtime real; no PASS de Cry trasladado |
| JER-059 | Navegación Recast/Detour, dirty tiles/links/queries | Navigation N9 | Provider Jolt, repath/obstáculo y unload |
| JER-060 | HTN explicable puro, acciones cancelables/budgets | AI N9 | Backtracking no muta World; debug de rechazos |
| JER-061 | Roles/species/relationships/groups/territory/knowledge | AI N9 | Targets distintos del player y reservas consistentes |
| JER-062 | Aprendizaje individual/colonia y bait, guardado | AI N9 | Resultados confirmados, comunicación no omnisciente |
| JER-063 | OpenXR nativo desde contrato inicial | XR N0/N1→N10 | Vistas/input/interacción compatibles desde foundation |
| JER-064 | XR hardware lifecycle/actions/haptics/UI/pose | XR N10 | Visor real, loss/resume y Windows/Linux por separado |
| JER-065 | 2D/2.5D/pixel/isométrico/híbrido, sprites con normales | Presentation N10 | Picking/sorting/pixel/focus y physics mismo motor |
| JER-066 | VehicleGame Jolt/input/posesión | Gameplay N10 | Suspensión/carga/entrada/salida; no flotabilidad implícita |
| JER-067 | Calidad Performance/Balanced/Quality/Cinematic-Editor/VR/Pixel | Render/Settings N12 | Budget y fallback medidos por perfil |
| JER-068 | Runtime formats r*/epak versioned/bounded/ABI independent | Assets N2→N11 | Readers negativos/migrations/dependencies/source-free |
| JER-069 | Directorio final executable/epak/libraries y legal notices | Packaging N11 | Arranque aislado sin checkout/SDK/editor/tools |
| JER-070 | Sin XML propio final; tools/terceros migran con consumers | Assets N11 | Inventario y readers reales, no grep global destructivo |
| JER-071 | Sin AWS account/servicios/SDK obligatorio ni hosting obligatorio final | Legal/Build N0→N11 | Graph/link/runtime/offline/bootstrap/LFS/3P separados |
| JER-072 | SBOM/licencias/Qt/linkage/avisos por dependencia | Legal N0/N11 | Expediente efectivo, no certificado por README |
| JER-073 | Sin FBX SDK Autodesk ni ATL/MFC impuestos | Build N0/N2 | Target efectivo audita y solución compatible |
| JER-074 | Crash/hang/Doctor/RuntimeQA/fuzz/security trust | QA N0→N12 | Fallo inducido, parsers corruptos, recovery/privacy |
| JER-075 | EditorAutomationBridge/MCP local, comandos tipados | Automation N1/N3 | UI/API misma transacción, auth/capabilities/límites |
| JER-076 | Identidad Jerico y owners por subsistema, no renombre 1:1 | Core N11 | Init/use/shutdown/package sin doble autoridad |
| JER-077 | Papers SIG-01..SIG-22 ligados a owner/fixture/coste | Research continuo | KEEP/ADAPT/REJECT/DEFER antes de añadir |
| JER-078 | Error book y evidencia histórica vs nueva | QA continuo | Causa demostrada distinta de hipótesis/PASS |
| JER-079 | Buoyancy y Enable Buoyancy por WaterVolume | D: DESIGN ONLY | Requiere autorización propia; contrato preservado |
| JER-080 | Underwater y costa Horizon | D: diferido | No reabre aceptación antigua ni bloquea N0 |
| JER-081 | Geología integral y BSP→VDB | H: no activo/cancelado | Conservar math/historia, no implementar roadmap geológico |
| JER-082 | DX12/Metal/WebGPU/móvil y render RT | D: fuera de fase | No se activan por Gems/baking/auto-select |
| JER-083 | Motion matching/facial/neural/full per-finger/multiplayer sessions | D: no foundation | No bloquear interacción básica |

| JER-084 | JericoMaterial Graph e instancias sobre Atom, shader cook/source-free | Materials N2/N11 | Fuente/graph editable, .rmat cooked en runtime y variante correcta sin compilador fuente |
| JER-085 | ORM R/AO G/Roughness B/Metallic y mapas separados, color linear/sRGB/normal Y | Materials N2 | A/B valores equivalentes; cambios de canal/convención/reimport conservan overrides |
| JER-086 | Textura .rtext mips/BC4-5-7 por semántica, budget real y datos sin fuente | Assets N2/N11 | Mips impares, normal vector, normal variance separado y reload seguro |
| JER-087 | Codificadores r* con magic/version/chunks/endian/bounds/hash/validator | Formats N2→N11 | Roundtrip y negativos corrupción/overflow/migración + consumidor real |
| JER-088 | .epak indexed streaming y compresión Zstd, hashes BLAKE3 por políticas fijadas | Packaging N11 | Chunk random access y límites de descompresión; pin/ABI/provenance de dependencias |
| JER-089 | Firma autenticada de paquetes/releases y separación de cifrado/integridad | Security N11/N12 | Signature failure rejects; key policy/revocation, no bypass Shipping |
| JER-090 | Base+delta .epak, overlay precedence, atomic publication y rollback | Packaging N11 | Actualización interrumpida mantiene último contenido válido |
| JER-091 | ContentPack/DLC, CampaignRegistry, CampaignId y saves aislados | Game/Packaging N6/N11 | Dos campañas comparten assets y ejecutable, dependencia/compatibilidad probada |
| JER-092 | Hot reload/incremental cook de material/texture por revisión/dep exacta | Assets/Render N2 | Texture-only reimport no recook todo; cancel/unload/fences seguros |
| JER-093 | Source import/cook con pruebas multi-slot/skin/UV/alpha y fixture corrupta | Import N2 | Casos de producto y negativos: sin aceptar por extensión o único GLB |

Para cerrar una fila: enlazar commit, evidencia de consumidor/editor/cook/runtime aplicable, resultado negativo y límites. Si una fila agrupa formatos/variantes, desglosar cobertura antes de marcar VERIFIED; un solo caso no los acredita a todos. Las filas D/H no son permisos de implementación ni features eliminadas silenciosamente.
