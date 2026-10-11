$ErrorActionPreference = 'Stop'
$repo = 'F:/JericoEngine/JericoEngine'
$base = Join-Path $repo 'build/n0-validation/local-packages/mcpp-rev2-validation'
$out = Join-Path $base 'mcpp-local-v2'
$freezePath = Join-Path $repo 'build/n0-validation/local-packages/mcpp-audit/source-freeze-parent17-rev2.json'
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
$before = @()
foreach ($input in $freeze.inputs) {
  $before += [ordered]@{ path=$input.path; bytes=(Get-Item -LiteralPath $input.path).Length; sha256=(Get-FileHash -LiteralPath $input.path -Algorithm SHA256).Hash.ToLowerInvariant() }
}
$cmake = 'C:/Program Files/CMake/bin/cmake.exe'
$argv = [string[]]@(
  '-DJE_MCPP_SOURCE_ARCHIVE:FILEPATH=F:/JericoEngine/Dependencies/archives/mcpp-2.7.2.tar.gz',
  '-DJE_MCPP_SOURCE_SHA256:STRING=3b9b4421888519876c4fc68ade324a3bbd81ceeb7092ecdbbc2055099fcb8864',
  '-DJE_MCPP_SOURCE_URL:STRING=https://sourceforge.net/projects/mcpp/files/mcpp/V.2.7.2/mcpp-2.7.2.tar.gz/download',
  "-DJE_MCPP_OUTPUT:PATH=$out",
  '-P',
  'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/mcpp/BuildMcppPackage.cmake'
)
if ($argv.Count -ne 6 -or $argv[4] -ne '-P' -or $argv[5] -notlike '*BuildMcppPackage.cmake') { throw 'argv guard failed' }
if (Test-Path -LiteralPath $out) { throw 'output exists before producer' }
[IO.File]::WriteAllText((Join-Path $base 'mcpp-producer-argv.json'), (@{ executable=$cmake; workingDirectory=$base; argumentCount=$argv.Count; arguments=$argv; freezeSha256=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash.ToLowerInvariant(); freezeInputsBefore=$before; startedUtc=[DateTime]::UtcNow.ToString('o') } | ConvertTo-Json -Depth 5), [Text.UTF8Encoding]::new($false))
$psi = [Diagnostics.ProcessStartInfo]::new()
$psi.FileName = $cmake
$psi.WorkingDirectory = $base
$psi.UseShellExecute = $false
$psi.CreateNoWindow = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
foreach ($arg in $argv) { [void]$psi.ArgumentList.Add($arg) }
$p = [Diagnostics.Process]::new()
$p.StartInfo = $psi
$sw = [Diagnostics.Stopwatch]::StartNew()
if (-not $p.Start()) { throw 'CMake did not start' }
$outTask = $p.StandardOutput.ReadToEndAsync()
$errTask = $p.StandardError.ReadToEndAsync()
$p.WaitForExit()
$nativeExit = $p.ExitCode
$stdout = $outTask.GetAwaiter().GetResult()
$stderr = $errTask.GetAwaiter().GetResult()
$sw.Stop()
[IO.File]::WriteAllText((Join-Path $base 'mcpp-producer.stdout.log'), $stdout, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $base 'mcpp-producer.stderr.log'), $stderr, [Text.UTF8Encoding]::new($false))
$after = @()
foreach ($input in $freeze.inputs) {
  $after += [ordered]@{ path=$input.path; bytes=(Get-Item -LiteralPath $input.path).Length; sha256=(Get-FileHash -LiteralPath $input.path -Algorithm SHA256).Hash.ToLowerInvariant() }
}
$record = [ordered]@{ exitCode=$nativeExit; elapsedSeconds=$sw.Elapsed.TotalSeconds; endedUtc=[DateTime]::UtcNow.ToString('o'); freezeSha256=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash.ToLowerInvariant(); freezeInputsAfter=$after; outputExists=(Test-Path -LiteralPath $out) }
[IO.File]::WriteAllText((Join-Path $base 'mcpp-producer-result.json'), ($record | ConvertTo-Json -Depth 5), [Text.UTF8Encoding]::new($false))
"ExitCode=$nativeExit ElapsedSeconds=$($sw.Elapsed.TotalSeconds) OutputExists=$($record.outputExists) Evidence=$base"
