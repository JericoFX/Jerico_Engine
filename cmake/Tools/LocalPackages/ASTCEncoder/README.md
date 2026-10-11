# Local ASTC encoder 3.2 provider

This Windows x64 Tools/Editor package preserves `3rdParty::astc-encoder`,
`astc-encoder/include/astcenc.h`, and Release/Debug static archives plus the
original debugging CLI in `astc-encoder/bin/{Release,Debug}`. ImageProcessingAtom
selects it only under `PAL_TRAIT_BUILD_HOST_TOOLS`; the Find module deploys no
Runtime executable. The codec retains all four profiles, U8/F16/F32,
normal/alpha swizzles, threading and mip reset. SSE4.1 is required as originally.

Source is the official ARM tag 3.2 commit
`f32d8d4dd9e06f8986cd5e10f12e199fc023f150`, archive SHA256
`291c26fcf20b8e23a6eb1ad4dd75f5afecf79d316c0477c532df6d5693291403` at
`https://codeload.github.com/ARM-software/astc-encoder/tar.gz/f32d8d4dd9e06f8986cd5e10f12e199fc023f150`.
Only the original O3DE 3.2-rev2 patch is applied, privately. The current newer
3p patch is not substituted. Upstream CMake minimum is 3.15; there is no policy
override, acquisition, submodule execution or global installation. GoogleTest
is unselected (`UNITTEST=OFF`). VS2022/v143 is the sole compiler adaptation.

Producer invocation (Luna owns execution; output must be absent):

```powershell
& 'C:/Program Files/CMake/bin/cmake.exe' `
  '-DJE_ASTC_SOURCE_ARCHIVE=F:/JericoEngine/Dependencies/archives/astc-encoder-f32d8d4dd9e06f8986cd5e10f12e199fc023f150.tar.gz' `
  '-DJE_ASTC_SOURCE_SHA256=291c26fcf20b8e23a6eb1ad4dd75f5afecf79d316c0477c532df6d5693291403' `
  '-DJE_ASTC_SOURCE_URL=https://codeload.github.com/ARM-software/astc-encoder/tar.gz/f32d8d4dd9e06f8986cd5e10f12e199fc023f150' `
  '-DJE_ASTC_OUTPUT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/astc-local-rev1' `
  -P 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/ASTCEncoder/BuildASTCEncoderPackage.cmake'
```

The producer captures configure/build argv, stdout/stderr and result/times,
builds both original targets with 8 jobs, and stages only after both configs
succeed. It reuses JECreateLocalPackage unchanged: `packages/astc-encoder-3.2-je-rev1-windows.tar.xz`,
`.tar.xz.SHA256SUMS`, and `.contents`. Never substitute the legacy artifact/hash.

After the parent audits and privately extracts the real archive, configure
`tests` with VS2022/x64/v143 and explicit `JE_ASTC_PACKAGE_ROOT` pointing to that
extraction. Build and run CTest separately in Release and Debug. Each config has
nine cases: eight API modes and the CLI version. Tests use real nonconstant ASTC
blocks, all 14 Atom footprints, all profiles/formats, threaded alpha and normal
swizzles, context reset, and API errors with unchanged sentinel output. The U8
gradient oracle permits four levels of lossy quantization, while constant/HDR
roundtrips use 0.008 absolute error. These are proposed native oracles, not
measured PASS results. Source header SHA binds the reported 3.2.0 version because
this API has no version macro/query. Native execution and real engine integration
remain pending; codec tests do not establish Editor visual quality or performance.

Producer guards reject missing args, URI/UNC/link ancestors, reused or out-of-build
outputs, source pin/hash mismatch and patch mismatch. Patch-context negatives
exercise ApplyASTCPatch in a private fixture rather than claiming a producer
case. Fixtures and native dispatch belong to Luna after source review.
