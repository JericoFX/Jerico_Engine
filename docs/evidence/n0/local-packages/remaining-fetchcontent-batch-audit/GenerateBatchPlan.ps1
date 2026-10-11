$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
$out=$PSScriptRoot
$prior=Get-Content -LiteralPath (Join-Path $repo 'build/n0-validation/local-packages/root-cli11-final/root-cli11-assignment.json') -Raw | ConvertFrom-Json
$overrides=@{}
foreach($arg in $prior.arguments){$m=[regex]::Match($arg,'^-DFETCHCONTENT_SOURCE_DIR_([^:]+):PATH=(.+)$');if($m.Success){$overrides[$m.Groups[1].Value]=$m.Groups[2].Value}}
$paths=@(rg -l -i 'o3de_fetch_content\s*\(' "$repo/Code" "$repo/Gems" "$repo/cmake" -g '*.cmake' -g 'CMakeLists.txt') | Where-Object {$_ -notlike '*cmake\tests\*' -and $_ -notlike '*cmake/Tools/LocalPackages/*'}
$declared=@(foreach($path in $paths){
    $text=[IO.File]::ReadAllText($path)
    $name=[regex]::Match($text,'(?i)o3de_fetch_content\(\s*([\w-]+)').Groups[1].Value
    $upper=$name.ToUpperInvariant()
    $category=if($name -eq 'pybind11'){'EXCLUDED_PRODUCT_PYTHON'}elseif($name -eq 'RGA'){'EXCLUDED_ACTUAL_CACHE_FALSE'}elseif($overrides.ContainsKey($upper)){'LOCAL_OVERRIDE_PROVISIONED_NOT_ALL_TARGETS_VALIDATED'}else{'REMAINING_SELECTED_DECLARATION'}
    [pscustomobject]@{path=$path;bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant();fetchName=$name;category=$category;override=$overrides[$upper]}
})
if($declared.Count -ne 24 -or @($declared | Where-Object {$_.category -eq 'REMAINING_SELECTED_DECLARATION'}).Count -ne 12){$declared | ConvertTo-Json -Depth 4;throw 'wrapper coverage classification mismatch'}
[ordered]@{search='Case-insensitive function and direct FetchContent searches throughout Code/Gems/cmake owning sources; tests and local provider candidates excluded';declarations=$declared;counts=[ordered]@{total=24;provisioned=10;remaining=12;excluded=2};transitives='TinyUSDZ13th source found in Assimp USD branch; this inventory is not total SBOM/legal/engine acceptance';rootNative='Root48 stopped at Assimp; later Gem composition is inferred rather than configure evidence'} | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $out 'complete-wrapper-inventory.json') -Encoding utf8
$rows=@(Get-Content -LiteralPath (Join-Path $out 'destination-pin-audit.json') -Raw | ConvertFrom-Json)+@(Get-Content -LiteralPath (Join-Path $out 'assimp-destination-pin-audit.json') -Raw | ConvertFrom-Json)+@(Get-Content -LiteralPath (Join-Path $out 'tinyusdz-destination-pin-audit.json') -Raw | ConvertFrom-Json)
$primary=@(Get-Content -LiteralPath (Join-Path $out 'primary-text-manifest-rev4.json') -Raw | ConvertFrom-Json)+@(Get-Content -LiteralPath (Join-Path $out 'tinyusdz-primary-text-manifest.json') -Raw | ConvertFrom-Json)
$licensePaths=@{
    xxhash=@('LICENSE');D3D12MemoryAllocator=@('LICENSE.txt','NOTICES.txt');meshoptimizer=@('LICENSE.md');'squish-ccr'=@('README','squish.h');sse2neon=@('LICENSE');mikktspace=@('mikktspace.h');ogg=@('COPYING');vorbis=@('COPYING');miniaudio=@('LICENSE');'v-hacd'=@('LICENSE');RecastNavigation=@('License.txt');assimp=@('LICENSE');tinyusdz_repo=@('LICENSE')
}
$behavior=@{
    xxhash='NO_ADD_SUBDIR; original wrapper compiles xxhash.c and copies xxhash.h; upstream tooling inactive'
    meshoptimizer='NO_ADD_SUBDIR; original static source filelist/header; upstream gltfpack/Basis/WebP branches inactive'
    'squish-ccr'='NO_ADD_SUBDIR; original static CCR/CPP/SSE2 definitions and local squish-ccr.patch; use prepatched private work copy. Immutable snapshot never patched'
    sse2neon='NO_ADD_SUBDIR; HEADERONLY, unconditional squish wrapper dependency even x64; preserve API and original licensing'
    mikktspace='Root primary tree has no CMakeLists.txt; original wrapper owns static mikktspace.c/header; no upstream configuration'
    'v-hacd'='NO_ADD_SUBDIR; original HEADERONLY VHACD.h for PhysX editor decomposition; app/submodules not consumed'
    D3D12MemoryAllocator='Upstream root/src CMake: static default, sample/docs OFF defaults, SDK d3d12/dxgi/dxguid only, generated config in binary; optional Agility path empty default, no acquisition'
    ogg='Static upstream; tests/docs/framework/pkgconfig disabled by wrapper; config_types.h output is binary relative path; no FetchContent/Git/network branch in selected closure'
    vorbis='Static upstream root/lib; find Ogg resolves original target first; tests/framework disabled. Original vorbis/vorbisfile/vorbisenc graph retained'
    miniaudio='Original wrapper provides Ogg/Vorbis before upstream; tests/examples OFF, MA_COINIT_VALUE=2. Existing vorbisfile prevents external fallback. Optional opus/SteamAudio system searches do not download, but require cache inspection for ambient target contamination'
    RecastNavigation='Static upstream five libraries; demo/tests/examples OFF and DT_POLYREF64 ON by wrapper; config/version outputs binary. DetourCrowd dependency retained'
    assimp='Original static USD ON; tools/tests/shared/Hunter/install/zlib build OFF; ZLIB target required. Private copy preapplies tinyusd-include.patch. USD FetchContent depends on separate tinyusdz_repo; VRML/Hunter/Draco defaults OFF'
    tinyusdz_repo='Official old Git pin now in renamed LightUSD. Apache2 main source + bundled notices. CMake root subproject defaults tests/examples/Python OFF; no active FetchContent/Git/download/execute_process. Assimp manually compiles 46 core +4 dependency paths from hardcoded _deps directory; upstream EXCLUDE_FROM_ALL targets alone do not change that path'
}
$items=@(foreach($row in $rows){
    $notices=@(foreach($sourcePath in $licensePaths[$row.name]){
        $hits=@($primary.files | Where-Object {$_.name -eq $row.name -and $_.sourcePath -eq $sourcePath})
        if($hits.Count -ne 1){throw "primary license mapping ambiguous: $($row.name)/$sourcePath"}
        $hit=$hits[0]
        if((Get-FileHash -LiteralPath $hit.path -Algorithm SHA256).Hash -ne $hit.sha256){throw "primary text changed: $($hit.path)"}
        $hit
    })
    [pscustomobject]@{name=$row.name;class=$(if($row.name -eq 'tinyusdz_repo'){'TRANSITIVE_USD'}else{'TOP_LEVEL'});versionPin=$row.expectedGitCommit;originalURL=$(if($row.name -eq 'tinyusdz_repo'){'https://github.com/lighttransport/tinyusdz'}else{$row.sourceURL});archiveCandidateURL=$row.sourceURL;directArchiveURL=$row.directArchiveURL;archiveSHA256=$row.expectedArchiveSHA256;archiveSHAStatus=$(if($row.expectedArchiveSHA256){'Original declared archive hash, not yet GET-verified'}else{'No original archive hash; approval must explicitly allow measuring new archive and verifying every source Git blob against exact commit'});commitCorroborated=$row.commitCorroborated;tagPinMatched=$row.tagPinMatched;license=$(if($row.name -eq 'tinyusdz_repo'){'Apache-2.0 main; bundled third-party licenses retained separately'}else{$row.licenseDeclared});primaryNotices=$notices;consumer=$row.consumer;selectedBehavior=$behavior[$row.name];overrideVariable=('FETCHCONTENT_SOURCE_DIR_'+$row.name.ToUpperInvariant());mutableSourceRequired=($row.name -in @('assimp','squish-ccr','tinyusdz_repo'));archiveAcquired=$false;tarPathTypeLinkCollisionAudit='PENDING acquisition; metadata is not a safe-extraction witness'}
})
$assimpPatch=Join-Path $repo 'Code/Tools/SceneAPI/SDKWrapper/3rdParty/tinyusd-include.patch'
$squishPatch=Join-Path $repo 'Code/3rdParty/squish-ccr/squish-ccr.patch'
$patches=@($assimpPatch,$squishPatch) | ForEach-Object {[pscustomobject]@{path=$_;bytes=(Get-Item -LiteralPath $_).Length;sha256=(Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant()}}
$plan=[ordered]@{
    status='READ-ONLY BATCH PREPARATION READY FOR PARENT REVIEW; NO ARCHIVE ACQUISITION/AUTHORIZATION INFERRED'
    directDeclarations=[ordered]@{total=24;alreadyProvisioned=10;remaining=12;excluded=2}
    transitiveSelectedSources=1
    items=$items
    groups=@(
        [pscustomobject]@{name='Original wrapper-owned source/header libraries';sources=@('xxhash','meshoptimizer','squish-ccr','sse2neon','mikktspace','v-hacd');dependencyEdges=@('squish-ccr→sse2neon');preparation='Exact primary archives/hash/Git blobs, safe immutable snapshots; only squish gets separately verified prepatched private copy'}
        [pscustomobject]@{name='Original Windows/Gem upstream CMake';sources=@('D3D12MemoryAllocator','ogg','vorbis','miniaudio','RecastNavigation');dependencyEdges=@('miniaudio→vorbis→ogg');preparation='Exact primary archives/hash/Git blobs, safe immutable snapshots; preserve all existing flags/targets and full licensing. No new system tool/install/network path'}
        [pscustomobject]@{name='Original SceneAPI importer + selected USD';sources=@('assimp','tinyusdz_repo');dependencyEdges=@('assimp→ZLIB already local','assimp USD→tinyusdz_repo');preparation='Exact Assimp archive hash; separately measured canonical primary TinyUSD archive verified by commit/blob tree. Parent-reviewed byteexact patches on new private copies, no Git/Python patch tool execution'}
    )
    patches=$patches
    sourcePreparation=[ordered]@{
        immutable='New F:/JericoEngine/Dependencies snapshots, archive receipt/tar audit/Git blob/full extraction SHA before use; never mutate existing snapshots'
        assimp='Private source under build; preapply original tinyusd-include.patch to contrib/tinyusdz/patches/tinyusdz.patch. Preserve before/after source file hashes and unique exact hunks'
        tinyusdz='Private source must be F:/JericoEngine/JericoEngine/build/n0-validation/original-engine/_deps/tinyusdz_repo-src unless parent authorizes a code path adaptation. SOURCE_DIR_TINYUSDZ_REPO points to exactly that path; preapply resulting original Assimp tinyusdz.patch to this private source. Standard SOURCE_DIR overrides bypass population/Git/PATCH_COMMAND; runtime test still pending'
        squish='Private source under build preapplies original squish-ccr.patch; corresponding exact SOURCE_DIR_SQUISH-CCR override bypasses inherited legacy Python PATCH_COMMAND. All immutable source/patch notices preserved'
        assimpGit='Proposed supported wrapper-local ASSIMP_IGNORE_GIT_HASH ON before MakeAvailable, plus GIT_COMMIT_HASH e0b52347 and GIT_BRANCH archive if parent wants accurate generated revision metadata. Upstream CMake259–281 proves guard skips two Git calls and uses provided hash; no wrapper edit performed'
        provenance='Existing exact source override path types/ancestor guards, Root before/after tree witnesses, pristine private-source preflight and measured archive hashes retained in subsequent derivation; no broad provider/fallback'
    }
    sourceBehaviorLimits=@('No archive bytes acquired: twelve declared hashes remain unverified until GET; TinyUSD archive has no original checksum','Optional miniaudio opus/SteamAudio ambient searches are not network fetches; current Root has not reached them; block unknown ambient deps rather than acquiring extras','All archives may contain inactive tests/docs/models/resources; safe tar audit and legal inventory required before extraction. Assimp LICENSE explicitly excludes test/models-nonbsd from its general BSD terms','TinyUSD LICENSE is a short Apache2 notice rather than full standard text; retain it and bundled notices, and preserve an authoritative full Apache2 text for eventual redistribution','Root48 resolved CLI11 but stopped at Assimp; no later Gem/Metal composition, native build, importer/runtime or Editor acceptance implied','This is source-selected FetchContent preparation, not full dependency SBOM or closure of all prebuilt packages/tooling')
    forbidden=@('pybind11/PySide/Python product bindings','RGA while actual cache FALSE','SPIRVCross for unsupported Metal Windows path','Any AWS/S3/CloudFront/LFS/Git clone/automatic install/fallback acquisition')
    archivesAcquired=0
    buildTestAcquiredToolProcesses=0
}
$plan | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $out 'batch-acquisition-plan.json') -Encoding utf8
$inputs=@($declared)+@($patches)+@($primary.files)
$inputMap=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($input in $inputs){$path=[IO.Path]::GetFullPath($input.path);if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $input.sha256 -or (Get-Item -LiteralPath $path).Length -ne $input.bytes){throw "batch input changed: $path"};if(-not $inputMap.ContainsKey($path)){$inputMap.Add($path,[pscustomobject]@{path=$path;bytes=$input.bytes;sha256=$input.sha256.ToLowerInvariant()})}}
@($inputMap.Keys | Sort-Object | ForEach-Object {$inputMap[$_]}) | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'source-freeze-batch-preparation.json') -Encoding utf8
[pscustomobject]@{directRemaining=12;transitive=1;HEADAndCommitSources=$rows.Count;primaryTexts=@($primary.files).Count;frozenInputs=$inputMap.Count;planSHA256=(Get-FileHash -LiteralPath (Join-Path $out 'batch-acquisition-plan.json') -Algorithm SHA256).Hash;freezeSHA256=(Get-FileHash -LiteralPath (Join-Path $out 'source-freeze-batch-preparation.json') -Algorithm SHA256).Hash;archivesAcquired=0} | ConvertTo-Json
