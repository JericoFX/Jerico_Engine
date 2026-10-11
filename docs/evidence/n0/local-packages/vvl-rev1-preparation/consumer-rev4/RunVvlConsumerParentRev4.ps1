#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Core') { throw 'PowerShell Core is required' }
$checkoutRoot = 'F:/JericoEngine/JericoEngine'
$sourceRoot = 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/VulkanValidationLayers/tests'
$binaryRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev4-consumer/build'
$runRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev4-validation/attempt1'
$packageRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package'
$cmakeTool = 'C:/Program Files/CMake/bin/cmake.exe'
$freezePath = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-preparation/consumer-rev4/source-freeze-vvl-provider-consumer-rev4.json'
function Require-Hash($path, $hash) {
    if ((Get-FileHash -LiteralPath $path).Hash -ne $hash) { throw "Hash differs: $path" }
}
Require-Hash $cmakeTool '1AA884BF1F4949327FFFCC8EE4A97C2D684BDC1D0A64B71F01DC16321C7FBC64'
Require-Hash $freezePath 'ABE7DE1FB29A4A1296337F756A7AFC4F0E65514948AE8A45A6E99F21F7DF34F2'
Require-Hash "$packageRoot/SHA256SUMS" '99C9654B300D1BBB88A04DBDEF4389D05BD4D91B0C3B2027595DFE37F8AA420A'
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
$packageInputs = @(foreach ($line in Get-Content -LiteralPath "$packageRoot/SHA256SUMS") {
    if ($line -notmatch '^([a-f0-9]{64}) \*([^:]+)$') { throw 'Malformed package manifest' }
    $digest = $Matches[1]; $relative = $Matches[2]
    $fullPath = [IO.Path]::GetFullPath((Join-Path $packageRoot $relative))
    if (-not $fullPath.StartsWith([IO.Path]::GetFullPath("$packageRoot/"), [StringComparison]::OrdinalIgnoreCase)) { throw 'Package manifest escaped root' }
    [pscustomobject]@{path=$fullPath;sha256=$digest}
})
$packageInputs += [pscustomobject]@{path="$packageRoot/SHA256SUMS";sha256='99C9654B300D1BBB88A04DBDEF4389D05BD4D91B0C3B2027595DFE37F8AA420A'}
if ($freeze.inputs.Count -ne 6471 -or $packageInputs.Count -ne 35) { throw 'Unexpected input counts' }
function Test-Inputs {
    $changes = @()
    foreach ($entry in @($freeze.inputs) + @($packageInputs)) {
        if (-not (Test-Path -LiteralPath $entry.path -PathType Leaf)) { $changes += $entry.path; continue }
        if ((Get-FileHash -LiteralPath $entry.path).Hash -ne $entry.sha256) { $changes += $entry.path }
        elseif ($null -ne $entry.bytes -and (Get-Item -LiteralPath $entry.path).Length -ne $entry.bytes) { $changes += $entry.path }
    }
    $packageFiles = @(Get-ChildItem -LiteralPath $packageRoot -Recurse -File)
    if ($packageFiles.Count -ne $packageInputs.Count) { $changes += 'Package file inventory differs' }
    return ,$changes
}
foreach ($path in @($binaryRoot, $runRoot)) {
    if (-not [IO.Path]::GetFullPath($path).StartsWith([IO.Path]::GetFullPath("$checkoutRoot/build/"), [StringComparison]::OrdinalIgnoreCase)) { throw 'Output escaped checkout/build' }
    if (Test-Path -LiteralPath $path) { throw "Output exists; preserve attempt: $path" }
}
$changesBefore = Test-Inputs
if ($changesBefore.Count) { throw "Input drift before diagnosis: $($changesBefore -join ', ')" }
[IO.Directory]::CreateDirectory($runRoot) | Out-Null
function Write-Evidence($path, $value) {
    [IO.File]::WriteAllText($path, ($value | ConvertTo-Json -Depth 10) + "`n", [Text.UTF8Encoding]::new($false))
}
Write-Evidence "$runRoot/preflight.json" @{sourceInputs=6471;packageFiles=35;status='PASS';changes=@()}
$results = [Collections.Generic.List[object]]::new()
function Invoke-ConsumerNative($label, $executable, [string[]]$arguments) {
    $prefix = "$runRoot/$label"
    Write-Evidence "$prefix.argv.json" @{executable=$executable;argv=@($arguments);workingDirectory=$checkoutRoot}
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName=$executable; $start.WorkingDirectory=$checkoutRoot
    $start.UseShellExecute=$false; $start.CreateNoWindow=$true
    $start.RedirectStandardOutput=$true; $start.RedirectStandardError=$true
    foreach ($argument in $arguments) { $start.ArgumentList.Add($argument) }
    $stdout=[IO.File]::Create("$prefix.stdout.txt"); $stderr=[IO.File]::Create("$prefix.stderr.txt")
    $process=[Diagnostics.Process]::new(); $process.StartInfo=$start
    $watch=[Diagnostics.Stopwatch]::StartNew()
    try {
        if (-not $process.Start()) { throw 'Native start failed' }
        $outTask=$process.StandardOutput.BaseStream.CopyToAsync($stdout)
        $errTask=$process.StandardError.BaseStream.CopyToAsync($stderr)
        Write-Evidence "$prefix.running.json" @{pid=$process.Id;startedUTC=[DateTime]::UtcNow.ToString('o')}
        $process.WaitForExit()
        $nativeExit=$process.ExitCode
        $watch.Stop()
        [Threading.Tasks.Task]::WaitAll(@($outTask,$errTask))
        $result=[pscustomobject]@{label=$label;nativeExit=$nativeExit;elapsedSeconds=$watch.Elapsed.TotalSeconds;scope='FOCAL VVL CONSUMER; NOT ROOT/EDITOR ACCEPTANCE'}
        Write-Evidence "$prefix.result.json" $result
        $results.Add($result)
        Write-Host "$label`: exit=$nativeExit elapsed=$([Math]::Round($watch.Elapsed.TotalSeconds,3))s"
        return $nativeExit
    } finally { $stdout.Dispose(); $stderr.Dispose(); $process.Dispose() }
}

$ctestTool = 'C:/Program Files/CMake/bin/ctest.exe'
Require-Hash $ctestTool '73BAACBEB272CA6F40422B4F789403390AF678BEB491783CEF1727D69CD3E1CB'
$cases=@()
$ctestNativeExit=$null
$failed=$false
try {
    $configureArguments = @(
        '-S', 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/VulkanValidationLayers/tests',
        '-B', 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev4-consumer/build',
        '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143',
        '-DJE_VVL_PACKAGE_ROOT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package',
        '-DJE_VVL_HEADERS_ROOT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-preparation/acquisition/headers/snapshot',
        '-DJE_VVL_LOADER=C:/Windows/System32/vulkan-1.dll',
        '-DJE_VVL_LOADER_SHA256=0419974F00E82A3D619077BA414DA265A774F8DB9D45AD93BC1843F44B2C2C1F'
    )
    if ((Invoke-ConsumerNative 'configure' $cmakeTool $configureArguments) -ne 0) { throw 'Consumer configure failed' }
    $cache = Get-Content -LiteralPath "$binaryRoot/CMakeCache.txt"
    if ($cache -notcontains "CMAKE_HOME_DIRECTORY:INTERNAL=$sourceRoot" -or
        $cache -notcontains 'CMAKE_PROJECT_NAME:STATIC=JELocalVulkanValidationLayersConsumer' -or
        -not (Test-Path -LiteralPath "$binaryRoot/JEVulkanValidationLayersConsumer.vcxproj" -PathType Leaf)) {
        throw 'Consumer configure scope/target differs; build forbidden'
    }
    $buildArguments = @('--build', 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev4-consumer/build',
        '--config', 'Release', '--target', 'JEVulkanValidationLayersConsumer', '--parallel', '2')
    if ((Invoke-ConsumerNative 'build' $cmakeTool $buildArguments) -ne 0) { throw 'Consumer build failed' }
    $testArguments = @('--test-dir', 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev4-consumer/build',
        '-C', 'Release', '--output-on-failure', '--output-junit',
        'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev4-validation/attempt1/ctest.junit.xml')
    $ctestNativeExit = Invoke-ConsumerNative 'ctest' $ctestTool $testArguments
    [xml]$junit=Get-Content -LiteralPath "$runRoot/ctest.junit.xml" -Raw
    $cases=@($junit.SelectNodes('//testcase'))
    $expectedCases=@('JE.LocalPackages.VVL.release.exports','JE.LocalPackages.VVL.release.runtime',
        'JE.LocalPackages.VVL.debug.exports','JE.LocalPackages.VVL.debug.runtime')
    $caseNames = @($cases | ForEach-Object { [string]$_.name })
    $junitProblems = @($junit.SelectNodes('//failure|//error|//skipped'))
    Write-Evidence "$runRoot/ctest-review.json" @{nativeExit=$ctestNativeExit;cases=$caseNames;problemCount=$junitProblems.Count;expectedCases=$expectedCases}
    if($cases.Count -ne 4 -or $junitProblems.Count -or
        (($caseNames | Sort-Object) -join "`n") -ne (($expectedCases | Sort-Object) -join "`n")) { throw 'Expected four real CTest cases were not passed' }
    if ($ctestNativeExit -ne 0) { throw 'Consumer CTest failed; JUnit retained' }
} catch {
    $failed=$true
    Write-Evidence "$runRoot/driver-error.json" @{message=$_.Exception.Message;scriptStack=$_.ScriptStackTrace}
} finally {
    $changesAfter=Test-Inputs
    if ($changesAfter.Count) { $failed=$true }
    Write-Evidence "$runRoot/postflight.json" @{status=if($changesAfter.Count){'FAIL'}else{'PASS'};sourceInputs=6471;packageFiles=35;changes=@($changesAfter)}
    Write-Evidence "$runRoot/validation-result.json" @{status=if($failed){'FAIL; RETAIN ACTUAL RESULTS'}else{'FOCAL FOUR CASES PASS; PARENT/GUARDS/ROOT/EDITOR PENDING'};commands=@($results.ToArray());sourceChanges=@($changesAfter);packageRebuilt=$false;ctestCases=if($null-ne$cases){@($cases|ForEach-Object name)}else{@()}}
}
if ($failed) { exit 1 }
exit 0
