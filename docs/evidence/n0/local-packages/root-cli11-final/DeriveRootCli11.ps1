param(
    [Parameter(Mandatory=$true)][string]$ParentApprovalReceipt,
    [Parameter(Mandatory=$true)][string]$ParentApprovalSHA256
)
$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
$base=Join-Path $repo 'build/n0-validation/local-packages'
$out=$PSScriptRoot
$prior=Join-Path $base 'root-jsoncpp-final'
$audit=Join-Path $base 'cli11-audit'
$composition=Join-Path $base 'python-product-composition-audit'
$priorFreeze=Join-Path $prior 'source-freeze-root-jsoncpp.json'
$priorAssignment=Join-Path $prior 'root-jsoncpp-assignment.json'
$priorDriver=Join-Path $prior 'run-root-configure-jsoncpp.ps1'
$compositionFreeze=Join-Path $composition 'source-freeze-python-composition-rev2.json'
$compositionBefore=Join-Path $composition 'source-before-expanded.json'
$authority=@{
    $priorFreeze='2C8496737E9D90609E47761EFDC8C7A6AF4D30BEE7BBC7BA28CDF857120526B8'
    $priorAssignment='544A7A75222108044CCBE42737569BC7173EAFE0444B153C1D02D63D099A51C2'
    $priorDriver='448ADBC86F5DCA42CB006E7C5A0DA7ADEE0BA1623E50E8ED89093BD6F88CF95A'
    $compositionFreeze='1A2CFBA5065746932558F96D8F47D4E267FB42C2F96500BCA8332295D9DF770C'
    (Join-Path $composition 'source-diff-rev2.patch')='2BF7AE14CFCB63049ADD835160154D460C731EE91ED16B4C7F59449CC0F04FE8'
    (Join-Path $composition 'composition-change-receipt-rev2.json')='C2F632FE622145DB71694AA5FF3B04419C46E49E38601AD275754854688DE52C'
    $ParentApprovalReceipt=$ParentApprovalSHA256
}
foreach($entry in $authority.GetEnumerator()){if((Get-FileHash -Algorithm SHA256 -LiteralPath $entry.Key).Hash -ne $entry.Value){throw "authority receipt changed: $($entry.Key)"}}
$baseline=Get-Content -Raw -LiteralPath $priorFreeze | ConvertFrom-Json
$assignment=Get-Content -Raw -LiteralPath $priorAssignment | ConvertFrom-Json
$tree=Get-Content -Raw -LiteralPath (Join-Path $audit 'source-tree-manifest.json') | ConvertFrom-Json
$summary=Get-Content -Raw -LiteralPath (Join-Path $audit 'audit-summary.json') | ConvertFrom-Json
$six=Get-Content -Raw -LiteralPath $compositionFreeze | ConvertFrom-Json
$beforeSix=Get-Content -Raw -LiteralPath $compositionBefore | ConvertFrom-Json
if($baseline.Count -ne 2203 -or $assignment.arguments.Count -ne 47 -or $tree.inputs.Count -ne 304 -or $six.files.Count -ne 6 -or $beforeSix.Count -ne 6){throw 'baseline/source counts changed'}
$map=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$origins=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$approved=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$deltas=[Collections.Generic.List[object]]::new()
$counts=[ordered]@{}
function FullPath([string]$path){[IO.Path]::GetFullPath($(if([IO.Path]::IsPathRooted($path)){$path}else{Join-Path $repo $path}))}
foreach($row in $six.files){
    $path=FullPath $row.path
    $before=@($beforeSix | Where-Object {(FullPath $_.path) -eq $path})
    if($before.Count -ne 1){throw "composition before/after mapping ambiguous: $path"}
    $approved.Add($path,[pscustomobject]@{before=$before[0];after=$row})
}
function Merge([string]$origin,[object[]]$rows){
    $added=0;$duplicates=0
    foreach($row in $rows){
        $path=FullPath $row.path
        $bytes=(Get-Item -LiteralPath $path).Length
        $hash=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
        if($bytes -ne $row.bytes -or $hash -ne $row.sha256){
            if($origin -ne 'reviewed-root2203' -or -not $approved.ContainsKey($path) -or $row.sha256 -ne $approved[$path].before.sha256 -or $row.bytes -ne $approved[$path].before.bytes -or $hash -ne $approved[$path].after.sha256 -or $bytes -ne $approved[$path].after.bytes){throw "UNAPPROVED delta: $path"}
            $deltas.Add([pscustomobject]@{path=$path;beforeBytes=$row.bytes;beforeSHA256=$row.sha256;afterBytes=$bytes;afterSHA256=$hash;reason='Parent-reviewed exclusion of Python Gem product composition; six-file receipt pinned'})
        }
        if($map.ContainsKey($path)){if($map[$path].bytes -ne $bytes -or $map[$path].sha256 -ne $hash){throw "duplicate conflict: $path"};$duplicates++}
        else{$map.Add($path,[pscustomobject]@{path=$row.path;bytes=$bytes;sha256=$hash});$origins.Add($path,[Collections.Generic.List[string]]::new());$added++}
        $origins[$path].Add($origin)
    }
    $counts[$origin]=[pscustomobject]@{validated=$rows.Count;added=$added;duplicates=$duplicates}
}
Merge 'reviewed-root2203' $baseline
if($deltas.Count -ne 1 -or (FullPath $deltas[0].path) -ne (FullPath 'engine.json')){throw 'expected exactly one authorized delta to existing Root inputs: engine.json'}
Merge 'parent-reviewed-python-composition6' $six.files
Merge 'cli11-snapshot304' $tree.inputs
Merge 'cli11-original-wrapper-consumers5' $summary.rootReadOnlyInputs
$metaPaths=@('source-destination-head.json','source-acquisition.json','source-tag.json','source-commit.json','source-tree.json','source-tar-audit.json','source-tree-manifest.json','audit-summary.json','remaining-fetchcontent-windows.json','engine-external-composition-inference.json') | ForEach-Object {Join-Path $audit $_}
$metaPaths += 'F:/JericoEngine/Dependencies/archives/cli11-2.6.2.tar.gz'
$metaPaths += @($compositionBefore,$compositionFreeze,(Join-Path $composition 'source-diff-rev2.patch'),(Join-Path $composition 'composition-change-receipt-rev2.json'),$ParentApprovalReceipt)
$metaPaths += Join-Path $out 'remaining-fetchcontent-preparation.json'
$metaRows=@($metaPaths | ForEach-Object {[pscustomobject]@{path=$_;bytes=(Get-Item -LiteralPath $_).Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $_).Hash.ToLowerInvariant()}})
Merge 'cli11-provenance-and-composition-receipts17' $metaRows
$freezePath=Join-Path $out 'source-freeze-root-cli11.json'
$assignmentPath=Join-Path $out 'root-cli11-assignment.json'
$driverPath=Join-Path $out 'run-root-configure-cli11.ps1'
foreach($path in @($freezePath,$assignmentPath,$driverPath,(Join-Path $out 'attempt1'))){if(Test-Path -LiteralPath $path){throw "fresh output already exists: $path"}}
$rows=@($map.Keys | Sort-Object | ForEach-Object {$map[$_]})
$rows | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $freezePath -Encoding utf8
$freezeSHA=(Get-FileHash -Algorithm SHA256 -LiteralPath $freezePath).Hash
$override='-DFETCHCONTENT_SOURCE_DIR_CLI11:PATH=F:/JericoEngine/Dependencies/cli11-2.6.2'
if(@($assignment.arguments | Where-Object {$_ -like '-DFETCHCONTENT_SOURCE_DIR_CLI11*'}).Count){throw 'CLI11 override unexpectedly exists'}
$assignment.arguments=@($assignment.arguments)+$override
$assignment.sourceFreeze=$freezePath.Replace('\','/')
$assignment.sourceFreezeSHA256=$freezeSHA
$assignment.sourceCount=$rows.Count
$assignment.purpose='Next original offline Root configure: exact prior47 argv plus pinned CLI11 source override; six parent-reviewed Python composition inputs explicit. No native acceptance inferred.'
$assignment.preparationStatus='READY FOR PARENT REVIEW/DISPATCH; NOT EXECUTED'
$assignment | Add-Member -NotePropertyName previousRoot2203FreezeSHA256 -NotePropertyValue $authority[$priorFreeze]
$assignment | Add-Member -NotePropertyName cli11SourceRoot -NotePropertyValue 'F:/JericoEngine/Dependencies/cli11-2.6.2'
$assignment | Add-Member -NotePropertyName cli11ArchiveSHA256 -NotePropertyValue 'c6ea6b2e5608b3ea8617999bd5f47420c71b2ebdb8dc4767c1034d1da5785711'
$assignment | Add-Member -NotePropertyName pythonCompositionReceipt -NotePropertyValue $ParentApprovalReceipt
$assignment | Add-Member -NotePropertyName currentCompositionDeltas -NotePropertyValue $deltas.ToArray()
$assignment | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $assignmentPath -Encoding utf8
$assignmentSHA=(Get-FileHash -Algorithm SHA256 -LiteralPath $assignmentPath).Hash
$script=[IO.File]::ReadAllText($priorDriver)
$changes=[Collections.Generic.List[object]]::new()
function ReplaceOnce([string]$before,[string]$after){if([regex]::Matches($script:script,[regex]::Escape($before)).Count -ne 1){throw "driver replacement ambiguous: $before"};$script:script=$script:script.Replace($before,$after);$changes.Add([pscustomobject]@{before=$before;after=$after})}
ReplaceOnce "'build/n0-validation/local-packages/root-jsoncpp-final'" "'build/n0-validation/local-packages/root-cli11-final'"
ReplaceOnce "'build/n0-validation/local-packages/root-jsoncpp-final/root-jsoncpp-assignment.json'" "'build/n0-validation/local-packages/root-cli11-final/root-cli11-assignment.json'"
ReplaceOnce "'build/n0-validation/local-packages/root-jsoncpp-final/source-freeze-root-jsoncpp.json'" "'build/n0-validation/local-packages/root-cli11-final/source-freeze-root-cli11.json'"
ReplaceOnce $authority[$priorAssignment] $assignmentSHA
ReplaceOnce $authority[$priorFreeze] $freezeSHA
ReplaceOnce '$arguments.Count -ne 47 -or $freeze.Count -ne 2203' "`$arguments.Count -ne 48 -or `$freeze.Count -ne $($rows.Count)"
$anchor='if ($arguments -notcontains ''-DFETCHCONTENT_SOURCE_DIR_JSONCPP:PATH=F:/JericoEngine/Dependencies/jsoncpp-1.6.2'') { throw ''exact local JSONCpp override missing'' }'
ReplaceOnce $anchor ($anchor+"`nif (`$arguments -notcontains '$override') { throw 'exact local CLI11 override missing' }")
$treeAnchor="    @{name='jsoncpp-tree-before';path='F:\JericoEngine\Dependencies\jsoncpp-1.6.2'},"
ReplaceOnce $treeAnchor ($treeAnchor+"`n    @{name='cli11-tree-before';path='F:\JericoEngine\Dependencies\cli11-2.6.2'},")
[IO.File]::WriteAllText($driverPath,$script,[Text.UTF8Encoding]::new($false))
$rawCount=2203+6+304+5+$metaRows.Count
$sixChanges=@($approved.Keys | Sort-Object | ForEach-Object {[pscustomobject]@{path=$_;before=$approved[$_].before;after=$approved[$_].after}})
$proof=[ordered]@{status='READY FOR PARENT REVIEW; NO NATIVE EXECUTION';sourceGroups=$counts;rawInputCount=$rawCount;uniqueInputCount=$rows.Count;deduplicatedCount=($rawCount-$rows.Count);approvedDeltas=$deltas.ToArray();approvedCompositionChanges=$sixChanges;parentApprovalReceipt=$ParentApprovalReceipt;parentApprovalSHA256=$ParentApprovalSHA256;argumentsCount=48;cacheCount=32;wheelhouseCount=44;acceptedPackagePairs=8;priorDriverSHA256=$authority[$priorDriver];driverSHA256=(Get-FileHash -Algorithm SHA256 -LiteralPath $driverPath).Hash;assignmentSHA256=$assignmentSHA;freezeSHA256=$freezeSHA;driverChanges=$changes.ToArray();preserved='All instrumentation, offline/process environment guards, cache32/packagePairs8, exact prior47 argv, private-source preflight, existing snapshot before/after and postflight. Adds only CLI11 argv guard/full immutable tree.';unverified='No Root/native build run; removal of Python Gem composition and Metal composition not yet configured or runtime-validated'}
$proof | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $out 'derivation-proof.json') -Encoding utf8
$changes | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'driver-diff.json') -Encoding utf8
@($map.Keys | Sort-Object | ForEach-Object {[pscustomobject]@{fullPath=$_;origins=$origins[$_].ToArray()}}) | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'merge-origin-manifest.json') -Encoding utf8
[pscustomobject]$proof | Select-Object status,sourceGroups,uniqueInputCount,deduplicatedCount,approvedDeltas,driverSHA256,assignmentSHA256,freezeSHA256 | ConvertTo-Json -Depth 7
