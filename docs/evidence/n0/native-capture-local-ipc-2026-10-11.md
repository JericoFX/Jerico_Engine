# Native capture and local Asset Processor IPC — 2026-10-11

Scope: seven source/test files. Ten-bit Atom capture produces a normalized RGBA8 PNG preview and preserves the same original packed readback in a DDS sidecar. Existing RGBA8 output remains unchanged. Both Asset Processor listeners bind to IPv4 loopback for local tools/builders.

Validation: `build/n0-validation/local-packages/framecapture-ap-native-build-rev1/attempt1/validation-result.json`. Editor, GUI/Batch AP, Atom Feature tests and AzTestRunner build exited 0 (321.186s). Native Atom suite ran 68 tests, including seven capture regressions; CTest exited 0. The original validation preserved 16942 inputs. Parent rechecked all seven source hashes against its freeze and all recorded log/JUnit hashes before publication. JUnit SHA256: C875357D6C61462EEB3F9ECBC3278371ED87CA6B8C2BB95B07EE0D058AAA3848.

Real DX12 Editor/game-mode captures in `build/n0-validation/editor-fixture-dx12-attempt9` produced PNG plus original DDS. Vulkan captures in `editor-fixture-vulkan-attempt2` retained the existing path. Parent reviewed original pixels. GUI AP PID132108 was observed listening on 127.0.0.1:45643; Batch runtime was not separately exercised.

These results use the current local Editor baseline, which contains additional uncommitted work. This commit does not claim a clean-checkout Editor build, complete BSP integration, dependency removal, performance acceptance, or VR/OpenXR support. N0/N1 and the full goal remain open. Detailed logs and captures are retained locally under the paths above.