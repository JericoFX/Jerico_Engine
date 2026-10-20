# AtomToolsFramework — integración nativa del framework editorial

Owner: `Editor/Tools`, target `JE::EditorTools` STATIC, namespace `JE::Editor::Tools`. Conserva documentos, previews, ventanas e interacción con materiales y nodos; no añade una DLL ni incorpora autoría al runtime. El [plan](../../../work/gems/AtomToolsFramework.md) conserva contratos, análisis de upstream y límites.

Estado: comprobaciones focales revisadas por el padre; publicación selectiva pendiente. No declara cierre global de Gem, N0 ni GOAL. Las pruebas se ejecutaron en una ventana canonical controlada dentro del árbol mixto, no en un checkout limpio del futuro commit.

## Evidencia

- `atomtools104-canonical-test-r1-attempt1`: XML de EditorTools, 18 casos. El XML de materiales de esa fase es histórico y anterior a los fixes posteriores; no presentarlo como reejecutado.
- `atomtools104-canonical-test-materials-ownsave-r5-attempt1`: ocho regresiones de cambios/guardado de materiales.
- `atomtools104-canonical-test-materials-zoom-r10-attempt1`: tres regresiones de primer encuadre y conservación de cámara manual.
- `atomtools104-canonical-editor-r5-attempt1`: reapertura del material guardado, graph legible, Inspector, zoom e interacción hide/show; capturas originales y revisión visual. RGB80/120/160 y roughness0.25 preservados en copia autorizada; originales conservados. No se probó resize. Menú principal vacío, viewport BSP negro y errores Vulkan Preview mip8 impiden afirmar GUI/render global limpios.
- `atomtools104-family-materialeditor-startup-r1-attempt1`: arranque real de la familia AtomToolsApplication, preview Vulkan y cierre; avisos conservados.
- `atomtools104-family-ap-startup-r1-attempt1` y `atomtools104-family-ap-startup-r2-attempt1`: RED de cuatro aserciones al reflejar presets antes de sus tipos; GREEN después de cargar la factory editorial tras registrar los tipos de render. R2 procesó 14 assets, cero fallidos, seis warnings en el resumen. No se suprimieron aserciones. El arranque del Builder real complementa Editor y MaterialEditor como cuarta familia.
- `atomtools104-configure-sdk-consumer-r1-attempt1`: imports del SDK configurados; aviso OpenColorIO/OpenImageIO conservado.
- `atomtools104-sdk-build-consumer-r1-attempt1`: RED LNK2019 porque el SDK conservaba EditorRenderScene.lib con la referencia antigua. La fuente y el archive canonical ya usaban JE. Se actualizaron cuatro bibliotecas omitidas de consumidores (EditorRenderScene, EditorMaterials, AssetProcessor.Static y EditorLib), sin modificar fuentes ni repetir configure.
- `atomtools104-sdk-build-consumer-r2-attempt1`: enlace GREEN en5.44s. EXE SHA256 `A8F232AC3D462399A259A426DAF05EDF4B85C7799FE4CCE4FC9E86E0A23EAE1D`.
- `atomtools104-sdk-pe-imports-r1-attempt1`: 32 imports directos, sin DLL AtomToolsFramework ni EditorTools. Esta tabla no es una auditoría de todas las cargas transitivas.
- `atomtools104-sdk-loader-r1-attempt1`: ejecución GREEN en0.119s, dos ciclos EditorTools con cuatro descriptores/requeridos/servicios y reflexión; seis proveedores resueltos al RenderFeatures.dll desplegado, byteidéntico al SDK. Prueba CPU de imports/exports y construcción/liberación, no activación de ModuleManager, UI, GPU ni runtime-only link.

Los postflights de las comprobaciones finales conservan cero deriva y censos nativos vacíos. Los tiempos de herramientas no son mediciones de rendimiento del engine. [provenance.json](provenance.json) registra fuentes, tamaños y SHA-256 de las copias byteexactas.

## Límites y continuidad

Los arranques reales de Editor, AP/Builder y MaterialEditor complementan las pruebas del consumidor; no se usa la factory aislada como sustituto de esos arranques. Linux, VR/OpenXR, Shipping, SDK global sin Python y métricas globales permanecen abiertos. No hay incremento certificado del contador global de Gems.

La publicación debe incorporar los siete fixes finales sobre el traslado canonical; el snapshot inicial T2 no los contiene. Los cambios previos protegidos y sus backups no forman parte automáticamente de este corte.
