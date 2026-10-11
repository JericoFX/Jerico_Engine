param(
    [switch]$PrepareOnly,
    [string]$SMCFreezePath,
    [string]$SMCFreezeSHA256,
    [string]$SMCParentReviewPath,
    [string]$SMCParentReviewSHA256,
    [string[]]$SMCMetadataPaths
)
$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
$base=Join-Path $repo 'build/n0-validation/local-packages'
$out=$PSScriptRoot
$prior=Join-Path $base 'root-cli11-final'
$batch=Join-Path $base 'remaining-fetchcontent-batch-acquisition'
$mp=Join-Path $base 'multiplayer-composition-audit'
$priorFreeze=Join-Path $prior 'source-freeze-root-cli11.json'
$priorAssignment=Join-Path $prior 'root-cli11-assignment.json'
$priorDriver=Join-Path $prior 'run-root-configure-cli11.ps1'
$batchFreeze=Join-Path $batch 'source-freeze-batch-acquired.json'
$draft=Join-Path $out 'run-root-configure-batch-draft-rev2.ps1'
$authorities=@{
    $priorFreeze='5134492C5F6C397BDD6C2F533BB2D5A0C6830CA7F9A4F7915684C186395C2292'
    $priorAssignment='699B4DDF5F094FC1A8834FD2FC986264DE1EE59AD05AD2341180C8038F775C53'
    $priorDriver='85C9883998921D17BA4C7DF70B185C6C686EDA5B89B9A5576DA4A2DA50F64C41'
    $batchFreeze='2B33F44E52E624C76425963C0F24E10239F34CD5E8A925693F8902BD488070C0'
    $draft='E4C25C639081E5F044FBFD37E967278357DB768948955D874CD8FDD794855DD1'
    (Join-Path $batch 'parent-batch-input-review.json')='F62224334651424208108740000A926ADBBAD6FBA33AFE5E2134DE8B3F0DC950'
    (Join-Path $batch 'Findassimp-source-freeze.json')='DE8B5D396AD888332B2039C5A8690D93E9B43BCE2EA605C5CB833A609BA91E9F'
    (Join-Path $batch 'squish-after-index-review-snippets.json')='434AB6819491F0E16F6A9094D46A8D7DE56B7772F4D4ED29D3C10A301E2E045D'
    (Join-Path $batch 'assimp/parent-Findassimp-review.json')='DFEA9E36DB58DF587FFE8F5E7F21264337412E3BEC1B8E07A1465864C4A6B5F8'
    (Join-Path $out 'driver-structure-proof.json')='754B660B4EA48BF7FDFA3BAC5BCB8FC360CA9FB69675AB56AD2DC8D70BF3B91C'
    (Join-Path $out 'parent-driver-draft-review.json')='EF3C8F3A079D4EF50D66267673778DD4BED4E88EF8FEFE0AF92A26699700D171'
    (Join-Path $mp 'parent-review.json')='67ADB8C81369FB54C5D3B5C2915A9315C075B31083617669EA7F85F2235FC580'
    (Join-Path $mp 'source-freeze-multiplayer-composition.json')='B7E8E468FED66D9C44F24E49003F1DABC89B201D40A257A571D3A203E97D03B8'
    (Join-Path $mp 'source-diff.patch')='969346A4B5459864DAE7768CE72B23781BDBB788A9B9EA84D01CED162C580482'
    (Join-Path $mp 'composition-change-receipt.json')='8D54BD2777E448EA5DF52403398E7AF9C1F398C6F7A79E64B0DC38867FFFAB59'
}
foreach($entry in $authorities.GetEnumerator()){if((Get-FileHash -LiteralPath $entry.Key).Hash -ne $entry.Value){throw "authority changed: $($entry.Key)"}}
if(-not $PrepareOnly){
    foreach($value in @($SMCFreezePath,$SMCFreezeSHA256,$SMCParentReviewPath,$SMCParentReviewSHA256)){if(-not $value){throw 'approved SMC freeze and parent receipt arguments required'}}
    if((Get-FileHash -LiteralPath $SMCFreezePath).Hash -ne $SMCFreezeSHA256 -or (Get-FileHash -LiteralPath $SMCParentReviewPath).Hash -ne $SMCParentReviewSHA256){throw 'SMC authority changed'}
}
function FullPath([string]$path){[IO.Path]::GetFullPath($(if([IO.Path]::IsPathRooted($path)){$path}else{Join-Path $repo $path}))}
$map=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$origins=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$counts=[ordered]@{}
$oldDeltas=[Collections.Generic.List[object]]::new()
function Merge([string]$origin,[object[]]$rows){
    $added=0;$duplicates=0
    foreach($row in $rows){
        $path=FullPath $row.path
        $item=Get-Item -LiteralPath $path
        if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "input link: $path"}
        $hash=(Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant()
        if($item.Length -ne $row.bytes -or $hash -ne $row.sha256){
            if($origin -ne 'reviewed-root2532' -or $path -ine (FullPath 'engine.json') -or $row.bytes -ne 3827 -or $row.sha256 -ne '0bf845bedef7f84dcf51f5ec986fe0b7ad6fefccb7b4e823c260d062240618a3' -or $item.Length -ne 3758 -or $hash -ne '605d32ccdd7dfa055560bc81ef79e743dc167318683e1e30733a507f3c582226'){throw "UNAPPROVED existing input delta: $path"}
            $oldDeltas.Add([pscustomobject]@{path=$path;beforeSHA256=$row.sha256;afterSHA256=$hash;beforeBytes=$row.bytes;afterBytes=$item.Length;reason='Only two parent-approved Multiplayer composition rows,69bytes; parent-review67ADB pinned'})
        }
        if($map.ContainsKey($path)){if($map[$path].sha256 -ne $hash -or $map[$path].bytes -ne $item.Length){throw "duplicate conflict: $path"};$duplicates++}
        else{$map.Add($path,[pscustomobject]@{path=$row.path;bytes=$item.Length;sha256=$hash});$origins.Add($path,[Collections.Generic.List[string]]::new());$added++}
        $origins[$path].Add($origin)
    }
    $counts[$origin]=[pscustomobject]@{validated=$rows.Count;added=$added;duplicates=$duplicates}
}
$root=Get-Content -LiteralPath $priorFreeze -Raw|ConvertFrom-Json
$batchInputs=Get-Content -LiteralPath $batchFreeze -Raw|ConvertFrom-Json
if($root.Count -ne 2532 -or $batchInputs.inputCount -ne 12957 -or $batchInputs.inputs.Count -ne 12957){throw 'reviewed counts changed'}
Merge 'reviewed-root2532' $root
if($oldDeltas.Count -ne 1){throw 'expected exactly one old Root delta: approved MP engine.json'}
Merge 'reviewed-batch12957' $batchInputs.inputs
$metadata=@($authorities.Keys)
$metadata+=@(Get-ChildItem -LiteralPath $mp -File|ForEach-Object FullName)
$metadata+=@((Join-Path $batch 'source-freeze-batch-acquired.json'),(Join-Path $batch 'Findassimp-source-freeze.json'),(Join-Path $batch 'squish-after-index-review-snippets.json'),(Join-Path $batch 'parent-batch-input-review.json'),(Join-Path $out 'argv61-draft.json'),$PSCommandPath)
$metaRows=@($metadata|Sort-Object -Unique|ForEach-Object {[pscustomobject]@{path=$_;bytes=(Get-Item -LiteralPath $_).Length;sha256=(Get-FileHash -LiteralPath $_).Hash.ToLowerInvariant()}})
Merge 'approved-batch-MP-driver-provenance' $metaRows
if($PrepareOnly){
    $path=Join-Path $out 'approved-base-inputs.json'
    $proof=Join-Path $out 'approved-base-merge-proof.json'
    foreach($p in @($path,$proof,(Join-Path $out 'approved-base-origin-manifest.json'))){if(Test-Path -LiteralPath $p){throw 'preparation output exists'}}
    $rows=@($map.Keys|Sort-Object|ForEach-Object{$map[$_]})
    $rows|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $path -Encoding utf8
    @($map.Keys|Sort-Object|ForEach-Object{[pscustomobject]@{absolutePath=$_;origins=$origins[$_].ToArray()}})|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $out 'approved-base-origin-manifest.json') -Encoding utf8
    [ordered]@{status='APPROVED BASE PREPARED; SMC NOT READ/HASHED/RECONCILED; NO EXECUTABLE ASSIGNMENT';counts=$counts;rawInputCount=(($counts.Values|Measure-Object validated -Sum).Sum);uniqueInputCount=$map.Count;oldRootDeltas=$oldDeltas.ToArray();normalization='GetFullPath+OrdinalIgnoreCase dedup; retains first row spelling for legacySQLite relative-path guards';baseFreezeSHA256=(Get-FileHash -LiteralPath $path).Hash;pending='Parent-reviewed SMC actual source/reference counts, hash and receipt required; finalize command must revalidate all inputs'}|ConvertTo-Json -Depth 7|Set-Content -LiteralPath $proof -Encoding utf8
    [pscustomobject]@{baseInputs=$map.Count;oldRootDeltas=$oldDeltas.Count;baseFreezeSHA256=(Get-FileHash -LiteralPath $path).Hash;proofSHA256=(Get-FileHash -LiteralPath $proof).Hash;SMCFrozen=$false;nativeExecuted=$false}|ConvertTo-Json
    exit 0
}
$smc=Get-Content -LiteralPath $SMCFreezePath -Raw|ConvertFrom-Json
if(-not $smc.files.Count -or -not $smc.referenceInputs.Count){throw 'approved SMC freeze missing sources/references'}
$smcSourceCount=$smc.files.Count
$smcReferenceCount=$smc.referenceInputs.Count
Merge ('parent-reviewed-SMC'+$smcSourceCount+'sources') $smc.files
Merge ('parent-reviewed-SMC'+$smcReferenceCount+'references') $smc.referenceInputs
$smcDirectory=Split-Path -Parent $SMCFreezePath
if(-not $SMCMetadataPaths.Count){throw 'exact parent-approved SMC provenance paths required'}
$smcMetadata=@($SMCFreezePath,$SMCParentReviewPath)+$SMCMetadataPaths
$smcMetadata+=@((Join-Path $out 'approved-base-inputs.json'),(Join-Path $out 'approved-base-merge-proof.json'),(Join-Path $out 'approved-base-origin-manifest.json'))
$smcRows=@($smcMetadata|Sort-Object -Unique|ForEach-Object{[pscustomobject]@{path=$_;bytes=(Get-Item -LiteralPath $_).Length;sha256=(Get-FileHash -LiteralPath $_).Hash.ToLowerInvariant()}})
Merge 'SMC-authority-and-base-preparation' $smcRows
$freezePath=Join-Path $out 'source-freeze-root-batch.json'
$assignmentPath=Join-Path $out 'root-batch-assignment.json'
$driverPath=Join-Path $out 'run-root-configure-batch.ps1'
foreach($path in @($freezePath,$assignmentPath,$driverPath,(Join-Path $out 'attempt1'))){if(Test-Path -LiteralPath $path){throw "fresh final output exists: $path"}}
$rows=@($map.Keys|Sort-Object|ForEach-Object{$map[$_]})
$rows|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $freezePath -Encoding utf8
$assignment=Get-Content -LiteralPath $priorAssignment -Raw|ConvertFrom-Json
$argsDraft=Get-Content -LiteralPath (Join-Path $out 'argv61-draft.json') -Raw|ConvertFrom-Json
if($argsDraft.arguments.Count -ne 61){throw '61args required'}
for($i=0;$i -lt 48;$i++){if($assignment.arguments[$i] -cne $argsDraft.arguments[$i]){throw 'old48prefix differs'}}
$assignment.arguments=@($argsDraft.arguments)
$assignment.sourceFreeze=$freezePath.Replace('\','/')
$assignment.sourceFreezeSHA256=(Get-FileHash -LiteralPath $freezePath).Hash
$assignment.sourceCount=$map.Count
$assignment.purpose='Original offline Root61: exact prior48argv plus12selectedtop-level sources and1TinyUSDZUSD transitive,3faithfully patched private baselines; parent-reviewed MP composition and native SMC sources explicit. Native acceptance pending.'
$assignment.preparationStatus='FINAL CANDIDATE FOR PARENT REVIEW; NOT DISPATCHED OR EXECUTED'
$assignment|Add-Member -NotePropertyName priorRoot2532FreezeSHA256 -NotePropertyValue $authorities[$priorFreeze]
$assignment|Add-Member -NotePropertyName batchInputReviewSHA256 -NotePropertyValue $authorities[(Join-Path $batch 'parent-batch-input-review.json')]
$assignment|Add-Member -NotePropertyName multiplayerCompositionSHA256 -NotePropertyValue $authorities[(Join-Path $mp 'parent-review.json')]
$assignment|Add-Member -NotePropertyName nativeSMCFreezeSHA256 -NotePropertyValue $SMCFreezeSHA256
$assignment|Add-Member -NotePropertyName nativeSMCParentReviewSHA256 -NotePropertyValue $SMCParentReviewSHA256
$assignment|Add-Member -NotePropertyName batchPrivateBaselineReceipts -NotePropertyValue @('squish-ccr','assimp','tinyusdz_repo')
$assignment|ConvertTo-Json -Depth 12|Set-Content -LiteralPath $assignmentPath -Encoding utf8
$driver=[IO.File]::ReadAllText($draft)
$finalChanges=@(
    @{before="throw 'DRAFT ONLY: pending approved SMC source freeze and final assignment hashes; no execution allowed'`n";after=''},
    @{before='PENDING_FINAL_ASSIGNMENT_SHA256';after=(Get-FileHash -LiteralPath $assignmentPath).Hash},
    @{before='PENDING_FINAL_FREEZE_SHA256';after=(Get-FileHash -LiteralPath $freezePath).Hash},
    @{before='$arguments.Count -ne 61 -or $freeze.Count -ne 0';after="`$arguments.Count -ne 61 -or `$freeze.Count -ne $($map.Count)"}
)
foreach($change in $finalChanges){if([regex]::Matches($driver,[regex]::Escape($change.before)).Count -ne 1){throw 'final driver placeholder not unique'};$driver=$driver.Replace($change.before,$change.after)}
[IO.File]::WriteAllText($driverPath,$driver,[Text.UTF8Encoding]::new($false))
$tokens=$null;$errors=$null;[Management.Automation.Language.Parser]::ParseFile($driverPath,[ref]$tokens,[ref]$errors)|Out-Null
if($errors.Count){throw ($errors.Message -join '; ')}
$restored=$driver
for($i=$finalChanges.Count-1;$i -ge 0;$i--){$change=$finalChanges[$i];if($change.after -eq ''){$restored=$change.before+$restored}else{$restored=$restored.Replace($change.after,$change.before)}}
if($restored -cne [IO.File]::ReadAllText($draft)){throw 'inverse final to draft differs'}
$draftProof=Get-Content -LiteralPath (Join-Path $out 'driver-structure-proof.json') -Raw|ConvertFrom-Json
for($i=$draftProof.driverReplacements.Count-1;$i -ge 0;$i--){$c=$draftProof.driverReplacements[$i];$restored=$restored.Replace($c.after,$c.before)}
if($restored -cne [IO.File]::ReadAllText($priorDriver)){throw 'inverse final to Root48 differs'}
[ordered]@{status='FINAL ROOT61 CANDIDATE; PARENT REVIEW/DISPATCH PENDING; NATIVE UNEXECUTED';counts=$counts;rawInputCount=(($counts.Values|Measure-Object validated -Sum).Sum);uniqueInputCount=$map.Count;dedupCount=(($counts.Values|Measure-Object duplicates -Sum).Sum);oldRootDeltas=$oldDeltas.ToArray();SMCSourceCount=$smcSourceCount;SMCReferenceCount=$smcReferenceCount;argumentCount=61;prior48PrefixExact=$true;newTrees=16;newPatchedBaselines=3;cacheFileCount=32;wheelhouseCount=44;packagePairs=8;ASTErrors=0;inverseToRoot48DriverByteExact=$true;driverFinalChanges=$finalChanges;draftDriverReplacements=$draftProof.driverReplacements;sourceFreezeSHA256=(Get-FileHash -LiteralPath $freezePath).Hash;assignmentSHA256=(Get-FileHash -LiteralPath $assignmentPath).Hash;driverSHA256=(Get-FileHash -LiteralPath $driverPath).Hash;sourceFreezeSMCSHA256=$SMCFreezeSHA256;parentSMCReviewSHA256=$SMCParentReviewSHA256;nativeExecuted=$false}|ConvertTo-Json -Depth 10|Set-Content -LiteralPath (Join-Path $out 'final-derivation-proof.json') -Encoding utf8
@($map.Keys|Sort-Object|ForEach-Object{[pscustomobject]@{absolutePath=$_;origins=$origins[$_].ToArray()}})|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $out 'final-merge-origin-manifest.json') -Encoding utf8
[pscustomobject]@{inputCount=$map.Count;oldRootDeltas=$oldDeltas.Count;argumentCount=61;ASTErrors=0;inverseRoot48ByteExact=$true;freezeSHA256=(Get-FileHash -LiteralPath $freezePath).Hash;assignmentSHA256=(Get-FileHash -LiteralPath $assignmentPath).Hash;driverSHA256=(Get-FileHash -LiteralPath $driverPath).Hash}|ConvertTo-Json
