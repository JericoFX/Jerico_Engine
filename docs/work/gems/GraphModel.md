# GraphModel — plan de absorción nativa

**Spec:** GOAL.md, ROADMAP N2 y contrato JE. **Stack:** C++ y Qt existentes, Vulkan activo. Ejecución: dos escritores con scopes disjuntos, padre integrador y Luna como único dueño de validación asignada.

Estado: integración focal Windows/Profile/Vulkan implementada y verificada sobre la cohorte canónica congelada; revisión final del padre completada y publicación selectiva confirmada en development be3ff3974f91d34f8d582ef187dfb3294bfbce9f. Configure/build, GraphModel13/13, EditorTools18/18 y Materials99/99; GUI Editor R6 conexión/Undo/Redo/Save y R7 recarga en sesión nueva, consumidor SDK CPU739/0 y arranque representativo MaterialEditor105 aceptados focalmente. GraphModel se integra en JE::EditorTools STATIC, quinto descriptor/servicio, sin DLL productiva propia y conservando GraphCanvas. Boundary17 restaurado y WIP preservado; la coherencia binaria de ese working no está verificada ni se adopta. No Gem global/all-platform completa ni incremento del contador. Propiedades | Nodos/asociación material-grafo sigue como requisito global del GOAL, distinto de esta absorción.

## Alcance y destino implementado

Origen: `Gems/GraphModel`. Cuerpos y API absorbidos en el owner existente `Editor/Tools`, `JE::EditorTools` STATIC, namespace `JE::Editor::Tools::GraphModel` y subnamespace `Integration`; no se creó otro editor, VM ni DLL de capacidad. Contratos de tipos/servicios/factory revisados antes de aplicar; la composición canónica y el consumidor instalado se verificaron con cinco componentes.

GraphModel es host-tools. Targets y activación legacy `GraphModel.Editor.Static`, `GraphModel.Editor` y `GraphModel.Editor.Tests` retirados del corte; la implementación vive en JE::EditorTools y la suite en `JE::EditorGraphModel.Tests`, con environment separado. GraphCanvasWidgets/GraphCanvas.Editor permanecen como dependencias funcionales; su uso real se verificó en Editor R6/R7, no se infiere del arranque MaterialEditor sin GraphCanvas cargado.

## Inventario de configuración revisado

- `Gems/GraphModel/Code/CMakeLists.txt` y filelists: 54 entradas static, una de módulo y seis de tests; inventario físico 28 CPP, 32 H y una INL.
- Consumidores/configuración: `engine.json`, `AutomatedTesting/project.json`, manifest y CMake de LandscapeCanvas, `cmake/Subdirectories.cmake`, `Editor/Tools/CMakeLists.txt` y dependencias de tests de EditorMaterials.
- Baseline T1: ocho registries globales mencionaban GraphModel.Editor.dll. Configure105 retiró esas activaciones conservando GraphCanvas; no se añade GameLauncher ni se certifican cargas de todos los runtimes.
- No se encontraron raíces Assets/Registry, QRC/UI ni portablekey propios que trasladar.
- SDK afectado: 29 headers y una INL, biblioteca estática, DLL Editor, cuatro metadatos CMake y entrada de subdirectorio. Reconciliar sólo el delta real y sus consumidores cuando las fuentes estén estables.

## Contratos revisados antes de escribir

- Inventario de tipos/UUID, campos y versiones serializadas, componentes y GraphModelService revisado en T1; identidades persistidas preservadas en la migración.
- Consumidores C++ y tests migrados; LandscapeCanvas permanece inactivo, con referencias preservadas sin activación global.
- Cambios previos GraphCompiler/Pass/materiales separados mediante Boundary17 y restaurados después de validar. No incorporarlos automáticamente ni declarar coherencia binaria del árbol WIP.
- Composición única revisada; factory/servicios CPU y documento real con GraphCanvas verificados por gates distintos. Una factory aislada no sustituye GUI/activación.

Registro localizado: GraphModelModule `{217B9E5D-C0FC-4D9D-AD75-AA3B23566A96}` registra/requiere GraphModelSystemComponent `{58CE2D43-2DDC-4CEB-BB9F-61B77C50C35D}`, que proporciona e incompatibiliza `GraphModelService`. Activate/Deactivate crean/destruyen el manager. Required/Dependent vacíos no eliminan la necesidad funcional de GraphCanvas en CreateScene.

Persistencia localizada: Graph v2 conserva nodos, conexiones, metadata visual y wrappers; Node v0 conserva slots; Slot v1 conserva valor/subId; Connection v0 conserva endpoints. GraphDocument usa JSON y reconstruye contexto/controladores. El inventario definitivo debe incluir también UUID y nombres externos de metadata, módulos y MIME events, no sólo estos cuatro tipos.

La corrección existente del ciclo Node→Slot→Node mediante weak_ptr y sus dos regresiones se preservan; consultar ENGINE_DEBUGGING_BOOK, líneas 302–306, antes de modificar esa vida útil. Mantener el modelo común sin dependencias hacia GraphDocument, Materials o Workspace.

### Consumidores encontrados en el análisis C++

W1 encontró 155 archivos externos con referencias directas delimitadas a `GraphModel::`, `GraphModelIntegration::` o includes `GraphModel/`: 17 en Editor/Tools, nueve en Editor/Materials, dos en PassCanvas y 127 en LandscapeCanvas. Los 63 archivos con include directo no cubren todos los consumidores. `AnimGraphModel::` de EMotionFX no es una referencia a este namespace.

Los consumidores JE incluyen GraphDocument, GraphCompiler, GraphUtil, DynamicNode y sus managers/paletas/configuración; MaterialGraphCompiler, MaterialGraphEnvironment, PassGraphCompiler y sus pruebas; y PassCanvasApplication. LandscapeCanvas permanece inactivo y fuera de absorción, pero sus referencias deben migrar junto con la API común para no dejar código roto.

Graft respondió `Transport closed` durante esta revisión. Se utilizó búsqueda acotada bajo Code/Engine/Editor/Gems sin iniciar otro servidor ni cargar otro índice. El inventario textual no prueba activación ni compilación de los consumidores.

## Referencias upstream revisadas

- [O3DE PR 20109](https://github.com/o3de/o3de/pull/20109): referencia para determinismo, escrituras idénticas, cancelación y espera del Asset Processor. No importar: la revisión local de GraphCompiler y su trabajo previo es independiente de retirar este módulo Gem.
- [O3DE PR 20092](https://github.com/o3de/o3de/pull/20092): reroute/passthrough, serialización y Undo/Redo; no adoptar dentro de esta absorción. Su alcance no demuestra la integración requerida del material JE.

La revisión de Luna es informativa, no aceptación ni autorización de copiar código. Preservar atribuciones y revisar la licencia concreta de cualquier cambio que se proponga reutilizar.

## Verificación prevista

1. Baseline focal actual y contratos serializados antes del traslado. El XML 13/0 del 16 de octubre es histórico y no se presenta como baseline vigente.
2. Configure/enlaces afectados, servicio registrado una vez y grafo real: abrir, editar, Undo/Redo, guardar y reabrir preservando conexiones y constantes. Reutilizar evidencia sólo para entradas sin cambios.
3. Arranques de familias consumidoras y delta SDK/consumidor instalado; comprobar retiro de GraphModel.Editor.dll conservando GraphCanvas. Publicación selectiva y reporte individual sólo después de revisión del padre.

### Verificación focal vigente — integración publicada

- Tests canónicos: GraphModel13/13, EditorTools18/18 y Materials99/99. Dos ciclos de factory con cinco componentes; los casos release/extender/ownership y tipos conservan sus oráculos. No certifica el WIP restaurado.
- EditorLib0/14.0899401s y siete consumidores0/113.6376576s; Editor GUI R6 conexión/Undo/Redo/Save0/608.096897s, R7 recarga nueva sesión sin Save0/78.3938536s. Copia guardada18203B/C987D5 idéntica tras recarga.
- SDK R7:13reemplazos sin altas/retiros, sobre R2yaaplicado69/40. Consumidor incremental0/8.0305097s, audit130destinos/132slots sin mismatch, PE0/0.0852435s sin imports prohibidos y loader0/0.1600509s,739checks/0fallos/2ciclos/5descriptores. Aceptación focal CPU D5CB6E0F; no ModuleManager/Activate/UI/GPU ni fuente físicamente oculta.
- MaterialEditor1050/61.2883343s: ventana Vulkan, Shaderball/Neutral Urban, cierre AltF4, sin edit/Save ni modal. Legacy DLLs no cargadas en el censo; GraphCanvas tampoco se cargó aquí: su uso real corresponde a EditorR6/R7. Aceptación padre724909AC2451BAC7CE8B6467E8687D2C54691FC8BD663A4062AA7C4306EF4603 en `build/n0-validation/native-corefeature102/graphmodel105-family-materialeditor-startup-r1-attempt1/parent-acceptance.json`; captura1282x1006 `parent-startup.jpg`, A261A6B128CB04B24B8104EEE15A73BBC4ECFB0C4CB87CC9ADD233148B2DADE8. Result86902E4C/stdoutF03CAB92; postflight0drift/censo vacío. Cuatro warnings coinciden con104 (rechazo AP inicial→conexión1.652584s, API prefab deprecada, dos avisos de contexto JSON), sin reparación atribuida; errors/asserts0.
- Boundary17 restaurado17filas+11protegidos; recibo868B00AB, snapshotjournalBAC5 y cicloRestored5C3EE9. Test WIPC7F/155629B y CMake2F5DD/2243B preservados; workingBinaryCoherenceVerified=false. No repetir canónico ni reconstruir/adoptar WIP por rutina.

Evidencia privada: outputs `graphmodel105-ownership-test-r2-attempt1`, `graphmodel105-editor-release-r6-attempt1`, `graphmodel105-editor-release-r7-attempt1` y `graphmodel105-sdk-post-release-*-r1-attempt1` bajo build/n0-validation/native-corefeature102; restauración en native-graphmodel105/analysis-source/boundary-restoration-preparation-r1/application-r1. La integración focal está publicada en development be3ff3974f91d34f8d582ef187dfb3294bfbce9f, SHA remoto confirmado; ninguna casilla cambia el ledger nominal11 ni cierra el GOAL.

Windows/Vulkan es el perfil activo. Linux/VR/Shipping y los defectos gráficos conocidos conservan sus estados; no atribuir el error Preview mip8 a GraphModel sin diagnóstico causal. No repetir un SDK completo por esta planificación.

## Unidad de implementación y ownership

- [x] W1 — IMPLEMENTADO/VERIFICADO: mapa UUID/serialización/preimágenes, traslado de modelos/integración a Editor/Tools y migración de155consumidores/tests; claves externas y WIP preservados.
- [x] W1 — IMPLEMENTADO/VERIFICADO: factory JE::EditorTools ampliada4→5 con GraphModelSystemComponent, sin factory/módulo nuevo; UUID/servicios y expectativas de tests/SDK preservados.
- [x] VERIFICADO: suite GraphModel13 en target separado JE::EditorGraphModel.Tests con los seis archivos/environment existentes; hooks no fusionados y sin DLL productiva nueva.
- [x] W2 — IMPLEMENTADO/VERIFICADO: filelists/CMake/manifests/discovery, dependencias y delta SDK focal; GraphModel legacy retirado, GraphCanvas conservado y LandscapeCanvas inactivo.
- [x] Padre — análisis/baseline, gates focales y revisión final completados; publicación selectiva be3ff3974f91d34f8d582ef187dfb3294bfbce9f con SHA remoto confirmado. No adopción automática de GraphCompiler/PassCanvas/materiales WIP ni cierre global/all-platform.
- [x] Luna — VERIFICADO: fases asignadas con freeze/admisión y terminales revisados por el padre; sin fuente/config/Git ni decisión de aceptación. GUI e integración permanecen bajo review del padre.

## Review focus

1. Cambios de namespace no alteran UUID, nombres externos ni claves serializadas de módulos/MIME/metadata.
2. El componente GraphModel se registra una vez y GraphCanvas está disponible antes de CreateScene.
3. Ningún modelo común depende de GraphDocument/Materials/Workspace por conveniencia del traslado.
4. LandscapeCanvas y otros consumidores inactivos conservan referencias válidas sin activarse en el perfil.
5. La prueba de destrucción del ciclo Node/Slot y la carga de documentos previos siguen pasando; tests/factory aislados no sustituyen el documento real.

## Histórico de preparación de baseline (cerrado)

Receta privada preparada en `build/n0-validation/native-graphmodel105/analysis-wiring/baseline-preparation-r1/recipe.candidate.json`; no autorizada para ejecutar. Antes de aprobarla, revisar ProjectReference y ZERO_CHECK/generate.stamp.depend: un target focal puede regenerar configuración global y compilar cambios previos fuera de sus 69 entradas directas. No cambiar flags ni considerar coherente la DLL histórica sin procedencia demostrada. El siguiente paso es resolver esa frontera y fijar las entradas realmente afectadas.

Revisión W2/Luna: los 69 pins y cuatro outputs existentes coinciden, pero la receta conserva HEAD anterior85729 y debe usar8d0606. El cierre generado contiene22proyectos, sin EditorTools/Materials/EditorLib, pero incluye ZERO_CHECK: dos CMake de Materials restaurados a17:36 son posteriores al stamp16:44 y dispararían regeneración. No se ejecutó esa receta. Se prepara alternativa de build directo de Static y Tests sin ProjectReferences, sólo si proyectos/filelists/flags actuales y proveedores enlazados quedan verificados y congelados; no alterar timestamps ni desactivar CMake globalmente. Las dos pruebas con nombre PythonBindings verifican API en BehaviorContext; no ejecutan ni acreditan Python en producto.

## Baseline focal original — 2026-10-20

Ejecución única de AzTestRunner: exit 0, 0.1101917 s; XML fresco con 13 casos, 0 fallos/errores/skips, dos suites (11 integración y 2 ApiExists). SHA256 XML D3379347D435DB2F8979D353886FF4CE0F7F643F2DCBED06B08A896BF1299946. Evidencia: build/n0-validation/native-corefeature102/graphmodel105-test-baseline-r1-attempt1/. Padre inspeccionó resultado, postflight y XML; freeze verificado sin deriva, proceso terminado.

Los builds focales previos Static/Tests terminaron exit 0 pero ClCompile/Lib/Link estaban actualizados; no recompilaron ni relinkaron. El baseline prueba el binario previo congelado BA11A424EEBE56D7A4CA09D359186B51026560D5CD19F9C78E0BF12F722FB034 y sus proveedores, no una recompilación completa ni el candidato nativo. No repetir este baseline sin cambios. Próximo paso: cerrar revisión de candidatos, aplicar preservando WIP y verificar la integración nativa.

### Revisión posterior a aplicación de fuentes

Padre verificó las 60 entradas de las dos filelists candidatas contra las rutas ya trasladadas: ninguna ausente. Los seis includes privados GraphModel/ resuelven bajo Source o Tests, sin depender del antiguo include público Gem. ModuleTests.cpp ya está registrado en editor_tools_tests_files.cmake; factory y test actuales/candidatos esperan cinco componentes. Se descartó el hallazgo inicial de revisión sobre falta de registro: confundía las suites; no añadir el test factory al environment de GraphModel.

Delta SDK pendiente identificado por W2: fixture native-scene100/sdk-preparation/source/main.cpp mantiene cuatro UUIDs/identidades y texto de diagnóstico cuatro; incorporar el quinto GraphModelService preservando otros contratos de cuatro de RPI/Scene. Las cantidades de descriptores/Required se derivan del array. CMake del consumidor ya importa JE::EditorTools; ampliar sólo guard de retiros GraphModel y exclusión del target de tests. No ejecutar ni editar SDK mientras configuración productiva siga bloqueada.

Fixture SDK105 adaptada por padre (sin ejecutar): main.cpp SHA655FC8C5872748A2BEC42007141CA02EEA189B6AF3B3AB1A088B3F523682656D; CMakeLists.txt SHA3C6F8B055D7A6F147F12B6ABDFAB2D791F7FE0794046F8BB556264DBE5A9EA25. Respaldos exactos/receipt en native-graphmodel105/sdk-fixture-preparation-r1. Quinto componente/servicio, ausencia DLL y guards de SDK añadidos; no modifica otros contratos de cuatro. Reintento productivo W2 con autorización humana explícita también rechazado por policy antes de proceso: ningún cambio de las17ops. Espera resolución de herramienta, no repetir autorización ni afirmar integración compilada.

Preparación de separación WIP105 completada sin overlay: boundary.candidate.json SHA C0FED35D88257F443FD06FF9231924DCCBECC8068B4645C5018A48059F2E996F, 17 rutas (16 reemplazos temporales y 1 ausencia), 67 pins verificados por padre. Guard corregido reconoce el LF real del CMake y conserva comparación byteexacta del delta; no normaliza archivos. El journal104 permanece intacto. Config105 aún pendiente, por lo que este plan NO es un freeze ejecutable ni aceptación. No se aplicó a producto. Ruta: build/n0-validation/native-graphmodel105/analysis-source/canonical-validation-preparation-r1/.
