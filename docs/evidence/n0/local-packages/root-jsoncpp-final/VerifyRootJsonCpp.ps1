$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
$base=Join-Path $repo 'build/n0-validation/local-packages'
$out=Join-Path $base 'root-jsoncpp-final'
$prior=Join-Path $base 'root-antlr-openexr-final'
$proof=Get-Content -Raw -LiteralPath (Join-Path $out 'derivation-proof.json') | ConvertFrom-Json
$priorProof=Get-Content -Raw -LiteralPath (Join-Path $prior 'derivation-proof.json') | ConvertFrom-Json
$assignment=Get-Content -Raw -LiteralPath (Join-Path $out 'root-jsoncpp-assignment.json') | ConvertFrom-Json
$priorAssignment=Get-Content -Raw -LiteralPath (Join-Path $prior 'root-antlr-openexr-assignment.json') | ConvertFrom-Json
$freeze=Get-Content -Raw -LiteralPath (Join-Path $out 'source-freeze-root-jsoncpp.json') | ConvertFrom-Json
$driverPath=Join-Path $out 'run-root-configure-jsoncpp.ps1'
if($freeze.Count -ne 2203 -or $assignment.arguments.Count -ne 47 -or $assignment.packageCacheFileCount -ne 32){throw 'final counts differ'}
for($i=0;$i -lt 46;$i++){if($assignment.arguments[$i] -cne $priorAssignment.arguments[$i]){throw "argv changed: $i"}}
if($assignment.arguments[-1] -cne '-DFETCHCONTENT_SOURCE_DIR_JSONCPP:PATH=F:/JericoEngine/Dependencies/jsoncpp-1.6.2'){throw 'exact JSONCpp argv guard failed'}
$keyMap=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($row in $freeze){
    $path=[IO.Path]::GetFullPath($(if([IO.Path]::IsPathRooted($row.path)){$row.path}else{Join-Path $repo $row.path}))
    if(-not $keyMap.Add($path)){throw "duplicate final input: $path"}
    if(-not(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -ne $row.bytes -or (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant() -ne $row.sha256){throw "final input changed: $path"}
}
$text=[IO.File]::ReadAllText($driverPath)
$restored=$text
$oldestProof=Get-Content -Raw -LiteralPath (Join-Path $base 'root-antlr-preparation/preparation-proof.json') | ConvertFrom-Json
foreach($changes in @($proof.driverChanges,$priorProof.driverChanges,$oldestProof.driverChanges)){
    for($i=$changes.Count-1;$i -ge 0;$i--){
        $row=$changes[$i]
        if([regex]::Matches($restored,[regex]::Escape($row.after)).Count -ne 1){throw 'reverse diff ambiguous'}
        $restored=$restored.Replace($row.after,$row.before)
    }
}
$originalPath=Join-Path $base 'original-engine-native-packages-configure/run-root-configure-native-packages.ps1'
if($restored -cne [IO.File]::ReadAllText($originalPath) -or (Get-FileHash -Algorithm SHA256 -LiteralPath $originalPath).Hash -ne '61BFFEB337B4EE19FCDB37A94B0BFECE760692EAA86E6C50BF853605D6588FC8'){throw 'unaccounted driver change'}
$driverBytes=[IO.File]::ReadAllBytes($driverPath)
if([regex]::Matches($text,'\r\n').Count -or ($driverBytes[0] -eq 239 -and $driverBytes[1] -eq 187 -and $driverBytes[2] -eq 191)){throw 'LF/no-BOM preservation failed'}
$pairsMatch=[regex]::Match($text,'(?m)^\$packagePairs = @\(([^\r\n]+)\)$')
if(-not $pairsMatch.Success -or $pairsMatch.Groups[1].Value.Split(',').Count -ne 8){throw 'packagePairs8 check failed'}
$parser=@()
foreach($path in @($driverPath,(Join-Path $out 'DeriveRootJsonCpp.ps1'),(Join-Path $base 'jsoncpp-audit/CheckAzslcReflection.ps1'),$PSCommandPath)){
    $tokens=$null; $errors=$null
    [Management.Automation.Language.Parser]::ParseFile($path,[ref]$tokens,[ref]$errors) | Out-Null
    if($errors.Count){throw "PowerShell parse failed: $path"}
    $parser += [pscustomobject]@{path=$path;errors=0;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash}
}
function TreeMap([string]$path){
    $root=[IO.Path]::GetFullPath($path).TrimEnd('\')
    $map=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach($fi in Get-ChildItem -LiteralPath $root -File -Recurse -Force){
        $relative=$fi.FullName.Substring($root.Length).TrimStart('\')
        $map.Add($relative,[pscustomobject]@{bytes=$fi.Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $fi.FullName).Hash.ToLowerInvariant()})
    }
    return ,$map
}
$private=@()
foreach($pair in @(
    @{name='Manifold';source='F:/JericoEngine/Dependencies/manifold-3.5.1';copy=(Join-Path $base 'source-work/manifold-3.5.1-root')},
    @{name='Clipper2';source='F:/JericoEngine/Dependencies/clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4';copy=(Join-Path $base 'source-work/clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root')},
    @{name='OpenMesh';source='F:/JericoEngine/Dependencies/OpenMesh-11.0.0';copy=(Join-Path $base 'source-work/OpenMesh-11.0.0-root')}
)){
    $source=TreeMap $pair.source
    $copy=TreeMap $pair.copy
    $delta=@()
    foreach($key in $source.Keys){if(-not $copy.ContainsKey($key) -or $source[$key].bytes -ne $copy[$key].bytes -or $source[$key].sha256 -ne $copy[$key].sha256){$delta += $key}}
    foreach($key in $copy.Keys){if(-not $source.ContainsKey($key)){$delta += "extra:$key"}}
    $private += [pscustomobject]@{name=$pair.name;source=$pair.source;copy=$pair.copy;sourceFiles=$source.Count;copyFiles=$copy.Count;deltaCount=$delta.Count;deltas=$delta}
    if($delta.Count){$private | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'private-copy-blocker.json') -Encoding utf8;throw "private copy not pristine: $($pair.name)"}
}
$receipt=[ordered]@{status='STATIC FINAL DERIVATION VERIFIED; NATIVE EXECUTION PENDING PARENT/LUNA DISPATCH';inputsVerified=$freeze.Count;priorArgumentsExactlyPreserved=46;argumentsAppended=1;sourceDeltasOnlyApproved=$proof.approvedDeltas;cache32=$true;wheelhouse44=$true;acceptedPackagePairs=8;parser=$parser;reverseDiffToOriginalSHA61BFF=$true;driverLFNoBOM=$true;fullPrivateCopyComparison=$private;immutableFullTreesBeforeAfter='ANTLR4 and JSONCpp preserved in driver treePaths, manifests and postflight loop';nativeProcessesExecuted=0;driverSHA256=(Get-FileHash -Algorithm SHA256 -LiteralPath $driverPath).Hash;assignmentSHA256=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $out 'root-jsoncpp-assignment.json')).Hash;freezeSHA256=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $out 'source-freeze-root-jsoncpp.json')).Hash}
$receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $out 'static-verification.json') -Encoding utf8
[pscustomobject]$receipt | Select-Object status,inputsVerified,priorArgumentsExactlyPreserved,argumentsAppended,reverseDiffToOriginalSHA61BFF,fullPrivateCopyComparison,driverSHA256,assignmentSHA256,freezeSHA256 | ConvertTo-Json -Depth 5
