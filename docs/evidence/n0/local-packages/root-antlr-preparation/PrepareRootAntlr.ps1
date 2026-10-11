$ErrorActionPreference = 'Stop'
$repo = 'F:/JericoEngine/JericoEngine'
$base = Join-Path $repo 'build/n0-validation/local-packages'
$output = Join-Path $base 'root-antlr-preparation'
$driverPath = Join-Path $base 'original-engine-native-packages-configure/run-root-configure-native-packages.ps1'
$assignmentPath = Join-Path $base 'root-native-packages-assignment.json'
$baselinePath = Join-Path $base 'source-freeze-root-native-packages.json'
$antlrManifestPath = Join-Path $base 'antlr4-audit/root-source-manifest.json'
$antlrFreezePath = Join-Path $base 'antlr4-audit/source-freeze-root10.json'
$moveFreezePath = Join-Path $base 'spirvcross-audit/source-freeze-runtime-move2.json'
$expected = @{
    $driverPath = '61BFFEB337B4EE19FCDB37A94B0BFECE760692EAA86E6C50BF853605D6588FC8'
    $assignmentPath = 'DF6EC8FA0BE69BD19B0D76B378FF429D090CFDA110788525E3C6CAE85068D36C'
    $baselinePath = 'CAD89C5B29770A34F165EC51D1D9A5DB2577EB867A0C636BBB57314976417DE6'
    $antlrManifestPath = '60E661CD3EBF35C12DA71F64A16DB9611B75B245C1906F9AAAD919B1049EDDC7'
    $antlrFreezePath = '7117E9A85C5D5EE2E4CA7AA9A5F7CA13BD1C72A3100EA30C039BFB1714E18092'
    $moveFreezePath = '7DDDDD35D3DDFFCAE48478713C3FF72F397FF663E6691EEC0A78B294E89FA852'
}
foreach ($entry in $expected.GetEnumerator()) {
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $entry.Key).Hash -ne $entry.Value) { throw "preparation input changed: $($entry.Key)" }
}
$assignment = Get-Content -Raw -LiteralPath $assignmentPath | ConvertFrom-Json
$baseline = Get-Content -Raw -LiteralPath $baselinePath | ConvertFrom-Json
$antlrManifest = Get-Content -Raw -LiteralPath $antlrManifestPath | ConvertFrom-Json
$antlrFreeze = Get-Content -Raw -LiteralPath $antlrFreezePath | ConvertFrom-Json
$moveFreeze = Get-Content -Raw -LiteralPath $moveFreezePath | ConvertFrom-Json
if ($assignment.arguments.Count -ne 45 -or $baseline.Count -ne 444 -or $antlrManifest.inputs.Count -ne 309 -or $antlrFreeze.inputs.Count -ne 10 -or $moveFreeze.inputs.Count -ne 2) { throw 'preparation input counts changed' }
$map = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$origins = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$counts = [ordered]@{}
function FullPath([string]$path) {
    [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($path)) { $path } else { Join-Path $repo $path }))
}
function Add-Inputs([string]$origin, [object[]]$items) {
    $added = 0; $duplicates = 0
    foreach ($item in $items) {
        $key = FullPath $item.path
        if (-not (Test-Path -LiteralPath $key -PathType Leaf)) { throw "input missing: $key" }
        $fi = Get-Item -LiteralPath $key
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $key).Hash.ToLowerInvariant()
        if ($fi.Length -ne $item.bytes -or $hash -ne $item.sha256) { throw "input differs from assigned hash: $key" }
        if ($map.ContainsKey($key)) {
            if ($map[$key].bytes -ne $item.bytes -or $map[$key].sha256 -ne $item.sha256) { throw "conflicting duplicate: $key" }
            $duplicates++
        } else {
            $map.Add($key, [pscustomobject]@{path=$item.path;bytes=$item.bytes;sha256=$hash})
            $origins.Add($key, [Collections.Generic.List[string]]::new())
            $added++
        }
        $origins[$key].Add($origin)
    }
    $counts[$origin] = [pscustomobject]@{validated=$items.Count;added=$added;duplicates=$duplicates}
}
Add-Inputs 'root444' $baseline
Add-Inputs 'antlr-consumed309' $antlrManifest.inputs
Add-Inputs 'antlr-root10' $antlrFreeze.inputs
Add-Inputs 'spirvcross-runtime-move2' $moveFreeze.inputs
Add-Inputs 'antlr-root10-receipt' @([pscustomobject]@{path=$antlrFreezePath;bytes=(Get-Item -LiteralPath $antlrFreezePath).Length;sha256=$expected[$antlrFreezePath].ToLowerInvariant()})
$candidateFreeze = Join-Path $output 'source-freeze-root-candidate.json'
$candidateAssignment = Join-Path $output 'root-antlr-candidate-assignment.json'
$candidateDriver = Join-Path $output 'run-root-configure-antlr-candidate.ps1'
foreach ($path in @($candidateFreeze,$candidateAssignment,$candidateDriver)) { if (Test-Path -LiteralPath $path) { throw "candidate output exists: $path" } }
$rows = @($map.Keys | Sort-Object | ForEach-Object { $map[$_] })
$rows | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $candidateFreeze -Encoding utf8
$freezeSha = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidateFreeze).Hash
$override = '-DFETCHCONTENT_SOURCE_DIR_ANTLR4:PATH=F:/JericoEngine/Dependencies/antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b'
if (@($assignment.arguments | Where-Object { $_ -like '-DFETCHCONTENT_SOURCE_DIR_ANTLR4*' }).Count) { throw 'ANTLR override already present in baseline' }
$assignment.arguments = @($assignment.arguments) + $override
$assignment.sourceFreeze = $candidateFreeze.Replace('\','/')
$assignment.sourceFreezeSHA256 = $freezeSha
$assignment.sourceCount = $rows.Count
$assignment.purpose = 'PREPARATORY CANDIDATE ONLY: original 45 offline arguments plus exact ANTLR4 source override; cache30 unchanged; parent review required before final freeze/dispatch'
$assignment | Add-Member -NotePropertyName preparationStatus -NotePropertyValue 'NOT FINAL / NOT DISPATCHED / PARENT REVIEW PENDING'
$assignment | Add-Member -NotePropertyName baselineAssignmentSHA256 -NotePropertyValue $expected[$assignmentPath]
$assignment | Add-Member -NotePropertyName baselineFreezeSHA256 -NotePropertyValue $expected[$baselinePath]
$assignment | Add-Member -NotePropertyName antlrSourceFreezeSHA256 -NotePropertyValue $expected[$antlrFreezePath]
$assignment | Add-Member -NotePropertyName antlrSourceRoot -NotePropertyValue $antlrManifest.sourceRoot
$assignment | Add-Member -NotePropertyName spirvcrossRuntimeMoveFreezeSHA256 -NotePropertyValue $expected[$moveFreezePath]
$assignment | Add-Member -NotePropertyName openExrStatus -NotePropertyValue 'NOT ADDED: rev1 validation failed; any accepted package/cache32/catalogue changes require a new reviewed candidate'
$assignment | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $candidateAssignment -Encoding utf8
$assignmentSha = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidateAssignment).Hash
$script = [IO.File]::ReadAllText($driverPath)
$changes = [Collections.Generic.List[object]]::new()
function Replace-Once([string]$before, [string]$after) {
    $count = [regex]::Matches($script:script,[regex]::Escape($before)).Count
    if ($count -ne 1) { throw "driver replacement is not unique: $before ($count)" }
    $script:script = $script:script.Replace($before,$after)
    $changes.Add([pscustomobject]@{before=$before;after=$after})
}
Replace-Once "'build/n0-validation/local-packages/original-engine-native-packages-configure'" "'build/n0-validation/local-packages/root-antlr-preparation'"
Replace-Once "'build/n0-validation/local-packages/root-native-packages-assignment.json'" "'build/n0-validation/local-packages/root-antlr-preparation/root-antlr-candidate-assignment.json'"
Replace-Once "'build/n0-validation/local-packages/source-freeze-root-native-packages.json'" "'build/n0-validation/local-packages/root-antlr-preparation/source-freeze-root-candidate.json'"
Replace-Once $expected[$assignmentPath] $assignmentSha
Replace-Once $expected[$baselinePath] $freezeSha
Replace-Once '$arguments.Count -ne 45 -or $freeze.Count -ne 444' "`$arguments.Count -ne 46 -or `$freeze.Count -ne $($rows.Count)"
$googleGuard = 'if ($arguments -notcontains ''-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST:PATH=F:/JericoEngine/Dependencies/googletest-v1.18.0'') { throw ''exact local GoogleTest override missing'' }'
Replace-Once $googleGuard ($googleGuard + "`nif (`$arguments -notcontains '$override') { throw 'exact local ANTLR4 override missing' }")
$treeAnchor = "    @{name='googletest-tree-before';path='F:\JericoEngine\Dependencies\googletest-v1.18.0'},"
Replace-Once $treeAnchor ($treeAnchor + "`n    @{name='antlr4-tree-before';path='F:\JericoEngine\Dependencies\antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b'},")
[IO.File]::WriteAllText($candidateDriver,$script,[Text.UTF8Encoding]::new($false))
$tokens=$null; $parseErrors=$null
[Management.Automation.Language.Parser]::ParseFile($candidateDriver,[ref]$tokens,[ref]$parseErrors) | Out-Null
if ($parseErrors.Count) { throw ($parseErrors | Out-String) }
$cache = Join-Path $base 'root-package-cache'
$cacheFiles = @(Get-ChildItem -LiteralPath $cache -File -Force)
$wheelFiles = @(Get-ChildItem -LiteralPath (Join-Path $cache 'python-wheelhouse') -File -Force)
if ($cacheFiles.Count -ne 30 -or $wheelFiles.Count -ne 44) { throw 'cache30/wheelhouse44 changed during preparation' }
$cacheRows = @($cacheFiles | Sort-Object FullName | ForEach-Object { [pscustomobject]@{path=$_.FullName;bytes=$_.Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()} })
$cacheRows | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $output 'cache30-preparation-manifest.json') -Encoding utf8
$proof = [ordered]@{status='PREPARATORY ONLY; NO NATIVE PROCESS EXECUTED';baseline444Deltas=0;sourceGroups=$counts;rawInputCount=($baseline.Count+$antlrManifest.inputs.Count+$antlrFreeze.inputs.Count+$moveFreeze.inputs.Count+1);mergedInputCount=$rows.Count;deduplicatedCount=($baseline.Count+$antlrManifest.inputs.Count+$antlrFreeze.inputs.Count+$moveFreeze.inputs.Count+1-$rows.Count);argumentsCount=$assignment.arguments.Count;argumentsAppended=@($override);cacheFiles=$cacheFiles.Count;wheelhouseFiles=$wheelFiles.Count;parserErrors=$parseErrors.Count;driverSHA256=(Get-FileHash -Algorithm SHA256 -LiteralPath $candidateDriver).Hash;assignmentSHA256=$assignmentSha;freezeSHA256=$freezeSha;driverChanges=$changes.ToArray();preserved='All original guards, argv order, source private-copy checks, cache30/44 checks, environment isolation, process measurements and before/after postflight; ANTLR full tree added to existing tree loop';futureDispatch='Parent review and final freeze required. OpenEXR acceptance/cache32/catalogue changes invalidate this candidate.'}
$proof | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'preparation-proof.json') -Encoding utf8
$originRows = @($map.Keys | Sort-Object | ForEach-Object { [pscustomobject]@{fullPath=$_;origins=$origins[$_].ToArray()} })
$originRows | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $output 'merge-origin-manifest.json') -Encoding utf8
$changes | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $output 'driver-diff.json') -Encoding utf8
[pscustomobject]$proof | Select-Object status,baseline444Deltas,sourceGroups,rawInputCount,mergedInputCount,deduplicatedCount,argumentsCount,cacheFiles,wheelhouseFiles,parserErrors,driverSHA256,assignmentSHA256,freezeSHA256 | ConvertTo-Json -Depth 6
