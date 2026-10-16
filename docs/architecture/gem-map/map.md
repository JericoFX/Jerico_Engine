# Mapa de composición de Gems y owners nativos

Captura privada del checkout actual. Flechas de manifest indican dependencia declarada; no representan llamadas C++. `[configured]` significa presencia en un plan de módulos generado, no ejecución observada. Los grupos son una clasificación visual, no nuevos owners del motor.

Cobertura: **110 manifests actuales de Gems fuente**, **14 fixtures/plantillas separados**, **4066 referencias de composición y llamadas delimitadas**, **2186 archivos CMake fuente leídos**, **22 planes de módulos**. Entradas cambiadas durante captura: **0**.

El contador histórico de 11 Gems aceptadas se conserva como dato existente; sus once identidades se reconstruyen en [historical-nominal11.json](historical-nominal11.json), sin convertir los slices publicados en cierre de todos los gates. Las absorciones identificadas llevan su evidencia y límites propios. Bootstrap es candidato con gates posteriores todavía abiertos; no aumenta ese contador.

## Vista por familias

```mermaid
flowchart LR
  f0["asset-pipeline: 1 Gems"]
  f1["atom-core: 1 Gems"]
  f2["atom-rhi: 5 Gems"]
  f3["atom-rpi: 1 Gems"]
  f4["atom-tools: 5 Gems"]
  f5["audio-media: 4 Gems"]
  f6["content: 4 Gems"]
  f7["editor-script: 13 Gems"]
  f8["engine-gameplay: 32 Gems"]
  f9["online-platform: 7 Gems"]
  f10["physics-animation: 7 Gems"]
  f11["render-features: 16 Gems"]
  f12["testing: 3 Gems"]
  f13["ui: 5 Gems"]
  f14["world: 6 Gems"]
  f0 -->|"1 manifest"| f2
  f0 -->|"1 manifest"| f3
  f1 -->|"1 manifest"| f0
  f1 -->|"2 manifest"| f2
  f1 -->|"1 manifest"| f3
  f1 -->|"2 manifest"| f4
  f1 -->|"7 manifest"| f11
  f3 -->|"1 manifest"| f2
  f4 -->|"4 manifest"| f0
  f4 -->|"5 manifest"| f2
  f4 -->|"5 manifest"| f3
  f4 -->|"2 manifest"| f7
  f4 -->|"8 manifest"| f11
  f5 -->|"1 manifest"| f8
  f8 -->|"1 manifest"| f1
  f8 -->|"1 manifest"| f2
  f8 -->|"4 manifest"| f3
  f8 -->|"1 manifest"| f7
  f8 -->|"6 manifest"| f13
  f9 -->|"1 manifest"| f7
  f9 -->|"1 manifest"| f11
  f10 -->|"1 manifest"| f3
  f10 -->|"2 manifest"| f8
  f10 -->|"2 manifest"| f11
  f11 -->|"1 manifest"| f0
  f11 -->|"1 manifest"| f1
  f11 -->|"8 manifest"| f2
  f11 -->|"12 manifest"| f3
  f11 -->|"4 manifest"| f4
  f11 -->|"1 manifest"| f7
  f11 -->|"2 manifest"| f8
  f11 -->|"1 manifest"| f10
  f11 -->|"2 manifest"| f14
  f13 -->|"1 manifest"| f0
  f13 -->|"1 manifest"| f1
  f13 -->|"2 manifest"| f3
  f13 -->|"1 manifest"| f4
  f13 -->|"2 manifest"| f8
  f13 -->|"1 manifest"| f11
  f14 -->|"1 manifest"| f0
  f14 -->|"1 manifest"| f1
  f14 -->|"2 manifest"| f3
  f14 -->|"2 manifest"| f7
  f14 -->|"6 manifest"| f8
  f14 -->|"2 manifest"| f11
  u0["Sponza: multiple local manifests; PENDIENTE"]
  f6 -->|"manifest PENDIENTE"| u0
  u1["PhysX: local provider not resolved; PENDIENTE"]
  f9 -->|"manifest PENDIENTE"| u1
```

## Dónde y cuándo

| Arista | Fase | Qué demuestra |
|---|---|---|
| manifest-dependency / selection-root | configure/selección | dependencia declarada y alcance por nombres locales |
| manifest-external-subdirectory | configure/discovery | catálogo disponible, no activación |
| cmake-* | configure/link declarado | referencia literal y comando con archivo/línea; condiciones no evaluadas |
| configured-module-composition | plan de carga generado | host, target y DLL planificados; no carga realmente observada |
| cpp-factory-call | registro estático en runtime/Editor/asset processing | sitio real y argumento; false de Bootstrap registra sin Required |
| runtime-ebus-contract | inicialización de viewport | contrato trazado; receptor efectivo y timing siguen delimitados |
| native-absorption-evidence | owner/integración | owner actual y alcance histórico documentado |

Cada arista está en [edges.json](edges.json) con `file`, `line`, `phase`, `kind`, `resolution` y detalle. [cmake-reference-contexts.json](cmake-reference-contexts.json) conserva el comando completo para no confundir link con llamadas. [configured-module-composition.json](configured-module-composition.json) enumera todos los targets/DLL de los planes capturados.

## Registro estático por host

Sitios C++ verificados con la cobertura Graft reutilizada y SHA actual. Registro de módulo no equivale por sí solo a activar todos sus componentes.

```mermaid
flowchart LR
  h0["Launcher"]
  c0["JE::Geometry::CreateRuntimeModule"]
  h0 -->|"register module"| c0
  c1["JE::World::Camera::CreateRuntimeModule"]
  h0 -->|"register module"| c1
  c2["JE::Debug::CreateRuntimeModule"]
  h0 -->|"register module"| c2
  c3["JE::Debug::Profiler::CreateCaptureModule"]
  h0 -->|"register module"| c3
  c4["JE::Debug::Profiler::CreateVisualModule"]
  h0 -->|"register module"| c4
  c5["JE::Render::Debug::CreateViewportModule"]
  h0 -->|"register module"| c5
  c6["JE::Render::Debug::CreateToolsModule"]
  h0 -->|"register module"| c6
  c7["JE::World::Camera::Debug::CreateDebugModule"]
  h0 -->|"register module"| c7
  c8["JE::Render::Integration::CreateBootstrapModule"]
  h0 -->|"true: Required bootstrap"| c8
  c9["JE::Render::Integration::CreateRuntimeModule"]
  h0 -->|"register module"| c9
  h1["EditorToolsApplication"]
  c10["JE::Geometry::CreateAuthoringModule"]
  h1 -->|"register module"| c10
  c11["JE::Editor::Camera::CreateAuthoringModule"]
  h1 -->|"register module"| c11
  h1 -->|"register module"| c7
  h1 -->|"true: Required bootstrap"| c8
  c12["JE::Editor::RenderIntegration::CreateAuthoringModule"]
  h1 -->|"register module"| c12
  c13["JE::Assets::Prefab::CreateToolsModule"]
  h1 -->|"register module"| c13
  c14["JE::Assets::RenderBuilders::CreateToolsModule"]
  h1 -->|"register module"| c14
  c15["JE::Assets::RenderBuilders::CreateShaderCompilerToolsModule"]
  h1 -->|"register module"| c15
  c16["JE::Debug::CreateToolsModule"]
  h1 -->|"register module"| c16
  h1 -->|"register module"| c4
  h1 -->|"register module"| c5
  h1 -->|"register module"| c6
  c17["JE::Editor::CreateMaterialsModule"]
  h1 -->|"register module"| c17
  h2["ApplicationManager"]
  h2 -->|"register module"| c10
  h2 -->|"register module"| c11
  h2 -->|"register module"| c7
  h2 -->|"false: descriptor only"| c8
  c18["JE::Editor::RenderIntegration::CreateAssetProcessingModule"]
  h2 -->|"register module"| c18
  h2 -->|"register module"| c13
  h2 -->|"register module"| c14
  h2 -->|"register module"| c15
  h2 -->|"register module"| c16
  h2 -->|"register module"| c3
  h3["AssetBuilderApplication"]
  h3 -->|"register module"| c10
  h3 -->|"register module"| c11
  h3 -->|"register module"| c7
  h3 -->|"false: descriptor only"| c8
  h3 -->|"register module"| c18
  h3 -->|"register module"| c13
  h3 -->|"register module"| c14
  h3 -->|"register module"| c15
  h3 -->|"register module"| c16
  h3 -->|"register module"| c3
  h4["AtomToolsApplication"]
  h4 -->|"register module"| c4
  h4 -->|"register module"| c5
  h4 -->|"register module"| c6
  h4 -->|"register module"| c7
  h4 -->|"true: Required bootstrap"| c8
  h4 -->|"register module"| c12
  h4 -->|"register module"| c14
  h4 -->|"register module"| c15
```

| Host / archivo | Línea Bootstrap | Modo | Fase |
|---|---|---|---|
| `Code/LauncherUnified/Launcher.cpp` | 85 | true por default: Required bootstrap | runtime/non-headless-static-module-registration |
| `Code/Editor/EditorToolsApplication.cpp` | 127 | true por default: Required bootstrap | Editor/tools-static-module-registration |
| `Code/Tools/AssetProcessor/native/utilities/ApplicationManager.cpp` | 234 | false: descriptor sin Required | asset-processing/static-module-registration |
| `Code/Tools/AssetProcessor/AssetBuilder/AssetBuilderApplication.cpp` | 134 | false: descriptor sin Required | asset-processing/static-module-registration |
| `Gems/Atom/Tools/AtomToolsFramework/Code/Source/Application/AtomToolsApplication.cpp` | 174 | true por default: Required bootstrap | Editor/tools-static-module-registration |

## Catálogo completo

| Gem | Familia | Configurado | Alcance desde manifests | Estado nativo | Fuente |
|---|---|---|---|---|---|
| Achievements | online-platform | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/Achievements/gem.json` |
| AFR | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/AFR/gem.json` |
| Archive | engine-gameplay | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/Archive/gem.json` |
| AssetValidation | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/AssetValidation/gem.json` |
| Atom | atom-core | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/gem.json` |
| Atom_Feature_Common | render-features | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/Feature/Common/gem.json` |
| Atom_RHI | atom-rhi | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/RHI/gem.json` |
| Atom_RHI_DX12 | atom-rhi | 0 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/RHI/DX12/gem.json` |
| Atom_RHI_Metal | atom-rhi | 0 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/RHI/Metal/gem.json` |
| Atom_RHI_Null | atom-rhi | 10 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/RHI/Null/gem.json` |
| Atom_RHI_Vulkan | atom-rhi | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/RHI/Vulkan/gem.json` |
| Atom_RPI | atom-rpi | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/RPI/gem.json` |
| Atom_TestData | content | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/AtomContent/TestData/gem.json` |
| AtomContent | content | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/AtomContent/gem.json` |
| AtomFont | render-features | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/AtomLyIntegration/AtomFont/gem.json` |
| AtomLyIntegration | render-features | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/AtomLyIntegration/gem.json` |
| AtomRenderOptions | render-features | 20 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/AtomLyIntegration/AtomRenderOptions/gem.json` |
| AtomToolsFramework | atom-tools | 20 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/Tools/AtomToolsFramework/gem.json` |
| AtomTressFX | render-features | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/AtomTressFX/gem.json` |
| AtomViewportDisplayIcons | render-features | 20 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/AtomLyIntegration/AtomViewportDisplayIcons/gem.json` |
| AtomViewportDisplayInfo | render-features | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/AtomLyIntegration/AtomViewportDisplayInfo/gem.json` |
| AudioSystem | audio-media | 11 hosts | selection:engine, selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/AudioSystem/gem.json` |
| AutomatedTesting | testing | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `AutomatedTesting/Gem/gem.json` |
| BarrierInput | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/BarrierInput/gem.json` |
| Camera | engine-gameplay | 0 hosts |  | published bounded integration slice; global gates remain open | `Gems/Camera/gem.json` |
| CameraFramework | engine-gameplay | 0 hosts |  | published bounded integration slice; global gates remain open | `Gems/CameraFramework/gem.json` |
| CertificateManager | online-platform | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/CertificateManager/gem.json` |
| CommonFeaturesAtom | render-features | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/AtomLyIntegration/CommonFeatures/gem.json` |
| Compression | engine-gameplay | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/Compression/gem.json` |
| CrashReporting | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/CrashReporting/gem.json` |
| CustomAssetExample | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/CustomAssetExample/gem.json` |
| DccScriptingInterface | render-features | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/gem.json` |
| DebugDraw | render-features | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/DebugDraw/gem.json` |
| DevTextures | engine-gameplay | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/DevTextures/gem.json` |
| DiffuseProbeGrid | render-features | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/DiffuseProbeGrid/gem.json` |
| EditorModeFeedback | render-features | 10 hosts | selection:engine, selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/AtomLyIntegration/EditorModeFeedback/gem.json` |
| EditorPythonBindings | editor-script | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/EditorPythonBindings/gem.json` |
| EMotionFX | physics-animation | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/EMotionFX/gem.json` |
| EMotionFX_Atom | render-features | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/AtomLyIntegration/EMotionFXAtom/gem.json` |
| ExpressionEvaluation | editor-script | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/ExpressionEvaluation/gem.json` |
| FastNoise | world | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/FastNoise/gem.json` |
| GameState | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/GameState/gem.json` |
| GameStateSamples | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/GameStateSamples/gem.json` |
| Gestures | engine-gameplay | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/Gestures/gem.json` |
| GradientSignal | world | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/GradientSignal/gem.json` |
| GraphCanvas | editor-script | 20 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/GraphCanvas/gem.json` |
| GraphModel | editor-script | 20 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/GraphModel/gem.json` |
| ImageProcessingAtom | asset-pipeline | 20 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/Asset/ImageProcessingAtom/gem.json` |
| ImGui | engine-gameplay | 0 hosts |  | published bounded integration slice; global gates remain open | `Gems/ImGui/gem.json` |
| InAppPurchases | online-platform | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/InAppPurchases/gem.json` |
| LandscapeCanvas | world | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/LandscapeCanvas/gem.json` |
| LmbrCentral | engine-gameplay | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/LmbrCentral/gem.json` |
| LocalUser | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/LocalUser/gem.json` |
| LyShine | ui | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/LyShine/gem.json` |
| LyShineExamples | ui | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/LyShineExamples/gem.json` |
| Maestro | engine-gameplay | 11 hosts | selection:engine, selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/Maestro/gem.json` |
| MaterialCanvas | atom-tools | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/Atom/Tools/MaterialCanvas/gem.json` |
| MaterialEditor | atom-tools | 0 hosts |  | workspace-slice-published; metadata-wrapper-still-present | `Gems/Atom/Tools/MaterialEditor/gem.json` |
| Meshlets | render-features | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/Meshlets/gem.json` |
| MessagePopup | ui | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/MessagePopup/gem.json` |
| Microphone | audio-media | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/Microphone/gem.json` |
| MiniAudio | audio-media | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/MiniAudio/gem.json` |
| MotionMatching | physics-animation | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/MotionMatching/gem.json` |
| Multiplayer | online-platform | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/Multiplayer/gem.json` |
| Multiplayer_ScriptCanvas | online-platform | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/Multiplayer/Multiplayer_ScriptCanvas/gem.json` |
| MultiplayerCompression | online-platform | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/MultiplayerCompression/gem.json` |
| NvCloth | physics-animation | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/NvCloth/gem.json` |
| OpenParticleSystem | engine-gameplay | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/OpenParticleSystem/gem.json` |
| PassCanvas | atom-tools | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/Atom/Tools/PassCanvas/gem.json` |
| PhysX5 | physics-animation | 11 hosts | selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/PhysX/Core/PhysX5/gem.json` |
| PhysX5Debug | physics-animation | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/PhysX/Debug/PhysX5/gem.json` |
| PhysXCommon | physics-animation | 5 hosts | selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/PhysX/Common/gem.json` |
| PrefabBuilder | engine-gameplay | 0 hosts |  | published bounded integration slice; global gates remain open | `Gems/Prefab/PrefabBuilder/gem.json` |
| Presence | online-platform | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/Presence/gem.json` |
| PrimitiveAssets | engine-gameplay | 0 hosts |  | published bounded integration slice; global gates remain open | `Gems/PrimitiveAssets/gem.json` |
| PythonAssetBuilder | editor-script | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/PythonAssetBuilder/gem.json` |
| PythonCoverage | testing | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `AutomatedTesting/Gem/PythonCoverage/gem.json` |
| QtForPython | editor-script | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/QtForPython/gem.json` |
| RecastNavigation | physics-animation | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/RecastNavigation/gem.json` |
| ReferenceMaterials | content | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/AtomContent/ReferenceMaterials/gem.json` |
| RemoteTools | engine-gameplay | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/RemoteTools/gem.json` |
| SaveData | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/SaveData/gem.json` |
| SceneLoggingExample | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/SceneLoggingExample/gem.json` |
| SceneProcessing | engine-gameplay | 10 hosts | selection:engine, selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/SceneProcessing/gem.json` |
| ScriptAutomation | editor-script | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/ScriptAutomation/gem.json` |
| ScriptCanvas | editor-script | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/ScriptCanvas/gem.json` |
| ScriptCanvasDeveloper | editor-script | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/ScriptCanvasDeveloper/gem.json` |
| ScriptCanvasPhysics | editor-script | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/ScriptCanvasPhysics/gem.json` |
| ScriptCanvasTesting | editor-script | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/ScriptCanvasTesting/gem.json` |
| ScriptedEntityTweener | editor-script | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/ScriptedEntityTweener/gem.json` |
| ScriptEvents | editor-script | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/ScriptEvents/gem.json` |
| ShaderManagementConsole | atom-tools | 20 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/Atom/Tools/ShaderManagementConsole/gem.json` |
| SkyAtmosphere | render-features | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/SkyAtmosphere/gem.json` |
| Sponza | content | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/AtomContent/Sponza/gem.json` |
| Sponza | testing | 0 hosts |  | Gem presente; no aceptación inferida | `AutomatedTesting/Gem/Sponza/gem.json` |
| Stars | render-features | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/Stars/gem.json` |
| StartingPointCamera | engine-gameplay | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/StartingPointCamera/gem.json` |
| StartingPointInput | engine-gameplay | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/StartingPointInput/gem.json` |
| StartingPointMovement | engine-gameplay | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/StartingPointMovement/gem.json` |
| Streamer | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/Streamer/gem.json` |
| StreamerProfiler | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/Streamer/StreamerProfiler/gem.json` |
| SurfaceData | world | 22 hosts | selection:engine, selection:project:AutomatedTesting, selection:project:JEEditorFixture | Gem presente; no aceptación inferida | `Gems/SurfaceData/gem.json` |
| Terrain | world | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/Terrain/gem.json` |
| TestAssetBuilder | engine-gameplay | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/TestAssetBuilder/gem.json` |
| TextureAtlas | ui | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/TextureAtlas/gem.json` |
| TickBusOrderViewer | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/TickBusOrderViewer/gem.json` |
| UiBasics | ui | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/UiBasics/gem.json` |
| Vegetation | world | 0 hosts | selection:project:AutomatedTesting | Gem presente; no aceptación inferida | `Gems/Vegetation/gem.json` |
| VideoPlaybackFramework | audio-media | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/VideoPlaybackFramework/gem.json` |
| VirtualGamepad | engine-gameplay | 0 hosts |  | Gem presente; no aceptación inferida | `Gems/VirtualGamepad/gem.json` |

## asset-pipeline

```mermaid
flowchart LR
  n8["Atom_RHI [configured]"]
  n13["Atom_RPI [configured]"]
  n52["ImageProcessingAtom [configured]"]
  n52 -->|"manifest"| n13
  n52 -->|"manifest"| n8
```

## atom-core

```mermaid
flowchart LR
  n4["Atom [configured]"]
  n7["Atom_Feature_Common [configured]"]
  n8["Atom_RHI [configured]"]
  n11["Atom_RHI_Null [configured]"]
  n13["Atom_RPI [configured]"]
  n16["AtomFont [configured]"]
  n19["AtomRenderOptions [configured]"]
  n21["AtomToolsFramework [configured]"]
  n23["AtomViewportDisplayIcons [configured]"]
  n24["AtomViewportDisplayInfo [configured]"]
  n31["CommonFeaturesAtom [configured]"]
  n43["EMotionFX_Atom [configured]"]
  n52["ImageProcessingAtom [configured]"]
  n98["ShaderManagementConsole [configured]"]
  n4 -->|"manifest"| n7
  n4 -->|"manifest"| n8
  n4 -->|"manifest"| n13
  n4 -->|"manifest"| n21
  n4 -->|"manifest"| n98
  n4 -->|"manifest"| n11
  n4 -->|"manifest"| n19
  n4 -->|"manifest"| n31
  n4 -->|"manifest"| n43
  n4 -->|"manifest"| n16
  n4 -->|"manifest"| n24
  n4 -->|"manifest"| n52
  n4 -->|"manifest"| n23
```

## atom-rhi

```mermaid
flowchart LR
  n8["Atom_RHI [configured]"]
  n9["Atom_RHI_DX12"]
  n10["Atom_RHI_Metal"]
  n11["Atom_RHI_Null [configured]"]
  n12["Atom_RHI_Vulkan [configured]"]
  n8 -->|"manifest"| n9
  n8 -->|"manifest"| n10
  n8 -->|"manifest"| n12
  n8 -->|"manifest"| n11
```

## atom-rpi

```mermaid
flowchart LR
  n8["Atom_RHI [configured]"]
  n13["Atom_RPI [configured]"]
  n13 -->|"manifest"| n8
```

## atom-tools

```mermaid
flowchart LR
  n7["Atom_Feature_Common [configured]"]
  n8["Atom_RHI [configured]"]
  n13["Atom_RPI [configured]"]
  n21["AtomToolsFramework [configured]"]
  n31["CommonFeaturesAtom [configured]"]
  n50["GraphCanvas [configured]"]
  n51["GraphModel [configured]"]
  n52["ImageProcessingAtom [configured]"]
  n62["MaterialCanvas"]
  n63["MaterialEditor [native-owner evidence]"]
  n74["PassCanvas"]
  n98["ShaderManagementConsole [configured]"]
  n119["Editor/Materials"]
  n21 -->|"manifest"| n13
  n21 -->|"manifest"| n8
  n21 -->|"manifest"| n50
  n21 -->|"manifest"| n51
  n62 -->|"manifest"| n21
  n62 -->|"manifest"| n13
  n62 -->|"manifest"| n8
  n62 -->|"manifest"| n7
  n62 -->|"manifest"| n52
  n62 -->|"manifest"| n31
  n63 -->|"manifest"| n21
  n63 -->|"manifest"| n13
  n63 -->|"manifest"| n8
  n63 -->|"manifest"| n7
  n63 -->|"manifest"| n52
  n63 -->|"manifest"| n31
  n74 -->|"manifest"| n21
  n74 -->|"manifest"| n13
  n74 -->|"manifest"| n8
  n74 -->|"manifest"| n7
  n74 -->|"manifest"| n52
  n74 -->|"manifest"| n31
  n98 -->|"manifest"| n21
  n98 -->|"manifest"| n13
  n98 -->|"manifest"| n8
  n98 -->|"manifest"| n7
  n98 -->|"manifest"| n52
  n98 -->|"manifest"| n31
  n63 -.->|"native ownership"| n119
  n62 -.->|"native ownership"| n119
  n74 -.->|"native ownership"| n119
```

## audio-media

```mermaid
flowchart LR
  n25["AudioSystem [configured]"]
  n57["LmbrCentral [configured]"]
  n66["Microphone"]
  n67["MiniAudio"]
  n115["VideoPlaybackFramework"]
  n25 -->|"manifest"| n57
  n66 -->|"manifest"| n25
```

## content

```mermaid
flowchart LR
  n14["Atom_TestData"]
  n15["AtomContent"]
  n86["ReferenceMaterials"]
  n101["Sponza (Gems/AtomContent/Sponza/gem.json)"]
  n132["unresolved-gem:Sponza [PENDIENTE]"]
  n15 -->|"manifest"| n132
  n15 -->|"manifest"| n86
```

## editor-script

```mermaid
flowchart LR
  n41["EditorPythonBindings"]
  n44["ExpressionEvaluation"]
  n50["GraphCanvas [configured]"]
  n51["GraphModel [configured]"]
  n82["PythonAssetBuilder"]
  n84["QtForPython"]
  n91["ScriptAutomation"]
  n92["ScriptCanvas"]
  n93["ScriptCanvasDeveloper"]
  n94["ScriptCanvasPhysics"]
  n95["ScriptCanvasTesting"]
  n96["ScriptedEntityTweener"]
  n97["ScriptEvents"]
  n51 -->|"manifest"| n50
  n82 -->|"manifest"| n41
  n84 -->|"manifest"| n41
  n92 -->|"manifest"| n97
  n92 -->|"manifest"| n44
  n92 -->|"manifest"| n50
  n93 -->|"manifest"| n92
  n93 -->|"manifest"| n50
  n94 -->|"manifest"| n92
  n95 -->|"manifest"| n92
  n95 -->|"manifest"| n50
  n95 -->|"manifest"| n97
```

## engine-gameplay

```mermaid
flowchart LR
  n1["AFR"]
  n2["Archive"]
  n3["AssetValidation"]
  n4["Atom [configured]"]
  n8["Atom_RHI [configured]"]
  n13["Atom_RPI [configured]"]
  n27["BarrierInput"]
  n28["Camera [native-owner evidence]"]
  n29["CameraFramework [native-owner evidence]"]
  n32["Compression"]
  n33["CrashReporting"]
  n34["CustomAssetExample"]
  n38["DevTextures"]
  n46["GameState"]
  n47["GameStateSamples"]
  n48["Gestures"]
  n53["ImGui [native-owner evidence]"]
  n57["LmbrCentral [configured]"]
  n58["LocalUser"]
  n59["LyShine"]
  n60["LyShineExamples"]
  n61["Maestro [configured]"]
  n65["MessagePopup"]
  n73["OpenParticleSystem"]
  n78["PrefabBuilder [native-owner evidence]"]
  n80["PrimitiveAssets [native-owner evidence]"]
  n87["RemoteTools"]
  n88["SaveData"]
  n89["SceneLoggingExample"]
  n90["SceneProcessing [configured]"]
  n92["ScriptCanvas"]
  n103["StartingPointCamera"]
  n104["StartingPointInput"]
  n105["StartingPointMovement"]
  n106["Streamer"]
  n107["StreamerProfiler"]
  n110["TestAssetBuilder"]
  n112["TickBusOrderViewer"]
  n113["UiBasics"]
  n116["VirtualGamepad"]
  n118["Editor/Camera"]
  n121["Engine/Assets/PrefabBuilder"]
  n122["Engine/Assets/Primitives"]
  n124["Engine/Debug/ImGui"]
  n130["Engine/World/Camera"]
  n2 -->|"manifest"| n32
  n27 -->|"manifest"| n13
  n28 -->|"manifest"| n13
  n47 -->|"manifest"| n46
  n47 -->|"manifest"| n58
  n47 -->|"manifest"| n59
  n47 -->|"manifest"| n88
  n47 -->|"manifest"| n65
  n47 -->|"manifest"| n57
  n47 -->|"manifest"| n113
  n47 -->|"manifest"| n60
  n48 -->|"manifest"| n13
  n53 -->|"manifest"| n57
  n61 -->|"manifest"| n57
  n61 -->|"manifest"| n4
  n61 -->|"manifest"| n8
  n61 -->|"manifest"| n13
  n103 -->|"manifest"| n57
  n104 -->|"manifest"| n92
  n116 -->|"manifest"| n59
  n116 -->|"manifest"| n113
  n28 -.->|"native ownership"| n130
  n29 -.->|"native ownership"| n118
  n80 -.->|"native ownership"| n122
  n78 -.->|"native ownership"| n121
  n53 -.->|"native ownership"| n124
```

## online-platform

```mermaid
flowchart LR
  n0["Achievements"]
  n7["Atom_Feature_Common [configured]"]
  n30["CertificateManager"]
  n55["InAppPurchases"]
  n69["Multiplayer"]
  n70["Multiplayer_ScriptCanvas"]
  n71["MultiplayerCompression"]
  n79["Presence"]
  n92["ScriptCanvas"]
  n131["unresolved-gem:PhysX [PENDIENTE]"]
  n69 -->|"manifest"| n131
  n69 -->|"manifest"| n30
  n69 -->|"manifest"| n7
  n70 -->|"manifest"| n92
```

## physics-animation

```mermaid
flowchart LR
  n13["Atom_RPI [configured]"]
  n31["CommonFeaturesAtom [configured]"]
  n42["EMotionFX [configured]"]
  n57["LmbrCentral [configured]"]
  n68["MotionMatching"]
  n72["NvCloth"]
  n75["PhysX5 [configured]"]
  n76["PhysX5Debug"]
  n77["PhysXCommon [configured]"]
  n85["RecastNavigation"]
  n42 -->|"manifest"| n13
  n42 -->|"manifest"| n57
  n68 -->|"manifest"| n42
  n72 -->|"manifest"| n31
  n72 -->|"manifest"| n42
  n75 -->|"manifest"| n77
  n75 -->|"manifest"| n57
  n75 -->|"manifest"| n31
  n76 -->|"manifest"| n75
```

## render-features

```mermaid
flowchart LR
  n4["Atom [configured]"]
  n7["Atom_Feature_Common [configured]"]
  n8["Atom_RHI [configured]"]
  n11["Atom_RHI_Null [configured]"]
  n13["Atom_RPI [configured]"]
  n16["AtomFont [configured]"]
  n18["AtomLyIntegration"]
  n19["AtomRenderOptions [configured]"]
  n21["AtomToolsFramework [configured]"]
  n22["AtomTressFX"]
  n23["AtomViewportDisplayIcons [configured]"]
  n24["AtomViewportDisplayInfo [configured]"]
  n31["CommonFeaturesAtom [configured]"]
  n35["DccScriptingInterface"]
  n37["DebugDraw"]
  n39["DiffuseProbeGrid"]
  n40["EditorModeFeedback [configured]"]
  n42["EMotionFX [configured]"]
  n43["EMotionFX_Atom [configured]"]
  n49["GradientSignal [configured]"]
  n52["ImageProcessingAtom [configured]"]
  n57["LmbrCentral [configured]"]
  n64["Meshlets"]
  n74["PassCanvas"]
  n84["QtForPython"]
  n98["ShaderManagementConsole [configured]"]
  n99["SkyAtmosphere"]
  n102["Stars"]
  n108["SurfaceData [configured]"]
  n7 -->|"manifest"| n13
  n7 -->|"manifest"| n8
  n16 -->|"manifest"| n8
  n16 -->|"manifest"| n13
  n19 -->|"manifest"| n13
  n23 -->|"manifest"| n8
  n23 -->|"manifest"| n13
  n24 -->|"manifest"| n8
  n24 -->|"manifest"| n13
  n31 -->|"manifest"| n7
  n31 -->|"manifest"| n57
  n31 -->|"manifest"| n49
  n31 -->|"manifest"| n108
  n31 -->|"manifest"| n13
  n31 -->|"manifest"| n21
  n40 -->|"manifest"| n7
  n40 -->|"manifest"| n13
  n40 -->|"manifest"| n8
  n40 -->|"manifest"| n31
  n43 -->|"manifest"| n42
  n43 -->|"manifest"| n7
  n43 -->|"manifest"| n13
  n43 -->|"manifest"| n8
  n43 -->|"manifest"| n31
  n18 -->|"manifest"| n16
  n18 -->|"manifest"| n19
  n18 -->|"manifest"| n23
  n18 -->|"manifest"| n24
  n18 -->|"manifest"| n31
  n18 -->|"manifest"| n40
  n18 -->|"manifest"| n43
  n18 -->|"manifest"| n13
  n18 -->|"manifest"| n8
  n18 -->|"manifest"| n11
  n18 -->|"manifest"| n7
  n18 -->|"manifest"| n52
  n18 -->|"manifest"| n21
  n18 -->|"manifest"| n74
  n18 -->|"manifest"| n98
  n35 -->|"manifest"| n84
  n37 -->|"manifest"| n13
  n39 -->|"manifest"| n13
  n39 -->|"manifest"| n4
  n39 -->|"manifest"| n57
  n99 -->|"manifest"| n13
  n99 -->|"manifest"| n31
```

## testing

```mermaid
flowchart LR
  n26["AutomatedTesting"]
  n83["PythonCoverage"]
  n100["Sponza (AutomatedTesting/Gem/Sponza/gem.json)"]
```

## ui

```mermaid
flowchart LR
  n4["Atom [configured]"]
  n13["Atom_RPI [configured]"]
  n16["AtomFont [configured]"]
  n21["AtomToolsFramework [configured]"]
  n52["ImageProcessingAtom [configured]"]
  n57["LmbrCentral [configured]"]
  n59["LyShine"]
  n60["LyShineExamples"]
  n65["MessagePopup"]
  n111["TextureAtlas"]
  n113["UiBasics"]
  n59 -->|"manifest"| n57
  n59 -->|"manifest"| n13
  n59 -->|"manifest"| n4
  n59 -->|"manifest"| n16
  n59 -->|"manifest"| n111
  n59 -->|"manifest"| n21
  n59 -->|"manifest"| n113
  n60 -->|"manifest"| n57
  n60 -->|"manifest"| n59
  n65 -->|"manifest"| n59
  n111 -->|"manifest"| n13
  n111 -->|"manifest"| n52
```

## world

```mermaid
flowchart LR
  n4["Atom [configured]"]
  n7["Atom_Feature_Common [configured]"]
  n13["Atom_RPI [configured]"]
  n31["CommonFeaturesAtom [configured]"]
  n45["FastNoise"]
  n49["GradientSignal [configured]"]
  n50["GraphCanvas [configured]"]
  n51["GraphModel [configured]"]
  n52["ImageProcessingAtom [configured]"]
  n56["LandscapeCanvas"]
  n57["LmbrCentral [configured]"]
  n108["SurfaceData [configured]"]
  n109["Terrain"]
  n114["Vegetation"]
  n45 -->|"manifest"| n49
  n45 -->|"manifest"| n57
  n45 -->|"manifest"| n108
  n49 -->|"manifest"| n108
  n49 -->|"manifest"| n52
  n49 -->|"manifest"| n57
  n56 -->|"manifest"| n51
  n56 -->|"manifest"| n49
  n56 -->|"manifest"| n108
  n56 -->|"manifest"| n57
  n56 -->|"manifest"| n50
  n108 -->|"manifest"| n57
  n108 -->|"manifest"| n13
  n108 -->|"manifest"| n7
  n109 -->|"manifest"| n13
  n109 -->|"manifest"| n4
  n109 -->|"manifest"| n49
  n109 -->|"manifest"| n108
  n109 -->|"manifest"| n57
  n114 -->|"manifest"| n57
  n114 -->|"manifest"| n108
  n114 -->|"manifest"| n31
  n114 -->|"manifest"| n49
```

## native-absorptions

```mermaid
flowchart LR
  n5["Atom_AtomBridge [native-owner evidence]"]
  n6["Atom_Bootstrap [native-owner evidence] [candidate, gates pending]"]
  n7["Atom_Feature_Common [configured]"]
  n8["Atom_RHI [configured]"]
  n13["Atom_RPI [configured]"]
  n17["AtomImGuiTools [native-owner evidence]"]
  n20["AtomShader"]
  n21["AtomToolsFramework [configured]"]
  n28["Camera [native-owner evidence]"]
  n29["CameraFramework [native-owner evidence]"]
  n31["CommonFeaturesAtom [configured]"]
  n36["DebugCamera [native-owner evidence]"]
  n52["ImageProcessingAtom [configured]"]
  n53["ImGui [native-owner evidence]"]
  n54["ImguiAtom [native-owner evidence]"]
  n57["LmbrCentral [configured]"]
  n62["MaterialCanvas"]
  n63["MaterialEditor [native-owner evidence]"]
  n74["PassCanvas"]
  n78["PrefabBuilder [native-owner evidence]"]
  n80["PrimitiveAssets [native-owner evidence]"]
  n81["Profiler [native-owner evidence]"]
  n117["WhiteBox [native-owner evidence]"]
  n118["Editor/Camera"]
  n119["Editor/Materials"]
  n121["Engine/Assets/PrefabBuilder"]
  n122["Engine/Assets/Primitives"]
  n123["Engine/Assets/RenderBuilders"]
  n124["Engine/Debug/ImGui"]
  n125["Engine/Debug/Profiler"]
  n126["Engine/Geometry"]
  n127["Engine/Render/Debug"]
  n128["Engine/Render/Integration"]
  n130["Engine/World/Camera"]
  n62 -->|"manifest"| n21
  n62 -->|"manifest"| n13
  n62 -->|"manifest"| n8
  n62 -->|"manifest"| n7
  n62 -->|"manifest"| n52
  n62 -->|"manifest"| n31
  n63 -->|"manifest"| n21
  n63 -->|"manifest"| n13
  n63 -->|"manifest"| n8
  n63 -->|"manifest"| n7
  n63 -->|"manifest"| n52
  n63 -->|"manifest"| n31
  n74 -->|"manifest"| n21
  n74 -->|"manifest"| n13
  n74 -->|"manifest"| n8
  n74 -->|"manifest"| n7
  n74 -->|"manifest"| n52
  n74 -->|"manifest"| n31
  n28 -->|"manifest"| n13
  n53 -->|"manifest"| n57
  n28 -.->|"native ownership"| n130
  n29 -.->|"native ownership"| n118
  n80 -.->|"native ownership"| n122
  n78 -.->|"native ownership"| n121
  n53 -.->|"native ownership"| n124
  n117 -.->|"native ownership"| n126
  n81 -.->|"native ownership"| n125
  n54 -.->|"native ownership"| n127
  n17 -.->|"native ownership"| n127
  n5 -.->|"native ownership"| n128
  n36 -.->|"native ownership"| n130
  n63 -.->|"native ownership"| n119
  n6 -.->|"native ownership"| n128
  n62 -.->|"native ownership"| n119
  n74 -.->|"native ownership"| n119
  n20 -.->|"native ownership"| n123
```

## Límites verificables

- Inventario de manifests completo para Gems/Engine/Editor/Code/Projects/AutomatedTesting/Tools/Templates del checkout, excluyendo Cache/user/build y terceros; fixtures y plantillas no se cuentan como Gems productivas. No incluye Gems instaladas fuera del checkout.
- CMake se inspecciona como fuente, no se ejecuta un evaluador alternativo. Alias, variables y ramas dinámicas pueden requerir resolución adicional; las referencias extraídas no prueban que el target se enlace en la plataforma actual.
- DX12/Metal/Null y capacidades online/Python siguen inventariadas aunque no formen parte del runtime Vulkan autorizado. Presencia de manifest no equivale a aceptación ni habilitación.
- No se demuestra un callgraph C++ absoluto: EBuses, callbacks, factories, reflection y assets requieren trazas por contrato. Este mapa conserva esa frontera PENDIENTE en lugar de inventar aristas.
- Passes, material types, materiales y otras fuentes de authoring siguen siendo datos editables; un archivo JSON no es por sí mismo configuración fija sobrante. Este inventario no prescribe eliminarlo.
- Build94 estaba activo durante la lectura. No se ejecutó configure/build/test/GUI ni se leyó su resultado como cerrado. Los hashes detectan cambios en metadatos capturados; cualquier drift invalida esa parte de la captura.
- La evidencia histórica tiene fecha y alcance propios; no sustituye gates runtime/Ready/SDK/Beauty/performance/VR/Linux/Shipping abiertos.

## Delta98 — inicialización RPI nativa (16/10/2026)

El mapa anterior sigue siendo su snapshot estático; este delta registra el ownership verificado sin inventar un callgraph completo. Las dos activaciones Atom_RPI.Private/Editor ya no forman parte de los22 planes actuales de once ejecutables. Las cinco funciones de host enlazan las factorías nativas antes de Bootstrap. Public/Edit/assets/Registry/manifest siguen residuales; Atom_RPI continúa PARTIAL y el incremento de Gems completas es0.

```mermaid
flowchart LR
  Runtime["Launcher: CreateStaticModules"] --> RF["JE::Render::Integration::CreateRpiRuntimeModule"]
  Editor["Editor + AtomToolsApplication"] --> EF["JE::Editor::RenderIntegration::CreateRpiEditorModule"]
  AP["AssetProcessor + AssetBuilder"] --> EF
  RF --> RI["RenderIntegration.dll: JE::Render::RPI"]
  EF --> ERI["EditorRenderIntegration.dll: editor RPI"]
  ERI --> RI
  RI --> Public["Atom_RPI.Public residual"]
  ERI --> Edit["Atom_RPI.Edit residual"]
  Metadata["Registry/rpi_assets.setreg: Atom_RPI vacío"] -. "descubrimiento sin módulo" .-> Assets["Assets + Registry + aliases residuales"]
```

Pruebas588/588, PE/consumidor SDK focal, Editor/White Box/Material Workspace y runtime Vulkan reales. Dos DLL de módulos menos; no se afirma absorción de todo Atom, cierre SDK packaging, AP global, Linux, VR, Shipping ni mejora global de RAM. La evidencia y límites están en [corte98](../../evidence/n0/native-rpi-initialization-2026-10-16/README.md). Reutilizar SDK96; no repetir install/configure/build por cambios que no invalidan esa evidencia.