# Auditoría de ramas — decisión de una sola línea de trabajo

Corte: 2026-10-10. Repositorio: JericoFX/Jerico_Engine. Se enumeraron 23 ramas y se compararon las 22 alternativas con development mediante la API GitHub. Auditoría de historia/deltas, no compilación ni garantía de equivalencia semántica de patches.

## Decisión

**development es la única rama de trabajo Jerico.** Todas las directivas, masters, planes, evidencia nueva y commits de producto viven allí. No crear otra rama llamada Jerico/main/research ni consolidar mediante merge de las 23.

Base inspeccionada: `241c78b8064898a709cf1c5dd3535c6406763ef3`; árbol `db4b7d4a5d784d2283c95903f461e311d69704c8`. El objeto coincide con o3de/o3de. El commit corresponde a una corrección del editor del 2026-10-09. Es una base de desarrollo elegida, no una versión estable certificada.

Al comienzo la rama predeterminada del repositorio era `optimize_scriptcanvas_1`. La API describe esta copia privada como `fork:false`: eso es metadata de GitHub, no prueba de falta de origen; el commit y árbol sí se verificaron contra O3DE. Hay que distinguir copia con historial de relación formal de fork.

## Todas las ramas

`Propios/Faltantes` significa commits alcanzables solo desde esa rama / solo desde development. No mide calidad ni cantidad de funcionalidades. Cherry-picks pueden representar patches equivalentes con SHA distintos.

| Rama | HEAD de auditoría | Propios/Faltantes | Clasificación y acción |
|---|---|---:|---|
| development | 241c78b8064898a709cf1c5dd3535c6406763ef3 | 0/0 | Única base activa; versión interna 2.8.0 |
| main | a5e1628d90a44cfdc0d831818f81b0e8912cb9b3 | 436/357 | Historia divergente, versión interna 2.6.0; referencia, no fusionar completa |
| stabilization/26100 | b706d4f5b315df69b4dafa8f67ac2ea101571378 | 14/105 | Versión interna 2.7.0; revisar fixes puntuales contra development |
| optimize_scriptcanvas_1 | e32ee24b06ed4ccb2c903eb695b1d93f34640351 | 1/453 | Cuatro archivos de styling/PseudoElement de GraphCanvas; candidato focal, no base |
| material_refactoring | ee928b849510ccc0ffbc9d6c6eb4aa19c11ee895 | 0/779 | Ancestro de development; no hay commits nuevos que fusionar |
| multi-device-resources | 5bd6550bd97088ef148963486ba4a3beb59c4cc4 | 70/1408 | Refactor RHI amplio y antiguo; no importar, auditar solo necesidad concreta |
| standalone_meshinfo | b6aca7ff57f53ab549e891f03b10e3fb89a357f5 | 6/528 | MeshInfo/FallbackPBR y cambios RT; estudio condicionado al render actual |
| webgpu-rhi | 40953f6d0170deb89a7fbeff09b0282cafe31211 | 6/1124 | Backend experimental; fuera del objetivo Vulkan actual |
| shine | aa43d61f5a8585c03eadc35beae2683ff8daffcd | 4/160 | Candidato UI: Shine y conversor de slices a prefabs; no asumir completo/estable |
| i18nsupport | 2a45d6c20ff72d437a8bac7771865a0ec4277fdf | 1/167 | Traducciones/editor/property framework; referencia para localización, no `.rloc` resuelto |
| scripting/light_visibility | 625fb78c111280b8c6e6df7a94544f496decf610 | 6/1514 | Light visibility y binding; contrastar API actual, no traer scripting antiguo |
| mp/add_d6_dev | d3e18adf48549bfc493d166df0b49481c78e3964 | 1/313 | D6 joints, componentes editor y tests PhysX; ideas de authoring para Jolt, no segundo solver |
| gadams3/cherry_pick_smc_generate_svl_fixes | 670705c7cfcc5a71f2162133254fdcb0b1108814 | 762/2745 | Historia muy divergente; no seleccionar por nombre. Buscar fix equivalente en shader tools actuales |
| copilot/fix-ios-asset-build-job | 056ae06fd468873d7e22380dc34fbb4c4421748b | 1/137 | Comparación devolvió cero archivos; sin motivo para importar; iOS fuera de alcance |
| xcb-splashScreen | fec967f8dccd6e5fefe9fb242b94607e474c859f | 6/766 | Splash/Xcb/packaging Linux; solo si una reproducción actual lo necesita |
| update-3p-DirectXShaderCompilerDxc-1.8.2505.1-cmake-file | 94535b01ad3acd3ecff5aac5ea2a7159ccd1793b | 1/142 | Delta de paquete Mac ARM; no mejora Vulkan Windows por su nombre |
| update-3p-Imath-3.2.2-cmake-file | 891cf8476b7bb606c937340dee301430e21a8704 | 1/121 | Paquetes Android/iOS; fuera de prioridad |
| update-3p-OpenSSL-3.6.3-cmake-file | 638fe80502d23b83fb8364fc275a30be1e47aff9 | 1/139 | Actualización multiplaforma; prioridad de auditoría de seguridad/ABI si la dependencia es usada |
| update-3p-pyside6-6.11.2-cmake-file | da61e022c41cc771678aa09b995d8a41f0252f53 | 1/81 | No fusionar automáticamente: development ya declara PySide6 6.11.2 en Windows |
| update-3p-qt-6.10.2-cmake-file | fd364cffe6bc2027e1d8f6e9ddf40c6ea63a890c | 1/105 | Delta Mac ARM; no seleccionar Qt por rama |
| update-3p-qt-6.11.2-cmake-file | 47deb6f6a8ba1205c0e2e8cf18129ebc84ed4a92 | 1/89 | development ya declara Qt 6.11.2 en Windows; comprobar patch equivalence |
| update-3p-qt-6.12.0-cmake-file | 73f096249c33dd26b7a14729b34b5370da2da954 | 1/6 | Nueva versión propuesta; no romper pareja Qt/PySide/toolchain sin baseline |
| update-3p-sdformat-16.0.1-cmake-file | f4ff56168e411e52ee80812305f38e0cc46417f3 | 2/121 | Paquetes Linux de formato de simulación; no confundir sdformat con SDF geométrico |

## Selección de rescate

No se incorporó ninguno de estos patches en la publicación documental. Primero probar development. Después, el owner del vertical compara el código actual con un patch candidato y registra: problema reproducible, dependencias, si ya existe solución equivalente, licencia, tests, impacto y decisión KEEP/ADAPT/REJECT/DEFER.

`shine` sí merece lectura: `Gems/LYShineToShine/README.md` documenta conversión de `.slice` a `.uiprefab` y actualización de `.uicanvas`, con mutación in-place y necesidad de verificar contenido. Eso no prueba ausencia total de XML, compatibilidad con Luau, rendimiento ni que debamos adoptar su formato final. No ejecutar el conversor sobre fuentes únicas.

`i18nsupport` puede aportar mecanismos del editor; las traducciones Qt no sustituyen el contrato de localización de juego. `mp/add_d6_dev` puede aportar UX/test cases de constraints, pero los cuerpos Jerico serán de Jolt. `standalone_meshinfo` no se admite por prometer GPU Scene: requiere revisar qué ya tiene el Atom actual y excluir dependencias RT.

## Limits de esta comparación

GitHub limita la lista de archivos en comparaciones grandes; no tratar las primeras 300 entradas como diff exhaustivo. Los conteos de commits son metadata de historia. No se hizo `git patch-id` ni auditoría completa de cada delta. Las ramas con cero commits propios son alcanzables desde development; eso no prueba que ninguna funcionalidad haya sido cambiada después.

Para un candidato real, Codex debe usar historia/diff local completo en el mismo checkout, sin cambiar la rama de trabajo:

```sh
git log --left-right --cherry-mark development...origin/shine
git diff development...origin/shine -- Gems/Shine Gems/LYShineToShine
```

Estas son lecturas. No ejecutar merge/cherry-pick hasta haber clasificado el patch y sus consumidores. Nunca aplicar esta vía a commits de CryEngine2.

## Default branch y preservación

El objetivo administrativo es `development` como default de GitHub. La publicación documental por API no dispone de una acción para editar esa propiedad. `CODEX_START.md` incluye el ajuste con GitHub CLI y verificación; no se declara realizado por escribirlo.

No borrar las ramas antiguas ni reescribir sus refs para hacerlas parecer development. Mantenerlas inactivas no viola trabajar en una sola rama. Una eventual limpieza requiere permiso concreto y plan de preservación de commits únicos.

Fuentes de auditoría: API `/repos/JericoFX/Jerico_Engine/branches?per_page=100`; compare de cada ref respecto al SHA base; `engine.json` de development/main/stabilization; `cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake`; `Gems/LYShineToShine/README.md` de shine. Fuente oficial de identidad: https://github.com/o3de/o3de/commit/241c78b8064898a709cf1c5dd3535c6406763ef3 .
