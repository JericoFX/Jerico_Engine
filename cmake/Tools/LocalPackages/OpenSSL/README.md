# Local OpenSSL 1.1.1o candidate

This recipe reconstructs the engine's Windows static OpenSSL dependency from
the audited official release archive. It performs no acquisition, bootstrap,
global installation or tool download. The historical package's ABI and archive
hash are not asserted. OpenSSL remains a candidate until the original root
configure reaches and accepts it; isolated consumers do not establish Editor,
Asset Processor, networking-product or runtime acceptance.

The exact source is OpenSSL_1_1_1o, commit
`ca2e0784d2c38edcefd5d68028f4d954bd8faddb`, SHA256
`9384a2b0570dd80358841464677115df785edb941c71211f75076d72fe6b438f`.
The producer also checks the official SHA512 recorded in its script. It extracts
a private work copy beneath this checkout's `build` directory. It never patches
the immutable snapshot. The original Windows vcpkg recipe's dtls1.h/rand.h
replacement patterns are absent in this exact release. Their hashes and zero
match counts are retained, with `applied=false`; headers remain unchanged.

The native configuration is `VC-WIN64A enable-static-engine enable-capieng
no-ssl2 no-tests -utf-8 no-shared -FS`, built with v143/14.44.35207 using
`nmake build_libs` and `install_dev` to a private prefix. Threads, assembler and
the full enabled library/engine features remain upstream defaults. Upstream's
static `/MT /Zl` settings are preserved: `/Zl` suppresses automatic default RTL
linkage. The archive is not described as an `/MD` build. `/MD` consumers verify
actual linking, object ownership and cryptography; they do not prove historical
binary ABI compatibility. Python's separate OpenSSL 1.1.1u and Qt's TLS backend
are separate inputs and are not substituted.

Required explicit inputs to `cmake -P BuildOpenSSLPackage.cmake` are:

- `JE_OPENSSL_SOURCE_ARCHIVE`, `JE_OPENSSL_SOURCE_SHA256`,
  `JE_OPENSSL_SOURCE_URL` (the audited official 1.1.1o URL);
- `JE_OPENSSL_PERL_ARCHIVE`, `JE_OPENSSL_PERL_ROOT`,
  `JE_OPENSSL_PERL_MANIFEST` (audited Strawberry 5.42.3.1 portable ZIP,
  immutable extracted tree and full 20,858-file SHA256 manifest);
- `JE_OPENSSL_VS_ROOT` (the audited VS 2022 BuildTools installation),
  `JE_OPENSSL_NASM` (the audited installed NASM executable);
- `JE_OPENSSL_OUTPUT` (a new absolute path beneath this checkout/build).

Archive, manifest, vcvarsall, cl, nmake and NASM pins are literal in the producer.
Every Perl payload file is checked before output creation. URI/UNC/symlink
paths, missing inputs, mismatched hashes and old outputs are rejected. Paths
with batch expansion characters are unsupported. A verified NASM executable is
copied alone to private `tool-bin`; WinLibs/GCC directories never enter PATH.
The native environment resets compiler/Perl/config variables and calls the
explicit vcvarsall for x64. No portable shell, CPAN or GCC configure is invoked.

The shared JECreateLocalPackage helper creates
`packages/OpenSSL-1.1.1o-je-rev1-windows.tar.xz`, its measured SHA256 sidecar and
`.contents` payload with root FindOpenSSL.cmake. Layout is
`OpenSSL/include/openssl`, `OpenSSL/lib/libcrypto.lib`, `libssl.lib`, and
`OpenSSL/LICENSE`. The Find module preserves OpenSSL::Crypto, OpenSSL::SSL and
3rdParty::OpenSSL, version/include/library variables, and links the complete
upstream Windows system libraries. It rejects competing/partial providers and
never searches for system OpenSSL. Full embedded library notices are packaged
under LICENSES; no Perl/NASM executables or DLLs enter the payload.

Validation belongs to Luna after parent source review and freeze. Configure
`tests` with VS 17 2022, x64/v143, a new local build directory,
`JE_OPENSSL_PACKAGE_ROOT=<extracted .contents>`, and
`JE_OPENSSL_VALID_INPUTS=<producer output>/producer-inputs.cmake`. Build Release
and run its CTest suite. The consumers check exact linked version, known SHA256
and AES-GCM answers, authentication rejection for corrupted ciphertext/tag,
RAND, OpenSSL allocation/reallocation/free, BIO and SSL context ownership, and
an authenticated TLS 1.3 memory-BIO handshake using a locally generated key and
certificate. No socket, certificate file, network service or external trust
input is used. AzNetworking's TLS/DTLS initialization, cookie, curve, context
and thread APIs plus both supported Windows/OpenSSL header orders are compiled.
The `static-engines` case requires the static/dynamic/CAPI/thread macros and
registers CAPI with configuration loading disabled. It checks the exact engine
identity, RSA method and lifecycle/key/certificate callbacks, rejecting the SDK's
empty CAPI fallback. Enumeration holds structural references only; it never
initializes CAPI or accesses certificate stores or external keys. CPU-dependent
PadLock/RDRAND availability is not required. This proves registration when run,
not the operation of a hardware or Windows certificate provider.
The negative harness demands the intended diagnostic and preserved outputs;
an arbitrary nonzero exit is insufficient.

This old release carries unresolved security maintenance debt. Reconstructing
the pinned baseline is not an endorsement for public network deployment and
does not authorize an unreviewed version/ABI upgrade. The current root blocker
is independently evidenced by the parent; this producer does not change it.
