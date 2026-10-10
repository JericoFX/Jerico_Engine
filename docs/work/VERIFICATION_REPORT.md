# Verificación del paquete de planificación

Fecha 2026-10-10. Tipo: revisión remota GitHub de fuentes y documentación. **No compilación ni ejecución del motor.**

## Evidencia observada

| Área | Resultado | Límite |
|---|---|---|
| Repositorio nuevo | Acceso privado de lectura/escritura confirmado; default inicial optimize_scriptcanvas_1 | No cambio administrativo de default por este conector |
| Ramas | 23 refs enumeradas; todas las 22 alternativas comparadas con development | Diffs grandes GitHub pueden truncar archivos; sin patch-id completo |
| Base O3DE | Commit 241c78b8064898a709cf1c5dd3535c6406763ef3 y tree db4b7d4a5d784d2283c95903f461e311d69704c8 coinciden con upstream oficial | No prueba objetos LFS/3P descargados |
| Versiones internas | development 2.8.0, stabilization/26100 2.7.0, main 2.6.0 en engine.json | No etiquetas de estabilidad ni pruebas de release |
| Qt/toolchain | Qt/PySide6 6.11.2 Windows declarados; CMake min3.25 y recomendación4.2.3 leídos | Artefactos efectivos/build no probados; otras plataformas requieren auditoría |
| AWS/LFS | Catálogo AWS e IoT; .lfsconfig CloudFront observados | Catálogo no acredita linkage; independencia aún no cerrada |
| Legal | LICENSE raíz Apache2/MIT y obligación separada de terceros leída | No SBOM completo ni aprobación jurídica |
| Gem candidatos | Manifests WhiteBox/RecastNavigation/MiniAudio/OPS y engine.json leídos; README conversor shine leído | No auditoría de todas las transitivas/source/funciones |
| Fuente histórica | JericoEngine2 fijado a 304d9d2e13704e4967e82a8d5a13a5a506be01d4 | Repo anterior no modificado ni historia importada |
| Requisitos | GOAL/AGENTS y secciones de masters/transfer/scripts/render/physics/modular/formatos consultadas; decisiones anteriores contrastadas | Lectura focal y consolidación, no auditoría línea por línea de todo el motor |
| SIGGRAPH | Atlas/idTech6 previos y fuentes públicas/indexes pertinentes | No se releyeron todos los PDFs ni se implementó ninguno |
| Publicación | Debe comprobarse por commit diff/ref remota y lectura de documentos | SHA final obtenido del commit, no hash autorreferencial |

## Revisión documental realizada al redactar

Precedence nueva vs instrucciones Cry; una rama development; fuentes históricas separadas; requisitos vigentes y D/H explícitos; owners N0-N12; XR desde foundation; no reactivar flotabilidad/geología/DX12/RT; `.rpart` reservado; cookbook de referencias/persistencia/formatos; Git seguro, licencia/notice preservados y tareas proporcionadas al resultado.

Los links internos del índice apuntan a documentos incluidos en el paquete. Los comandos de CODEX_START/N0 son instrucciones para la máquina de Codex, no ejecuciones realizadas en esta auditoría. No se ejecutó `git diff --check` local ni validators de Markdown sobre un checkout completo; no atribuirles PASS. La revisión final de paths/árbol/commit se realiza con el conector GitHub.

## No ejecutado

CMake configure/build; tests C++/Python del engine; Editor/launcher/Asset Processor; Vulkan GPU/driver; Linux; OpenXR headset; Jolt/Luau integration; shader compilation/cook; performance/VRAM/RAM actuales; asset LFS download; SBOM exhaustivo y revisión de distribución. Todo permanece NOT TESTED/NOT VERIFIED según master.

## Cómo validar esta publicación y continuar

Leer el commit frente a su padre O3DE: solo Markdown nuevo y README de navegación/documentación; no cambios en Code, Gems, Engine assets, CMake, engine.json, `.lfsconfig` o LICENSEs. Confirmar ref development y mismo SHA; si el remoto avanzó, reconciliar sin fuerza y revisar nuevo diff.

Codex debe sustituir las filas pendientes de N0 con evidencia real en `docs/work/evidence/N0/`, conservando los límites. Un paquete documental completo no equivale a un motor completo.

## Enmienda de alcance documental

Añadido master específico de materiales, formatos r* y empaquetado incremental, más JER-084..JER-093. Fuente histórica de políticas `.epak`, DLC y protección revisada como diseño de JericoEngine1; algoritmos/licencias de zstd/BLAKE3/libsodium aún requieren pins y pruebas antes de usar. No se fabricaron resultados de validación de formatos, shader cook ni firmas reales.
