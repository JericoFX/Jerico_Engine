# Auditoría de imágenes y materiales — 2026-10-14

**Estado: IMPLEMENTED / FOCAL CPU VERIFIED / NATIVE NOT RUN. N2, N0 y GOAL siguen abiertos.**

Base leída: `development@10d53925a5ed73e32f5eeae16d781d3168af9582`, que conserva la integración `388e618aab651cf02e9fe01125f5428514639751`. Se revisaron los dos masters, GOAL/AGENTS/CODEX_START, contrato JE, ROADMAP, N2/N11, ACTIVE_SLICE/HANDOFF y ENGINE_DEBUGGING_BOOK. Se trabajó con un snapshot de fuentes por API: Git no tuvo autenticación; no se ejecutó un clone/pull completo ni se crearon ramas/worktrees.

## Resultado principal

Los nombres arbitrarios ya se admiten en el recorrido examinado. Los sufijos sugieren presets; no son una condición para registrar imágenes ni para asignar BaseColor o Normal. **Instalar el proveedor OIIO/OCIO no lo conecta al importador ImageProcessingAtom.** El builder sigue usando TIFF/DDS/TGA/Qt/EXR y no depende de OIIO. KTX2/Basis y `.rtext` siguen pendientes en ese recorrido.

Se corrigieron dos defectos localizados sobre el pipeline existente:

1. Los consumidores PBR escalares leían siempre rojo. Se añadieron selectores explícitos Red/Green/Blue/Alpha: ORM usa AO=R, Roughness=G, Metallic=B; los materiales anteriores siguen usando rojo por defecto.
2. Si faltaba un preset explícito, abrir el editor lo sustituía por uno sugerido y cambiaba overrides. Ahora conserva el nombre y las opciones, evita preview/resoluciones inválidas y permite elegir manualmente un preset disponible.

Hay RED/GREEN ejecutado sobre cuerpos productivos aislados, revisión independiente y una fixture reproducible. **No hay nueva evidencia de importación nativa, metadatos reales, cook, reimportación, Save/reopen del Editor ni render.** El nuevo test nativo de persistencia está escrito pero NOT_RUN.

## Recorrido real y propietarios

| Tramo | Implementación observada | Alcance acreditado aquí |
|---|---|---|
| Proveedor | `cmake/Tools/LocalPackages/OpenImageIO/FindOpenImageIO.cmake`, `FindOpenColorIO.cmake`; catálogo Windows `BuiltInPackages_windows.cmake` | Lectura: OIIO 2.3.17.0, OCIO 2.1.1, archive SHA `268ecbe940f16f729248679b586542e0154195eb66ae7ac29bccc409e5cf6611`; exige Windows x64. Sin bindings Python en ese paquete. |
| Uso OIIO de autoría | `Gems/GradientSignal/Code/CMakeLists.txt`; `Source/Editor/EditorGradientImageCreatorUtils.cpp::WriteImage` | El target enlaza OIIO y el escritor utiliza `ImageOutput/ImageSpec/write_image`. Es un escritor de imágenes de GradientSignal, no el decoder de AP. |
| Admisión AP | `Gems/Atom/Asset/ImageProcessingAtom/Code/Source/ImageBuilderComponent.cpp`, `Code/Include/Atom/ImageProcessing/ImageProcessingDefines.h` | Registro por extensiones, source+sidecar+dependencia del preset. No filtros `_diff/_ddn/_orm` obligatorios. |
| Decoder/conversión | `Code/Source/ImageLoader/ImageLoaders.cpp`, `Processing/ImageConvert.cpp`, `BuilderSettings/PresetSettings.*` bajo ese mismo owner | TIFF/DDS/TGA/Qt/EXR. Qt depende de sus plugins reales; ninguna ejecución aquí. |
| Configuración de textura | `BuilderSettings/TextureSettings.*`, `Editor/EditorCommon.cpp`, selector y preview existentes | `.assetinfo` XML y DataPatch de plataforma; prioridad de selección explícita y reparación de preset ausente. |
| Material nativo | `Editor/Materials/Source/Document/MaterialDocument.cpp`, `Engine/Render/Materials/CMakeLists.txt`, MaterialPropertyUtil de AtomToolsFramework y MaterialTypeSourceData de RPI | `JE::EditorMaterials` / `JE::RenderMaterials` reutilizan identidad y material Atom; propiedades de imagen explícitas, export de ruta relativa y enum por nombre. Lectura, no ejecución nativa. |
| Producto de imagen | `Processing/ImageAssetProducer.cpp` | Usa GUID de la fuente y subIDs existentes; escribe `nombre.ext.streamingimage` y `nombre.ext.N.imagemipchain`. No escritor/lector `.rtext` aquí. |

ImageProcessing conserva `Gem::ImageProcessingAtom.Editor/Editor.Static` e implementación con nombres heredados; su absorción en JE está incompleta. No se creó una segunda autoridad, otro importador, cooker, registro ni dependencia AWS. Las correcciones permanecen en sus consumidores existentes y los contratos JE se conservan.

## Nombres, semántica y metadatos

`BuilderSettingManager::GetFileMask/GetSuggestedPreset` obtiene un token tras el último `_`, busca una sugerencia y, si no hay coincidencia, usa el preset de color opaco/alfa. `TexturePresetSelectionWidget` enumera la lista completa; `.assetinfo` leído por `TextureSettings::GetMultiplatformTextureSetting` y `GetImagePreset` tiene prioridad. Una textura llamada `vacaciones.png` puede asignarse a Normal; **también debe configurarse su preset Normals**, porque asignar el slot no cambia automáticamente cómo AP cocina esa fuente.

`MaterialDocument::SaveSourceData` enumera las propiedades del tipo; `ConvertToExportFormat` guarda imágenes como rutas relativas y enums como strings. `Reopen` captura/restaura overrides del documento. Son mecanismos revisados por fuente: no sustituyen la prueba de reimportación y reapertura real.

Existe una excepción heredada que se conserva: `AssetBuilderSDK.cpp::InferSubIDFromProductFileName` aplica `SUBID_FLAG_DIFF` al nombre `_diff.` de productos DDS de tipos legacy para distinguir salidas de cubemap. No exige ese sufijo para aceptar texturas ni asigna roles PBR. ImageAssetProducer usa tipos/IDs explícitos de StreamingImage/ImageMipChain. Se mantiene la compatibilidad de IDs; no se eliminó esa regla por grep.

| Requisito | Situación |
|---|---|
| BaseColor/Normal manual | Propiedades Image con nombre/ruta libre; recorrido Editor/AP completo NOT_RUN. |
| Color-space | PresetSettings tiene SourceColor/DestColor y Swizzle. TextureSettings no tiene overrides individuales equivalentes; BaseColor shader mantiene conversión LinearSRGB→ACEScg fija. OCIO arbitrario extremo a extremo no acreditado. |
| Normal Y | El material tiene `normal.flipX/flipY`; el sidecar de imagen no tiene normal-Y propio. El shader conserva una compensación histórica R/G para el cook de normales; falta verificar la dirección final con datos ±Y y el preset real. |
| ORM | Selectores implementados; cuerpos escalares y wiring probados aquí. La carga/SRG/render/cook real sigue abierta. |
| Preset lineal RGB | `ReferenceImage_Linear` existente: Linear→Linear, RGBA8 por default, sin swizzle destructivo, sin reducción; apropiado para la comparación de base level. No declara MipmapSetting: no prueba cadena completa de mips. |
| Sidecar válido/reimport | UUID de TextureSettings `{980132FF-C450-425D-8AE0-BD96A8486177}` y versión 2 intactos. Save XML y patches conservados; nueva regresión nativa NOT_RUN. |
| Sidecar corrupto | Sigue registrando error y generando defaults. Requiere resultado de error propagado a consumidores existentes y regresión nativa antes de modificarlo; no se dio por reparado. |
| KTX2/Basis/HDR | No aparecen en `s_SupportedImageExtensions`. Sin admisión AP comprobada; no se añadió una extensión sin decoder. |
| PNG/JPEG/TGA/TIFF/EXR/DDS | Registro/rutas fuente presentes. Variantes, precisión, plugins y recorrido final no se certifican por reconocer la extensión. |
| `.rtext` / `.rmat` | Contratos de destino conservados; esta intervención no crea ni acredita esos formatos. |

## Corrección de canales y compatibilidad

Se añadieron `roughness.textureMapChannel`, `metallic.textureMapChannel`, `occlusion.diffuseTextureMapChannel` y `occlusion.specularTextureMapChannel`. Enum estable: Red, Green, Blue, Alpha; default Red. Los helpers conservan los argumentos existentes y agregan un último `uint textureChannel=0`; valores fuera de rango vuelven a rojo. Se mantienen factores, límites de rugosidad, AO strength y overrides de debug.

Se propagó el canal en 17 llamadas activas de BasePBR, StandardPBR, EnhancedPBR, Skin, Eye y StandardMultilayerPBR; incluye iris/sclera y cuatro transferencias por capa. Inspector Lua y UseTexture respetan presencia/activación de mapas. Los seis tipos incrementan su versión con una actualización vacía: no renombra propiedades ni cambia valores; las acciones anteriores permanecen intactas. AP debe recocinar las dependencias del tipo/shader, usando el graph existente.

La reparación de preset ausente conserva todos los settings cargados. Las consultas de resolución/mips devuelven vacío cuando falta el preset; las vistas correspondientes lo toleran y no inician conversión. El combo muestra la ausencia sin elegir automáticamente el primer preset; info/reset inválidos están protegidos. Una elección manual válida reutiliza el evento existente de refresh. Queda una limitación menor de presentación: si un preset desaparece con el diálogo abierto, algunos labels de tamaño pueden mostrar el valor anterior; no modifica datos.

## Evidencia ejecutada y límites

Directorio: [image-material-audit-2026-10-14](../evidence/n2/image-material-audit-2026-10-14/).

| Prueba | RED | GREEN | Qué prueba |
|---|---|---|---|
| `verify_scalar_channels.py` | C++ build 0/run 1 en ambos modos; 26 fallos por modo, ORM devolvía R para Roughness/Metallic | Build/run 0, cero fallos en pixel y `CS_SAMPLERS` | Incluye directamente los cuerpos `.azsli` productivos con tipos/sampler de test. Selección RGBA, fallback rojo, Sample/SampleGrad/SampleLevel, factores/overrides y equivalencia ORM/separados. No es compilación AZSL, filtrado GPU ni shader ejecutado. |
| `verify_preset_control_flow.py` | Build 0/run 1; cuatro pérdidas de preset/reducción/mips | Build/run 0; cero fallos | Definición exacta de InitFromPath con seams de decoding/config/settings; dos nombres y dos plataformas. No ejecuta decoder/serializer reales. |
| `verify_material_wiring.py` | Primer diagnóstico del harness incluyó llamadas AO comentadas de Eye; corregido el parser, sin cambio productivo | Exit 0 | Defaults/propiedades anteriores, versiones, 17 llamadas y cuatro copias; ejecuta los dos functors originales en Lua 5.4 con contexto de test. No upload SRG/Qt. |
| Fixture de 11 inputs | Generada en directorio nuevo | PNGs decodificados independientemente, RGB ORM igual a mapas separados; KTX2 límites/header/payload comprobados | Bytes reproducibles y assignments `.material`; no importación. KTX2 sin validator Khronos ni decoder. |
| Test nativo `EditorTextureSetting_UnavailableExplicitPreset_PreservesSettingsUntilManualSelection` | NOT_RUN | NOT_RUN | Preparado en la suite original: copia temporal, XML/patches, reapertura, mips explícitos, recuperación Normals y fuentes intactas. |

Los 18 casos OIIO anteriores son antecedente publicado, no se repitieron: nueve por configuración en `cmake/Tools/LocalPackages/OpenImageIO/tests`, TIFF8/PNG16/EXR32/PNG RGBA/JPEG, OCIO matrix, inválidos y tools. No cubren KTX2, este ImageBuilder, bindings de materiales ni reimportación. Los binarios del proveedor son Windows; este entorno Linux tiene g++/Lua pero carece de CMake/Qt y Editor/AP/proveedor nativos. Windows-DX12, Windows-Vulkan y Linux-Vulkan: **NOT_RUN para este cambio**; no aceptación visual/performance/VR.

Antecedente AP conservado: `docs/evidence/n0/native-pass-workspace-and-sdk-2026-10-13/overnight-ap-ui-layout/ap/failure-classification.json` registra exit 1, 347 assets fallidos; `parent-lfs-headers-reviewed.json` verifica esos 347 headers/punteros sin adquirir payloads. No se reemplazaron inputs LFS ni se ejecutaron descargas para ocultar ese bloqueo.

## Recorrido nativo diseñado, pendiente

1. Compilar los targets de imagen/material/shader existentes con entradas congeladas. Ejecutar la suite ImageProcessingAtom original y la regresión nueva; compilar los seis tipos con sus shaders en DX12 y Vulkan.
2. Generar fuentes sintéticas en un directorio NUEVO bajo los Assets del proyecto fixture:

   ```text
   python3 docs/evidence/n2/image-material-audit-2026-10-14/generate_fixture.py --repo <checkout> --output <directorio-nuevo-de-assets>
   ```

   No ejecutar el generador encima de assets existentes. No crea `.assetinfo` ni UUID ficticios; los escribe/asigna el pipeline real.

3. En Texture Settings elegir Albedo para `vacaciones.png`, Normals para `tornillo.png` y `cuaderno.png`, ReferenceImage_Linear para `caja.png`/`papel.png`/`boton.png`/`taza.png`. Guardar el `.assetinfo` genuino y comprobar preset/color/formato por plataforma. La comparación base level excluye mips/compresión; ejecutar otro gate con mip chain real y tolerancias de codec justificadas.
4. Abrir JePacked y JeSeparate en el workspace JE existente; comprobar BaseColor/Normal manual y los tres mapas/canales. Registrar SourceUUID, source hash, Product GUID/subIDs, deps, presets, SRG y overrides, sin renombrar fuentes. Guardar/cerrar/reabrir y comparar valores serializados.
5. Cook mediante AP existente, inspeccionar descriptors/formato/sRGB/mips/productos y comparar muestras/control de iluminación con AO=51/255, Roughness=153/255, Metallic=230/255. Registrar capturas reales y backend. El archivo `archivo.ktx2` aporta el caso negativo de admisión actual; no confundir ausencia de job con importación exitosa. Antes de admitirlo hacen falta decoder/transcoder real, variantes/negativos y el mismo owner AP.
6. Reimportar cambiando únicamente una copia fuente propia; conservar su UUID, actualizar sólo los productos dependientes y comprobar que bindings, canales, normal flip y factores persisten. Error/cancel mantiene la revisión anterior; reabrir material/escena y repetir por cada backend disponible.
7. Probar `tornillo.png` frente a `cuaderno.png` y normal.flipY con el cook Normals real: dirección/tangentes/handedness final, no sólo estado del checkbox. La entrada KTX2 es RGBA8 sin supercompression y un nivel; Basis y otras variantes necesitan gates separados.

Los scripts son herramientas externas de prueba; no se incorpora Python al Editor/Runtime. Cambios y hashes de 25 paths en [source-changes.json](../evidence/n2/image-material-audit-2026-10-14/source-changes.json); RED/GREEN, patches y estado en el mismo directorio. Fuentes originales, UUIDs, acciones anteriores y evidencia N0 preservados. La revisión independiente no encontró defectos source críticos/importantes, pero no sustituye los tests nativos.
