# RenderAssetBuilders + Shader: evidencia nativa del 16/10/2026

Evidencia delimitada de la integración nativa. Los JSON/scripts de preparación e intentos fallidos conservan su estado histórico; el cierre vigente se resume aquí y en el manifest.

## Gates delimitados

- CPU82: ocho XML nativos, 811 casos sin fallos,JUnit y seis fases cerradas; diez hosts compilados. Baseline17 original y compile RED81 permanecen disponibles. Atom_RHI.Tests e ImageProcessingAtom.Editor.Tests quedaron fuera de este filtro y son gates aparte; los ocho XML tienen disabled=0 y no representan un recuento de exclusiones RHI/LFS.
- SDK83: install/configure/build,PE e imports y loader de consola cerrados; imports/ABI/factories/descriptores/servicios/destrucción/recreación únicamente. Tres mensajes esperados, stderr vacío, 11.218 archivos de inventario y rehash padre de 14.034 inputs. No activa módulos ni acredita cook/render/Shipping.
- AP84: seis jobs frescos y13 productos; producto precompilado con dependencia al root correcto y AssetLoadBehavior::PreLoad representado por0. Los dos jobs de variantes sin productos no acreditaban su rama positiva. El copy builder exitoso puede no generar log por la política lazy verificada; se conserva esa distinción.
- AP85: cuatro jobs Status4/errors0; siete productos semánticos obligatorios y15 productos físicos con identidades/bytes/SHA. Options contiene specializationConstants booleanfalse real. Un warning de C.UTF-8 en el shader. APBatch raw exit 1 / 11,613 s,NO PASS global. Los 347 fallos globales conservan todos los campos salvo JobRunKey, LastFailLogTime, LastFailLogFile, LastLogTime y LastLogFile, modificados por el reintento.
- El padre rehasheó 14.037 inputs de 85, 40 de autoría y 26 texturas del propietario; tres capturas de módulos sin legacy. Retiró sólo las tres fuentes temporales verificadas y restauró la cohorte fuente82 de 14.034 inputs.

## Reproducción y procedencia

Los scripts y recetas originales se seleccionan byteexactos bajo pbr/: atomshader-native80-baseline-preparation-rev2 (baseline), atomshader-native80-parent-composition-rev1 (RED81), atomshader-native82-private-include-preparation-rev1 (CPU82), atomshader-native83-sdk-preparation-rev1 (SDK e intentos preservados), y atomshader-native84/85-ap-preparation-rev1 (AP). ParentReviewAndArchive85.ps1 conserva la revisión real del padre. Las cuatro fuentes privadas del consumidor están incluidas; no se publica su ejecutable. Los scripts retienen rutas/contratos originales y no se ejecutan al archivar; cualquier replay requiere inputs verificados, recursos y dueño nativo asignado.

Las consultas readonly: 8 before / 8 after se conservan sin copiar DB/WAL/SHM. Productos compilados se representan mediante manifests de identidades/hashes,NO binarios; sólo se selecciona el options.json textual pertinente de cada intento. El final-native-census.json original de85 tiene 0 bytes: se mantiene privado y se referencia como salida inválida, no como JSON/censo válido. parent-final-native-census85.json es el censo válido del cierre. Las salidas vacías históricas se conservan o referencian explícitamente sin convertirlas en evidencia válida.

## Límites abiertos

No hubo replay limpio de HEAD22cc+las294 rutas seleccionadas; las pruebas corresponden al árbol dirty preservado y congelado. Gem completas 11 / incremento 0. VISUAL y DEPENDENCY globales abiertos; UI/GPU, VR, Linux y Shipping no aceptados por este paquete. La autorización GUI está revocada y no se ejecutaron ventanas en esta preparación. pending=[] del manifest sólo indica que CPU82 / SDK83 / AP84 / AP85 ya tienen cierre delimitado; no equivale al cierre del GOAL. No DLL/lib/PDB/dump/DB/ZIP/LFS ni escenas/texturas del propietario se copian mediante esta selección.
