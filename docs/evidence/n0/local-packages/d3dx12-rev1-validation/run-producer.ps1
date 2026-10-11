$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $repo 'build\n0-validation\local-packages\d3dx12-rev1-validation'
$out = Join-Path $base 'd3dx12-local-v1'
$freezePath = Join-Path $repo 'build\n0-validation\local-packages\d3dx12-audit\source-freeze-writer12.json'
$freeze = Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$script = Join-Path $repo 'cmake\Tools\LocalPackages\d3dx12\BuildD3dx12Package.cmake'
$archive = 'F:/JericoEngine/Dependencies/archives/o3de-3p-70f0b7b5257bf7cf935bf1cfd270e1b53ccead18.tar.gz'
$sdkHeader = 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\d3d12.h'

if (Test-Path -LiteralPath $out) { throw "Output already exists: $out" }
if (!(Test-Path -LiteralPath $cmake)) { throw "CMake missing: $cmake" }
$before = @()
foreach ($i in $freeze.inputs) {
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $i.path).Hash.ToLowerInvariant()
    $before += [pscustomobject]@{ path = $i.path; expected = $i.sha256; actual = $actual; match = ($actual -eq $i.sha256) }
}
if ($before.Count -ne 12 -or @($before | Where-Object { !$_.match }).Count) {
    $before | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $base 'producer-inputs-before.json')
    throw 'Freeze mismatch before producer'
}
$sdkBefore = [pscustomobject]@{ path = $sdkHeader; exists = (Test-Path -LiteralPath $sdkHeader); bytes = (Get-Item -LiteralPath $sdkHeader).Length; sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $sdkHeader).Hash.ToLowerInvariant() }
$argv = [string[]]@(
    "-DJE_D3DX12_SOURCE_ARCHIVE=$archive",
    '-DJE_D3DX12_SOURCE_SHA256=f69edb8bdf9d3dcd83cf296db31130f4d16c82efb9b4cdebd76c8035ccaa6042',
    '-DJE_D3DX12_SOURCE_URL=https://codeload.github.com/o3de/3p-package-source/tar.gz/70f0b7b5257bf7cf935bf1cfd270e1b53ccead18',
    "-DJE_D3DX12_OUTPUT=$out",
    '-P',
    $script
)
if ($argv.Count -ne 6 -or $argv[4] -ne '-P' -or $argv[5] -ne $script) { throw 'Producer argv guard failed' }
$argv | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $base 'producer-argv.json')
$before | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $base 'producer-inputs-before.json')
$sdkBefore | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $base 'windows-sdk-witness-before.json')

$psi = [Diagnostics.ProcessStartInfo]::new()
$psi.FileName = $cmake
$psi.WorkingDirectory = $base
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
foreach ($arg in $argv) { [void]$psi.ArgumentList.Add($arg) }
$process = [Diagnostics.Process]::new()
$process.StartInfo = $psi
$timer = [Diagnostics.Stopwatch]::StartNew()
if (!$process.Start()) { throw 'Failed to start CMake' }
$stdoutTask = $process.StandardOutput.ReadToEndAsync()
$stderrTask = $process.StandardError.ReadToEndAsync()
$process.WaitForExit()
$nativeExit = $process.ExitCode
$timer.Stop()
$stdout = $stdoutTask.GetAwaiter().GetResult()
$stderr = $stderrTask.GetAwaiter().GetResult()
[IO.File]::WriteAllText((Join-Path $base 'producer.stdout.log'), $stdout, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $base 'producer.stderr.log'), $stderr, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $base 'producer.exitcode'), [string]$nativeExit)
[IO.File]::WriteAllText((Join-Path $base 'producer.elapsed.txt'), ('{0:N3}s' -f $timer.Elapsed.TotalSeconds))
$after = @()
foreach ($i in $freeze.inputs) {
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $i.path).Hash.ToLowerInvariant()
    $after += [pscustomobject]@{ path = $i.path; expected = $i.sha256; actual = $actual; match = ($actual -eq $i.sha256) }
}
$after | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $base 'producer-inputs-after.json')
$sdkAfter = [pscustomobject]@{ path = $sdkHeader; exists = (Test-Path -LiteralPath $sdkHeader); bytes = (Get-Item -LiteralPath $sdkHeader).Length; sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $sdkHeader).Hash.ToLowerInvariant() }
$sdkAfter | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $base 'windows-sdk-witness-after.json')
[pscustomobject]@{
    exitCode = $nativeExit
    elapsedSeconds = [math]::Round($timer.Elapsed.TotalSeconds, 3)
    freezeCount = $after.Count
    freezeAllMatch = (@($after | Where-Object { !$_.match }).Count -eq 0)
    sdkBefore = $sdkBefore
    sdkAfter = $sdkAfter
    outputExists = (Test-Path -LiteralPath $out)
    stdout = Join-Path $base 'producer.stdout.log'
    stderr = Join-Path $base 'producer.stderr.log'
} | ConvertTo-Json -Depth 5
