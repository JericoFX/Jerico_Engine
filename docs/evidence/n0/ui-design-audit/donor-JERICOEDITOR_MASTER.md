# JericoEditor Master — CRYENGINE Transformation Edition

JericoEditor is the main product for authoring the future JericoEngine.

We transform Sandbox instead of throwing away mature editor behavior, but the final product uses Jerico architecture, UX, formats and visual identity.

## Technology

- Qt 6 Widgets.
- Real embedded Jerico renderer viewport.
- Shared document/World/selection/history.
- One Inspector/property system driven by Reflection.
- One command/transaction system.
- Same commands feed UI and MCP.
- DPI aware.
- dockable/saveable layouts.
- multi-monitor capable.

Qt Quick/QML is not required to reach the visual target.

## Editor states

```text
EDIT <-> SIMULATE <-> PLAY
```

Use the real runtime systems.

Safe live edits may include materials, lights, environment, audio, VFX, Luau, selected physics/AI settings and procedural previews.

On exit:
- revert simulation changes by default;
- optionally apply explicitly selected safe changes.

## Shared shell

Default:
- top menu/workspace/tool strip;
- left World/Assets/navigation;
- central viewport;
- right Inspector;
- lower Output/Problems/Profiler/timeline/curves;
- status bar.

Workspace changes tools/panels/overlays, not product identity.

The shell must preserve visible separation between the application menu, workspace tabs, panel tabs, viewport, Inspector and lower docks. Avoid collapsing everything into one visually continuous panel. See the authoritative raster blueprints named in `docs/JERICOEDITOR_VISUAL_DESIGN.md`.

Entity Components are a first-class Inspector concept, not hidden implementation detail. Python/plugin extensibility may be exposed as editor tooling/plugin capability alongside Luau gameplay scripting; Python does not replace Luau as the gameplay VM.

## Shared viewport and tab contract

JericoEditor is one coherent application, not a pile of independent mini-editors.

The approved direction is:
- persistent application shell and Jerico iconography;
- explicit workspace tabs;
- shared central viewport whenever the task is spatial;
- left/right/lower panels change by workspace while the viewport/document/selection remain coherent;
- opening Material, Physics, Rig/Motion, AI, Audio, VFX, Sequencer, UI or Luau tools must not silently create duplicate World/selection/asset authorities;
- dock/panel state is saveable and DPI/multi-monitor aware.

The visual target follows the current approved JericoEditor blueprint references and Source 2/Hammer-style clarity: strong separation between menu, workspace tabs, panel tabs, viewport, Inspector and lower timeline/log/profiler regions.

## Workspaces

- World / Entities
- Assets / Models
- Import
- Geometry / JericoBSP
- Procedural Studio
- PWAS / EPLS / RBrush
- Terrain / Vegetation
- Materials / Textures
- Physics / Constraints / Destruction
- Rigging / Skinning / IK
- Motion / Animation Graph
- AI / Navigation
- Audio
- VFX / Particles
- Sequencer
- UI Studio
- Item Studio
- Gameplay Class / Roles / Factions
- Narrative Studio (Dialogue / Quest / Journal)
- Project / User Settings & Accessibility
- Luau IDE
- XR
- Profiling / RuntimeQA

## Direct physical authoring

The editor must make physical setup closer to classic focused game editors, but much more capable.

Example door workflow:

```text
select door mesh/entity
-> Add Component: RigidBody
-> Fit/Create Collider
-> Add Constraint: Hinge
-> click/pick pivot in viewport
-> orient hinge axis with gizmo
-> set min/max angle
-> optional spring/motor/friction/break threshold
-> Physics Play
-> inspect constraint frame/contact/debug
-> Apply/Undo
```

No external DCC required to move a hinge pivot.

Equivalent tools:
- Slider
- Distance
- Cone
- 6DOF
- Gear/pulley/rack where admitted by Jolt integration
- motors/springs

## Bones / rig / movement authoring

Asset Workbench keeps the mesh visible while switching:
Mesh -> Skeleton -> Skin -> Rig -> IK -> Physics -> Animation.

Required actions:
- create/delete/reparent bone;
- position/orient bone;
- mirror;
- assign semantic role;
- edit skin weights;
- auto-weight proposal + preview/diff;
- create IK chain;
- limits/constraints;
- control rig handles;
- pose;
- root motion;
- contact/grip points;
- retarget mapping;
- animation recording/keying/bake.

Same evaluator semantics in editor and runtime.

## Asset Workbench

Opening an imported asset keeps one canonical asset document and viewport.

Modes:
- mesh;
- normals/tangents;
- UV;
- materials;
- LOD/meshlets;
- collision;
- sockets;
- rig;
- skin;
- IK;
- physics;
- prefab;
- dependencies/validation.

No hidden duplicate model per tool.

## Geometry authoring

JericoEditor absorbs useful Designer/Hammer-like speed:
- primitives;
- additive/subtractive structural brushes;
- face/edge/vertex edit;
- extrude/inset;
- bevel/chamfer;
- slice/knife/loop;
- bridge/fill;
- doors/windows/arches/stairs;
- spline extrusion;
- conversion to mesh/assembly;
- material per face;
- UV tools;
- damage/procedural stack.

Structural authoring is non-destructive where practical.

## Surface authoring / id Tech-style stamping

One viewport gesture family supports:
- Material/Mask Paint;
- Decal Stamp;
- Damage Projector;
- Terrain Sculpt/Stamp.

Palette left, controls/Inspector right, visible cursor/volume, preview before commit.

Outputs remain distinct and editable.

## Procedural Studio

Normal use is visual/preset-driven, with advanced Graph hidden until needed.

Canonical abstractions:
Profile + Path + Operations + Pattern + Host + Assembly.

Supports doors/windows/stairs, pipes/ducts/cable trays, rails/fences, bridges/piers/catwalks/scaffolds, webs, geological structures and reusable assemblies.

Every generator:
- deterministic seed;
- preview/diff;
- cancel;
- locks;
- partial regenerate;
- Apply;
- Undo/Redo;
- cook.

## Materials

New Material workspace:
- PBR preview sphere/cube/selected asset;
- Material Graph;
- instances;
- layers/functions;
- texture packing/inspection;
- physical material binding;
- variant/performance diagnostics;
- Cook.

No `.mtl` editor remains as product authority.

## Physics / Destruction workspace

- bodies/colliders;
- Physical Materials;
- constraints;
- COM/inertia;
- character/ragdoll/vehicle setup;
- fracture profile;
- bonds/anchors;
- damage areas/paths/projectors;
- fresh interior surfaces;
- fragment preview;
- Jolt simulation;
- debug overlays.

## Luau IDE

- project/script tree;
- multi-file editor;
- completion from generated Reflection API;
- diagnostics;
- debugger/breakpoints/call stack;
- symbols/docs;
- Run/Reload/Profile;
- drag entity/component from World into typed reference;
- script attachment in Inspector.

## XR workspace

- OpenXR runtime/system status;
- view/frustum debug;
- tracking space;
- controller/action state;
- recenter/reference-space tools;
- composition-layer preview;
- performance/frame timing;
- simulate desktop fallback where useful;
- launch headset preview without redesigning the level.

## Editor automation

All meaningful authoring operations should have typed command forms reusable by MCP. UI is not a privileged mutation bypass.

## Problems / validation

One Problems model across all workspaces:
severity, message, object/asset identity, property/path, subsystem, safe fix.

Click selects the relevant thing.

## Performance

Editor must remain responsive:
- heavy work jobs are cancelable;
- previews are bounded/debounced;
- no blocking full recook for routine edits;
- show truthful progress;
- retain timings.

## Acceptance

A workspace does not pass because it looks correct. It must manipulate the real document/runtime contract with Undo/Redo and cook/runtime proof.


## Motion Studio editor contract

Motion Studio must expose the binding animation workflow from `docs/AUDIO_VFX_ANIMATION_UI_MASTER.md` inside JericoEditor.

Minimum product controls include:
- Skeleton hierarchy;
- bone create/delete/reparent/insert/mirror;
- skin weights and weight diagnostics;
- Rig/Control Rig;
- FK/IK effectors and poles;
- hands/fingers/contacts/GripPoints;
- Dope Sheet;
- Curve Editor;
- Animation Graph;
- Correction Layers;
- Play/Pause/Stop/Step/scrub/loop/range;
- Auto Key;
- Record Correction;
- Freeze Evaluated Pose;
- Match FK to IK;
- Match IK to FK;
- Snap Hand to GripPoint;
- Bake correction;
- root-motion and motion-path visualization;
- save/reopen/reimport diagnostics.

A user must be able to play an imported animation, pause on a bad hand/limb pose, adjust it through FK/IK, key the correction, save it and cook it without a mandatory external DCC round trip.

The editor preview and runtime use the same evaluator semantics.

## UI Studio editor contract

UI Studio authors JericoUI, not Qt widgets for Shipping.

Required surfaces:
- Hierarchy;
- 2D Canvas;
- world/3D UI preview;
- Inspector;
- styles;
- bindings;
- local state;
- keyframe/tween timeline;
- Sequencer integration;
- Luau action/binding integration;
- inventory/container scenario preview;
- localization preview;
- accessibility/focus graph;
- performance/overdraw/text diagnostics;
- causality/event trace.

`.rui2d` and `.rui3d` are authoring documents and cook to `.rui`.

Local visual tweens may be requested from Luau, but C++/JericoUI owns per-frame curve evaluation. Cross-system UI/camera/world/audio/VFX timelines use `.rseq`.

UI Studio preview uses the real JericoUI runtime path; Qt is editor chrome only.

## Audio and VFX editor contract

Audio and VFX are first-class workspaces, not future external tools.

Audio workspace exposes real JericoAudio events, emitters/listeners, attenuation, buses/mix, reverb/environment, physical-surface sounds, streaming, concurrency/virtualization and occlusion diagnostics.

VFX workspace exposes GraphCore, CPU/GPU effect preview, Play/Pause/Step/Reset, deterministic seed, flipbook/EmberGen inspection, LOD/budgets, overdraw/particle counts and cook/dependency diagnostics.

Both previews use their real runtime systems and source-free cooked products.
