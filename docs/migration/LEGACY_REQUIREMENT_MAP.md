# Mapa de rescate y decisiones sustituidas — JE

Vigente 2026-10-10. Fuente histórica principal: `JericoFX/JericoEngine2@304d9d2e13704e4967e82a8d5a13a5a506be01d4`; planificación anterior de este repo: `a536fb78c6195c3edf10afa34bfdf65c6f841ee5`. No son fuentes autorizadas para copiar código en bloque ni autoridad superior a GOAL actual.

## Cambios vinculantes

| Regla anterior | Regla vigente |
|---|---|
| Cry5.7 como base / O3DE reference-only | Source O3DE admitido como base JE, sin importar código Cry |
| Diligent obligatorio / MNM / port SVOGI | Atom adaptado a JE, Recast auditado, GI propia permitida sin source Cry |
| Vulkan-only / DX12 diferido | DX12+Vulkan únicos; Windows ambos, Linux Vulkan |
| AZ/Gem internos conservables indefinidamente | Código del engine adoptado migra realmente a JE; bridges temporales privados |
| Gems ocultos al usuario como destino suficiente | Código/registro/builders/assets de capacidades requeridas integrados en owners del engine, sin Gem manager productivo |
| Multiplayer optional/deferred | EXCLUDED, igual servidores/replicación/matchmaking/cuentas online |
| Geología integral no activa | REACTIVADA: forma/stack/mesher/material/cook visibles |
| BSP→OpenVDB descartado | Sigue descartado: la reactivación de geología no reinstala ese backend |
| BSP después de todas las foundations/gameplay | UI/BSP N1, materiales N2, colisión N4 y geología N8 adelantado |
| Mejoras gráficas al final | Medición y SIGGRAPH útil desde BSP/materiales/geología |
| Papers/otras engines como prohibición genérica | Ideas permitidas; source/assets propietarios no se copian sin licencia |
| Hosting AWS tolerado como paso implícito | Cache/fuentes controladas y no fallback AWS; missing package falla sin descargar |

## Contratos que se conservan

| Origen histórico | Qué se rescata | Autoridad JE |
|---|---|---|
| GOAL/AGENTS/DoD/debug book | Principios, un integrador, evidence no ficticia, verticales útiles | GOAL/AGENTS/DoD actuales |
| RUNTIME_FORMATS/transfer map | Catálogo r*, chunks/version/bounds, source-free | Asset/material masters y N2/N11 |
| Asset Import/Cooking/incremental delta/hot reload de Jerico1 | glTF/ufbx/texturas/ORM/IR/recipes/deps/GUID/reimport | JE Assets/Cook |
| JERICOEDITOR_MASTER/VISUAL/M10.5 | Viewport compartido, context panels/docking/UI/asset/IDE/transacciones | Editor master/N1 |
| PROCEDURAL_PWAS_EPLS/Geometry/Playground | Brushes/CSG/RMF/edge/catenaria/RMod y stack de geología | Procedural master/N1/N8 |
| Geology experiments | Surface Nets/Hermite/QEF/QR/SVD, native planes, dirty region, material fields | JE Geology con nueva validación |
| RBRUSH_IDTECH6 | BRUSH/PAYLOAD/TARGET, OBB/gradients/channels/sort/stamps | Surface y Render masters |
| Rendering/SIGGRAPH atlas/idTech6 audit | Papers completos por familias, comparativas y disciplina de budget | Render master SIG-01..22 |
| AUDIO_VFX_ANIMATION_UI_MASTER | Skeleton/rig/clip, IK, grip, bimanual, Fit, UI/timeline, audio/XR | Motion/UI/audio master |
| SCRIPTING_GRAPHCORE_GAMEPLAY/WORLD_AI | Luau/types/self/references/Store/triggers/input/save/HTN/relaciones | Gameplay/AI master |
| 2D_2_5D_PIXEL_ISOMETRIC_VERTEX_VEHICLE | Perfiles del mismo engine, normales/sprites/vertex/Jolt | Profiles master |
| BUOYANCY_AND_FLUIDS_DESIGN | Enable Buoyancy/WaterVolume, provider sin renderer, fuerzas Jolt | DESIGN ONLY conservado |
| CRY source/CE audit/Reset-to-Cry | Historia y comportamientos observables, no source autorizado | Legal/history, ninguna orden de reactivación |

Documentos extensos históricos se consultaron por partes de contrato/objetivo, no se afirma auditoría línea a línea de todas sus implementaciones. Antes de reutilizar código/test/shader/asset: registrar fuente exacta/autoría/licencia/derivación/fixture y owner. Código original de Jerico y matemáticas independientes son candidatos valiosos; código adaptado de Cry no se vuelve legal por tener prefijo Jerico.

## Guardas específicas

Mantener forma Rock/RockFamily/Cliff/Overhang/Cave separada de asignación material; mismos campos de estratos/erosión/daño en geometría/shading; navegación/colisión y espacio libre manual. No reactivar VDB global ni prometer roca por noise genérico.

Formatos y materiales no se pierden al integrar Atom/Asset Processor: rmat/rtext/rmesh, ORM/separados, conventions/mips/tangentes, rskel/rrig/ranim, rseq/rui/rloc y epak/delta/version/rollback. ContentPacks/campañas existentes se limitan a datos offline; no online commerce/entitlements ni multiplayer encubierto.

No atribuir a O3DE las medidas o PASS de Cry, ni hipótesis viejas de agua/atmósfera como causa de un fallo nuevo. Conservación de fuente/historia no exige portar los fallos, wrappers ni contadores triviales. No borrar repos históricos ni archivos no versionados. No copiar PDFs/arte de terceros sin permiso.
