$ErrorActionPreference = 'Stop'
$base = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openexr-atomics-diagnostic'
$project = Join-Path $base 'project'
$build = Join-Path $base 'explicit-c-standard-11-experimental'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
if (Test-Path -LiteralPath $build) { throw 'Experimental build directory already exists' }

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
    $status = $proc.ExitCode
    $timer.Stop()
    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    [IO.File]::WriteAllText((Join-Path $base "$name.stdout.log"), $stdout, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $base "$name.stderr.log"), $stderr, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $base "$name.exitcode"), [string]$status)
    [IO.File]::WriteAllText((Join-Path $base "$name.elapsed.txt"), ('{0:N3}s' -f $timer.Elapsed.TotalSeconds))
    return [pscustomobject]@{ name=$name; nativeExit=$status; elapsedSeconds=[math]::Round($timer.Elapsed.TotalSeconds,3) }
}

$results = @()
$configureArgs = [string[]]@('-S', $project, '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143', '-DJE_PROBE_MODE:STRING=explicit-experimental')
if ($configureArgs.Count -ne 11) { throw 'Experimental configure argv count mismatch' }
$results += Invoke-Native 'experimental-c11atomics-configure' $cmake $configureArgs $base
if ($results[-1].nativeExit -eq 0) {
    $vcxproj = Join-Path $build 'JEAtomicProbe.vcxproj'
    if (!(Test-Path -LiteralPath $vcxproj)) { throw 'Experimental project file missing' }
    Copy-Item -LiteralPath $vcxproj -Destination (Join-Path $base 'explicit-c-standard-11-experimental.vcxproj')
    foreach ($configuration in @('Debug','Release')) {
        $buildArgs = [string[]]@('--build', $build, '--config', $configuration, '--target', 'JEAtomicProbe', '--verbose')
        if ($buildArgs.Count -ne 7) { throw 'Experimental build argv count mismatch' }
        $results += Invoke-Native "experimental-c11atomics-build-$configuration" $cmake $buildArgs $base
        if ($results[-1].nativeExit -eq 0) {
            $exe = Join-Path $build "$configuration\JEAtomicProbe.exe"
            $results += Invoke-Native "experimental-c11atomics-run-$configuration" $exe ([string[]]@()) $base
        }
    }
}
$results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $base 'experimental-results.json')
$results | Select-Object name,nativeExit,elapsedSeconds | Format-Table -AutoSize
