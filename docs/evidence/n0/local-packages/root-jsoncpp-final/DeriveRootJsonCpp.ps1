$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
$base=Join-Path $repo 'build/n0-validation/local-packages'
$out=$PSScriptRoot
$prior=Join-Path $base 'root-antlr-openexr-final'
$audit=Join-Path $base 'jsoncpp-audit'
$priorFreeze=Join-Path $prior 'source-freeze-root-final.json'
$priorAssignment=Join-Path $prior 'root-antlr-openexr-assignment.json'
$priorDriver=Join-Path $prior 'run-root-configure-antlr-openexr.ps1'
$authority=@{
    $priorFreeze='36CED3BAE28693DCAD99F89356B5ED2C72657C5B6CFE1A56DA6AE0A0A83D532E'
    $priorAssignment='2BBE66DB2F75D8D223BBFC61F6CD8E1A9A26C7219D2812301046078EA195DCCF'
    $priorDriver='5FD4EDAEC14E30405C612ECEC36FE9AF38A1DF7A562413C535EC196FB866FA6E'
    (Join-Path $base 'openexr-audit/whitespace-normalization-parent.json')='39928EEA8755CAFA410749F482D8D0F66B11876468052E07A2E150F4AE80153F'
}
foreach($entry in $authority.GetEnumerator()){if((Get-FileHash -Algorithm SHA256 -LiteralPath $entry.Key).Hash -ne $entry.Value){throw "authority receipt changed: $($entry.Key)"}}
$baseline=Get-Content -Raw -LiteralPath $priorFreeze | ConvertFrom-Json
$assignment=Get-Content -Raw -LiteralPath $priorAssignment | ConvertFrom-Json
$tree=Get-Content -Raw -LiteralPath (Join-Path $audit 'source-tree-manifest.json') | ConvertFrom-Json
$summary=Get-Content -Raw -LiteralPath (Join-Path $audit 'audit-summary.json') | ConvertFrom-Json
if($baseline.Count -ne 1953 -or $assignment.arguments.Count -ne 46 -or $tree.inputs.Count -ne 229){throw 'baseline/source counts changed'}
$approved=@{
    'cmake/Tools/LocalPackages/OpenEXR/FindImath.cmake'=@{before='0837b72ef5853db21e7543a7ca40bd00b428b7f36b9fd7143921b7685016dd9f';after='8d6943660b969394da20ce1467f7a0479b5da0eb1679b3db99d882c974637328';bytes=1718}
    'cmake/Tools/LocalPackages/OpenEXR/FindOpenEXR.cmake'=@{before='8574ecc7d2a4381cb2152939181c9a1aafeafcb76862aedf0dfd81e94d79a899';after='767b82230223b555ad7458b20fccf0a0b29ac1adddb092096df01b07c1ee0f2c';bytes=3595}
}
$map=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$origins=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$deltas=[Collections.Generic.List[object]]::new()
$counts=[ordered]@{}
function FullPath([string]$path){[IO.Path]::GetFullPath($(if([IO.Path]::IsPathRooted($path)){$path}else{Join-Path $repo $path}))}
function Merge([string]$origin,[object[]]$rows){
    $added=0;$duplicates=0
    foreach($row in $rows){
        $path=FullPath $row.path
        $bytes=(Get-Item -LiteralPath $path).Length
        $hash=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
        if($bytes -ne $row.bytes -or $hash -ne $row.sha256){
            $relative=$path.Substring([IO.Path]::GetFullPath($repo).Length).TrimStart('\').Replace('\','/')
            if($origin -ne 'reviewed-root1953' -or -not $approved.ContainsKey($relative) -or $row.sha256 -ne $approved[$relative].before -or $hash -ne $approved[$relative].after -or $bytes -ne $approved[$relative].bytes){throw "UNAPPROVED delta: $path"}
            $deltas.Add([pscustomobject]@{path=$path;beforeBytes=$row.bytes;beforeSHA256=$row.sha256;afterBytes=$bytes;afterSHA256=$hash;reason='Parent removed trailing spaces only; normalization receipt pinned'})
        }
        if($map.ContainsKey($path)){if($map[$path].bytes -ne $bytes -or $map[$path].sha256 -ne $hash){throw "duplicate conflict: $path"};$duplicates++}
        else{$map.Add($path,[pscustomobject]@{path=$row.path;bytes=$bytes;sha256=$hash});$origins.Add($path,[Collections.Generic.List[string]]::new());$added++}
        $origins[$path].Add($origin)
    }
    $counts[$origin]=[pscustomobject]@{validated=$rows.Count;added=$added;duplicates=$duplicates}
}
Merge 'reviewed-root1953' $baseline
if($deltas.Count -ne 2){throw 'expected exactly two authorized Find module whitespace deltas'}
Merge 'jsoncpp-snapshot229' $tree.inputs
Merge 'jsoncpp-original-wrapper-consumers10' $summary.rootReadOnlyInputs
$metaPaths=@('source-destination-head.json','source-acquisition.json','source-tag.json','source-commit.json','source-tree.json','source-tar-audit.json','source-tree-manifest.json','audit-summary.json','CheckAzslcReflection.ps1') | ForEach-Object {Join-Path $audit $_}
$metaPaths += 'F:/JericoEngine/Dependencies/archives/jsoncpp-1.6.2.tar.gz'
$metaPaths += Join-Path $base 'openexr-audit/whitespace-normalization-parent.json'
$metaRows=@($metaPaths | ForEach-Object {[pscustomobject]@{path=$_;bytes=(Get-Item -LiteralPath $_).Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $_).Hash.ToLowerInvariant()}})
Merge 'jsoncpp-provenance-and-check11' $metaRows
$freezePath=Join-Path $out 'source-freeze-root-jsoncpp.json'
$assignmentPath=Join-Path $out 'root-jsoncpp-assignment.json'
$driverPath=Join-Path $out 'run-root-configure-jsoncpp.ps1'
foreach($path in @($freezePath,$assignmentPath,$driverPath,(Join-Path $out 'attempt1'))){if(Test-Path -LiteralPath $path){throw "fresh output already exists: $path"}}
$rows=@($map.Keys | Sort-Object | ForEach-Object {$map[$_]})
$rows | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $freezePath -Encoding utf8
$freezeSHA=(Get-FileHash -Algorithm SHA256 -LiteralPath $freezePath).Hash
$override='-DFETCHCONTENT_SOURCE_DIR_JSONCPP:PATH=F:/JericoEngine/Dependencies/jsoncpp-1.6.2'
if(@($assignment.arguments | Where-Object {$_ -like '-DFETCHCONTENT_SOURCE_DIR_JSONCPP*'}).Count){throw 'JSONCpp override unexpectedly exists'}
$assignment.arguments=@($assignment.arguments)+$override
$assignment.sourceFreeze=$freezePath.Replace('\','/')
$assignment.sourceFreezeSHA256=$freezeSHA
$assignment.sourceCount=$rows.Count
$assignment.purpose='Next original offline Root configure: prior 46 argv plus exact pinned JSONCpp source override; only parent-authorized whitespace deltas reconciled. No native acceptance inferred.'
$assignment.preparationStatus='READY FOR PARENT REVIEW/DISPATCH; NOT EXECUTED'
$assignment | Add-Member -NotePropertyName previousRoot1953FreezeSHA256 -NotePropertyValue $authority[$priorFreeze]
$assignment | Add-Member -NotePropertyName jsoncppSourceRoot -NotePropertyValue 'F:/JericoEngine/Dependencies/jsoncpp-1.6.2'
$assignment | Add-Member -NotePropertyName jsoncppArchiveSHA256 -NotePropertyValue 'c40e5a2583df633b904525305bda40b622d9971fee1ece7ce48ca723a4907d36'
$assignment | Add-Member -NotePropertyName currentWhitespaceDeltas -NotePropertyValue $deltas.ToArray()
$assignment | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $assignmentPath -Encoding utf8
$assignmentSHA=(Get-FileHash -Algorithm SHA256 -LiteralPath $assignmentPath).Hash
$script=[IO.File]::ReadAllText($priorDriver)
$changes=[Collections.Generic.List[object]]::new()
function ReplaceOnce([string]$before,[string]$after){if([regex]::Matches($script:script,[regex]::Escape($before)).Count -ne 1){throw "driver replacement ambiguous: $before"};$script:script=$script:script.Replace($before,$after);$changes.Add([pscustomobject]@{before=$before;after=$after})}
ReplaceOnce "'build/n0-validation/local-packages/root-antlr-openexr-final'" "'build/n0-validation/local-packages/root-jsoncpp-final'"
ReplaceOnce "'build/n0-validation/local-packages/root-antlr-openexr-final/root-antlr-openexr-assignment.json'" "'build/n0-validation/local-packages/root-jsoncpp-final/root-jsoncpp-assignment.json'"
ReplaceOnce "'build/n0-validation/local-packages/root-antlr-openexr-final/source-freeze-root-final.json'" "'build/n0-validation/local-packages/root-jsoncpp-final/source-freeze-root-jsoncpp.json'"
ReplaceOnce $authority[$priorAssignment] $assignmentSHA
ReplaceOnce $authority[$priorFreeze] $freezeSHA
ReplaceOnce '$arguments.Count -ne 46 -or $freeze.Count -ne 1953' "`$arguments.Count -ne 47 -or `$freeze.Count -ne $($rows.Count)"
$anchor='if ($arguments -notcontains ''-DFETCHCONTENT_SOURCE_DIR_ANTLR4:PATH=F:/JericoEngine/Dependencies/antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b'') { throw ''exact local ANTLR4 override missing'' }'
ReplaceOnce $anchor ($anchor+"`nif (`$arguments -notcontains '$override') { throw 'exact local JSONCpp override missing' }")
$treeAnchor="    @{name='antlr4-tree-before';path='F:\JericoEngine\Dependencies\antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b'},"
ReplaceOnce $treeAnchor ($treeAnchor+"`n    @{name='jsoncpp-tree-before';path='F:\JericoEngine\Dependencies\jsoncpp-1.6.2'},")
[IO.File]::WriteAllText($driverPath,$script,[Text.UTF8Encoding]::new($false))
$proof=[ordered]@{status='READY FOR PARENT REVIEW; NO NATIVE EXECUTION';sourceGroups=$counts;rawInputCount=(1953+229+10+11);uniqueInputCount=$rows.Count;deduplicatedCount=(1953+229+10+11-$rows.Count);approvedDeltas=$deltas.ToArray();argumentsCount=47;cacheCount=32;wheelhouseCount=44;acceptedPackagePairs=8;priorDriverSHA256=$authority[$priorDriver];driverSHA256=(Get-FileHash -Algorithm SHA256 -LiteralPath $driverPath).Hash;assignmentSHA256=$assignmentSHA;freezeSHA256=$freezeSHA;driverChanges=$changes.ToArray();preserved='All instrumentation, offline/process environment guards, cache32/packagePairs8, exact prior46 argv, private-source preflight, existing snapshot before/after and postflight. Adds only JSONCpp argv guard/full immutable tree.';unverified='No Root/native build run; Metal composition not yet reached or validated'}
$proof | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $out 'derivation-proof.json') -Encoding utf8
$changes | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'driver-diff.json') -Encoding utf8
@($map.Keys | Sort-Object | ForEach-Object {[pscustomobject]@{fullPath=$_;origins=$origins[$_].ToArray()}}) | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'merge-origin-manifest.json') -Encoding utf8
[pscustomobject]$proof | Select-Object status,sourceGroups,uniqueInputCount,deduplicatedCount,approvedDeltas,driverSHA256,assignmentSHA256,freezeSHA256 | ConvertTo-Json -Depth 7
