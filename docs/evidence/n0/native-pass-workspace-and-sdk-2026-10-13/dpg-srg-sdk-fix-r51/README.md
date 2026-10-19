# Native shared Terrain SRG: SDK shader regression

The engine's SceneSrg aggregate included a Terrain declaration through its Gem path. The installed SDK did not carry that path, so shaders using the common scene layout failed preprocessing. The private SDK validation project also lacked its authored scene/view SRG hooks.

The shared declaration now lives in the existing native Render Features ShaderLib. Its bytes, fields, guard and attribution are unchanged; the aggregate includes the native location and the old provider is removed. The three Terrain shader consumers still use the same common scene wrapper. This moves one shared shader declaration, not the Terrain Gem or its rendering implementation.

The private validation project received the two unchanged hooks from the installed DefaultProject template. This preserves the wrapper plus project contract described in [upstream PR18566](https://github.com/o3de/o3de/pull/18566), head `cb0a7cf789d75c402fd844f6013500c0de03d15c`, merged as `b8a8d94a5b3fb0819eec2e8886fdf297ab074c9d`; no PR patch was imported. The [upstream review](upstream-review.json) and [application receipt](application-receipt.json) retain the decision and exact data changes.

The [producer applicability review](producer-applicability.json) compared 24 ShaderCompiler, shader builder and MCPP source/recipe files against published HEAD. All match after CRLF/LF normalization. This fix requires their already published behavior and no additional C++ hunk; it does not accept unrelated working-tree changes or claim a clean-HEAD rebuild.

## Actual checks

Windows/Profile tools targeted Vulkan shader products. These are cook and installed-data checks, not a GPU rendering test.

| Check | Observed result |
| --- | --- |
| Existing Features Assets install leaf, CORE/profile/LOCAL_ONLY | Native exit 0, 0.1665556s; one added SDK file, one changed file, no removals |
| Remaining SDK and producer cohort | 11,317 other SDK files and 120 producer outputs protected; no drift; 82 inactive DX12/Null payloads remain absent |
| Three DPG source shaders and Core DiffuseGlobalFullscreen | Four actual jobs with status 4, zero errors, two warnings each; 17 physical products per job |
| Previous 19 precompiled shaders | Same successful jobs, identities and payloads retained; not fresh work in AP4 |
| Previous 41 Vulkan dependencies | Same exact tuples, PreLoad DependencyFlags 0, no extras; not 41 newly executed jobs |
| DPG seed references | All 29 join to actual GUID + SubId 0 + product path, including Core Fullscreen |
| Readonly database capture | 91 sources/jobs, 155 products; DB byte exact; Node reader exited |

The seed loader constructs new entries. AssetId initializes SubId to 0 and the JSON deserializer preserves that default when the field is omitted. [The source proof](seed-default-proof.json) records this condition and the limit for loading an already populated object.

The [initial three DPG source jobs](red-products-review.json) had status 2 with missing project scene/view hooks and missing Terrain include. Their corrected status 4 records, physical products and hashes are in [products-review.json](products-review.json), with [the actual captured rows](query-result.json) and the four original JobLogs. [manifest.json](manifest.json) pins every copied artifact. The executed reader recipe had two abbreviated names in an unused metadata array; [the erratum](reader-metadata-erratum.json) preserves that discrepancy. SQL selected the DPG owner plus the exact Core source, and the independent product comparison used full actual names.

## Limits retained

AP4 globally returned 1 in 303.2514853s: 24 other assets failed. The first global fatal is a 261-character header candidate in MaterialCanvas output templates. The four focal jobs each retain two `Unknown encoding: C.UTF-8` warnings. No failures or warnings were suppressed, and template role does not authorize excluding those files from processing.

This evidence does not accept the complete SDK, clean-HEAD product, AssetBundler execution, runtime GI, Baked/AutoSelect, no-RT device policy, GUI, Linux, VR, Shipping, N0 or a whole Gem. C++ tests and unaffected binaries were reused; no C++ rebuild or whole-SDK reinstall was performed for this asset-only correction. Implementation source and evidence were reviewed separately from unrelated working-tree changes. Base cohort: `e04567254e22ee9288d8ad8443a53754265a5dd3`.
