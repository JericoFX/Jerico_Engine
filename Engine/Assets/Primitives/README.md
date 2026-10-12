# Native primitive content

The engine owns these box, cylinder and sphere meshes, material sources and
checker textures. Asset Processor discovers `Assets` through
`Registry/primitives_assetprocessor.setreg`; projects do not enable a Gem.
The existing scan-folder identity and relative `Objects/_Primitives` and
`Textures/_Primitives` paths preserve asset references.

Content comes from O3DE commit `241c78b8064898a709cf1c5dd3535c6406763ef3`, formerly
`Gems/PrimitiveAssets/Assets`, under the repository's Apache-2.0 OR MIT license.
FBX payloads match their original SHA-256 LFS object IDs. The three TIFF payloads
match locally audited Atom default textures byte for byte. Materials and scene
manifests retain their original bytes. No geometry generator or second material
system is introduced.

These are authoring sources. Games consume their cooked products through the
existing asset pipeline; this directory adds no runtime library.
