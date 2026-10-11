# JericoEditor Visual Design System

This is the current binding JericoEngine2 editor visual design. Its approved blueprint images originated from earlier Jerico design work, but older milestone naming is obsolete and must not be used as a JericoEngine2 roadmap stage or implementation authority.

## Character

Professional dense game-engine/DCC editor:
- dark graphite/navy;
- compact controls;
- shallow tonal depth;
- thin cool borders;
- viewport/working surface dominates;
- semantic accent, not decorative color noise;
- almost-square/small-radius controls;
- no oversized mobile/web-card visual language.

## Palette

```text
Shell                 #081420
Surface               #0D1A26
Panel                 #112334
Raised                #162C40
Border                #24465D
Primary cyan          #00C8E8
Active orange         #FF9418
Selection             #FFB347
Primary text          #E8EEF4
Secondary text        #AEBBC6
Muted text            #748897
Grid minor            #173347
Grid major            #29536B
Success               #2BC76F
Warning               #F3B43F
Error                 #EF5350
Axis X                 #EF5350
Axis Y                 #42D37A
Axis Z                 #42A5F5
```

Workspaces may use domain overlay colors in the viewport but do not change global editor chrome.

## Layout grammar

Three compact chrome layers are allowed where useful:
- application/context;
- menu;
- workspace tools.

Default body:
- left navigation/hierarchy/content;
- central real viewport/asset/material/graph working surface;
- right Inspector;
- integrated lower docks;
- persistent status bar.

## Shared viewport

Keep:
- navigation grammar;
- selection outline;
- gizmo colors;
- grid hierarchy;
- viewport header;
- perspective/orthographic naming;
- Lit/Debug views;
- status feedback.

Workspace may replace domain tools/panels/overlays.

## Density targets

Approximate 100% scale:
- toolbar/control 28–30 px;
- compact tree/table rows 22–24 px;
- standard list 26–30 px;
- tab strip 28–30 px;
- panel spacing 6–8 px;
- group spacing 8–12 px.

DPI scaling must preserve relationships.

## Controls

Provide one JericoStyle/shared widget/delegate layer for:
- tabs;
- tree rows;
- Inspector sections;
- vector/transform editors;
- asset pickers;
- color/material fields;
- sliders/numerics;
- search;
- asset tiles;
- icon buttons;
- validation/busy states;
- drag/drop.

QSS may help, but QSS alone is not the design system.

## Workspace compositions

### Scene
Outliner + Entity Palette left; viewport center; Inspector right; Output/Problems/Profiler below.

### Import
Source/Queue left; preview and Settings/Dependencies/Validation center; Import Settings right; log/summary below.

### Models & Assets
Content/filter left; large model preview center; thumbnail browser below; asset inspector/dependencies/LOD/collision right.

### Materials
material hierarchy/library left; PBR preview + graph center; surface/layer parameters right.

### Environment/EPLS
environment hierarchy/presets left; real scene center; sky/time/fog/probe controls right; curves/timeline/diagnostics below.

### RBrush / Stamp
brush/alpha/material palette and layer stack left; real surface center; brush inspector right; stroke history/overlays below.

### PWAS / Geometry
hierarchy/action stack left; large perspective viewport; synchronized orthographic views when useful; inspector right; operation/diff/volumes below.

### Scripting
project tree left; IDE center; symbols/docs right; console/debug/call stack/breakpoints below.

### Physics
World/asset left; real viewport center with colliders/constraints/contact overlays; body/constraint inspector right; simulation/profiler below.

### Rig/Motion
asset/rig hierarchy left; mesh/skeleton viewport center; rig/IK/property inspector right; timeline/dope/curves below.

### XR
World/action/reference-space status left; stereo/desktop preview center; session/actions/perf inspector right; frame diagnostics below.

## Interaction states

Every shared control supports coherent Idle/Hover/Pressed/Focused/Disabled/Busy/Warning/Error/Success states.

Keyboard focus remains visible independently of mouse hover.

## Rule

**One JericoEditor. One viewport language. One panel grammar. Tabs change the task, not the identity of the application.**


## Authoritative raster blueprint references

The old SVG workspace mockups in `images/30_*.svg` through `images/39_*.svg` are **layout/coverage sketches only**. They are not the final aesthetic authority and must not be copied literally into the product.

The owner-supplied visual authority is:

- `images/40_jericoeditor_blueprint_reference_board.jpg` — canonical Scene/World blueprint board from `main`.
- `images/41_jerico_brand_icon_reference.jpg` — canonical product mark and editor composition reference from `main`.
- `images/40_jericoeditor_scene_blueprint.png` — owner-supplied full-resolution Scene/World detail.
- `images/41_jericoeditor_physics_blueprint.png` — owner-supplied full-resolution Physics detail.
- `images/42_jericoeditor_materials_blueprint.png` — owner-supplied full-resolution Materials detail.

Use this same application frame, tab and panel grammar for later workspace references the owner supplies. The three images show one product shell with workspace-specific left tools, central work area, right Inspector and lower tabs. They set visual direction; they do not prove those depicted systems already run. The outer annotations and dimension callouts explain the design and are not application chrome.

The blueprint references define the intended UI more strongly than the old SVGs:
- clearly separated top application menu and workspace tabs;
- distinct panels with visible tab bars instead of one undifferentiated dense surface;
- large central viewport remains dominant;
- lower dock area uses tabs for Content Browser, Console, Graph, Simulation, Profiler, timeline/curves and other contextual tools;
- right Inspector is component-oriented and explicitly exposes Entity Components;
- workspace tabs include Scene, Terrain, Geometry, Materials, Lighting, VFX, Audio, AI, Animation, Physics, UI, Scripting and Plugins as appropriate;
- Python/plugin extensibility shown in the concepts is editor tooling only where implemented; gameplay scripting remains Luau `--!strict`;
- Source 2 / Hammer-like clarity and task separation are desired without cloning Source 2;
- Cry/Sandbox depth and productivity may be retained, but Jerico presentation must feel cleaner, more separated and easier to scan.

### Brand/icon rule

Use the **orange + white modular metallic J** shown in `images/41_jerico_brand_icon_reference.jpg` as the JericoEngine application/product mark. The full-resolution workspace references show the same identity more clearly.

Do not invent a cyan triangle, animal head, generic cube, or alternate Jerico logo for the application chrome.

The editor iconography for tools/components remains a coherent functional icon set, but the **brand mark itself is the supplied J**.

### Blueprint intent

These images are not screenshots to pixel-copy. They are implementation blueprints for hierarchy, separation, tabs, density, component visibility, and visual language. Codex should translate them into Qt 6 Widgets using real Jerico editor/runtime contracts.
