#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Core') { throw 'PowerShell Core is required' }
$checkoutRoot = 'F:/JericoEngine/JericoEngine'
$sourceRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-runtime-diagnostic-source'
$binaryRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-runtime-diagnostic-build'
$runRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev3-validation/attempt1/runtime-diagnostic'
$packageRoot = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package'
$cmakeTool = 'C:/Program Files/CMake/bin/cmake.exe'
$freezePath = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-preparation/consumer-rev3/source-freeze-vvl-provider-consumer-rev3.json'
function Require-Hash($path, $hash) {
    if ((Get-FileHash -LiteralPath $path).Hash -ne $hash) { throw "Hash differs: $path" }
}
Require-Hash $cmakeTool '1AA884BF1F4949327FFFCC8EE4A97C2D684BDC1D0A64B71F01DC16321C7FBC64'
Require-Hash $freezePath 'CE6EC1352E4F911A62D04FB3637218FECC1F5191FD34C915C3A9328ADAAF492B'
Require-Hash "$packageRoot/SHA256SUMS" '99C9654B300D1BBB88A04DBDEF4389D05BD4D91B0C3B2027595DFE37F8AA420A'
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
Require-Hash "$sourceRoot/parent-diagnostic-preparation.json" '7FCA28ACC43AF2A08A50616096592595D0041E1946313B161010FFEA0090AEEC'
$diagnostic = Get-Content -LiteralPath "$sourceRoot/parent-diagnostic-preparation.json" -Raw | ConvertFrom-Json
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
    foreach ($entry in @($freeze.inputs) + @($packageInputs) + @($diagnostic.inputs)) {
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
function Invoke-ConsumerNative($label, $executable, [string[]]$arguments, [hashtable]$environmentOverrides=@{}) {
    $prefix = "$runRoot/$label"
    Write-Evidence "$prefix.argv.json" @{executable=$executable;argv=@($arguments);workingDirectory=$checkoutRoot;processEnvironmentOverrides=$environmentOverrides}
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName=$executable; $start.WorkingDirectory=$checkoutRoot
    $start.UseShellExecute=$false; $start.CreateNoWindow=$true
    $start.RedirectStandardOutput=$true; $start.RedirectStandardError=$true
    foreach ($argument in $arguments) { $start.ArgumentList.Add($argument) }
    foreach ($pair in $environmentOverrides.GetEnumerator()) { $start.Environment[$pair.Key]=[string]$pair.Value }
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
        $result=[pscustomobject]@{label=$label;nativeExit=$nativeExit;elapsedSeconds=$watch.Elapsed.TotalSeconds;scope='PRIVATE RUNTIME DIAGNOSTIC, NOT PACKAGE ACCEPTANCE'}
        Write-Evidence "$prefix.result.json" $result
        $results.Add($result)
        Write-Host "$label`: exit=$nativeExit elapsed=$([Math]::Round($watch.Elapsed.TotalSeconds,3))s"
        return $nativeExit
    } finally { $stdout.Dispose(); $stderr.Dispose(); $process.Dispose() }
}


$failed=$false
$emptyImplicitRoot="$runRoot/empty-implicit-layers"
[IO.Directory]::CreateDirectory($emptyImplicitRoot)|Out-Null
try {
    $configureArguments = @(
        '-S', 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-runtime-diagnostic-source',
        '-B', 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-runtime-diagnostic-build',
        '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143',
        '-DJE_VVL_PACKAGE_ROOT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package',
        '-DJE_VVL_HEADERS_ROOT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-preparation/acquisition/headers/snapshot',
        '-DJE_VVL_LOADER=C:/Windows/System32/vulkan-1.dll',
        '-DJE_VVL_LOADER_SHA256=0419974F00E82A3D619077BA414DA265A774F8DB9D45AD93BC1843F44B2C2C1F'
    )
    if((Invoke-ConsumerNative 'configure' $cmakeTool $configureArguments)-ne0){throw 'Diagnostic configure failed'}
    $cache=Get-Content -LiteralPath "$binaryRoot/CMakeCache.txt"
    if($cache-notcontains"CMAKE_HOME_DIRECTORY:INTERNAL=$sourceRoot"-or$cache-notcontains'CMAKE_PROJECT_NAME:STATIC=JELocalVulkanValidationLayersConsumer'-or
        -not(Test-Path -LiteralPath "$binaryRoot/JEVulkanValidationLayersConsumer.vcxproj" -PathType Leaf)){throw 'Diagnostic scope/target differs'}
    $buildArguments=@('--build','F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-runtime-diagnostic-build',
        '--config','Release','--target','JEVulkanValidationLayersConsumer','--parallel','2')
    if((Invoke-ConsumerNative 'build' $cmakeTool $buildArguments)-ne0){throw 'Diagnostic build failed'}
    $runtimeArguments=@('runtime',
        'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package/vulkan-validationlayers/lib/release/VkLayer_khronos_validation.dll',
        'C:/Windows/System32/vulkan-1.dll')
    $inheritedExit=Invoke-ConsumerNative 'runtime-inherited' "$binaryRoot/Release/JEVulkanValidationLayersConsumer.exe" $runtimeArguments
    $isolatedExit=Invoke-ConsumerNative 'runtime-empty-implicit-path' "$binaryRoot/Release/JEVulkanValidationLayersConsumer.exe" $runtimeArguments @{
        VK_IMPLICIT_LAYER_PATH='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev3-validation/attempt1/runtime-diagnostic/empty-implicit-layers'
    }
} catch {
    $failed=$true
    Write-Evidence "$runRoot/driver-error.json" @{message=$_.Exception.Message;scriptStack=$_.ScriptStackTrace}
} finally {
    $changesAfter=Test-Inputs
    if($changesAfter.Count-or@(Get-ChildItem -LiteralPath $emptyImplicitRoot -Force).Count){$failed=$true}
    Write-Evidence "$runRoot/postflight.json" @{status=if($changesAfter.Count){'FAIL'}else{'PASS'};sourceInputs=6471;packageFiles=35;privateDiagnosticInputs=2;changes=@($changesAfter);implicitDirectoryEmpty=(@(Get-ChildItem -LiteralPath $emptyImplicitRoot -Force).Count-eq0)}
    Write-Evidence "$runRoot/diagnostic-result.json" @{status='PRIVATE A/B DIAGNOSTIC; PARENT MUST REVIEW REAL VkResult/ERROR COUNT';commands=@($results.ToArray());changes=@($changesAfter);globalEnvironmentModified=$false;registryModified=$false;productSourceModified=$false;packageRebuilt=$false;implicitIsolation='Per child process only; same package explicit VVL remains active'}
}
if($failed){exit 1}
exit 0
