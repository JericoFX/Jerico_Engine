$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $repo 'build\n0-validation\local-packages\openexr-atomics-diagnostic'
$project = Join-Path $base 'project'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$ctest = 'C:\Program Files\CMake\bin\ctest.exe'
$openexrOutput = Join-Path $repo 'build\n0-validation\local-packages\openexr-rev1'
$msvcHeader = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\include\vcruntime_c11_stdatomic.h'
$upstreamFeatureFile = Join-Path $openexrOutput 'source-work\openexr-697cc449cc589ed780acda88e79e6145ddcd121f\cmake\OpenEXRLibraryDefine.cmake'

if (Test-Path -LiteralPath $project) { throw 'Diagnostic project directory already exists' }
[IO.Directory]::CreateDirectory($project) | Out-Null
$cmakeLists = @'
cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
project(JEOpenEXRAtomicsDiagnostic LANGUAGES C)
add_executable(JEAtomicProbe atomic_probe.c)
message(STATUS "JE probe mode=${JE_PROBE_MODE}")
message(STATUS "CMAKE_C_STANDARD_DEFAULT=${CMAKE_C_STANDARD_DEFAULT}")
if(JE_PROBE_MODE STREQUAL "feature")
  target_compile_features(JEAtomicProbe PRIVATE c_std_11)
elseif(JE_PROBE_MODE STREQUAL "explicit")
  set_target_properties(JEAtomicProbe PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
elseif(JE_PROBE_MODE STREQUAL "explicit-experimental")
  set_target_properties(JEAtomicProbe PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  target_compile_options(JEAtomicProbe PRIVATE /experimental:c11atomics)
elseif(NOT JE_PROBE_MODE STREQUAL "default")
  message(FATAL_ERROR "Unknown JE_PROBE_MODE")
endif()
'@
$cFile = @'
#include <stdint.h>
#include <stdatomic.h>

int main(void)
{
    atomic_uintptr_t value;
    atomic_init(&value, (uintptr_t)0);
    atomic_store_explicit(&value, (uintptr_t)17, memory_order_release);
    return atomic_load_explicit(&value, memory_order_acquire) == (uintptr_t)17 ? 0 : 1;
}
'@
[IO.File]::WriteAllText((Join-Path $project 'CMakeLists.txt'), $cmakeLists, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $project 'atomic_probe.c'), $cFile, [Text.UTF8Encoding]::new($false))
$upstream = Get-Content -LiteralPath $upstreamFeatureFile
$upstreamHit = $upstream | Where-Object { $_ -match 'target_compile_features\(\$\{objlib\} PRIVATE c_std_11\)' }
$headerHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $msvcHeader).Hash.ToLowerInvariant()
[pscustomobject]@{ upstreamFeatureFile=$upstreamFeatureFile; privateC11FeaturePresent=([bool]$upstreamHit); upstreamCStandardPropertyPresent=([bool]($upstream -match 'C_STANDARD 11')); msvcHeader=$msvcHeader; msvcHeaderSha256=$headerHash; msvcHeaderCondition='__STDC_VERSION__ < 201112L triggers C1189' } | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $base 'source-observation.json')

function Invoke-Native([string]$name, [string]$exe, [string[]]$argumentVector, [string]$workingDirectory) {
    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $exe
    $psi.WorkingDirectory = $workingDirectory
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    foreach ($value in $argumentVector) { [void]$psi.ArgumentList.Add($value) }
    $argumentVector | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $base "$name.argv.json")
    $proc = [Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    $timer = [Diagnostics.Stopwatch]::StartNew()
    if (!$proc.Start()) { throw "Failed to start $name" }
    $stdoutTask = $proc.StandardOutput.ReadToEndAsync()
    $stderrTask = $proc.StandardError.ReadToEndAsync()
    $proc.WaitForExit()
    $nativeExit = $proc.ExitCode
    $timer.Stop()
    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    [IO.File]::WriteAllText((Join-Path $base "$name.stdout.log"), $stdout, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $base "$name.stderr.log"), $stderr, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $base "$name.exitcode"), [string]$nativeExit)
    [IO.File]::WriteAllText((Join-Path $base "$name.elapsed.txt"), ('{0:N3}s' -f $timer.Elapsed.TotalSeconds))
    return [pscustomobject]@{ name=$name; nativeExit=$nativeExit; elapsedSeconds=[math]::Round($timer.Elapsed.TotalSeconds,3); stdoutPath=Join-Path $base "$name.stdout.log"; stderrPath=Join-Path $base "$name.stderr.log" }
}

$variants = @(
    [pscustomobject]@{ name='default'; mode='default' },
    [pscustomobject]@{ name='private-c-std-11-feature'; mode='feature' },
    [pscustomobject]@{ name='explicit-c-standard-11'; mode='explicit' }
)
$all = @()
foreach ($variant in $variants) {
    $build = Join-Path $base $variant.name
    $configureArgs = [string[]]@('-S', $project, '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143', "-DJE_PROBE_MODE:STRING=$($variant.mode)")
    if ($configureArgs.Count -ne 11) { throw "Configure argv count mismatch for $($variant.name)" }
    $configResult = Invoke-Native "$($variant.name)-configure" $cmake $configureArgs $base
    $all += $configResult
    if ($configResult.nativeExit -ne 0) { continue }
    $vcxproj = Join-Path $build 'JEAtomicProbe.vcxproj'
    if (!(Test-Path -LiteralPath $vcxproj)) { throw "Generated project missing for $($variant.name)" }
    Copy-Item -LiteralPath $vcxproj -Destination (Join-Path $base "$($variant.name).vcxproj")
    foreach ($configuration in @('Debug','Release')) {
        $buildArgs = [string[]]@('--build', $build, '--config', $configuration, '--target', 'JEAtomicProbe', '--verbose')
        if ($buildArgs.Count -ne 7) { throw "Build argv count mismatch for $($variant.name)/$configuration" }
        $buildResult = Invoke-Native "$($variant.name)-build-$configuration" $cmake $buildArgs $base
        $all += $buildResult
        if ($buildResult.nativeExit -eq 0) {
            $exe = Join-Path $build "$configuration\JEAtomicProbe.exe"
            if (!(Test-Path -LiteralPath $exe)) { throw "Probe executable missing: $exe" }
            $runResult = Invoke-Native "$($variant.name)-run-$configuration" $exe ([string[]]@()) $base
            $all += $runResult
        }
    }
}
$all | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $base 'diagnostic-results.json')
$all | Select-Object name,nativeExit,elapsedSeconds | Format-Table -AutoSize
