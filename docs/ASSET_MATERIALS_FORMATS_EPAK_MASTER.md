# JE — materiales, formatos propios y EPAK

**Revisión vinculante 2026-10-10. Estado: CONTRACT ADOPTED / IMPLEMENTATION NOT VERIFIED.** Owner Assets/Material/Render N2 y Packaging N11; otros formatos entran con su consumidor. GOAL/contrato JE superiores. Solo DX12/Vulkan, sin AWS/multiplayer/plataformas online.

## 1. Propiedad y ruta única

O3DE aporta implementación, no contrato final de disco/API del juego. Assets/Material/Render y código adaptado convergen a JE:: con CMake/codegen/UUID/reflection/serialization coherentes. No otro asset registry/cooker/MaterialVM/VFS/RenderGraph. Reutilizar Asset Processor/SceneProcessing/Material Editor/Canvas/Shader Builder/AssetBundler y migrar consumidores; library privada no expone ABI de tercero en datos del juego.

MaterialRegistry/AssetRegistry son nombres de responsabilidades, no orden de inventar otra tabla independiente. Reconciliar identidad source/product/GUID/revision con implementación existente. Third-party source/API/license reales conservados en límites privados; namespace JE no borra obligaciones ni autoriza source Cry.

## 2. Admisión de fuentes

| Familia | Inputs de autoría | Restricción |
|---|---|---|
| Scene/mesh | glTF2/GLB, FBX por ufbx, OBJ/MTL | Parsers Editor/Tools, nunca Shipping |
| Texturas | PNG/JPEG/TGA/TIFF/HDR/EXR/DDS/KTX2/Basis | Decode/semantic metadata/conversión offline |
| Alturas | PNG16/RAW16/32/EXR/TIFF/GeoTIFF donde admitido | Unidad/ejes/precision explícitos |
| Audio | WAV/FLAC/OGG | Transcode/metadata a producto propio |
| Rig/clip | Skin/animation/morph importados y autoría nativa | rskel/rrig/ranim separados |
| Procedural/UI/localización | rmod.toml/rbrush/graphs/recipes/rui2d/rui3d/rlang | Sources de autoría, cook a productos canónicos |

No Autodesk FBX SDK/ATL-MFC impuestos. Soporte por variante: submesh/slots/jerarquía/instances/transforms/multiple UVs/skin/morph/clips/extensiones; reconocer extensión no acredita todos. Unknown required extension rechaza; opcional política explícita. Distinguir contenedor/codec/semántica y licencia de datos.

Normalizar y limitar paths/counts/sizes, rechazar escape/links/case-fold collisions, snapshots externos inmutables con hash y safe relative path. Datos enormes no se reservan sin presupuesto. UI permite progreso/cancel sin descartar revision anterior. Renderer no importa source on demand.

## 3. Pipeline de publicación

Fuente+metadata→snapshot(source bytes/external deps/importer version/settings)→Canonical IR→typed GUID/deps/recipe hashes→builders en el único Asset Processor→candidate `.rmesh/.rmat/.rtext` y demás cierre→validación→selección activa al final→runtime/paquete.

Preservar raw sources/overrides. Build/version recipe y perfil backend/plataforma son parte de cache identity. Invalidation sólo por inputs reales; texture-only reimport no recook geometría/colisión innecesariamente. Active reference publica candidato entero; un fallo no deja un prefab/material a medias. Recursos viejos se retiran tras lifetimes/fences seguros de GPU/audio/physics.

Cache local, fuentes/artefactos permitidos pinneados; falta de dependencia falla sin descarga AWS/CloudFront/S3/marketplace. No otras plataformas sustitutas. Import/cook/Shipping pueden ejecutarse offline con la toolchain y deps admitidas. No confundir inicial adquisición de biblioteca con servicio runtime.

## 4. JericoMaterials y PBR semántico

BaseColor/Albedo, Normal, Roughness, Metallic, AO, Emission, Opacity/Cutout; material físico separado. Instancias/graph/functions/layers y familias clearcoat/anisotropy/transmission/rough glass/skin/height/parallax por implementación admitida, no sólo campos visibles.

Nombres libres; `_diff/_ddn/_spec/_orm` sólo sugerencias, nunca autoridad. Metadata editable fija source texture, channel/swizzle, color-space, normal convention/strength, UV index/transform/scalar defaults. Save/Undo/Redo/prefab/reimport conserva elecciones.

### ORM y mapas separados

- ORM: R=AO, G=perceptual roughness, B=metallic, canales lineales.
- Separados: AO/Roughness/Metallic con selección de canal/factor/default independientes.
- Mismos valores/UV/factores producen shading equivalente dentro de tolerancia justificada por precision/compression. No obligar al autor a reempaquetar imágenes.
- Color BaseColor/emission sRGB según metadata; normals/masks/roughness/metal/AO/height lineales. No derivar color-space de extensión sola. Alpha sin conversion sRGB arbitraria.
- Normals exponen orientación Y OpenGL/DirectX, fuerza y compatibilidad tangent basis. Cooker convierte sin modificar archivos fuente.
- Alpha mode Opaque/Mask/Blend, cutoff/two-sided/winding/orden/transparencia con fallback explícitos. Decode correcto no acredita transparencia/luz correctas.

### Tangentes, mips y compresión

Normales por inverse-transpose bajo transform, tangentes ortogonalizadas y handedness/winding correctos en escala negativa/no uniforme. Conservar tangentes authored válidas o generarlas por implementación permitida Mikk; normal-mapped UV degenerada se diagnostica/rechaza cuando no existe basis correcto. Fallback tangent de un mesh sin normal map no acredita equivalencia.

Mip chains completas con dimensiones impares y edge coverage. Data channels se filtran linealmente; normal signed vector promedio+renormalización. Alpha coverage, specular AA y normal variance son decisiones independientes, no resueltas sólo por normalizar. BC1/3/4/5/7 u otro target soportado según semántica/preset/capability; no dar ASTC u otra plataforma por integrada al mencionar formato. Medir archivo, RAM temporal y VRAM real por separado.

### Graph/cook e integración gráfica

Material Editor/Canvas y Shader Builder existentes adaptados a JE; fuente editable→IR/material products/variants→`.rmat`. SRGs/reflection/PSO/cache son del mismo render owner. DXIL para DX12 y SPIR-V para Vulkan en productos de perfil correspondientes: un GUID material común no autoriza intercambiar bytecode.

Limitar permutations, errores missing pins/types/channels/variants visibles, candidate rejected preserva material anterior. No source compiler/remote shader compiler en Shipping. Functors Lua internos de herramientas/materiales sólo bajo auditoría/encapsulado transitorio; no segunda VM pública de gameplay junto a Luau. Su migración no debe romper material pipeline por cambiar bibliotecas a ciegas.

## 5. BSP/geología, capas y RBrush

Material/UV por cara y region/material lineage deben sobrevivir CSG y generated interiors. Source WhiteBox core contiene geometría, no prueba todos esos atributos. Incorporar tests del reconstruidor completo antes de declarar material preservation.

Geología usa mismos campos que generan forma para strata/material domain/joints/erosión/sedimento/slope/curvature/cavities/FreshInterior. Triplanar/world-space opcional sin UV manual, overrides/layers/masks editables. No un noise visual independiente que contradiga la masa/corte. Hard-surface conserva UV exacta donde corresponde.

RBrush separa BRUSH coverage, PAYLOAD y TARGET. Vertex paint/material masks/decal instances/sculpt/damage comparten UI no output. Stroke identity/seed/locks/receiver/order/undo/reimport preservados. Decals proyección OBB/explicit gradients/channel blending/sort/receiver/LOD/attachment admitido; probar metal/dielectric/grazing/moving/overlap/array overflow. No copiar shaders propietarios idTech.

## 6. Catálogo runtime canónico

| Formato | Owner/dato |
|---|---|
| .rproject | Boot/proyecto resuelto |
| .rlevel / .rpref | World/cells/scene y composición prefab/variantes |
| .rmesh | Streams/LOD/meshlets admitidos/skin bindings |
| .rtext | Texturas/dimensiones/formato/color-space/mips/streaming; NO .rtex |
| .rmat | Material parameters/semantic refs/cooked variant identity |
| .rcoll / .rnav | Colisión Jolt y navegación |
| .rskel / .rrig / .ranim | Hierarchy/rest pose, rig/IK/FK/constraints y clips |
| .rgraph / .rseq | Producto de graph de dominio y secuencia reusable |
| .rscript / .rinput | Luau cooked/checked y acciones/perfiles |
| .rui / .rloc / .rfont | UI/bindings/accessibility, textos y font metrics/subsets/fallbacks/atlas refs |
| .raudio / .rsound / .rsoundsurface | Samples/streams, event policy y contactos de material físico |
| .rpart | Partículas/VFX, NO pieza modular |
| .rsave | Providers/schema/snapshot seguro |
| .epak | Indexed content package y delta/overlay |

Autoría `.rui2d/.rui3d/.rlang/.rbrush/.rmod.toml` y recipes/graphs. RMod normalmente produce mesh/coll/prefab, no runtime format adicional. `.rgraph` no convierte todos los evaluadores en VM universal; extensión nueva exige owner/lifecycle/ADR. N2 incorpora formatos con consumidor real; catálogo no congela el wire experimental de Cry ni se satisface renombrando archivos O3DE.

### Requisitos de readers/writers

Magic/type ID, major/minor schema, required/optional flags, deterministic endian/exact length. Chunk directory acotada, order/alignment/offset/length/non-overlap/overflow, required desconocido rechaza y optional policy explícita. GUID/deps/content hashes, profile/cooker/schema identity pertinente. Bounded allocations/decompression, enums/finite values/transforms. Sin ABI C++/VM closures/pointers/structs de terceros/paths absolutos.

Validator/dump y validación compartidos editor/runtime, roundtrip/determinismo/endianness/version migration. Corrupt/truncated/huge counts/chunk overlap/cyclic or missing deps/inconsistent GUID/hash/unsupported required versions deben fallar antes de publicar. Hash no autentica origen. Cada writer/reader necesita consumidor real y fixture negativa, no sólo código compilado.

## 7. EPAK, actualización y contenido local

Adaptar AssetBundler/VFS al único contrato JE; `.epak` no es `.pak` renombrado. Header/index identifica package/content ID, project/build/schema/profile y directorio/deps/hashes. Chunks independientes/random access con limites de ratio/output/descompresión y verificación antes de entregar bytes.

Direcciones históricas de diseño preservadas para auditar: Zstandard por chunks, BLAKE3 content digests con algorithm ID (SHA256 puede seguir para provenance), firma autenticada de release/updates, Ed25519/libsodium como candidatos y política explícita de cifrado configurable/Shipping. **No implementados por este documento, no elegir dependencia sin licencia/pin/pruebas y no crypto propia.** Separar integridad/autenticidad/confidencialidad, trust roots/key rotation/revocation, claves privadas fuera de cliente. Cifrar con clave local no impide extracción; no claim de seguridad sin amenaza/validación.

Development unsigned sólo bajo política no-Shipping visible; releases con firma requieren verificación fail-closed cuando esa capacidad se implemente. No servicio remoto para verificar contenido local ni cuenta/entitlement online.

Base+delta .epak: diff por exact hashes/deps, nuevos/cambiados y tombstones admitidos. Precedence/profile/project/version compatibles, validar cierre completo antes de activation. Stage→validate(space/integrity/signature/deps)→atomic manifest switch; interrupción/corrupto/disk full mantiene última versión. Compaction/rebase sin alterar GUID de autoría ni perder DLC/local packs. No .epatch por comodidad ni descargar datos nuevos desde una plataforma no autorizada.

ContentPack/campaña son paquetes/experiencias LOCALES de datos, shared assets por GUID, CampaignId/save scope/compatibility; no nueva DLL arbitraria, launcher online, marketplace/commerce/entitlement remoto ni multiplayer. Se desarrollan cuando los consumidores existentes lo requieren y no bloquean BSP/geología.

Distribución final ejecutable/.epak/libs/recursos y notices necesarios en layout Jerico; sin checkout/importers/compiler fuente/AWS/legacy source formats. Cold launch con source inaccesible y red desconectada prueba el contrato, no ejecutarlo desde Editor.

## 8. Fixtures obligatorias

MATERIAL-01: GLB dos materiales+caras BSP, mapas de nombres arbitrarios/normal/ORM, editar/save/reopen/cook/runtime. MATERIAL-02: ORM vs tres mapas equivalentes con mismos factores/UV, color-space/normal-Y y escala negativa. MATERIAL-03: stroke/layer/vertex/decal/UV después de CSG, Undo/reimport/Cancel y overrides conservados. MATERIAL-04: cliff/cueva estratos/FreshInterior y shading derivado del mismo campo.

FORMAT-01: writer/reader/validator por formato real y corruptos/overflow/versions/deps/hash antes de consumer. EPAK-01: cierre source-free reproducible. EPAK-02: base+delta/modificación/removal/interrupción/rollback/firma inválida cuando implementada y local packs compartidos. PERF-01: import/cook/UI stalls/RAM pico/resident VRAM/lifetime/frametime en escenas representativas.

Tres pares de render probados por separado: Windows-DX12, Windows-Vulkan, Linux-Vulkan. Un fixture de codec no acredita material visual. Ninguno de estos tests está declarado ejecutado aquí.

Fuentes históricas: `JericoEngine2@304d9d2e13704e4967e82a8d5a13a5a506be01d4` GOAL/RUNTIME_FORMATS/transfer map y Jerico1 Asset Import/Runtime Chunks/Incremental Cook/Hot Reload. Código reusable necesita provenance individual. Source actual: Asset Processor/Bundler/SceneProcessing/Atom Tools/RPI/Shader; paths AZ/Gem sólo para encontrar la implementación que debe migrarse.
