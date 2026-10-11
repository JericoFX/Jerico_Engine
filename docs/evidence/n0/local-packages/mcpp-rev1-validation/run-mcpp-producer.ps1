param()
$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $repo 'build/n0-validation/local-packages/mcpp-rev1-validation'
$freezePath = Join-Path $repo 'build/n0-validation/local-packages/mcpp-audit/source-freeze-parent17.json'
$freeze = Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $freezePath).Hash -ne '55E60069627B607D2C8B487FDE44222A830EEA6247BA4ACDFFC286C5353CA508') { throw 'freeze hash mismatch' }
$pre = [ordered]@{}
foreach ($item in $freeze.inputs) {
    if (-not (Test-Path -LiteralPath $item.path -PathType Leaf)) { throw "missing input $($item.path)" }
    $fi = Get-Item -LiteralPath $item.path
    $h = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.path).Hash.ToLowerInvariant()
    if ($fi.Length -ne $item.bytes -or $h -ne $item.sha256) { throw "freeze mismatch $($item.path)" }
    $pre[$item.path] = $h
}
$output = Join-Path $base 'mcpp-local-v1'
if (Test-Path -LiteralPath $output) { throw 'producer output already exists' }
$cmake = (Get-Command cmake).Source
$argv = [string[]]@(
    '-DJE_MCPP_SOURCE_ARCHIVE:FILEPATH=F:/JericoEngine/Dependencies/archives/mcpp-2.7.2.tar.gz',
    '-DJE_MCPP_SOURCE_SHA256:STRING=3b9b4421888519876c4fc68ade324a3bbd81ceeb7092ecdbbc2055099fcb8864',
    '-DJE_MCPP_SOURCE_URL:STRING=https://sourceforge.net/projects/mcpp/files/mcpp/V.2.7.2/mcpp-2.7.2.tar.gz/download',
    "-DJE_MCPP_OUTPUT:PATH=$output",
    '-P',
    "$repo/cmake/Tools/LocalPackages/mcpp/BuildMcppPackage.cmake"
)
if ($argv.Count -ne 6 -or $argv[4] -ne '-P' -or $argv[5] -notlike '*BuildMcppPackage.cmake') { throw "argv guard failed: $($argv.Count)" }
$record = [ordered]@{
    executable = $cmake
    workingDirectory = $base
    argumentCount = $argv.Count
    arguments = $argv
    freezeSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $freezePath).Hash.ToLowerInvariant()
    sourceArchiveSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath 'F:/JericoEngine/Dependencies/archives/mcpp-2.7.2.tar.gz').Hash.ToLowerInvariant()
    freezeInputsBefore = $pre
    startedUtc = [DateTime]::UtcNow.ToString('o')
}
$record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $base 'mcpp-producer-argv-and-before.json') -Encoding utf8
$psi = [Diagnostics.ProcessStartInfo]::new()
$psi.FileName = $cmake
$psi.WorkingDirectory = $base
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
foreach ($arg in $argv) { [void]$psi.ArgumentList.Add($arg) }
$proc = [Diagnostics.Process]::new()
$proc.StartInfo = $psi
$sw = [Diagnostics.Stopwatch]::StartNew()
[void]$proc.Start()
$outTask = $proc.StandardOutput.ReadToEndAsync()
$errTask = $proc.StandardError.ReadToEndAsync()
$peakWs = 0L
$peakVm = 0L
$samples = 0
while (-not $proc.HasExited) {
    try {
        $proc.Refresh()
        if ($proc.WorkingSet64 -gt $peakWs) { $peakWs = $proc.WorkingSet64 }
        if ($proc.VirtualMemorySize64 -gt $peakVm) { $peakVm = $proc.VirtualMemorySize64 }
        $samples++
    } catch {}
    Start-Sleep -Milliseconds 500
}
$proc.WaitForExit()
$nativeExit = $proc.ExitCode
$sw.Stop()
$stdout = $outTask.GetAwaiter().GetResult()
$stderr = $errTask.GetAwaiter().GetResult()
[IO.File]::WriteAllText((Join-Path $base 'mcpp-producer.log'), $stdout, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $base 'mcpp-producer.stderr.log'), $stderr, [Text.UTF8Encoding]::new($false))
$post = [ordered]@{}
$changed = @()
foreach ($item in $freeze.inputs) {
    $h = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.path).Hash.ToLowerInvariant()
    $post[$item.path] = $h
    if ($h -ne $pre[$item.path]) { $changed += $item.path }
}
$outputBytes = 0L
if (Test-Path -LiteralPath $output) {
    $outputBytes = (Get-ChildItem -LiteralPath $output -File -Recurse | Measure-Object -Property Length -Sum).Sum
}
$result = [ordered]@{
    exitCode = $nativeExit
    elapsedSeconds = [Math]::Round($sw.Elapsed.TotalSeconds, 3)
    endedUtc = [DateTime]::UtcNow.ToString('o')
    parentCmakePeakWorkingSetBytes = $peakWs
    parentCmakePeakVirtualBytes = $peakVm
    parentCmakeSamples = $samples
    resourceScope = 'direct CMake process only; child MSBuild/compiler/linker totals are not aggregated'
    freezeInputsAfter = $post
    changedFreezeInputs = $changed
    outputExists = (Test-Path -LiteralPath $output)
    outputBytes = $outputBytes
}
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $base 'mcpp-producer-result.json') -Encoding utf8
[pscustomobject]@{ ExitCode = $nativeExit; ElapsedSeconds = $result.elapsedSeconds; OutputExists = $result.outputExists; OutputBytes = $outputBytes; FreezeChanges = $changed.Count; PeakCMakeWorkingSetBytes = $peakWs; Samples = $samples }
