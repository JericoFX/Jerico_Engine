$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
$base=Join-Path $repo 'build/n0-validation/local-packages'
$out=$PSScriptRoot
$batch=Join-Path $base 'remaining-fetchcontent-batch-acquisition'
$prior=Join-Path $base 'root-cli11-final'
$priorDriver=Join-Path $prior 'run-root-configure-cli11.ps1'
$priorAssignment=Join-Path $prior 'root-cli11-assignment.json'
if((Get-FileHash -LiteralPath $priorDriver).Hash -ne '85C9883998921D17BA4C7DF70B185C6C686EDA5B89B9A5576DA4A2DA50F64C41'){throw 'Root48 driver changed'}
if((Get-FileHash -LiteralPath $priorAssignment).Hash -ne '699B4DDF5F094FC1A8834FD2FC986264DE1EE59AD05AD2341180C8038F775C53'){throw 'Root48 assignment changed'}
$batchFreeze=Join-Path $batch 'source-freeze-batch-acquired.json'
if((Get-FileHash -LiteralPath $batchFreeze).Hash -ne '2B33F44E52E624C76425963C0F24E10239F34CD5E8A925693F8902BD488070C0'){throw 'batch freeze changed'}
$summaryPath=Join-Path $batch 'batch-acquisition-final-summary.json'
if((Get-FileHash -LiteralPath $summaryPath).Hash -ne '9401D84015F8603E1C462EC2E0EE0A79A239957CF506123409BC1BAE45091D90'){throw 'batch summary changed'}
$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
$assignment=Get-Content -LiteralPath $priorAssignment -Raw|ConvertFrom-Json
if($assignment.arguments.Count -ne 48 -or $summary.items.Count -ne 13){throw 'counts changed'}
$script=[IO.File]::ReadAllText($priorDriver)
$changes=[Collections.Generic.List[object]]::new()
function ReplaceOnce([string]$before,[string]$after){
    if([regex]::Matches($script,[regex]::Escape($before)).Count -ne 1){throw "driver anchor is not unique: $before"}
    $script:script=$script.Replace($before,$after)
    $changes.Add([pscustomobject]@{before=$before;after=$after})
}
ReplaceOnce '$ErrorActionPreference = ''Stop''' "throw 'DRAFT ONLY: pending approved SMC source freeze and final assignment hashes; no execution allowed'`n`$ErrorActionPreference = 'Stop'"
ReplaceOnce "'build/n0-validation/local-packages/root-cli11-final'" "'build/n0-validation/local-packages/root-fetchcontent-batch-final'"
ReplaceOnce "'build/n0-validation/local-packages/root-cli11-final/root-cli11-assignment.json'" "'build/n0-validation/local-packages/root-fetchcontent-batch-final/root-batch-assignment.json'"
ReplaceOnce "'build/n0-validation/local-packages/root-cli11-final/source-freeze-root-cli11.json'" "'build/n0-validation/local-packages/root-fetchcontent-batch-final/source-freeze-root-batch.json'"
ReplaceOnce '699B4DDF5F094FC1A8834FD2FC986264DE1EE59AD05AD2341180C8038F775C53' 'PENDING_FINAL_ASSIGNMENT_SHA256'
ReplaceOnce '5134492C5F6C397BDD6C2F533BB2D5A0C6830CA7F9A4F7915684C186395C2292' 'PENDING_FINAL_FREEZE_SHA256'
ReplaceOnce '$arguments.Count -ne 48 -or $freeze.Count -ne 2532' '$arguments.Count -ne 61 -or $freeze.Count -ne 0'
$prefix=@('$expectedPriorArguments = @(')+@($assignment.arguments|ForEach-Object {"    '$($_.Replace("'","''"))',"})+@(')','for ($index = 0; $index -lt 48; $index++) {','    if ($arguments[$index] -cne $expectedPriorArguments[$index]) { throw "prior48 argv prefix differs at $index" }','}')
$prefix[48]=$prefix[48].TrimEnd(',')
$guards=@($summary.items|ForEach-Object {"if (`$arguments -notcontains '$($_.override)') { throw 'exact local $($_.name) override missing' }"})
$anchor="if (`$arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_CLI11:PATH=F:/JericoEngine/Dependencies/cli11-2.6.2') { throw 'exact local CLI11 override missing' }"
ReplaceOnce $anchor ($anchor+"`n"+([string]::Join("`n",$prefix+$guards)))
$trees=[Collections.Generic.List[string]]::new()
foreach($item in $summary.items){
    $trees.Add("    @{name='batch-$($item.name)-original-before';path='$($item.sourceRoot.Replace('/','\'))'},")
    if($item.privatePatch){$trees.Add("    @{name='batch-$($item.name)-private-before';path='$($item.privateRoot.Replace('/','\'))'},")}
}
if($trees.Count -ne 16){throw 'batch tree count differs'}
$anchor="    @{name='cli11-tree-before';path='F:\JericoEngine\Dependencies\cli11-2.6.2'},"
ReplaceOnce $anchor ($anchor+"`n"+([string]::Join("`n",$trees)))
$patchPairs=[Collections.Generic.List[string]]::new()
foreach($item in $summary.items|Where-Object privatePatch){
    $receipt=Join-Path $batch "$($item.name)/private-patch-receipt.json"
    $sha=(Get-FileHash -LiteralPath $receipt).Hash
    $patchPairs.Add("    @{name='$($item.name)';receipt='$receipt';sha256='$sha'},")
}
$patchPairs[$patchPairs.Count-1]=$patchPairs[$patchPairs.Count-1].TrimEnd(',')
$beforeBlock=@'
$batchPatchPairs = @(
__PATCH_PAIRS__
)
function Compare-BatchPatchBaseline([string]$phase) {
    $results = @()
    foreach ($pair in $batchPatchPairs) {
        if ((Get-FileHash -LiteralPath $pair.receipt).Hash -ne $pair.sha256) { throw "assigned patch receipt differs: $($pair.name)" }
        $expected = Get-Content -LiteralPath $pair.receipt -Raw | ConvertFrom-Json
        $source = Get-Content -LiteralPath (Join-Path $evidence "batch-$($pair.name)-original-$phase.json") -Raw | ConvertFrom-Json
        $copy = Get-Content -LiteralPath (Join-Path $evidence "batch-$($pair.name)-private-$phase.json") -Raw | ConvertFrom-Json
        $errors = @()
        foreach ($check in @(@{actual=$source;expected=$expected.immutableBefore;kind='immutable'},@{actual=$copy;expected=$expected.privateBaseline;kind='patched-private'})) {
            $map = @{}; foreach ($row in $check.expected) { $map[$row.relative]=$row }
            foreach ($row in $check.actual.files) {
                $relative = $row.relativePath.Replace('\','/')
                if (-not $map.ContainsKey($relative) -or $map[$relative].sha256 -ne $row.sha256 -or $map[$relative].bytes -ne $row.bytes) { $errors += "$($check.kind):changed-or-added:$relative" }
                $map.Remove($relative)
            }
            foreach ($missing in $map.Keys) { $errors += "$($check.kind):missing:$missing" }
            if ($check.actual.fileCount -ne $expected.fileCount) { $errors += "$($check.kind):file-count-differs" }
        }
        $results += [pscustomobject]@{name=$pair.name;immutableFiles=$source.fileCount;patchedPrivateFiles=$copy.fileCount;expectedPatchedFiles=$expected.changedFiles.Count;baselineErrors=$errors}
    }
    $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence "batch-private-copy-comparison-$phase.json") -Encoding utf8
    if (@($results | Where-Object { $_.baselineErrors.Count -ne 0 }).Count) { throw "batch assigned patched baseline differs: $phase" }
}
Compare-BatchPatchBaseline 'before'
'@
$beforeBlock=$beforeBlock.Replace('__PATCH_PAIRS__',[string]::Join("`n",$patchPairs))
$anchor="if (@(`$privateComparison | Where-Object { `$_.preconfigureDifferences.Count -ne 0 }).Count) { throw 'private source copies differ from the assigned pristine source before configure' }"
ReplaceOnce $anchor ($anchor+"`n"+$beforeBlock)
$anchor="    'private-copy-comparison-before.json','accepted-native-packages-before.json'"
ReplaceOnce $anchor ($anchor+",'batch-private-copy-comparison-before.json'")
$anchor="`$privateAfter | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path `$evidence 'private-copy-comparison-after.json') -Encoding utf8"
ReplaceOnce $anchor ($anchor+"`nCompare-BatchPatchBaseline 'after'")
$restored=$script
for($i=$changes.Count-1;$i -ge 0;$i--){$change=$changes[$i];if([regex]::Matches($restored,[regex]::Escape($change.after)).Count -ne 1){throw 'inverse draft replacement not unique'};$restored=$restored.Replace($change.after,$change.before)}
if($restored -cne [IO.File]::ReadAllText($priorDriver)){throw 'inverse draft driver differs'}
$draftPath=Join-Path $out 'run-root-configure-batch-draft-rev2.ps1'
if(Test-Path -LiteralPath $draftPath){throw 'draft driver exists'}
[IO.File]::WriteAllText($draftPath,$script,[Text.UTF8Encoding]::new($false))
$tokens=$null;$errors=$null
[Management.Automation.Language.Parser]::ParseFile($draftPath,[ref]$tokens,[ref]$errors)|Out-Null
if($errors.Count){throw ($errors.Message -join '; ')}
$arguments=@($assignment.arguments)+@($summary.items|ForEach-Object override)
for($i=0;$i -lt 48;$i++){if($arguments[$i] -cne $assignment.arguments[$i]){throw 'prior argv changed'}}
[ordered]@{status='DRIVER STRUCTURE PREPARED; DRAFT THROW PREVENTS DISPATCH; NO NATIVE EXECUTION';baselineDriver=$priorDriver;baselineDriverSHA256=(Get-FileHash -LiteralPath $priorDriver).Hash;draftDriver=$draftPath;draftDriverSHA256=(Get-FileHash -LiteralPath $draftPath).Hash;driverReplacements=$changes.ToArray();inversePreviousDriverByteExact=$true;argumentCount=$arguments.Count;prior48ArgumentPrefixExact=$true;addedOverrides=13;newTrees=16;newPatchedBaselines=3;ASTErrors=0;cacheCount=32;wheelhouseCount=44;packagePairs=8;preserved='Unchanged existing version/hash probes, inherited environment redaction/removal, CMake memory/process argv/timing/native exit, all existing manifests/deltas, legacy3private pristine checks and allowed writes';pending='SMC freeze/diff + parent approval; final Root input merge and assignment/freeze literal SHA/count replacement; remove draft throw only for parent-reviewed final driver'}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $out 'driver-structure-proof.json') -Encoding utf8
[ordered]@{status='DRAFT ARGUMENTS ONLY; NOT EXECUTED';arguments=$arguments;argumentCount=$arguments.Count;baselineAssignmentSHA256=(Get-FileHash -LiteralPath $priorAssignment).Hash;prior48PrefixExact=$true}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $out 'argv61-draft.json') -Encoding utf8
[pscustomobject]@{argumentCount=$arguments.Count;newTrees=16;patchedBaselines=3;ASTErrors=0;inversePreviousDriverByteExact=$true;draftDriverSHA256=(Get-FileHash -LiteralPath $draftPath).Hash;proofSHA256=(Get-FileHash -LiteralPath (Join-Path $out 'driver-structure-proof.json')).Hash}|ConvertTo-Json
