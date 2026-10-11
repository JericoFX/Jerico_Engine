# JE local LZ4 1.9.4

The producer accepts only the audited official archive and creates its own
work copy beneath this checkout's `build`. It preserves the static core,
HC, frame and xxHash library, the three public headers and `3rdParty::lz4`.
The package contains one Release `/MD` archive used for every configuration,
matching the historical Find interface. VS2022/v143 and the absence of unused
GPL command-line tools are explicit adaptations with a new package identity.

Only the designated validation owner runs these commands. Every output must
be new; failed outputs are retained, and the producer has no download step.

```powershell
cmake -DJE_LZ4_SOURCE_ARCHIVE=F:/JericoEngine/Dependencies/archives/lz4-v1.9.4.tar.gz -DJE_LZ4_SOURCE_SHA256=0b0e3aa07c8c063ddf40b082bdf7e37a1562bda40a0ff5272957f3e987e0e54b -DJE_LZ4_SOURCE_URL=https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.9.4 -DJE_LZ4_OUTPUT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-v143-rev1 -P cmake/Tools/LocalPackages/LZ4/BuildLz4Package.cmake
```

The shared helper creates
`packages/lz4-1.9.4-je-rev1-windows.tar.xz`, its `.SHA256SUMS` sidecar and
`.contents` directory. The sidecar contains the measured new artifact hash.
Validation must extract the produced archive into another new local directory,
verify its sidecar/content hashes, and consume that extraction (not the stage):

```powershell
cmake -S cmake/Tools/LocalPackages/LZ4/tests -B build/n0-validation/local-packages/lz4-consumer-rev1 -G "Visual Studio 17 2022" -A x64 -T v143 -DJE_LZ4_PACKAGE_ROOT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-extracted-rev1
cmake --build build/n0-validation/local-packages/lz4-consumer-rev1 --config Release
ctest --test-dir build/n0-validation/local-packages/lz4-consumer-rev1 -C Release --output-on-failure
cmake --build build/n0-validation/local-packages/lz4-consumer-rev1 --config Debug
ctest --test-dir build/n0-validation/local-packages/lz4-consumer-rev1 -C Debug --output-on-failure
```

The consumer checks exact linked/header version, default and HC block
roundtrips, AzFramework's default frame APIs, fragmented decoding, bounded
destination failure, invalid magic and truncated-frame incompleteness.
The validation owner also checks missing/local/UNC/URI paths, wrong source
URL/hash and preserved existing outputs before a build. These focused checks
do not prove an engine build, Editor frame, performance or historical binary
compatibility. Package/catalog integration and acceptance belong to the parent.
