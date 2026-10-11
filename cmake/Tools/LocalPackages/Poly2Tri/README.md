# JE local poly2tri

The producer builds the five core C++ files from the audited jhasse commit in
Recipe.json. It verifies the archive, pinned full snapshot manifest and every
source member before creating a private output below this checkout's build.
No downloads, VCS, upstream testbed/tests, vcpkg bootstrap or global install.
The immutable source snapshot is copied; no source patches are applied.

Required producer arguments: JE_POLY2TRI_SOURCE_ARCHIVE,
JE_POLY2TRI_SOURCE_MANIFEST, JE_POLY2TRI_VS_INSTANCE and JE_POLY2TRI_OUTPUT.
Run BuildPoly2TriPackage.cmake with CMake -P. The output must be new. Windows
x64 VS2022/v143 compiles Debug/MDd and Release/MD with six explicit jobs.
The package helper produces packages/poly2tri-7f0487a-je-rev1-windows.tar.xz,
its .tar.xz.SHA256SUMS sidecar and a .contents directory.

Findpoly2tri preserves 3rdParty::poly2tri, the original nested header layout,
Release lib and Debug debug/lib. Profile resolves Release as in the original
package. The tests project consumes this Find without ambient libraries and
runs four cases per configuration: concave polygon, hole, Steiner insertion
and coincident-edge rejection with untouched caller points. Core CDT outputs
must preserve vertex identity and area. The README upstream does not promise
general validation of arbitrary malformed polygons.

The producer and archive data audit completed. The contractual consumer
configure/build/CTest passed12 cases, zero failures/skips across Debug,
Profile and Release; Profile used the existing Release library mapping.
See the portable
[parent focal review](../../../../docs/evidence/n0/local-packages/physx-poly2tri-final-2026-10-11/poly2tri/consumer/parent-focal-review.json),
[archive audit](../../../../docs/evidence/n0/local-packages/physx-poly2tri-final-2026-10-11/poly2tri/consumer/archive-audit.json)
and [evidence index](../../../../docs/evidence/n0/local-packages/physx-poly2tri-final-2026-10-11/index.json).
The archive reader's COFF checks were data inspection; native linking and
the triangulation oracles provide the focal behavior evidence.

The subsequent Root configure resolved the integrated PhysX and Poly2Tri
packages, then failed generation (exit1) because Gem::AudioSystem.API and
Gem::ProfilerImGui were absent. Final engine link/deploy and
Editor/AP/physics behavior remain unverified. This Tools/Editor dependency
unblocks transitional PhysX authoring; final Jolt migration remains required.
