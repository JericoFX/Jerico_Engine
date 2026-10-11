$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
$evidence="$repo/build/n0-validation/local-packages/dxc-rev1-validation/pe-attempt1"
$root="$repo/build/n0-validation/local-packages/dxc-native-extracted-rev1"
$bin="$root/DirectXShaderCompilerDxc/bin"
$manifest="$root/SHA256SUMS"
$freeze="$repo/build/n0-validation/local-packages/dxc-rev1-preparation/consumer-candidate-rev2/source-freeze-dxc-consumer-rev2.json"
$dumpbin='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/dumpbin.exe'
$expectedTool='12A1CD87238BD66DFDB788B4FCDCB91CE4B3F81236AAB12A7A29E5AE1D85AF50'
$expectedManifest='B726ACCA8C762CE718C2A5EFAD71C312493D7DAD17DD71D85CF8E620CB6F061F'
$expectedMembers=@('dxc.exe','dxsc.exe','dxcompiler.dll','dxil.dll')
function Get-Hash([string]$path){
    $stream=[IO.File]::OpenRead($path)
    try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}
}
function Get-Membership{
    $items=@(Get-ChildItem -LiteralPath $root -File -Recurse -Force | Sort-Object FullName)
    return @($items | ForEach-Object {
        $relative=$_.FullName.Substring($root.Length).TrimStart([char]92,[char]47)
        $relative=$relative.Replace([char]92,[char]47)
        [pscustomobject]@{relativePath=$relative;bytes=$_.Length;sha256=(Get-Hash $_.FullName)}
    })
}
if((Get-Hash $dumpbin) -ne $expectedTool){throw 'dumpbin frozen SHA mismatch'}
if((Get-Hash $manifest) -ne $expectedManifest){throw 'actual package SHA256SUMS changed'}
$manifestRows=@()
foreach($line in [IO.File]::ReadAllLines($manifest)){
    if($line -match '^([a-fA-F0-9]{64}) \*(.+)$'){$manifestRows += [pscustomobject]@{sha256=$Matches[1].ToUpperInvariant();relativePath=$Matches[2]}}
    else{throw "Unexpected package manifest row: $line"}
}
if($manifestRows.Count -ne 14){throw "Expected 14 payload records, got $($manifestRows.Count)"}
$freezeObj=Get-Content -Raw -LiteralPath $freeze | ConvertFrom-Json
if($freezeObj.files.Count -ne 23373){throw 'DXC source freeze cardinality changed'}
$preMembership=Get-Membership
if($preMembership.Count -ne 15){throw "Expected 15 extracted package files total, got $($preMembership.Count)"}
foreach($row in $manifestRows){
    $relative=$row.relativePath.Replace([char]47,[IO.Path]::DirectorySeparatorChar)
    $targetPath=Join-Path $root $relative
    if(-not [IO.File]::Exists($targetPath) -or (Get-Hash $targetPath) -ne $row.sha256){throw "Package payload hash mismatch: $($row.relativePath)"}
}
$manifestRecordPaths=@($manifestRows.relativePath)
foreach($name in $expectedMembers){
    $relative="DirectXShaderCompilerDxc/bin/$name"
    if($relative -notin $manifestRecordPaths){throw "Binary absent from package SHA manifest: $relative"}
}
$preTargets=@()
foreach($name in $expectedMembers){
    $path=Join-Path $bin $name
    if(-not [IO.File]::Exists($path)){throw "Required binary missing: $name"}
    $info=[Diagnostics.FileVersionInfo]::GetVersionInfo($path)
    $preTargets += [pscustomobject]@{name=$name;path=$path;bytes=[IO.FileInfo]::new($path).Length;sha256=(Get-Hash $path);fileVersion=$info.FileVersion;productVersion=$info.ProductVersion}
}
$toolInfo=[Diagnostics.FileVersionInfo]::GetVersionInfo($dumpbin)
$preflight=[ordered]@{
    freezeSHA256=(Get-Hash $freeze)
    freezeFileCount=$freezeObj.files.Count
    packageManifestSHA256=(Get-Hash $manifest)
    packageManifestRecordCount=$manifestRows.Count
    extractedPackageFileCount=$preMembership.Count
    extractedMembership=$preMembership
    binaries=$preTargets
    dumpbin=[ordered]@{path=$dumpbin;bytes=[IO.FileInfo]::new($dumpbin).Length;sha256=(Get-Hash $dumpbin);fileVersion=$toolInfo.FileVersion;productVersion=$toolInfo.ProductVersion}
    status='PREFLIGHT PASS'
}
$preflight | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$evidence/preflight-v2.json" -Encoding utf8
[IO.File]::WriteAllLines("$evidence/package-SHA256SUMS-before.txt",[IO.File]::ReadAllLines($manifest),[Text.UTF8Encoding]::new($false))
$summary=@()
foreach($name in $expectedMembers){
    $path=Join-Path $bin $name
    $argumentVector=[string[]]@('/headers','/imports','/exports',$path)
    $stem=[IO.Path]::GetFileNameWithoutExtension($name)
    foreach($suffix in @('.argv-v2.json','.stdout-v2.txt','.stderr-v2.txt','.result-v2.json')){
        if(Test-Path -LiteralPath "$evidence/$stem$suffix"){throw "Evidence output exists; preserve it: $stem$suffix"}
    }
    $psi=[Diagnostics.ProcessStartInfo]::new()
    $psi.FileName=$dumpbin
    $psi.WorkingDirectory=$evidence
    $psi.UseShellExecute=$false
    $psi.CreateNoWindow=$true
    $psi.RedirectStandardOutput=$true
    $psi.RedirectStandardError=$true
    foreach($arg in $argumentVector){[void]$psi.ArgumentList.Add($arg)}
    $proc=[Diagnostics.Process]::new()
    $proc.StartInfo=$psi
    $watch=[Diagnostics.Stopwatch]::StartNew()
    if(-not $proc.Start()){throw "dumpbin failed to start: $name"}
    $outTask=$proc.StandardOutput.ReadToEndAsync()
    $errTask=$proc.StandardError.ReadToEndAsync()
    $proc.WaitForExit()
    $nativeExit=$proc.ExitCode
    $watch.Stop()
    $stdout=$outTask.GetAwaiter().GetResult()
    $stderr=$errTask.GetAwaiter().GetResult()
    $proc.Dispose()
    [IO.File]::WriteAllText("$evidence/$stem.stdout-v2.txt",$stdout,[Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText("$evidence/$stem.stderr-v2.txt",$stderr,[Text.UTF8Encoding]::new($false))
    [ordered]@{executable=$dumpbin;arguments=$argumentVector;argumentCount=$argumentVector.Count;targetPath=$path;targetSHA256=(Get-Hash $path)} | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath "$evidence/$stem.argv-v2.json" -Encoding utf8
    [ordered]@{nativeExit=$nativeExit;elapsedSeconds=[Math]::Round($watch.Elapsed.TotalSeconds,3);stdoutBytes=[Text.Encoding]::UTF8.GetByteCount($stdout);stderrBytes=[Text.Encoding]::UTF8.GetByteCount($stderr)} | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath "$evidence/$stem.result-v2.json" -Encoding utf8
    $machine=if($stdout -match '(?im)^\s*([0-9A-F]{4}) machine \(([^)]+)\)'){$Matches[2]}else{$null}
    $subsystem=if($stdout -match '(?im)^\s*[0-9A-F]+ subsystem \(([^)]+)\)'){$Matches[1]}else{$null}
    $imports=@([regex]::Matches($stdout,'(?im)^\s*([A-Za-z0-9_.+-]+\.dll)\s*$') | ForEach-Object {$_.Groups[1].Value} | Sort-Object -Unique)
    $exports=@([regex]::Matches($stdout,'(?im)^\s*\d+\s+[0-9A-F]{1,8}\s+[0-9A-F]{1,8}\s+\S+')).Count
    $version=$preTargets | Where-Object name -eq $name | Select-Object -First 1
    $summary += [pscustomobject]@{name=$name;sha256=$version.sha256;fileVersion=$version.fileVersion;productVersion=$version.productVersion;nativeExit=$nativeExit;machine=$machine;subsystem=$subsystem;exportsParsed=$exports;imports=$imports;relevantImportNames=@($imports | Where-Object { $_ -match '(?i)python|metal|xml|zlib|vcruntime|msvcp|ucrt|dxil' })}
}
$afterMembership=Get-Membership
$afterTargets=@()
foreach($name in $expectedMembers){
    $path=Join-Path $bin $name
    $info=[Diagnostics.FileVersionInfo]::GetVersionInfo($path)
    $afterTargets += [pscustomobject]@{name=$name;bytes=[IO.FileInfo]::new($path).Length;sha256=(Get-Hash $path);fileVersion=$info.FileVersion;productVersion=$info.ProductVersion}
}
$membershipEqual=(ConvertTo-Json -InputObject $preMembership -Depth 5 -Compress) -ceq (ConvertTo-Json -InputObject $afterMembership -Depth 5 -Compress)
$targetHashesEqual=@($preTargets | ForEach-Object {
    $before=$_
    $after=$afterTargets | Where-Object name -eq $before.name | Select-Object -First 1
    ($before.sha256 -eq $after.sha256) -and ($before.bytes -eq $after.bytes)
}).Count -eq $expectedMembers.Count
$manifestHashAfter=Get-Hash $manifest
$postflight=[ordered]@{
    freezeSHA256=(Get-Hash $freeze)
    packageManifestSHA256=$manifestHashAfter
    extractedPackageFileCount=$afterMembership.Count
    extractedMembership=$afterMembership
    binaries=$afterTargets
    dumpbinSHA256=(Get-Hash $dumpbin)
    allDumpbinExitsZero=(@($summary | Where-Object nativeExit -ne 0).Count -eq 0)
    membershipUnchanged=$membershipEqual
    fourBinaryHashesAndSizesUnchanged=$targetHashesEqual
    status=if($membershipEqual -and $targetHashesEqual -and $manifestHashAfter -eq $expectedManifest){'POSTFLIGHT PASS'}else{'POSTFLIGHT MISMATCH'}
}
$summary | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath "$evidence/dumpbin-summary-v2.json" -Encoding utf8
$postflight | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$evidence/postflight-v2.json" -Encoding utf8
if($postflight.status -ne 'POSTFLIGHT PASS' -or @($summary | Where-Object nativeExit -ne 0).Count){exit 1}