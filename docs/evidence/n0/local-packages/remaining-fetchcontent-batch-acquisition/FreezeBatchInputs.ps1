$ErrorActionPreference='Stop'
$base=$PSScriptRoot
$planPath='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/remaining-fetchcontent-batch-audit/batch-acquisition-plan.json'
$plan=Get-Content -LiteralPath $planPath -Raw|ConvertFrom-Json
$initial=Get-Content -LiteralPath (Join-Path $base 'acquisition-summary.json') -Raw|ConvertFrom-Json
$inputs=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
function AddExpected([string]$path,[long]$bytes,[string]$sha){
    $full=[IO.Path]::GetFullPath($path)
    if(-not (Test-Path -LiteralPath $full -PathType Leaf)){throw "missing input: $full"}
    $item=Get-Item -LiteralPath $full
    if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'input link'}
    $actual=(Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash.ToLowerInvariant()
    if($item.Length -ne $bytes -or $actual -ne $sha.ToLowerInvariant()){throw "input differs from assigned bytes: $full"}
    if($inputs.ContainsKey($full)){if($inputs[$full].sha256 -ne $actual){throw 'dedup differing hash'};return}
    $inputs.Add($full,[pscustomobject]@{path=$full;bytes=$item.Length;sha256=$actual})
}
$ready=[Collections.Generic.List[object]]::new();$snapshotCount=0;$snapshotBytes=0L;$privateCount=0;$archiveBytes=0L
foreach($row in $plan.items){
    $name=$row.name
    $folder=if($name -eq 'assimp'){Join-Path $base 'assimp/extraction-rev2'}else{Join-Path $base $name}
    $manifestPath=Join-Path $folder 'source-tree-manifest.json'
    $manifest=Get-Content -LiteralPath $manifestPath -Raw|ConvertFrom-Json
    $expectedArchive=if($name -eq 'tinyusdz_repo'){'5a9aa6702a1d922d9ef12f52ea7a7a4f11425fb03e210dd0fdd2c3a000d56e4b'}else{$row.archiveSHA256}
    if($manifest.archiveSHA256 -ne $expectedArchive){throw 'archive declared identity differs'}
    $archive=Get-Item -LiteralPath $manifest.archive
    AddExpected $archive.FullName $archive.Length $expectedArchive
    $archiveBytes+=$archive.Length
    $treeFiles=@(Get-ChildItem -LiteralPath $manifest.sourceRoot -File -Recurse -Force)
    if($treeFiles.Count -ne $manifest.files -or $manifest.inputs.Count -ne $manifest.files){throw 'snapshot file count differs'}
    foreach($file in $manifest.inputs){AddExpected $file.path $file.bytes $file.sha256;$snapshotBytes+=$file.bytes}
    $snapshotCount+=$manifest.files
    $overrideRoot=$manifest.sourceRoot
    $private=$null
    if($row.mutableSourceRequired){
        $patchReceiptPath=Join-Path $base "$name/private-patch-receipt.json"
        $private=Get-Content -LiteralPath $patchReceiptPath -Raw|ConvertFrom-Json
        if($private.source -ne $manifest.sourceRoot -or $private.fileCount -ne $manifest.files){throw 'private source identity differs'}
        if(@(Get-ChildItem -LiteralPath $private.privateRoot -File -Recurse -Force).Count -ne $private.fileCount){throw 'private file count differs'}
        foreach($file in $private.privateBaseline){AddExpected (Join-Path $private.privateRoot $file.relative) $file.bytes $file.sha256}
        $privateCount+=$private.fileCount
        $overrideRoot=$private.privateRoot
    }
    $ready.Add([pscustomobject]@{name=$name;archive=$manifest.archive;archiveSHA256=$expectedArchive;archiveBytes=$archive.Length;sourceRoot=$manifest.sourceRoot;sourceCommit=$manifest.commit;sourceFiles=$manifest.files;sourceBytes=(($manifest.inputs|Measure-Object bytes -Sum).Sum);gitBlobMatches=$manifest.gitBlobMatches;extractionHashMatches=$manifest.extractionHashMatches;links=0;license=$row.license;override="-D$($row.overrideVariable):PATH=$overrideRoot";privateRoot=$overrideRoot;privatePatch=if($private){$private.patch}else{$null};patchedFiles=if($private){$private.changedFiles}else{@()};nativeStatus='UNVERIFIED'})
}
$baselinePath='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/remaining-fetchcontent-batch-audit/source-freeze-batch-preparation.json'
$baseline=Get-Content -LiteralPath $baselinePath -Raw|ConvertFrom-Json
foreach($file in $baseline){
    if([IO.Path]::GetFullPath($file.path) -ieq 'F:\JericoEngine\JericoEngine\Code\Tools\SceneAPI\SDKWrapper\3rdParty\Findassimp.cmake'){
        if($file.sha256 -ne '0c20f26b27ac8ca5f100ee15f34970242bc54eda658b50c41a68b2a28cff7f17'){throw 'wrapper prior authority differs'}
        AddExpected $file.path 6773 '5834d8a3325179b0066891eab51e06243f7a0400c0bcaee8ed1b24e20f369933'
    }else{AddExpected $file.path $file.bytes $file.sha256}
}
$summaryPath=Join-Path $base 'batch-acquisition-final-summary.json'
if(Test-Path -LiteralPath $summaryPath){throw 'summary exists'}
[ordered]@{status='13 AUDITED SOURCES AND 3 EXACT-PATCH PRIVATE COPIES READY; ROOT/NATIVE UNVERIFIED';sourceCount=13;directSources=12;transitiveSources=1;immutableFiles=$snapshotCount;immutableBytes=$snapshotBytes;archiveBytes=$archiveBytes;privateFiles=$privateCount;patchedFiles=14;allPatchInverseRawByteExact=$true;archivePinRule='Original 12 archive SHA256; measured TinyUSDZ SHA256 approved independently and every2271Gitblob corroborated';assimpExportOmissionException='Exactly25 .git*/GitHub metadata paths, parent approval203512...; original tar unchanged,3116/3116 blobs retained';historicalFailuresPreserved=@('Assimp initial3141-vs3116 tar count rejection','Premature Assimp private copy71files, no patch written','Inverse auditor generic Span overload ambiguity, corrected rev2');patchIndexLimit='All14 before indices match;12/14 after indices match. Squish maths_sse.h/simd_sse.h after abbreviations stale; exacthunks+inversebyte proof and SHA256 authoritative. Original patches unchanged.';findAssimpOnlySourceDelta='Three block-local provenance settings before MakeAvailable, pinned e0b52347/archive; unchanged flags/pins/targets';rootPreparationStatus='61 argv candidate =48 original+13 exactSOURCE_DIR overrides; final driver/freeze waits approved SMC writer delta';productLimits='Source acquisition only; no compiler/CMake/test/Java/Python/Git/tool execution. NonBSD test/model source retained locally, not product deployment. Optional bindings/apps/models not enabled.';items=$ready.ToArray()}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $summaryPath -Encoding utf8
$assignment=Get-Content -LiteralPath 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/root-cli11-final/root-cli11-assignment.json' -Raw|ConvertFrom-Json
$arguments=@($assignment.arguments)+@($ready|ForEach-Object override)
if($arguments.Count -ne 61 -or @($arguments|Select-Object -First 48|Where-Object {$_ -notin $assignment.arguments}).Count){throw 'candidate argv differs'}
[ordered]@{status='DRAFT ONLY; NOT FROZEN OR DISPATCH APPROVED';arguments=$arguments;argumentCount=61;baselineArgumentCount=48;addedOverrideCount=13;pending='Approved SMC writer source delta; parent final driver/integration review';cacheFileCount=32;sourceSnapshots=13;privatePatchBaselines=3}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $base 'root61-argv-draft.json') -Encoding utf8
foreach($file in Get-ChildItem -LiteralPath $base -File -Recurse -Force){AddExpected $file.FullName $file.Length (Get-FileHash -LiteralPath $file.FullName).Hash}
foreach($path in @($planPath,$baselinePath,'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/remaining-fetchcontent-batch-audit/parent-acquisition-approval.json')){$item=Get-Item -LiteralPath $path;AddExpected $path $item.Length (Get-FileHash -LiteralPath $path).Hash}
$freezePath=Join-Path $base 'source-freeze-batch-acquired.json'
if(Test-Path -LiteralPath $freezePath){throw 'freeze exists'}
$rows=@($inputs.Values|Sort-Object path)
[ordered]@{status='FROZEN BATCH INPUTS; NO NATIVE VERIFICATION';inputCount=$rows.Count;immutableFiles=$snapshotCount;privateFiles=$privateCount;sourceWrappersAndPrimaryTexts=$baseline.Count;inputs=$rows}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $freezePath -Encoding utf8
[pscustomobject]@{sourceCount=13;immutableFiles=$snapshotCount;immutableBytes=$snapshotBytes;privateFiles=$privateCount;archiveBytes=$archiveBytes;inputCount=$rows.Count;freezeSHA256=(Get-FileHash -LiteralPath $freezePath).Hash;summarySHA256=(Get-FileHash -LiteralPath $summaryPath).Hash;draftArgumentCount=$arguments.Count}|ConvertTo-Json
