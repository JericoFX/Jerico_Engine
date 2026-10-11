$ErrorActionPreference = 'Stop'
$repo = 'F:/JericoEngine/JericoEngine'
$base = Join-Path $repo 'build/n0-validation/local-packages/expat-rev1-validation'
$freezePath = Join-Path $repo 'build/n0-validation/local-packages/expat-audit/source-freeze-writer14.json'
$expectedFreeze = '9B21A29F8AF2CF738B99FD45415A956C35F9DC23538AFFDC8C43BFC3CA5124B0'
$freezeActual = (Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash
if ($freezeActual -ne $expectedFreeze) { throw "Freeze manifest hash mismatch: $freezeActual" }
$freeze = Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json
$before = @()
foreach ($input in $freeze.inputs) {
  $path = [string]$input.path
  $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
  if ($hash -ne [string]$input.sha256) { throw "Frozen input mismatch: $path ($hash)" }
  $before += [ordered]@{ path=$path; bytes=(Get-Item -LiteralPath $path).Length; sha256=$hash }
}
if ($before.Count -ne 14) { throw "Expected 14 frozen inputs, found $($before.Count)" }
$output = Join-Path $base 'expat-local-v1'
if (Test-Path -LiteralPath $output) { throw 'Producer output already exists; refusing overwrite' }
$cmake = 'C:/Program Files/CMake/bin/cmake.exe'
if (-not (Test-Path -LiteralPath $cmake)) { throw "CMake missing: $cmake" }
$argumentVector = [string[]]@(
  '-DJE_EXPAT_SOURCE_ARCHIVE:FILEPATH=F:/JericoEngine/Dependencies/archives/expat-e26a263aa9182d1e4552cd615857f0f61df3ba42.tar.gz',
  '-DJE_EXPAT_SOURCE_SHA256:STRING=a6130cab18d9df124dbfe4045f57f8c2180811c35d37bc357a42cff61afa4057',
  '-DJE_EXPAT_SOURCE_URL:STRING=https://codeload.github.com/libexpat/libexpat/tar.gz/e26a263aa9182d1e4552cd615857f0f61df3ba42',
  "-DJE_EXPAT_OUTPUT:PATH=$output",
  '-P',
  "$repo/cmake/Tools/LocalPackages/Expat/BuildExpatPackage.cmake"
)
if ($argumentVector.Count -ne 6 -or $argumentVector[4] -ne '-P' -or $argumentVector[5] -ne "$repo/cmake/Tools/LocalPackages/Expat/BuildExpatPackage.cmake") { throw 'Argument vector failed mode/count guard' }
$argvPath = Join-Path $base 'expat-producer-argv.json'
$beforePath = Join-Path $base 'expat-producer-inputs-before.json'
$afterPath = Join-Path $base 'expat-producer-inputs-after.json'
$stdoutPath = Join-Path $base 'expat-producer.stdout.log'
$stderrPath = Join-Path $base 'expat-producer.stderr.log'
$resultPath = Join-Path $base 'expat-producer-result.json'
[IO.File]::WriteAllText($argvPath, (@{ executable=$cmake; workingDirectory=$base; arguments=$argumentVector } | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText($beforePath, (@{ freezeManifestSHA256=$freezeActual; inputCount=$before.Count; inputs=$before } | ConvertTo-Json -Depth 5), [Text.UTF8Encoding]::new($false))
$psi = [Diagnostics.ProcessStartInfo]::new()
$psi.FileName = $cmake
$psi.WorkingDirectory = $base
$psi.UseShellExecute = $false
$psi.CreateNoWindow = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
foreach ($arg in $argumentVector) { [void]$psi.ArgumentList.Add($arg) }
$proc = [Diagnostics.Process]::new()
$proc.StartInfo = $psi
$sw = [Diagnostics.Stopwatch]::StartNew()
if (-not $proc.Start()) { throw 'CMake process failed to start' }
$stdoutTask = $proc.StandardOutput.ReadToEndAsync()
$stderrTask = $proc.StandardError.ReadToEndAsync()
$proc.WaitForExit()
$nativeExit = $proc.ExitCode
$sw.Stop()
$stdout = $stdoutTask.GetAwaiter().GetResult()
$stderr = $stderrTask.GetAwaiter().GetResult()
[IO.File]::WriteAllText($stdoutPath, $stdout, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText($stderrPath, $stderr, [Text.UTF8Encoding]::new($false))
$after = @()
foreach ($input in $freeze.inputs) {
  $path = [string]$input.path
  $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
  $after += [ordered]@{ path=$path; bytes=(Get-Item -LiteralPath $path).Length; sha256=$hash; matchesFreeze=($hash -eq [string]$input.sha256) }
}
[IO.File]::WriteAllText($afterPath, (@{ inputCount=$after.Count; inputs=$after } | ConvertTo-Json -Depth 5), [Text.UTF8Encoding]::new($false))
$result = [ordered]@{ exitCode=$nativeExit; elapsedSeconds=[Math]::Round($sw.Elapsed.TotalSeconds,3); stdoutBytes=[Text.Encoding]::UTF8.GetByteCount($stdout); stderrBytes=[Text.Encoding]::UTF8.GetByteCount($stderr); outputExists=(Test-Path -LiteralPath $output); inputHashesAllMatch=(@($after | Where-Object { -not $_.matchesFreeze }).Count -eq 0) }
[IO.File]::WriteAllText($resultPath, ($result | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
Write-Output ($result | ConvertTo-Json -Compress)
