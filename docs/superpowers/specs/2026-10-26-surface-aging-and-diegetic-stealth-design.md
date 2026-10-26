# Silent Stealth and Surface Aging — JericoEngine Design

**Date:** 2026-10-26  
**Status:** DESIGN APPROVED IN PRINCIPLE / NOT IMPLEMENTED / TESTS PENDING  
**Target:** `JericoFX/Jerico_Engine`, `development`; O3DE-derived native `JE::` owners.  
**Owner decision:** No visible noise meter, bar, icon, detection warning, or numerical HUD. Players navigate by hearing and intuition.

## Intent

Preserve the claustrophobic feel, original spatial logic and puzzles of *Penumbra: Overture* while adding material-aware, persistent surface aging and meaningful stealth choices. Breaking planks should be faster and louder than carefully pulling nails with a hammer claw. Creature behavior must be legible through world sounds/animations, not through an omniscient UI.

## Approaches considered

- **Visual bar of noise/detection:** readable but reveals internal AI knowledge and spoils tension; rejected.
- **Contextual HUD indicator:** less intrusive but still supplies artificial certainty; rejected by owner.
- **Diegetic audio-only feedback (selected):** natural creaks, impacts, falling pieces, enemy footsteps, searching calls, breathing and animation. No artificial confidence indicator; maintain player agency through consistent cause/effect and distinguishable cues.

## A. Vertex material paint + automatic edge wear

Existing contracts: [Materials](../../ASSET_MATERIALS_FORMATS_EPAK_MASTER.md), [EPLS](../../EPLS_TECHNICAL_MASTER.md), [Playground handoff](../../PLAYGROUND_TO_JE_INTEGRATION_MASTER.md), [Physics and procedural](../../PROCEDURAL_PHYSICS_ENVIRONMENT_MASTER.md). Do not claim they are integrated simply because documented.

- `JE::Geometry` supplies stable surface/region identity, topology, signed curvature, convex/concave edge proximity, cavity and material eligibility (using a single adapted `SurfaceFeatureField` and `DamageField`).
- `RBrush` owns editable coverage/strokes/locks/Undo; payload can target vertex-color channels or authoring masks, material layers, decals, and separately explicit geometry-damage requests. Do not interpret painting as implicit mesh cutting.
- `JE::Materials/Atom` consumes vertex attributes or cooked masks for multi-material PBR blending; bind channel semantics rather than filename suffixes. Limit variant explosion and preserve neutral fallback when a mesh lacks vertex colors.
- `EPLS` drives corrosion, dirt, wetness, soot and edge wear using material/physical tags, environment and shared features. `Procedural Studio` may author chips, cavities, fractures; `JE::Geometry/Destruction` must own topology changes and Jolt recook.
- Bake expensive static effects into material/mask/mesh products. Runtime deformation is budgeted opt-in. Dirty-region invalidation must not recook unchanged mesh/collider/navigation for a mask-only edit.
- Import only audited mathematical ideas, fixtures and licensed code from pinned `JericoGeologyPlayground@7f7f09adaa25672ae2ac2af0d0033bab3f710fab`; never transplant its Diligent/ImGui host or claim tested parity.

**Visual acceptance scene:** Penumbra-style narrow mine hallway with metal door, wooden planks and stone wall. Artists paint exposed steel around joints and scratches, wet rust in lower recesses, timber scuffs, while an explicitly requested chip creates a new geometric edge. Compare same camera/lighting, verify Undo/Redo, Save/Reopen, reimport, cooked runtime, masks after BSP edits and performance.

## B. Sound-as-world-information stealth

**One event, separate meanings.** `JE::Physics/Jolt` and Interaction generate action/contact events. `JE::Audio` emits audible SoundEvents with physical-material surface mappings from `.rsoundsurface`. `JE::AI/Perception` consumes a bounded `NoiseStimulus` derived from the **same gameplay event**; never use rendered output decibels or player volume settings as the AI signal. A muted headset, audio accessibility preset or low-end device must not change enemy hearing. Do not create a second mixer, navigation planner, physics material system or universal evaluator.

Suggested canonical event fields (design shape, not code/API promise):
`eventId, worldPosition, sourceEntity, actionTag, surfacePair, intensityBand, duration, spectralHint, time, propagationSeed`.
`intensityBand` is an authored gameplay value, **not a visible HUD meter**. Tunable and logged in editor-only debug.

**Acoustic propagation:** Start with deterministic room/portal graph + door open state, distance falloff, obstruction/absorption from physical materials and fixed budget for exceptional raycasts. Static corridor bends attenuate but do not arbitrarily silence events; provide authoring diagnostics for disconnected portals. No physically exact wave solver. Couple to existing Audio zones/portals/occlusion where appropriate, with explicit conversion rather than assuming perceptual loudness equals stimulus strength.

**Creature response:** Perception records heard location, strength band, confidence and age; memory/facts feed existing HTN actions to orient, pause, investigate, search or escalate. A dog hearing a bang does not acquire precise player position or see through walls; it searches near the sound, can be distracted, and may return to patrol. Blind spiders rely more heavily on vibration/contact/sound, with surface transmission rules authored separately from airborne hearing. Cooldowns/hysteresis prevent flickering alert states. No omniscient pack coordination.

### First plank obstacle: mutually exclusive or combinable choices

1. **Strike/break:** rapid hammer impact + fracture sounds + loose boards fall to Jolt; high stimulus and a fast route.
2. **Extract nails with hammer claw:** slow repeated low-intensity metal friction/wood creaks, grip/tool alignment and progress per nail; risks an occasional sharp squeal if force exceeds threshold; valid lower-noise route.
3. **Pry plank:** variable impulse and break threshold; moderate noise with uncertainty, no scripted random punishment.
4. **Catch/hold plank:** physical contact lowers fall impulse and secondary sound if hands/space permit; releasing onto concrete or metal changes sound.
5. **Cancel/leave:** partial nail progress and board structural state persist appropriately; no magical restoration.

**Fairness:** Players can predict broad loud-vs-quiet tradeoff from real acoustics, friction/strain animations and enemy sounds. Hidden numerical values remain inspectable only in Editor debug. No universal “silent” action; hammer claw requires proximity/time, and a dog can still arrive for other reasons. AI responds even if player stands still, but no unavoidable scripted instant detection. Gameplay difficulty adjusts enemy thresholds/behavior, not fabricated stealth HUD.

## Shared ownership and sequence

1. Pin original source/fixtures and compare JE material/geometry/audio/AI interfaces before porting.
2. First playable vertical: **material painting + baked edge wear** on one BSP corridor (no physics damage).
3. Add **audible contact and noise stimulus** for hammer, nail, plank, landing; debug visualization restricted to Editor.
4. Connect dog perception → memory → HTN investigation without wall hacks.
5. Add geometry chips/board break, nail removal, Jolt collision, controlled rebuild and serialization.
6. Extend to blind spider vibration, alternative material surfaces, replay/save/cook/Shipping.
7. Only promote individual capabilities to IMPLEMENTED / VISUALLY VERIFIED / RUNTIME VERIFIED after their own evidence.

## Gates / tests (all PENDING)

- **SURF-01:** Vertex color and authored 2–4 material blend survive Undo/Redo, Save/Reopen and reimport.
- **SURF-02:** Edge wear follows correct convex/concave/region features, respects metal/wood/stone eligibility and `NoDamage` locks.
- **SURF-03:** Mask-only paint does not rebuild collision/nav; actual chip updates interior, UV, surface IDs and Jolt collider without cracks or NaNs.
- **SURF-04:** CSG/reimport preserves or explicitly diagnoses damaged surface lineage, never paints adjacent unrelated mesh.
- **NOISE-01:** Hammer strike louder than careful nail extraction under controlled geometry and material, including secondary plank drop sounds.
- **NOISE-02:** Hearing stimulus independent of user master volume, output device, subtitles and audio occlusion quality.
- **NOISE-03:** Closed doors, portals and bends attenuate predictably; editor debugger explains route and budget use.
- **NOISE-04:** Dog investigates last heard position, does not know unseen actor position; repeated stimuli update memory with hysteresis.
- **NOISE-05:** Quiet path has longer interaction time, small but interpretable physical risks, and no mandatory HUD feedback.
- **NOISE-06:** Blind-spider vibration distinct from airborne audio and tested on multiple surfaces.
- **NOISE-07:** Mid-interaction save/reopen, replay determinism, canceled jobs, Undo authoring, single-player cook and source-free runtime.
- **PERF-01:** CPU/audio voices, AI event count, Jolt contacts, GPU/VRAM and cook budget measured on corridors with multiple moving objects and enemies.

**Non-goals:** no HUD stealth gauge or minimap signal; no microphone-based listening, online/AWS service, network multiplayer, magical hearing, per-frame procedural authoring graph or second Physics/Audio/HTN runtime.
