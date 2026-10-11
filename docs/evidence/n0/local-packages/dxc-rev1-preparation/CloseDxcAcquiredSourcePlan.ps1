$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $PSScriptRoot
$oldAudit = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-current-audit'
$out = Join-Path $root 'dxil-signing-audit'
if (Test-Path -LiteralPath $out) { throw 'New audit output already exists' }
[IO.Directory]::CreateDirectory($out) | Out-Null
$handler = [Net.Http.HttpClientHandler]::new()
$handler.AllowAutoRedirect = $false; $handler.UseCookies = $false; $handler.UseDefaultCredentials = $false
$client = [Net.Http.HttpClient]::new($handler)
$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-primary-text-audit/1.0')
$texts = @(
    @{ url = 'https://api.github.com/repos/microsoft/DirectXShaderCompiler/releases/tags/v1.8.2502'; filename = 'microsoft-release-v1.8.2502.json' },
    @{ url = 'https://devblogs.microsoft.com/directx/open-sourcing-dxil-validator-hash/'; filename = 'microsoft-dxil-validator-hash.html' }
)
$receipts = @()
try {
    foreach ($item in $texts) {
        $response = $client.GetAsync($item.url).GetAwaiter().GetResult()
        try {
            if ([int]$response.StatusCode -ne 200 -or $response.RequestMessage.RequestUri.AbsoluteUri -ne $item.url) { throw 'Primary text endpoint did not return direct 200' }
            $bytes = $response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()
            $path = Join-Path $out $item.filename
            [IO.File]::WriteAllBytes($path, $bytes)
            $receipts += [ordered]@{ url = $item.url; status = 200; autoRedirect = $false; authentication = $false; bytes = $bytes.Length; sha256 = (Get-FileHash -LiteralPath $path).Hash; path = $path; fetchedUtc = [DateTime]::UtcNow.ToString('o'); binaryAcquired = $false }
        } finally { $response.Dispose() }
    }
} finally { $client.Dispose(); $handler.Dispose() }
$release = Get-Content -LiteralPath (Join-Path $out 'microsoft-release-v1.8.2502.json') -Raw | ConvertFrom-Json
if ($release.tag_name -ne 'v1.8.2502' -or -not $release.body.Contains('validator binaries will no longer be required')) { throw 'Official release statement differs' }
$source = Join-Path $root 'acquisition/dxc/snapshot'
$paths = @('README.md', 'LICENSE.TXT', 'ThirdPartyNotices.txt', 'lib/DxilHash/DxilHash.cpp', 'lib/DxilHash/CMakeLists.txt', 'tools/clang/tools/dxcvalidator/dxcvalidator.cpp', 'tools/clang/tools/dxcvalidator/CMakeLists.txt', 'tools/clang/tools/dxildll/CMakeLists.txt', 'tools/clang/tools/dxildll/dxcvalidator.cpp', 'tools/clang/tools/dxcompiler/dxcutil.cpp', 'tools/clang/tools/dxcompiler/CMakeLists.txt', 'tools/clang/tools/dxcompiler/dxcvalidator.cpp', 'tools/clang/tools/dxcompiler/dxillib.cpp', 'lib/DxilContainer/DxcContainerBuilder.cpp', 'unittests/DxilHash/DxilHashTest.cpp', 'tools/clang/tools/dxsc/dxsc.cpp', 'lib/DxilCompression/LICENSE.TXT')
$refs = @($paths | ForEach-Object { $path = Join-Path $source $_; [ordered]@{ relativePath = $_; path = $path; bytes = (Get-Item -LiteralPath $path).Length; sha256 = (Get-FileHash -LiteralPath $path).Hash; url = "https://raw.githubusercontent.com/o3de/DirectXShaderCompiler/1cc0ab76dafe08a4144e419e619a8951a3d219db/$_" } })
$signing = [ordered]@{
    status = 'READONLY SOURCE/PRIMARY DOCUMENTATION AUDIT; NATIVE VALIDATION PENDING'
    sourceCommit = '1cc0ab76dafe08a4144e419e619a8951a3d219db'
    conclusion = 'No additional closed/signed Microsoft validator binary acquisition is required by the audited 2025 source contract. The source includes retail DXIL hash computation; original four-tool payload still includes its source-built dxil.dll.'
    distinctions = @('DXIL container validator hash is not Windows DLL Authenticode signing.', 'The older 2021 wiki describes the pre-open-source validator flow, and cannot override the exact 2025 implementation.', 'Successful HRESULT alone is insufficient: consumer must inspect IDxcOperationResult status, hash/container, invalid-shader rejection and later real DX12 pipeline acceptance.')
    sourceChain = @(
        'lib/DxilHash/DxilHash.cpp:211 implements ComputeHashRetail,351 ComputeHashDebug under NCSA source license.'
        'tools/clang/tools/dxcvalidator/dxcvalidator.cpp:36-85 computes retail hash for ordinary shader models; preview models use PreviewByPassHash upstream. Do not request preview profiles or bypass.'
        'dxcvalidator.cpp:101 calls ValidateDxilContainer;222-270 retains validationStatus and returns it in IDxcOperationResult alongside hashed container. Require successful operation status.'
        'tools/clang/tools/dxcvalidator/CMakeLists.txt:5-24 links DxilHash and DxilValidation into dxcvalidator.'
        'tools/clang/tools/dxcompiler/dxcutil.cpp:53-79 Auto selects the internal validator; only explicit External requires dxil.dll.'
        'tools/clang/tools/dxildll/CMakeLists.txt links LLVMDxilHash and dxcvalidator; dxildll/dxcvalidator.cpp:76-94 delegates to the same validateWithDebug routine.'
        'lib/DxilContainer/DxcContainerBuilder.cpp:195-220 detects/preserves retail/debug hashing for supported container edits; this is relevant to specialization edits, not a substitute for runtime proof.'
    )
    publicSources = $receipts
    officialRelease = [ordered]@{ url = $release.html_url; tag = $release.tag_name; publishedAt = $release.published_at; interpretation = 'February2025 release explicitly removes separate validator requirement for hashed shaders and cites the open-sourcing change. No release asset requested.' }
    prohibitedShortcuts = @('-Vd', 'developer-mode enablement', 'experimental shader-model bypass', 'replacement vanilla compiler', 'omitting dxsc/dxil from the original payload')
    sourceInputs = $refs
    nativeExecuted = $false; gpuAcceptance = $false; redistributionCertified = $false
}
$signing | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath (Join-Path $out 'dxil-signing-contract.json') -Encoding utf8
$plan = Get-Content -LiteralPath (Join-Path $oldAudit 'source-plan-dxc-closed.json') -Raw | ConvertFrom-Json
$plan.status = 'THREE EXACT SOURCES ACQUIRED/EXTRACTED; CLOSED PLAN FOR PARENT OVERLAY/PRODUCER REVIEW; NO IMPLEMENTATION OR NATIVE ACCEPTANCE'
foreach ($item in $plan.sources) {
    $dir = Join-Path $root "acquisition/$($item.id)"
    $a = Get-Content -LiteralPath (Join-Path $dir 'source-acquisition.json') -Raw | ConvertFrom-Json
    $name = if ($item.id -eq 'dxc') { 'source-extraction-rev2.json' } else { 'source-extraction.json' }
    $ex = Get-Content -LiteralPath (Join-Path $dir $name) -Raw | ConvertFrom-Json
    $item.archiveSHA256 = $a.sha256; $item.archiveBytes = (Get-Item -LiteralPath $a.archivePath).Length
    $item.archiveStatus = 'ACQUIRED: direct HTTPS HEAD/GET200, no redirects/auth; safely extracted and full primary comparison verified'
    $item | Add-Member -NotePropertyName snapshot -NotePropertyValue $ex.snapshotPath
    $item | Add-Member -NotePropertyName fileCount -NotePropertyValue $ex.fileCount
    $item | Add-Member -NotePropertyName extractionReceipt -NotePropertyValue (Join-Path $dir $name)
    $item | Add-Member -NotePropertyName comparison -NotePropertyValue $(if ($item.id -eq 'dxc') { '17832 raw Gitblob matches +19 exact approved CRLF/LF derived proofs. Snapshot RAW bytes unchanged.' } else { 'All files raw Gitblob match; no links/omissions.' })
}
$plan.provenanceProposal.queryFeature = 'Keep HLSL_SUPPORT_QUERY_GIT_COMMIT_INFO ON. Proposed private GetCommitInfo.py emits short hash1cc0ab76 (8bytes required by all existing 8+1 allocations) and count4954 from the reviewed pinned GitHub REST history cardinality. Full40 source pin remains in package/provenance metadata, not the fixed-size runtime string. No count0/history fabrication, no E_NOTIMPL or API/allocator changes. Overlay still needs parent authorization.'
$plan.provenanceProposal | Add-Member -NotePropertyName runtimeContractEvidence -NotePropertyValue (Join-Path $root 'version-api-contract-addendum.json')
$plan.provenanceProposal | Add-Member -NotePropertyName historyEvidence -NotePropertyValue (Join-Path $root 'commit-history-metadata-receipt-rev3.json')
$plan.provenanceProposal | Add-Member -NotePropertyName historyMethod -NotePropertyValue 'Unfiltered pinned GitHub REST pages1/4954/4955;4954 is API-list cardinality, not a local git rev-list execution.'
$plan.legal.requiredFullTexts += 'lib/DxilCompression/LICENSE.TXT'
$plan.nativeValidationPlan[4] = 'IDxcVersionInfo2 exact8 hash1cc0ab76/count4954 under reviewed API-cardinality provenance, CoTaskMemFree ownership; full40 source pin in package metadata. Native consumer must inspect compiled retail DXIL hash, validation operation status, malformed-shader rejection; no -Vd/devmode/experimental bypass.'
$plan.outstanding = @('Parent authorization of exact private provenance overlays and producer source scope', 'Fresh private copies of DXC plus two local gitlinks, raw immutable snapshots must remain unchanged', 'Luna-only original Release compiler/CMake/PE/native consumers after source review/freeze', 'Real AP/DX12/Vulkan Editor pipeline acceptance and legal/product acceptance remain pending')
$plan.archiveAcquired = 3
$plan | Add-Member -NotePropertyName signingAudit -NotePropertyValue (Join-Path $out 'dxil-signing-contract.json')
$plan | Add-Member -NotePropertyName supersedes -NotePropertyValue 'Historical closed plan/count0/full40 runtime proposal remains preserved; this plan plus runtime contract addendum replaces that unsafe proposal.'
$plan | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath (Join-Path $root 'source-plan-dxc-acquired-final.json') -Encoding utf8
@((Join-Path $out 'dxil-signing-contract.json'), (Join-Path $root 'source-plan-dxc-acquired-final.json')) | ForEach-Object { [pscustomobject]@{ path = $_; bytes = (Get-Item -LiteralPath $_).Length; sha256 = (Get-FileHash -LiteralPath $_).Hash } } | ConvertTo-Json
