Local Windows x64 producer for the mcpp 2.7.2_az.2 interface used by Atom.
The source archive and original O3DE patch are pinned; only a newly extracted
work copy under this checkout's `build` directory is modified. Acquisition,
Git, upstream configure scripts and global installation are absent from the producer.

The producer preserves the original DLL/import library and CLI layout. The shared
JE package helper must support `JE_PACKAGE_SOURCE_RELEASE`; no upstream commit is
invented for this historical release. On Windows x64, the assigned native producer
and consumers passed: four CTests in Release and four in Debug, plus eight producer
refusal cases. The measured JE archive SHA256 is
`4c4ae3b71be82837b18090029d7ec49a19af80c12b35008b70a0d6f967acc589`.
See `docs/evidence/n0/local-packages/mcpp-rev2-validation/` for the actual commands
and logs; the earlier capture with empty arguments is NOT_RUN. This focal result
does not establish a working ShaderBuilder or Editor.

Producer command template (use a new absolute output each time):

```powershell
cmake -DJE_MCPP_SOURCE_ARCHIVE=F:/JericoEngine/Dependencies/archives/mcpp-2.7.2.tar.gz `
  -DJE_MCPP_SOURCE_SHA256=3b9b4421888519876c4fc68ade324a3bbd81ceeb7092ecdbbc2055099fcb8864 `
  -DJE_MCPP_SOURCE_URL=https://sourceforge.net/projects/mcpp/files/mcpp/V.2.7.2/mcpp-2.7.2.tar.gz/download `
  -DJE_MCPP_OUTPUT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-local-v1 `
  -P cmake/Tools/LocalPackages/mcpp/BuildMcppPackage.cmake
```

After hashing and extracting the produced archive to a separate local directory:

```powershell
cmake -S cmake/Tools/LocalPackages/mcpp/tests `
  -B F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-consumer-v1 `
  -G "Visual Studio 17 2022" -A x64 -T v143 `
  -DJE_MCPP_PACKAGE_ROOT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-extracted-v1
cmake --build F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-consumer-v1 --config Release
ctest --test-dir F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-consumer-v1 -C Release --output-on-failure
cmake --build F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-consumer-v1 --config Debug
ctest --test-dir F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-consumer-v1 -C Debug --output-on-failure
```

The real consumer imports the DLL as Atom does and tests nested include callbacks,
macro expansion/stringizing/pasting, absolute line origins, the _az.2 comment-line
fix, invalid inputs and recovery without leaked macro state. The original CLI has
a separate preprocessing test. Producer refusals cover URI/UNC paths, missing or
incorrect source hash, different release URL, existing output and output outside
checkout/build. Symlink refusal remains unexecuted; no privilege changes were made.
These gates do not prove ShaderBuilder or Editor
execution, complete legal redistribution clearance, or engine performance.
