#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Core') { throw 'PowerShell Core is required' }
$checkoutRoot = 'F:/JericoEngine/JericoEngine'
$sourceRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-diagnostic-source'
$binaryRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-diagnostic-build-rev2'
$runRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-validation/attempt1/diagnostic-trace-rev2'
$packageRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package'
$cmakeTool = 'C:/Program Files/CMake/bin/cmake.exe'
$freezePath = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-preparation/source-freeze-vvl-provider-rev1-rev2.json'
function Require-Hash($path, $hash) {
    if ((Get-FileHash -LiteralPath $path).Hash -ne $hash) { throw "Hash differs: $path" }
}
Require-Hash $cmakeTool '1AA884BF1F4949327FFFCC8EE4A97C2D684BDC1D0A64B71F01DC16321C7FBC64'
Require-Hash $freezePath 'C516374E93BED5F7B829A92CD0356D25A96A7289A9C17BE2D2FAA590AA3F90E3'
Require-Hash "$sourceRoot/parent-diagnostic-preparation.json" 'A47FC5CE2A07A98E4217A75D4847FAE9B4AFBDFD947D42C7B72A60E00F75D4E8'
Require-Hash "$packageRoot/SHA256SUMS" '99C9654B300D1BBB88A04DBDEF4389D05BD4D91B0C3B2027595DFE37F8AA420A'
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
$diagnostic = Get-Content -LiteralPath "$sourceRoot/parent-diagnostic-preparation.json" -Raw | ConvertFrom-Json
$packageInputs = @(foreach ($line in Get-Content -LiteralPath "$packageRoot/SHA256SUMS") {
    if ($line -notmatch '^([a-f0-9]{64}) \*([^:]+)$') { throw 'Malformed package manifest' }
    $digest = $Matches[1]; $relative = $Matches[2]
    $fullPath = [IO.Path]::GetFullPath((Join-Path $packageRoot $relative))
    if (-not $fullPath.StartsWith([IO.Path]::GetFullPath("$packageRoot/"), [StringComparison]::OrdinalIgnoreCase)) { throw 'Package manifest escaped root' }
    [pscustomobject]@{path=$fullPath;sha256=$digest}
})
$packageInputs += [pscustomobject]@{path="$packageRoot/SHA256SUMS";sha256='99C9654B300D1BBB88A04DBDEF4389D05BD4D91B0C3B2027595DFE37F8AA420A'}
if ($freeze.inputs.Count -ne 6471 -or $diagnostic.inputs.Count -ne 4 -or $packageInputs.Count -ne 35) { throw 'Unexpected input counts' }
function Test-Inputs {
    $changes = @()
    foreach ($entry in @($freeze.inputs) + @($diagnostic.inputs) + @($packageInputs)) {
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
Write-Evidence "$runRoot/preflight.json" @{sourceInputs=6471;diagnosticWitnesses=4;packageFiles=35;status='PASS';changes=@()}
$results = [Collections.Generic.List[object]]::new()
function Invoke-DiagnosticNative($label, $executable, [string[]]$arguments) {
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
        $result=[pscustomobject]@{label=$label;nativeExit=$nativeExit;elapsedSeconds=$watch.Elapsed.TotalSeconds;scope='PRIVATE DIAGNOSTIC, NOT PACKAGE ACCEPTANCE'}
        Write-Evidence "$prefix.result.json" $result
        $results.Add($result)
        Write-Host "$label`: exit=$nativeExit elapsed=$([Math]::Round($watch.Elapsed.TotalSeconds,3))s"
        return $nativeExit
    } finally { $stdout.Dispose(); $stderr.Dispose(); $process.Dispose() }
}
$failed=$false
try {
    # Every argument is a complete literal: no concatenation inside array expressions.
    $configureArguments = @(
        '-S', 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-diagnostic-source',
        '-B', 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-diagnostic-build-rev2',
        '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143',
        '-DJE_VVL_PACKAGE_ROOT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package',
        '-DJE_VVL_HEADERS_ROOT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-preparation/acquisition/headers/snapshot',
        '-DJE_VVL_LOADER=C:/Windows/System32/vulkan-1.dll',
        '-DJE_VVL_LOADER_SHA256=0419974F00E82A3D619077BA414DA265A774F8DB9D45AD93BC1843F44B2C2C1F'
    )
    if ((Invoke-DiagnosticNative 'configure' $cmakeTool $configureArguments) -ne 0) { throw 'Diagnostic configure failed' }
    $cache = Get-Content -LiteralPath "$binaryRoot/CMakeCache.txt"
    if ($cache -notcontains "CMAKE_HOME_DIRECTORY:INTERNAL=$sourceRoot" -or
        $cache -notcontains 'CMAKE_PROJECT_NAME:STATIC=JELocalVulkanValidationLayersConsumer' -or
        -not (Test-Path -LiteralPath "$binaryRoot/JEVulkanValidationLayersConsumer.vcxproj" -PathType Leaf)) {
        throw 'Diagnostic configure scope/target differs; build forbidden'
    }
    $buildArguments = @('--build', 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-diagnostic-build-rev2',
        '--config', 'Release', '--target', 'JEVulkanValidationLayersConsumer', '--parallel', '2')
    if ((Invoke-DiagnosticNative 'build' $cmakeTool $buildArguments) -ne 0) { throw 'Diagnostic build failed' }
    $traceArguments = @('exports',
        'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package/vulkan-validationlayers/lib/release/VkLayer_khronos_validation.dll',
        'C:/Windows/System32/vulkan-1.dll')
    $traceExit = Invoke-DiagnosticNative 'exports-trace' "$binaryRoot/Release/JEVulkanValidationLayersConsumer.exe" $traceArguments
    if ($traceExit -ne 0) { $failed=$true }
} catch {
    $failed=$true
    Write-Evidence "$runRoot/driver-error.json" @{message=$_.Exception.Message;scriptStack=$_.ScriptStackTrace}
} finally {
    $changesAfter=Test-Inputs
    if ($changesAfter.Count) { $failed=$true }
    Write-Evidence "$runRoot/postflight.json" @{status=if($changesAfter.Count){'FAIL'}else{'PASS'};sourceInputs=6471;diagnosticWitnesses=4;packageFiles=35;changes=@($changesAfter)}
    Write-Evidence "$runRoot/summary.json" @{status='PRIVATE DIAGNOSTIC RESULTS ONLY; PARENT MUST EXAMINE TRACE';commands=@($results.ToArray());sourceModified=$false;packageRebuilt=$false;inputChanges=@($changesAfter)}
}
if ($failed) { exit 1 }
exit 0
