param([switch]$Execute)
$ErrorActionPreference='Stop'
if(!$Execute){'Prepared parent review and archive only; no source deletion or native execution.';return}
$repo='F:\JericoEngine\JericoEngine'
$pbr=Join-Path $repo 'build/n0-validation/native-render-builders-integration-rev1/material-pbr-preview-integration-rev1'
$out=Join-Path $pbr 'native-atomshader-ap85-attempt1'
function Pin([string]$p){$f=Get-Item -LiteralPath $p;[ordered]@{path=$f.FullName;bytes=$f.Length;sha256=(Get-FileHash -LiteralPath $f.FullName).Hash}}
function Check($p){$n=Pin $p.path;if($n.bytes -ne $p.bytes -or $n.sha256 -cne $p.sha256){throw "Pin changed: $($p.path)"}}
function Save([string]$name,$data){$path=Join-Path $out $name;if(Test-Path -LiteralPath $path){throw "Preserve existing evidence: $name"};[IO.File]::WriteAllText($path,(ConvertTo-Json -InputObject $data -Depth 20),[Text.UTF8Encoding]::new($false))}
$alive=@(Get-CimInstance Win32_Process|Where-Object{$_.Name -in @('AssetProcessor.exe','AssetProcessorBatch.exe','AssetBuilder.exe','dxc.exe','azslc.exe','Editor.exe','JericoEditor.exe','MSBuild.exe','cl.exe','cmake.exe','JENativeSdkConsumer.exe')})
if($alive.Count){throw 'Native work remains active'}
$review=Get-Content -LiteralPath "$out/four-builders-review.json" -Raw|ConvertFrom-Json
$post=Get-Content -LiteralPath "$out/postflight.json" -Raw|ConvertFrom-Json
if($post.fatal -or !$post.allFrozenInputsMatched -or !$post.focalComplete -or $post.preCount -ne 14037 -or $post.postCount -ne 14037 -or $post.authored.matched -ne 40 -or $post.ownerTextures.matched -ne 26 -or !$post.cache.valid -or $post.GemIncrement -ne 0){throw 'Incomplete frozen postflight'}
$queries=@()
foreach($which in @('before','after')){
    $path="$out/$which-database/readonly-query.json"
    $q=Get-Content -LiteralPath $path -Raw|ConvertFrom-Json
    if($q.failure -or $q.openFlags -ne 65537 -or $q.openResult -ne 0 -or $q.closeResult -ne 0 -or $q.queries.Count -ne 8){throw 'Readonly SQLite snapshot failed'}
    foreach($query in $q.queries){if(!$query.statementReadonly -or $query.prepareResult -ne 0 -or $query.stepResult -ne 101 -or $query.finalizeResult -ne 0 -or $query.error){throw 'Readonly SELECT did not finish'}}
    $queries+=Pin $path
}
foreach($phase in $post.phases){
    if($phase.ramGuardTriggered -or (Get-FileHash -LiteralPath $phase.stdout).Hash -cne $phase.stdoutSha256 -or (Get-FileHash -LiteralPath $phase.stderr).Hash -cne $phase.stderrSha256){throw 'Native phase/log mismatch'}
    if($phase.phase -ceq 'assetprocessorbatch-shader85'){if($phase.exitCode -ne 1){throw 'Preserve actual AP global exit1'}}elseif($phase.exitCode -ne 0){throw 'Query phase failed'}
}
$after=@{};foreach($row in $review.afterGlobalNonCompleted){$after[[string]$row.JobID]=$row}
if($review.beforeGlobalNonCompleted.Count -ne 347 -or $after.Count -ne 347 -or $review.globalPass){throw 'Global baseline differs'}
$allowedChanges=@('JobRunKey','LastFailLogTime','LastFailLogFile','LastLogTime','LastLogFile')
$changed=@{}
foreach($old in $review.beforeGlobalNonCompleted){
    $new=$after[[string]$old.JobID];if(!$new){throw 'Global JobID disappeared'}
    foreach($property in $old.PSObject.Properties){
        $a=ConvertTo-Json -InputObject $property.Value -Depth 6 -Compress
        $b=ConvertTo-Json -InputObject $new.($property.Name) -Depth 6 -Compress
        if($a -cne $b){if($property.Name -cnotin $allowedChanges){throw "Global identity/status changed: $($property.Name)"};if(!$changed.ContainsKey($property.Name)){$changed[$property.Name]=0};$changed[$property.Name]++}
    }
}
$expectedBuilders=@{'Shader Asset'='C94DA15182BC447586FAE6C92A0BD6F8';'HashedShaderVariantList'='D7FB0C17131B43E8BCCC408C1763E538';'Shader Variant Asset_varianttree'='C959AEC220834488AD88F61B1144535B';'Shader Variant Asset_variantbatch'='C959AEC220834488AD88F61B1144535B'}
$semantic=@();$allProducts=@()
foreach($proof in $review.proofs){
    $job=$proof.job
    if($job.Status -ne 4 -or $job.ErrorCount -ne 0 -or $job.BuilderGuid.hex -cne $expectedBuilders[$job.JobKey] -or ($proof.oldJob -and $job.JobRunKey -le $proof.oldJob.JobRunKey)){throw 'Fresh native job mismatch'}
    $log=Pin $proof.log.archive
    if($log.bytes -ne $proof.log.bytes -or $log.sha256 -cne $proof.log.sha256){throw 'Fresh log changed'}
    if($job.WarningCount -gt 0 -and ($job.JobKey -cne 'Shader Asset' -or $job.WarningCount -ne 1 -or ![IO.File]::ReadAllText($proof.log.archive).Contains('C.UTF-8'))){throw 'Unexpected warning'}
    foreach($product in $proof.outputs){
        Check $product
        $sub=([long]$product.database.SubID -band 4294967295L)
        if($sub -ne $product.uint32SubId -or $product.bytes -le 0){throw 'Positive product/SubID mismatch'}
        $name=$product.database.ProductName;$type=$product.database.AssetType.hex
        if($name.EndsWith('.azshader')){if($type -cne '823395A3D57049F499A9D820CD1DEF98' -or $sub -ne 0){throw 'Shader type/SubID mismatch'}}
        elseif($name.EndsWith('.hashedvariantlist')){if($type -cne 'D86DA375DD7745F981D52E50C24C8469' -or $sub -ne 0){throw 'List type/SubID mismatch'}}
        elseif($name.EndsWith('.hashedvariantbatch')){if($type -cne 'D86DA375DD7745F981D52E50C24C8469' -or $sub -ne 1){throw 'Batch type/SubID mismatch'}}
        elseif($name.EndsWith('.azshadervarianttree')){if($type -cne 'EBF48506F8BB4B378FACF132BF83E42D' -or $sub -ne 0){throw 'Tree type/SubID mismatch'}}
        elseif($name -match '_vulkan_([123])\.azshadervariant$'){if($type -cne '51BED81536D8410E90F01FA9FF765FBA' -or $sub -ne (2147483648L -bor [long]$Matches[1])){throw 'Vulkan variant type/SubID mismatch'}}
        else{throw 'Unexpected semantic product'}
        $semantic+=$product
    }
    $allProducts+=@($proof.allJobProducts)
}
if($review.proofs.Count -ne 4 -or $semantic.Count -ne 7 -or $allProducts.Count -ne 15){throw '4 jobs/7 semantic/15 total products required'}
Check $review.options
$options=Get-Content -LiteralPath $review.options.path -Raw|ConvertFrom-Json
if($options.specializationConstants -isnot [bool] -or $options.specializationConstants -or $review.options.database.AssetType.hex -cne '6977AEB117AD4992957B23BB2E85B18B' -or ([long]$review.options.database.SubID -band 4294967295L) -ne 2147483654L){throw 'Options metadata must be typed explicit false'}
$modules=Get-Content -LiteralPath "$out/native-module-observation.json" -Raw|ConvertFrom-Json
if($modules.observations.Count -ne 3 -or @($modules.observations|Where-Object{$_.error -or $_.legacyBuilderObserved -or $_.nativeActivation -cne 'BOTH_NATIVE_DLLS_OBSERVED_AND_PINNED'}).Count){throw 'Native module observations incomplete'}
$freezePath=Join-Path $pbr 'atomshader-native85-ap-preparation-rev1/freeze85-positive-variants.json'
if((Get-FileHash -LiteralPath $freezePath).Hash -cne $post.freezeSHA256){throw 'Source85 freeze changed'}
$freeze=Get-Content -LiteralPath $freezePath -Raw|ConvertFrom-Json
foreach($pin in $freeze.inputs){Check $pin}
$archive=Join-Path $out 'selected-products'
if(Test-Path -LiteralPath $archive){throw 'Preserve existing product archive'}
[void][IO.Directory]::CreateDirectory($archive)
$manifest=@()
foreach($database in $allProducts){
    if($database.ProductName -cnotmatch '^(pc|common)/assets/tests/jenativeshader85/[^/]+$'){throw 'Product outside exact fixture scope'}
    $known=@($semantic|Where-Object{$_.database.ProductID -eq $database.ProductID})
    $physical=if($known.Count -eq 1){$known[0].path}else{Join-Path "$repo/Projects/JEEditorFixture/Cache" $database.ProductName}
    $origin=Pin $physical;if($origin.bytes -le 0){throw 'Empty native product'}
    $destination=[IO.Path]::GetFullPath((Join-Path $archive $database.ProductName))
    if(!$destination.StartsWith($archive+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Archive escaped owned root'}
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination))
    [IO.File]::Copy($physical,$destination,$false)
    $copy=Pin $destination;if($copy.sha256 -cne $origin.sha256){throw 'Archive copy mismatch'}
    $manifest+=@{database=$database;uint32SubId=([long]$database.SubID -band 4294967295L);physical=$origin;archive=$copy}
}
Save 'parent-products85.json' $manifest
Save 'parent-review85.json' ([ordered]@{approved=$true;reviewer='/root';utc=[DateTime]::UtcNow.ToString('o');scope='Focal positive Vulkan shader/list/tree/batch products only; precompiled builder and exact PreLoad edge reused separately from84';postflight=(Pin "$out/postflight.json");proofs=(Pin "$out/four-builders-review.json");readonlyQueries=$queries;nativeJobs=4;builderUUIDs=3;mandatorySemanticProducts=7;allPhysicalProducts=15;optionsExplicitBooleanFalse=$true;productArchive=(Pin "$out/parent-products85.json");allFrozenInputsRehashed=14037;authoredPreserved=40;ownerTexturesPreserved=26;moduleCaptures=3;rawGlobalExit=1;globalNonCompletedBefore=347;globalNonCompletedAfter=347;globalIdentityStatusErrorsWarningsUnchanged=$true;actualRetriedGlobalLogChanges=$changed;globalAPPass=$false;wholeGemAccepted=$false;GemIncrement=0;limitations=@('No AssetHandler Ready/runtime/GPU/UI/VR/Linux/Shipping acceptance','CPU native811 unchanged and SDK83 verified separately','Copy job84 has no guaranteed success log; proven policy plus GUID/SubID/type/dependency/physical bytes used')})
$os=Get-CimInstance Win32_OperatingSystem
Save 'parent-final-native-census85.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');nativeProcesses=@($alive);count=$alive.Count;usedPercent=[math]::Round((1-$os.FreePhysicalMemory/$os.TotalVisibleMemorySize)*100,2);freeGiB=[math]::Round($os.FreePhysicalMemory/1MB,2);taskOwnedHelpers=0;originalZeroByteCensusPreserved=(Pin "$out/final-native-census.json");noProcessTerminatedByThisReview=$true})
@((Pin "$out/parent-review85.json"),(Pin "$out/parent-products85.json"),(Pin "$out/parent-final-native-census85.json"))|ConvertTo-Json -Depth 4
