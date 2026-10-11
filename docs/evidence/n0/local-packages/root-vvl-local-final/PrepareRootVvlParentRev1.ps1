#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
$repoRoot = 'F:/JericoEngine/JericoEngine'
$oldRoot = "$repoRoot/build/n0-validation/local-packages/root-fetchcontent-batch-final"
$newRoot = "$repoRoot/build/n0-validation/local-packages/root-vvl-local-final"
function Get-TaskHash([string]$path) {
    $stream = [IO.File]::OpenRead($path)
    try { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) }
    finally { $stream.Dispose() }
}
if ((Get-TaskHash "$oldRoot/source-freeze-root-batch.json") -ne 'A91288DADED0D34246BEF87BA3F144B34D2C8775215ED09AAF12E0E6FF4A1A44') { throw 'Old root freeze changed' }
if ((Get-TaskHash "$oldRoot/run-root-configure-batch-attempt2.ps1") -ne '4B9FB818021BE196F2FDC47CF498C8F52797AF96B0FB381DB743303C4BBD9AA8') { throw 'Old root driver changed' }
$wbPath = "$repoRoot/build/n0-validation/local-packages/whitebox-uv-provenance-audit/source-freeze-whitebox-uv-provenance-rev2.json"
if ((Get-TaskHash $wbPath) -ne '0049D2383BDE7FB182A0D540E535C926DE2BEDCD56223F9578A29D1DF65F20A4') { throw 'WhiteBox freeze drift' }
$vvlPath = "$repoRoot/build/n0-validation/local-packages/vvl-rev1-preparation/consumer-rev4/source-freeze-vvl-provider-consumer-rev4.json"
if ((Get-TaskHash $vvlPath) -ne 'ABE7DE1FB29A4A1296337F756A7AFC4F0E65514948AE8A45A6E99F21F7DF34F2') { throw 'VVL freeze drift' }
$oldFreeze = Get-Content -LiteralPath "$oldRoot/source-freeze-root-batch.json" -Raw | ConvertFrom-Json
$wb = Get-Content -LiteralPath $wbPath -Raw | ConvertFrom-Json
$vvl = Get-Content -LiteralPath $vvlPath -Raw | ConvertFrom-Json
$map = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
$deltas = [Collections.Generic.List[object]]::new()
$approvedDrift = @(
    'Gems/WhiteBox/Code/Source/Core/WhiteBoxCsg.cpp',
    'Gems/WhiteBox/Code/Source/Core/WhiteBoxCsg.h',
    'Gems/WhiteBox/Code/Source/EditorWhiteBoxComponent.cpp',
    'Gems/WhiteBox/Code/Tests/WhiteBoxTest.cpp',
    'cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake'
)
function Add-TaskInput($entry, [string]$origin, [bool]$allowApprovedDrift) {
    $full = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($entry.path)) { $entry.path } else { Join-Path $repoRoot $entry.path }))
    $file = [IO.FileInfo]::new($full)
    if (-not $file.Exists -or ($file.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Missing/reparse input: $full" }
    $relative = if ($full.StartsWith([IO.Path]::GetFullPath("$repoRoot/"), [StringComparison]::OrdinalIgnoreCase)) { $full.Substring($repoRoot.Length + 1).Replace('\','/') } else { $full }
    $hash = Get-TaskHash $full
    if ($null -ne $entry.sha256 -and ($hash -ne $entry.sha256 -or $file.Length -ne $entry.bytes)) {
        if (-not $allowApprovedDrift -or $relative -notin $approvedDrift) { throw "Unreviewed input drift: $relative" }
        $deltas.Add([pscustomobject]@{path=$relative;beforeSHA256=$entry.sha256;beforeBytes=$entry.bytes;afterSHA256=$hash;afterBytes=$file.Length;reason='Reviewed WhiteBox UV/provenance rev2 or accepted VVL catalog row'})
    }
    if ($map.ContainsKey($full)) {
        if ($map[$full].sha256 -ne $hash -or $map[$full].bytes -ne $file.Length) { throw "Merge mismatch: $relative" }
        return
    }
    $map.Add($full,[pscustomobject]@{path=$relative;bytes=$file.Length;sha256=$hash.ToLowerInvariant();origin=$origin})
}
foreach ($entry in $oldFreeze) { Add-TaskInput $entry 'Root61 reviewed baseline' $true }
foreach ($entry in @($wb.files) + @($wb.referenceInputs)) { Add-TaskInput $entry 'WhiteBox UV/provenance rev2' $false }
foreach ($entry in $vvl.files) { Add-TaskInput $entry 'Accepted VVL provider current source' $false }
$packageRoot = "$repoRoot/build/n0-validation/local-packages/vvl-rev1-consumer/package"
foreach ($file in Get-ChildItem -LiteralPath $packageRoot -File -Recurse) { Add-TaskInput @{path=$file.FullName} 'Accepted extracted VVL payload' $false }
foreach ($path in @(
    'cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake',
    'build/n0-validation/local-packages/root-package-cache/vulkan-validationlayers-1.3.261-je-rev1-windows.tar.xz',
    'build/n0-validation/local-packages/root-package-cache/vulkan-validationlayers-1.3.261-je-rev1-windows.tar.xz.SHA256SUMS',
    'build/n0-validation/local-packages/vvl-rev1-preparation/parent-final-package-integration.json',
    'build/n0-validation/local-packages/whitebox-uv-provenance-audit/parent-source-review-whitebox-uv-provenance-rev2.json'
)) { Add-TaskInput @{path=$path} 'Parent accepted integration witness' $false }
$freezePath = "$newRoot/source-freeze-root-batch.json"
if (Test-Path -LiteralPath $freezePath) { throw 'New root freeze already exists' }
$encoding = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText($freezePath,($map.Values | Sort-Object path | ConvertTo-Json -Depth 7)+"`n",$encoding)
$assignment = Get-Content -LiteralPath "$oldRoot/root-batch-assignment.json" -Raw | ConvertFrom-Json
$oldArguments = @($assignment.arguments)
$assignment.purpose = 'Original offline Root61 after accepted VVL package and reviewed WhiteBox UV/provenance rev2; identical61argv, new current freeze; no engine/Editor acceptance.'
$assignment.sourceFreeze = $freezePath
$assignment.sourceFreezeSHA256 = Get-TaskHash $freezePath
$assignment.sourceCount = $map.Count
$assignmentPath = "$newRoot/root-batch-assignment.json"
[IO.File]::WriteAllText($assignmentPath,($assignment | ConvertTo-Json -Depth 10)+"`n",$encoding)
$driver = [IO.File]::ReadAllText("$oldRoot/run-root-configure-batch-attempt2.ps1")
$driver = $driver.Replace('root-fetchcontent-batch-final','root-vvl-local-final').Replace("'attempt2'","'attempt1'").Replace('fresh attempt2','fresh attempt1')
$driver = $driver.Replace('401F6E61E4B3526D1594D8F7E11929415DD18A6E8BF90A34E69E4A7A68CA5CE6',(Get-TaskHash $assignmentPath))
$driver = $driver.Replace('A91288DADED0D34246BEF87BA3F144B34D2C8775215ED09AAF12E0E6FF4A1A44',(Get-TaskHash $freezePath)).Replace('15529',[string]$map.Count)
$driver = $driver.Replace('.Count -ne 32 -or $wheelFiles.Count -ne 44','.Count -ne 34 -or $wheelFiles.Count -ne 44')
$driver = $driver.Replace("'OpenEXR-3.1.3-je-rev1-windows.tar.xz')","'OpenEXR-3.1.3-je-rev1-windows.tar.xz','vulkan-validationlayers-1.3.261-je-rev1-windows.tar.xz')")
$driver = $driver.Replace('$cmake = (Get-Command cmake).Source', '$cmake = ''C:/Program Files/CMake/bin/cmake.exe''' + "`n" + 'if ((Get-FileHash -LiteralPath $cmake).Hash -ne ''1AA884BF1F4949327FFFCC8EE4A97C2D684BDC1D0A64B71F01DC16321C7FBC64'') { throw ''Audited CMake tool drift'' }')
$driver += "`nexit `$nativeExit`n"
$driverPath = "$newRoot/run-root-configure-vvl-parent-rev1.ps1"
[IO.File]::WriteAllText($driverPath,$driver,$encoding)
$tokens=$null;$parseErrors=$null
[void][Management.Automation.Language.Parser]::ParseFile($driverPath,[ref]$tokens,[ref]$parseErrors)
if ($parseErrors.Count) { throw ($parseErrors | Out-String) }
if (($oldArguments -join "`n") -cne ($assignment.arguments -join "`n") -or $oldArguments.Count -ne 61) { throw 'Native arguments changed' }
$receipt = [ordered]@{status='PARENT CURRENT FREEZE AND DERIVED DRIVER REVIEW; NATIVE NOT RUN';sourceCount=$map.Count;oldSourceCount=$oldFreeze.Count;reviewedDeltas=@($deltas.ToArray());arguments=61;argumentsUnchanged=$true;cacheFiles=34;wheelhouseFiles=44;assignmentSHA256=(Get-TaskHash $assignmentPath);freezeSHA256=(Get-TaskHash $freezePath);driverPath=$driverPath;driverSHA256=(Get-TaskHash $driverPath);syntaxErrors=0;nativeOwner='Luna';review='Accepted VVL cache/catalog; actual WhiteBox27 witnesses; originaloffline61argv and all donor pre/post snapshots preserved. Explicit audited CMake and nativeexit propagated.';limits='Local Python/WhiteBox/SMC candidates uncompiled. Root configure is not baseline HEAD, Editor or runtime.'}
[IO.File]::WriteAllText("$newRoot/parent-preflight-review.json",($receipt | ConvertTo-Json -Depth 8)+"`n",$encoding)
$receipt | ConvertTo-Json -Depth 8
