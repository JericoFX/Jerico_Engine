$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $repo 'build\n0-validation\local-packages'
$evidence = Join-Path $base 'openexr-rev1-validation'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$argsPath = Join-Path $base 'openexr-audit\validation-arguments.json'
$freezePath = Join-Path $base 'openexr-audit\source-freeze-openexr-candidate.json'
$arguments = Get-Content -Raw -LiteralPath $argsPath | ConvertFrom-Json
$freeze = Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json
$vector = [string[]]$arguments.producer.argv
if ($vector.Count -ne 6 -or $vector[4] -ne '-P' -or $vector[5] -ne 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/OpenEXR/BuildOpenEXRPackage.cmake') { throw 'Producer argv guard failed' }
$out = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openexr-rev1'
if (Test-Path -LiteralPath $out) { throw 'Producer output unexpectedly exists' }
$assignedOutput = [IO.Path]::GetFullPath([string]$arguments.producer.outputMustBeAbsent).TrimEnd([IO.Path]::DirectorySeparatorChar)
$actualOutput = [IO.Path]::GetFullPath($out).TrimEnd([IO.Path]::DirectorySeparatorChar)
if (![string]::Equals($assignedOutput, $actualOutput, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output assignment mismatch after Windows path normalization' }
if (!(Test-Path -LiteralPath $cmake)) { throw "CMake executable missing: $cmake" }
$launchPre = @()
foreach ($item in $freeze.entries) {
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.path).Hash.ToLowerInvariant()
    $actualBytes = (Get-Item -LiteralPath $item.path).Length
    $launchPre += [pscustomobject]@{ path=$item.path; expectedBytes=$item.bytes; actualBytes=$actualBytes; expectedSha256=$item.sha256; actualSha256=$actualHash; match=($actualBytes -eq $item.bytes -and $actualHash -eq $item.sha256) }
}
if ($launchPre.Count -ne 1182 -or @($launchPre | Where-Object { !$_.match }).Count) { throw 'Current launch freeze mismatch' }
$launchPre | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'source-freeze-producer-launch-before.json')
$zlibLaunch = @($launchPre | Where-Object { $_.path -like '*zlib-1.2.11-je-rev1-windows.contents*' })
if ($zlibLaunch.Count -ne 4 -or @($zlibLaunch | Where-Object { !$_.match }).Count) { throw 'Current zlib input mismatch' }
$zlibLaunch | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'zlib-package-producer-launch-before.json')
[ordered]@{ assignmentValue=$arguments.producer.outputMustBeAbsent; runnerValue=$out; normalizedAssignment=$assignedOutput; normalizedRunner=$actualOutput; windowsPathEquality=$true } | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $evidence 'producer-path-normalization.json')
$pre = Get-Content -Raw -LiteralPath (Join-Path $evidence 'source-freeze-before.json') | ConvertFrom-Json
if ($pre.Count -ne 1182 -or @($pre | Where-Object { !$_.match }).Count) { throw 'Verified freeze preflight missing/mismatched' }
$sourceArchiveSha = ($pre | Where-Object { $_.path -like '*openexr-697cc449*.tar.gz' } | Select-Object -First 1 -ExpandProperty actualSha256)
$imathArchiveSha = ($pre | Where-Object { $_.path -like '*Imath-6dc082*.tar.gz' } | Select-Object -First 1 -ExpandProperty actualSha256)
$vector | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $evidence 'producer-argv.json')
[ordered]@{ executable=$cmake; executableSha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $cmake).Hash.ToLowerInvariant(); workingDirectory=$evidence; argumentCount=$vector.Count; arguments=$vector; sourceArchiveSha256=$sourceArchiveSha; imathArchiveSha256=$imathArchiveSha; freezeCount=$pre.Count } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'producer-launch.json')
$psi = [Diagnostics.ProcessStartInfo]::new()
$psi.FileName = $cmake
$psi.WorkingDirectory = $evidence
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
foreach ($arg in $vector) { [void]$psi.ArgumentList.Add($arg) }
$proc = [Diagnostics.Process]::new()
$proc.StartInfo = $psi
$timer = [Diagnostics.Stopwatch]::StartNew()
if (!$proc.Start()) { throw 'CMake process failed to start' }
$stdoutTask = $proc.StandardOutput.ReadToEndAsync()
$stderrTask = $proc.StandardError.ReadToEndAsync()
$proc.WaitForExit()
$nativeExit = $proc.ExitCode
$timer.Stop()
$stdout = $stdoutTask.GetAwaiter().GetResult()
$stderr = $stderrTask.GetAwaiter().GetResult()
[IO.File]::WriteAllText((Join-Path $evidence 'producer.stdout.log'), $stdout, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $evidence 'producer.stderr.log'), $stderr, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $evidence 'producer.exitcode'), [string]$nativeExit)
[IO.File]::WriteAllText((Join-Path $evidence 'producer.elapsed.txt'), ('{0:N3}s' -f $timer.Elapsed.TotalSeconds))
$post = @()
foreach ($item in $freeze.entries) {
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.path).Hash.ToLowerInvariant()
    $size = (Get-Item -LiteralPath $item.path).Length
    $post += [pscustomobject]@{ path=$item.path; expectedBytes=$item.bytes; actualBytes=$size; expectedSha256=$item.sha256; actualSha256=$actual; match=($size -eq $item.bytes -and $actual -eq $item.sha256) }
}
$post | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'source-freeze-after-producer.json')
[pscustomobject]@{ nativeExit=$nativeExit; elapsedSeconds=[math]::Round($timer.Elapsed.TotalSeconds,3); freezeCount=$post.Count; freezeAllMatch=(@($post|Where-Object{!$_.match}).Count -eq 0); outputExists=(Test-Path -LiteralPath $out); stdout=Join-Path $evidence 'producer.stdout.log'; stderr=Join-Path $evidence 'producer.stderr.log' } | ConvertTo-Json -Depth 4
