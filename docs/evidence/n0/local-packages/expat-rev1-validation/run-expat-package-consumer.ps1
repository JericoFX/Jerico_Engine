$ErrorActionPreference = 'Stop'
$repo = 'F:/JericoEngine/JericoEngine'
$base = Join-Path $repo 'build/n0-validation/local-packages/expat-rev1-validation'
$packageOutput = Join-Path $base 'expat-local-v1/packages/expat-2.4.2-je-rev1-windows.tar.xz'
$packageName = 'expat-2.4.2-je-rev1-windows'
$extract = Join-Path $base 'extracted'
$consumer = Join-Path $base 'consumer'
$cmake = 'C:/Program Files/CMake/bin/cmake.exe'
$ctest = 'C:/Program Files/CMake/bin/ctest.exe'
$tar = 'C:/Windows/system32/tar.exe'
function Invoke-Captured {
  param([string]$Label,[string]$Executable,[string[]]$ArgumentVector,[string]$WorkingDirectory)
  $record = Join-Path $base "$Label-argv.json"
  $stdoutPath = Join-Path $base "$Label.stdout.log"
  $stderrPath = Join-Path $base "$Label.stderr.log"
  $resultPath = Join-Path $base "$Label-result.json"
  [IO.File]::WriteAllText($record, (@{ executable=$Executable; workingDirectory=$WorkingDirectory; arguments=$ArgumentVector } | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
  $psi = [Diagnostics.ProcessStartInfo]::new()
  $psi.FileName = $Executable; $psi.WorkingDirectory = $WorkingDirectory; $psi.UseShellExecute = $false; $psi.CreateNoWindow = $true
  $psi.RedirectStandardOutput = $true; $psi.RedirectStandardError = $true
  foreach ($arg in $ArgumentVector) { [void]$psi.ArgumentList.Add($arg) }
  $proc = [Diagnostics.Process]::new(); $proc.StartInfo = $psi
  $sw = [Diagnostics.Stopwatch]::StartNew()
  if (-not $proc.Start()) { throw "$Label failed to start" }
  $outTask = $proc.StandardOutput.ReadToEndAsync(); $errTask = $proc.StandardError.ReadToEndAsync()
  $proc.WaitForExit(); $nativeExit = $proc.ExitCode; $sw.Stop()
  $stdout = $outTask.GetAwaiter().GetResult(); $stderr = $errTask.GetAwaiter().GetResult()
  [IO.File]::WriteAllText($stdoutPath,$stdout,[Text.UTF8Encoding]::new($false)); [IO.File]::WriteAllText($stderrPath,$stderr,[Text.UTF8Encoding]::new($false))
  $result = [ordered]@{ label=$Label; exitCode=$nativeExit; elapsedSeconds=[Math]::Round($sw.Elapsed.TotalSeconds,3); stdoutBytes=[Text.Encoding]::UTF8.GetByteCount($stdout); stderrBytes=[Text.Encoding]::UTF8.GetByteCount($stderr) }
  [IO.File]::WriteAllText($resultPath,($result | ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
  return [pscustomobject]$result
}
$freezePath = Join-Path $repo 'build/n0-validation/local-packages/expat-audit/source-freeze-writer14.json'
$freeze = Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json
$frozenBefore = @{}
foreach ($input in $freeze.inputs) { $frozenBefore[[string]$input.path]=(Get-FileHash -LiteralPath $input.path -Algorithm SHA256).Hash.ToLowerInvariant() }
$expectedPackageHash = 'c102370257ed28d9f9ea5e61ca10432edc14e8389aa781380beaa9b2a8ae6925'
$actualPackageHash = (Get-FileHash -LiteralPath $packageOutput -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actualPackageHash -ne $expectedPackageHash) { throw "Package archive hash mismatch: $actualPackageHash" }
$sidecar = Get-Content -Raw -LiteralPath "$packageOutput.SHA256SUMS"
if ($sidecar -notmatch [regex]::Escape($expectedPackageHash)) { throw 'Archive sidecar hash mismatch' }
if (Test-Path -LiteralPath $extract) { throw 'Extraction path already exists' }
[IO.Directory]::CreateDirectory($extract) | Out-Null
$extractArgs = [string[]]@('-E','tar','xJf',$packageOutput)
$extractResult = Invoke-Captured 'expat-package-extract' $cmake $extractArgs $extract
if ($extractResult.exitCode -ne 0) { throw "Archive extraction failed with $($extractResult.exitCode)" }
$archiveListing = Invoke-Captured 'expat-package-tar-list' $tar ([string[]]@('-tf',$packageOutput)) $base
if ($archiveListing.exitCode -ne 0) { throw 'tar listing failed' }
$archiveVerbose = Invoke-Captured 'expat-package-tar-verbose' $tar ([string[]]@('-tvf',$packageOutput)) $base
if ($archiveVerbose.exitCode -ne 0) { throw 'tar verbose listing failed' }
$archiveMembers = @(Get-Content -LiteralPath (Join-Path $base 'expat-package-tar-list.stdout.log') | Where-Object { $_ -ne '' })
$verboseLines = @(Get-Content -LiteralPath (Join-Path $base 'expat-package-tar-verbose.stdout.log') | Where-Object { $_ -ne '' })
$linkLines = @($verboseLines | Where-Object { $_ -match '^l|^h' })
$sumLines = @(Get-Content -LiteralPath (Join-Path $extract 'SHA256SUMS') | Where-Object { $_ -match '^[0-9a-fA-F]{64} \*' })
$checked = @()
foreach ($line in $sumLines) {
  if ($line -notmatch '^([0-9a-fA-F]{64}) \*(.+)$') { throw "Malformed SHA256SUMS line: $line" }
  $expected = $Matches[1].ToLowerInvariant(); $relative = $Matches[2]
  $path = Join-Path $extract $relative
  if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Manifest path missing: $relative" }
  $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
  if ($actual -ne $expected) { throw "Payload hash mismatch: $relative" }
  $checked += [ordered]@{ path=$relative; bytes=(Get-Item -LiteralPath $path).Length; sha256=$actual }
}
if ($checked.Count -ne $sumLines.Count -or $sumLines.Count -eq 0) { throw 'No complete package payload manifest validation' }
$packageInfo = Get-Content -Raw -LiteralPath (Join-Path $extract 'PackageInfo.json') | ConvertFrom-Json
$recipe = Get-Content -Raw -LiteralPath (Join-Path $extract 'expat/JERecipe.json') | ConvertFrom-Json
$manifest = [ordered]@{ archivePath=$packageOutput; archiveBytes=(Get-Item $packageOutput).Length; archiveSHA256=$actualPackageHash; archiveMemberCount=$archiveMembers.Count; archiveMembers=$archiveMembers; tarLinkEntryCount=$linkLines.Count; manifestEntryCount=$sumLines.Count; verifiedPayload=$checked; packageInfo=$packageInfo; recipe=$recipe; noReparsePoints=(@(Get-ChildItem -LiteralPath $extract -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }).Count -eq 0) }
[IO.File]::WriteAllText((Join-Path $base 'expat-package-audit.json'),($manifest | ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
if (-not $manifest.noReparsePoints -or $linkLines.Count -ne 0) { throw 'Unexpected link/reparse entry in package' }
if (Test-Path -LiteralPath $consumer) { throw 'Consumer path already exists' }
$configure = Invoke-Captured 'expat-consumer-configure' $cmake ([string[]]@('-S',"$repo/cmake/Tools/LocalPackages/Expat/tests",'-B',$consumer,'-G','Visual Studio 17 2022','-A','x64','-T','v143',"-DJE_EXPAT_PACKAGE_ROOT:PATH=$extract")) $base
if ($configure.exitCode -ne 0) { throw "Consumer configure failed: $($configure.exitCode)" }
foreach ($config in @('Release','Debug')) {
  $safe = $config.ToLowerInvariant()
  $buildResult = Invoke-Captured "expat-consumer-build-$safe" $cmake ([string[]]@('--build',$consumer,'--config',$config)) $base
  if ($buildResult.exitCode -ne 0) { throw "$config consumer build failed: $($buildResult.exitCode)" }
  $testResult = Invoke-Captured "expat-consumer-ctest-$safe" $ctest ([string[]]@('--test-dir',$consumer,'-C',$config,'--output-on-failure','--verbose')) $base
  $lastTest = Join-Path $consumer 'Testing/Temporary/LastTest.log'
  if (Test-Path -LiteralPath $lastTest) { Copy-Item -LiteralPath $lastTest -Destination (Join-Path $base "LastTest-$config.log") }
  if ($testResult.exitCode -ne 0) { throw "$config CTest failed: $($testResult.exitCode)" }
}
$frozenAfter = @()
foreach ($input in $freeze.inputs) { $path=[string]$input.path; $hash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant(); $frozenAfter += [ordered]@{ path=$path; sha256=$hash; matchesBefore=($hash -eq $frozenBefore[$path]); matchesFreeze=($hash -eq [string]$input.sha256) } }
[IO.File]::WriteAllText((Join-Path $base 'expat-source-freeze-after.json'),(@{ inputCount=$frozenAfter.Count; inputs=$frozenAfter } | ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
$summary = [ordered]@{ archiveSHA256=$actualPackageHash; memberCount=$archiveMembers.Count; payloadCount=$checked.Count; links=$linkLines.Count; packageName=$packageInfo.name; packageVersion=$packageInfo.version; configureExit=$configure.exitCode; releaseBuildAndCTest='exit 0'; debugBuildAndCTest='exit 0'; frozenInputsMatch=(@($frozenAfter | Where-Object { -not $_.matchesFreeze }).Count -eq 0) }
Write-Output ($summary | ConvertTo-Json -Compress)
