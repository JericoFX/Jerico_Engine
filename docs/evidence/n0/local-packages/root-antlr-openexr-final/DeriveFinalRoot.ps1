$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
$base=Join-Path $repo 'build/n0-validation/local-packages'
$out=Join-Path $base 'root-antlr-openexr-final'
$prior=Join-Path $base 'root-antlr-preparation'
$priorFreeze=Join-Path $prior 'source-freeze-root-candidate.json'
$priorAssignment=Join-Path $prior 'root-antlr-candidate-assignment.json'
$priorDriver=Join-Path $prior 'run-root-configure-antlr-candidate.ps1'
$openFreeze=Join-Path $base 'openexr-audit/source-freeze-openexr-candidate-v2.json'
$metadata=@(
    @{path=$openFreeze;sha256='7745DC711AC2F20AE812108434D5F338B7FCD88E0A3793496B8DB8BC664B8163'},
    @{path=(Join-Path $base 'openexr-audit/validation-arguments-v2.json');sha256='0D219066133D2A20EE8756ACEC2F1D06DF0EB1E2C34084F950D07D0943345880'},
    @{path=(Join-Path $base 'openexr-rev2-validation/validation-summary.json');sha256='92E569A6FC0786BEEE9DFC95C572938CF2CBCB8EDC875F92798A8F885BE28170'},
    @{path=(Join-Path $base 'root-openexr-copy-manifest.json');sha256='66C081C2793F02848E41832FDB2220BA53EBBAA42E89F3C3D1C25BD9BB13B483'},
    @{path=(Join-Path $base 'openexr-parent-integration.json');sha256='543221115CEBD03E77EFD8BFC731D9733B7D680B2AF1BE75D7D44DB129F21713'}
)
$pins=@{
    $priorFreeze='B665B0E78D833396D0D47615060F05ADFA489BEF4827408A1E161975A1B8F294'
    $priorAssignment='8F5F992B62124FC223D4E42C40734B0FC1012370D4137F9C60C487684CC07BE9'
    $priorDriver='35E0BC2660E568AC9B42332FBFC53564089F9E9F596B3AD5B647F6F3F4F5FC95'
}
foreach($entry in $metadata){$pins[$entry.path]=$entry.sha256}
foreach($entry in $pins.GetEnumerator()) {if((Get-FileHash -Algorithm SHA256 -LiteralPath $entry.Key).Hash -ne $entry.Value){throw "derivation authority changed: $($entry.Key)"}}
$before=Get-Content -Raw -LiteralPath $priorFreeze | ConvertFrom-Json
$open=Get-Content -Raw -LiteralPath $openFreeze | ConvertFrom-Json
$copies=Get-Content -Raw -LiteralPath (Join-Path $base 'root-openexr-copy-manifest.json') | ConvertFrom-Json
$assignment=Get-Content -Raw -LiteralPath $priorAssignment | ConvertFrom-Json
if($before.Count -ne 765 -or $open.entries.Count -ne 1182 -or $assignment.arguments.Count -ne 46 -or $copies.Count -ne 4){throw 'derivation counts changed'}
$catalogue=Join-Path $repo 'cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake'
$readme=Join-Path $repo 'cmake/Tools/LocalPackages/OpenEXR/README.md'
function FullPath([string]$path){[IO.Path]::GetFullPath($(if([IO.Path]::IsPathRooted($path)){$path}else{Join-Path $repo $path}))}
$approved=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$approved.Add((FullPath $catalogue),[pscustomobject]@{sha256='2d8c84c9bb2c8eff7ecd5a6df3aa5b22bc67ee2cc761e3e696319a6a3da346e5';bytes=$null;reason='Parent accepted OpenEXR catalogue entry'})
$approved.Add((FullPath $readme),[pscustomobject]@{sha256='3a2a380acfff140b67396e93e6254e4e4ef00063a4984f8aab22e813d9a45a6f';bytes=9501;reason='Parent status-only documentation update after OpenEXR freeze'})
$map=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$origins=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$deltas=[Collections.Generic.List[object]]::new()
$counts=[ordered]@{}
function Merge-Inputs([string]$origin,[object[]]$rows){
    $added=0; $duplicate=0
    foreach($row in $rows){
        $path=FullPath $row.path
        if(-not(Test-Path -LiteralPath $path -PathType Leaf)){throw "input missing: $path"}
        $bytes=(Get-Item -LiteralPath $path).Length
        $hash=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
        if($bytes -ne $row.bytes -or $hash -ne $row.sha256){
            if(-not $approved.ContainsKey($path) -or $hash -ne $approved[$path].sha256 -or ($null -ne $approved[$path].bytes -and $bytes -ne $approved[$path].bytes)){throw "UNAPPROVED delta: $path"}
            $deltas.Add([pscustomobject]@{origin=$origin;path=$path;beforeBytes=$row.bytes;beforeSHA256=$row.sha256;afterBytes=$bytes;afterSHA256=$hash;reason=$approved[$path].reason})
        }
        if($map.ContainsKey($path)){
            if($map[$path].bytes -ne $bytes -or $map[$path].sha256 -ne $hash){throw "conflicting duplicate: $path"}
            $duplicate++
        }else{
            $map.Add($path,[pscustomobject]@{path=$row.path;bytes=$bytes;sha256=$hash})
            $origins.Add($path,[Collections.Generic.List[string]]::new())
            $added++
        }
        $origins[$path].Add($origin)
    }
    $counts[$origin]=[pscustomobject]@{validated=$rows.Count;added=$added;duplicates=$duplicate}
}
Merge-Inputs 'approved-root765' $before
Merge-Inputs 'openexr-v2-1182' $open.entries
if($deltas.Count -ne 2 -or @($deltas | Where-Object {$_.origin -eq 'approved-root765'}).Count -ne 1 -or @($deltas | Where-Object {$_.origin -eq 'openexr-v2-1182'}).Count -ne 1){throw 'expected exactly catalogue/README authorized deltas'}
$metaRows=@($metadata | ForEach-Object {[pscustomobject]@{path=$_.path;bytes=(Get-Item -LiteralPath $_.path).Length;sha256=$_.sha256}})
Merge-Inputs 'openexr-provenance5' $metaRows
$copyRows=@()
foreach($copy in $copies){
    foreach($path in @($copy.source,$copy.destination)){$copyRows += [pscustomobject]@{path=$path;bytes=$copy.bytes;sha256=$copy.sha256}}
}
Merge-Inputs 'accepted-package-copy-witness8' $copyRows
$cache=Join-Path $base 'root-package-cache'
$cacheFiles=@(Get-ChildItem -LiteralPath $cache -File -Force)
$wheels=@(Get-ChildItem -LiteralPath (Join-Path $cache 'python-wheelhouse') -File -Force)
if($cacheFiles.Count -ne 32 -or $wheels.Count -ne 44){throw 'cache32/wheelhouse44 counts changed'}
$priorCache=Get-Content -Raw -LiteralPath (Join-Path $prior 'cache30-preparation-manifest.json') | ConvertFrom-Json
foreach($row in $priorCache){if((Get-Item -LiteralPath $row.path).Length -ne $row.bytes -or (Get-FileHash -Algorithm SHA256 -LiteralPath $row.path).Hash.ToLowerInvariant() -ne $row.sha256){throw "cache30 original changed: $($row.path)"}}
$archive=Join-Path $cache 'OpenEXR-3.1.3-je-rev1-windows.tar.xz'
if((Get-Item -LiteralPath $archive).Length -ne 6880112 -or (Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash -ne 'CD06701C7710FCAF68B29D99A1A4E110EA4C274B0656E1CC2182A1A7003F7FA0'){throw 'accepted OpenEXR archive mismatch'}
$sidecar=(Get-Content -Raw -LiteralPath "$archive.SHA256SUMS").Trim()
if($sidecar -cne 'cd06701c7710fcaf68b29d99a1a4e110ea4c274b0656e1cc2182a1a7003f7fa0 *OpenEXR-3.1.3-je-rev1-windows.tar.xz'){throw 'OpenEXR exact sidecar mismatch'}
$newCache=@($cacheFiles | Where-Object {$_.FullName -notin @($priorCache.path)} | ForEach-Object Name)
if($newCache.Count -ne 2 -or $newCache -notcontains 'OpenEXR-3.1.3-je-rev1-windows.tar.xz' -or $newCache -notcontains 'OpenEXR-3.1.3-je-rev1-windows.tar.xz.SHA256SUMS'){throw 'unexpected cache additions'}
$freezePath=Join-Path $out 'source-freeze-root-final.json'
$assignmentPath=Join-Path $out 'root-antlr-openexr-assignment.json'
$driverPath=Join-Path $out 'run-root-configure-antlr-openexr.ps1'
foreach($path in @($freezePath,$assignmentPath,$driverPath)){if(Test-Path -LiteralPath $path){throw "fresh final output exists: $path"}}
if(Test-Path -LiteralPath (Join-Path $out 'attempt1')){throw 'fresh final attempt1 already exists'}
$rows=@($map.Keys | Sort-Object | ForEach-Object {$map[$_]})
$rows | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $freezePath -Encoding utf8
$freezeSHA=(Get-FileHash -Algorithm SHA256 -LiteralPath $freezePath).Hash
$assignment.sourceFreeze=$freezePath.Replace('\','/')
$assignment.sourceFreezeSHA256=$freezeSHA
$assignment.sourceCount=$rows.Count
$assignment.packageCacheFileCount=32
$assignment.purpose='Root configure: original 45 offline argv plus exact ANTLR4 override; accepted OpenEXR package/cache32 and reviewed Metal-only SPIRVCross runtime move. Parent review/dispatch required; no native product acceptance inferred.'
$assignment.preparationStatus='FINAL DERIVATION READY FOR PARENT REVIEW; NOT EXECUTED'
$assignment.openExrStatus='Parent accepted focal producer, native consumer Release/Debug and 10 guards; original engine integration pending'
$assignment.newAcceptedPackages += [pscustomobject]@{name='OpenEXR-3.1.3-je-rev1-windows';sha256='cd06701c7710fcaf68b29d99a1a4e110ea4c274b0656e1cc2182a1a7003f7fa0';bytes=6880112}
$assignment | Add-Member -NotePropertyName priorApproved765FreezeSHA256 -NotePropertyValue $pins[$priorFreeze]
$assignment | Add-Member -NotePropertyName authorizedCurrentDeltas -NotePropertyValue $deltas.ToArray()
$assignment | Add-Member -NotePropertyName openExrProvenance -NotePropertyValue $metadata
$assignment | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath $assignmentPath -Encoding utf8
$assignmentSHA=(Get-FileHash -Algorithm SHA256 -LiteralPath $assignmentPath).Hash
$script=[IO.File]::ReadAllText($priorDriver)
$changes=[Collections.Generic.List[object]]::new()
function Replace-Once([string]$before,[string]$after){
    if([regex]::Matches($script:script,[regex]::Escape($before)).Count -ne 1){throw "driver replacement not unique: $before"}
    $script:script=$script:script.Replace($before,$after)
    $changes.Add([pscustomobject]@{before=$before;after=$after})
}
Replace-Once "'build/n0-validation/local-packages/root-antlr-preparation'" "'build/n0-validation/local-packages/root-antlr-openexr-final'"
Replace-Once "'build/n0-validation/local-packages/root-antlr-preparation/root-antlr-candidate-assignment.json'" "'build/n0-validation/local-packages/root-antlr-openexr-final/root-antlr-openexr-assignment.json'"
Replace-Once "'build/n0-validation/local-packages/root-antlr-preparation/source-freeze-root-candidate.json'" "'build/n0-validation/local-packages/root-antlr-openexr-final/source-freeze-root-final.json'"
Replace-Once $pins[$priorAssignment] $assignmentSHA
Replace-Once $pins[$priorFreeze] $freezeSHA
Replace-Once '$arguments.Count -ne 46 -or $freeze.Count -ne 765' "`$arguments.Count -ne 46 -or `$freeze.Count -ne $($rows.Count)"
Replace-Once '$cache }).Count -ne 30 -or $wheelFiles.Count -ne 44' '$cache }).Count -ne 32 -or $wheelFiles.Count -ne 44'
Replace-Once "'d3dx12-headers-je-rev1-windows.tar.xz')" "'d3dx12-headers-je-rev1-windows.tar.xz','OpenEXR-3.1.3-je-rev1-windows.tar.xz')"
[IO.File]::WriteAllText($driverPath,$script,[Text.UTF8Encoding]::new($false))
$proof=[ordered]@{status='FINAL DERIVATION; PARENT REVIEW/DISPATCH PENDING; NO NATIVE EXECUTION';sourceGroups=$counts;rawInputCount=(765+1182+5+8);uniqueInputCount=$rows.Count;deduplicatedCount=(765+1182+5+8-$rows.Count);approvedDeltas=$deltas.ToArray();argumentsCount=46;cacheCount=32;wheelhouseCount=44;newCacheFiles=$newCache;acceptedPackagePairs=8;originalDriverSHA256='61BFFEB337B4EE19FCDB37A94B0BFECE760692EAA86E6C50BF853605D6588FC8';priorDriverSHA256=$pins[$priorDriver];driverSHA256=(Get-FileHash -Algorithm SHA256 -LiteralPath $driverPath).Hash;assignmentSHA256=$assignmentSHA;freezeSHA256=$freezeSHA;driverChanges=$changes.ToArray();preserved='Original guards, 46 exact argv/order, full ANTLR before/after tree, all existing before/after manifests, full pristine three-private-copy preflight, environment isolation, process measurements/postflight. Only cache count/packagePairs/paths/hashes/freeze count change from approved driver.'}
$proof | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $out 'derivation-proof.json') -Encoding utf8
$changes | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'driver-diff.json') -Encoding utf8
@($map.Keys | Sort-Object | ForEach-Object {[pscustomobject]@{fullPath=$_;origins=$origins[$_].ToArray()}}) | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'merge-origin-manifest.json') -Encoding utf8
@($cacheFiles | Sort-Object FullName | ForEach-Object {[pscustomobject]@{path=$_.FullName;bytes=$_.Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()}}) | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $out 'cache32-manifest.json') -Encoding utf8
[pscustomobject]$proof | Select-Object status,sourceGroups,rawInputCount,uniqueInputCount,deduplicatedCount,approvedDeltas,driverSHA256,assignmentSHA256,freezeSHA256 | ConvertTo-Json -Depth 8
