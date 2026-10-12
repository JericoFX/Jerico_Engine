# Native prefab builder fixture

`NativePrefabCube.fbx` exercises default procedural prefab generation through the existing SceneAPI pipeline. It deliberately has no authored `.assetinfo`; preserve the input bytes when validating the native `JE::PrefabBuilder` integration.

The input is an unmodified copy of O3DE's `Gems/Atom/Tools/MaterialEditor/Assets/MaterialEditor/ViewportModels/Cube.fbx` at commit `241c78b8064898a709cf1c5dd3535c6406763ef3`, recovered previously from the pinned original source. Copyright (c) Contributors to the Open 3D Engine Project; SPDX-License-Identifier: Apache-2.0 OR MIT. See the repository license and notices.

Size: 30,656 bytes. SHA-256: `9c9e212ad50c78cd759156f4f47bf5c9fe61149472648dfd467341895f6ed914`.

The repository stores this small fixture directly; no Git LFS or external asset download is needed. A successful build alone does not verify this fixture: Asset Processor must produce its procedural prefab and mesh, and the editor must load the resulting asset.
