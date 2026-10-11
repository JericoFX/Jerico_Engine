# Local transition CPU PhysX5 provider

This recipe consumes the exact audited PhysX5.1.1 archive and regular-file
snapshot, plus the three pinned official4.1 build helpers. It uses private
copies, VS2022/v143 and four static configurations; it never runs packman
or generate_projects. `Recipe.json` records source pins, adaptations and
the CPU-only difference from the historical O3DE rev4 binary.

`PrepareSource.ps1` prepares fresh private source/helpers and emits the
single configure argv. The original fourteen targets were built in debug,
checked, profile and release by the assigned native executor. Rev2 reuses
those outputs without rebuilding the SDK.

`StagePhysXPackage.cmake` requires the source archive/root/manifest, a native
artifact witness and its explicit SHA256, and a fresh output below this
checkout/build. The witness retains exactly56 unique
`{config,target,path,bytes,sha256}` records and `generatedHeader` with
`{path,sha256}` for the real PxConfig.h. All source/header and56 native
artifact hashes are checked before staging. Only the eight main libraries
per configuration are copied:32 static archives in the new
`PhysX-5.1.1-je-cpu-rev2-windows` package. The six auxiliary archives remain
untouched in the original outputs and witness.

The Find module preserves 3rdParty::PhysX5, the eight main imported target
names, original link order, static macro/header layout and Profile-to-Checked
override. It requires32 main archives across four configurations. Upstream
Windows TARGET_OBJECTS embeds all six auxiliaries into PhysX_static_64.lib;
the parent-reviewed Profile comparison passed12 cases, and its link audit
matched110/110 auxiliary object members. The historical comparison and rev1
package remain preserved. GPU runtime dependencies are intentionally absent.

The single contractual consumer links only 3rdParty::PhysX5 and preserves
six cases: lifecycle/PVD creation without a transport, triangle/convex
cooking, CPU simulation/raycast, capsule controller, vehicular sprung
masses, and invalid cooking with unchanged caller output. It checks teardown
diagnostics before reporting completion. Final rev2 stage and contractual
configure/build/CTest passed24 cases, zero failures/skips: Debug, Profile,
Release and a separate Profile build with
LY_PHYSX_PROFILE_USE_CHECKED_LIBS=ON, six cases each. The preparation driver
records literal arguments, raw logs, JUnit and pre/post input hashes; it
never rebuilds SDK targets. See the portable
[parent final consumer review](../../../../docs/evidence/n0/local-packages/physx-poly2tri-final-2026-10-11/physx/final/parent-final-consumer-review.json),
[archive audit](../../../../docs/evidence/n0/local-packages/physx-poly2tri-final-2026-10-11/physx/archive/package-audit-summary.json)
and [evidence index](../../../../docs/evidence/n0/local-packages/physx-poly2tri-final-2026-10-11/index.json).

The Windows PAL/cache integration is recorded in the portable
[integration receipt](../../../../docs/evidence/n0/local-packages/physx-poly2tri-final-2026-10-11/integration/integration.json).
The subsequent Root configure resolved this package and Poly2Tri, then
failed generation (exit1) because Gem::AudioSystem.API and
Gem::ProfilerImGui were absent. No engine build or Editor/runtime acceptance
is established. PhysX remains a CPU transition provider, without historical
rev4 ABI/GPU equivalence; final Jolt migration remains required. The
vehicular oracle checks sprung-mass/API symbols, not a complete vehicle.
