$ErrorActionPreference = 'Stop'
$repo='F:/JericoEngine/JericoEngine'
$base=Join-Path $repo 'build/n0-validation/local-packages'
$output=Join-Path $base 'root-antlr-preparation'
$driver=Join-Path $output 'run-root-configure-antlr-candidate.ps1'
$proof=Get-Content -Raw -LiteralPath (Join-Path $output 'preparation-proof.json') | ConvertFrom-Json
$freeze=Get-Content -Raw -LiteralPath (Join-Path $output 'source-freeze-root-candidate.json') | ConvertFrom-Json
$assignment=Get-Content -Raw -LiteralPath (Join-Path $output 'root-antlr-candidate-assignment.json') | ConvertFrom-Json
$originalAssignment=Get-Content -Raw -LiteralPath (Join-Path $base 'root-native-packages-assignment.json') | ConvertFrom-Json
if ($freeze.Count -ne 765 -or $assignment.arguments.Count -ne 46 -or $assignment.packageCacheFileCount -ne 30) { throw 'candidate counts differ' }
for ($i=0; $i -lt 45; $i++) { if ($assignment.arguments[$i] -cne $originalAssignment.arguments[$i]) { throw "original argv changed at index $i" } }
foreach ($row in $freeze) {
    $path=[IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($row.path)) { $row.path } else { Join-Path $repo $row.path }))
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "candidate input missing: $path" }
    if ((Get-Item -LiteralPath $path).Length -ne $row.bytes -or (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant() -ne $row.sha256) { throw "candidate input changed: $path" }
}
$candidateText=[IO.File]::ReadAllText($driver)
$reconstructed=$candidateText
for ($i=$proof.driverChanges.Count-1; $i -ge 0; $i--) {
    $change=$proof.driverChanges[$i]
    if ([regex]::Matches($reconstructed,[regex]::Escape($change.after)).Count -ne 1) { throw 'reverse driver diff is not unique' }
    $reconstructed=$reconstructed.Replace($change.after,$change.before)
}
$originalDriver=Join-Path $base 'original-engine-native-packages-configure/run-root-configure-native-packages.ps1'
if ($reconstructed -cne [IO.File]::ReadAllText($originalDriver)) { throw 'unaccounted driver change' }
$parserRows=@()
foreach ($path in @($driver,(Join-Path $output 'PrepareRootAntlr.ps1'),$PSCommandPath)) {
    $tokens=$null; $errors=$null
    [Management.Automation.Language.Parser]::ParseFile($path,[ref]$tokens,[ref]$errors) | Out-Null
    if ($errors.Count) { throw "PowerShell parse errors: $path" }
    $parserRows += [pscustomobject]@{path=$path;errors=0;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash}
}
$driverBytes=[IO.File]::ReadAllBytes($driver)
$crlf=[regex]::Matches($candidateText,'\r\n').Count
if ($crlf -or ($driverBytes[0] -eq 239 -and $driverBytes[1] -eq 187 -and $driverBytes[2] -eq 191)) { throw 'driver LF/no-BOM preservation failed' }
$pairs=@(
    @{name='Manifold';source='F:/JericoEngine/Dependencies/manifold-3.5.1';copy=(Join-Path $base 'source-work/manifold-3.5.1-root');paths=@('src/csg_tree.h','src/edge_op.cpp')},
    @{name='Clipper2';source='F:/JericoEngine/Dependencies/clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4';copy=(Join-Path $base 'source-work/clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root');paths=@('CPP/Clipper2Lib/include/clipper2/clipper.version.h')}
)
$privateRows=@()
foreach ($pair in $pairs) {
    foreach ($relative in $pair.paths) {
        $sourcePath=Join-Path $pair.source $relative
        $copyPath=Join-Path $pair.copy $relative
        $sourceHash=(Get-FileHash -Algorithm SHA256 -LiteralPath $sourcePath).Hash
        $copyHash=(Get-FileHash -Algorithm SHA256 -LiteralPath $copyPath).Hash
        $privateRows += [pscustomobject]@{name=$pair.name;relativePath=$relative;originalSHA256=$sourceHash;copySHA256=$copyHash;equal=($sourceHash -eq $copyHash)}
    }
}
$receipt=[ordered]@{status='STATIC PREPARATION VERIFIED; NO NATIVE PROCESS EXECUTED; NOT FINAL FOR DISPATCH';mergedInputsVerified=$freeze.Count;originalArgumentsPreserved=45;appendedArguments=1;antlrConsumedInputs=309;driverExactReverseDiff=$true;driverChangeCount=$proof.driverChanges.Count;driverLFNoBOM=$true;parserChecks=$parserRows;privateGeneratedFilesReadOnlyCheck=$privateRows;privateCopyScope='Only the three known generated files checked here; full pristine private-copy equality remains an unchanged mandatory driver preflight. No private copy was reset or changed.';cache30SHA256=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $output 'cache30-preparation-manifest.json')).Hash;futureDispatch='Parent review/final freeze required. Cache32/OpenEXR/catalogue updates require new assignment, source freeze and driver hashes.'}
$receipt | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $output 'static-verification.json') -Encoding utf8
[pscustomobject]$receipt | Select-Object status,mergedInputsVerified,driverExactReverseDiff,driverChangeCount,privateGeneratedFilesReadOnlyCheck | ConvertTo-Json -Depth 6
