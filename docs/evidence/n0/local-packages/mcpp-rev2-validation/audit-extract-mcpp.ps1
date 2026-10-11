$ErrorActionPreference = 'Stop'
$base = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-rev2-validation'
$pkg = Join-Path $base 'mcpp-local-v2/packages'
$name = 'mcpp-2.7.2_az.2-je-rev1-windows'
$archive = Join-Path $pkg ($name + '.tar.xz')
$sidecar = Join-Path $pkg ($name + '.tar.xz.SHA256SUMS')
$fileList = Join-Path $pkg ($name + '.files')
$manifestSource = Join-Path $pkg ($name + '.contents/SHA256SUMS')
$manifest = Join-Path $pkg ($name + '.contents/SHA256SUMS')
$extract = Join-Path $base 'mcpp-extracted-v2'
$cmake = 'C:/Program Files/CMake/bin/cmake.exe'
if (Test-Path -LiteralPath $extract) { throw "extraction path already exists: $extract" }
$actualArchiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
$expectedArchiveHash = ((Get-Content -LiteralPath $sidecar -Raw) -split '\s+')[0].ToLowerInvariant()
if ($actualArchiveHash -ne $expectedArchiveHash) { throw "archive hash mismatch: $actualArchiveHash != $expectedArchiveHash" }
$expectedMembers = @(Get-Content -LiteralPath $fileList | ForEach-Object { if ($_ -like '--add-file=*') { $_.Substring(11) } else { $_ } } | Where-Object { $_ })
$listPsi = [Diagnostics.ProcessStartInfo]::new()
$listPsi.FileName = $cmake
$listPsi.WorkingDirectory = $pkg
$listPsi.UseShellExecute = $false
$listPsi.RedirectStandardOutput = $true
$listPsi.RedirectStandardError = $true
foreach ($arg in [string[]]@('-E','tar','tf',$archive)) { [void]$listPsi.ArgumentList.Add($arg) }
$listProc = [Diagnostics.Process]::new()
$listProc.StartInfo = $listPsi
$listWatch = [Diagnostics.Stopwatch]::StartNew()
[void]$listProc.Start()
$listOutTask = $listProc.StandardOutput.ReadToEndAsync()
$listErrTask = $listProc.StandardError.ReadToEndAsync()
$listProc.WaitForExit()
$listExit = $listProc.ExitCode
$membersText = $listOutTask.GetAwaiter().GetResult()
$listErr = $listErrTask.GetAwaiter().GetResult()
$listWatch.Stop()
[IO.File]::WriteAllText((Join-Path $base 'mcpp-archive-list.stdout.log'), $membersText, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $base 'mcpp-archive-list.stderr.log'), $listErr, [Text.UTF8Encoding]::new($false))
if ($listExit -ne 0) { throw "cmake tar list failed: $listExit" }
$members = @($membersText -split '\r?\n' | Where-Object { $_ })
foreach ($member in $members) {
  if ($member.StartsWith('/') -or $member -match '^[A-Za-z]:' -or $member -split '/' -contains '..') { throw "unsafe archive member: $member" }
}
$sortedExpected = @($expectedMembers | Sort-Object -Unique)
$sortedActual = @($members | Sort-Object -Unique)
if (Compare-Object -ReferenceObject $sortedExpected -DifferenceObject $sortedActual) { throw 'archive member list differs from producer file list' }
[IO.Directory]::CreateDirectory($extract) | Out-Null
$extractPsi = [Diagnostics.ProcessStartInfo]::new()
$extractPsi.FileName = $cmake
$extractPsi.WorkingDirectory = $extract
$extractPsi.UseShellExecute = $false
$extractPsi.RedirectStandardOutput = $true
$extractPsi.RedirectStandardError = $true
foreach ($arg in [string[]]@('-E','tar','xJf',$archive)) { [void]$extractPsi.ArgumentList.Add($arg) }
$extractProc = [Diagnostics.Process]::new()
$extractProc.StartInfo = $extractPsi
$extractWatch = [Diagnostics.Stopwatch]::StartNew()
[void]$extractProc.Start()
$extractOutTask = $extractProc.StandardOutput.ReadToEndAsync()
$extractErrTask = $extractProc.StandardError.ReadToEndAsync()
$extractProc.WaitForExit()
$extractExit = $extractProc.ExitCode
$extractOut = $extractOutTask.GetAwaiter().GetResult()
$extractErr = $extractErrTask.GetAwaiter().GetResult()
$extractWatch.Stop()
[IO.File]::WriteAllText((Join-Path $base 'mcpp-archive-extract.stdout.log'), $extractOut, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $base 'mcpp-archive-extract.stderr.log'), $extractErr, [Text.UTF8Encoding]::new($false))
if ($extractExit -ne 0) { throw "cmake tar extract failed: $extractExit" }
$manifestExtracted = Join-Path $extract 'SHA256SUMS'
$manifestLines = Get-Content -LiteralPath $manifestExtracted
$checked = 0
foreach ($line in $manifestLines) {
  if ($line -match '^([0-9a-fA-F]{64}) \*(.+)$') {
    $relative = $Matches[2]
    if ($relative.StartsWith('/') -or $relative -match '^[A-Za-z]:' -or $relative -split '/' -contains '..') { throw "unsafe manifest path: $relative" }
    $path = Join-Path $extract ($relative -replace '/', '\\')
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "missing manifest payload: $relative" }
    $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $Matches[1].ToLowerInvariant()) { throw "payload hash mismatch: $relative" }
    $checked++
  } else { throw "invalid manifest line: $line" }
}
$info = Get-Content -LiteralPath (Join-Path $extract 'PackageInfo.json') -Raw | ConvertFrom-Json
$record = [ordered]@{ archiveSha256=$actualArchiveHash; expectedArchiveSha256=$expectedArchiveHash; listedMembers=$members.Count; manifestPayloadFiles=$checked; listExit=$listExit; listSeconds=$listWatch.Elapsed.TotalSeconds; extractExit=$extractExit; extractSeconds=$extractWatch.Elapsed.TotalSeconds; extractedRoot=$extract; packageInfo=$info; extractedFiles=@(Get-ChildItem -LiteralPath $extract -Recurse -File | ForEach-Object { [ordered]@{ path=$_.FullName.Substring($extract.Length+1); bytes=$_.Length; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() } }) }
[IO.File]::WriteAllText((Join-Path $base 'mcpp-archive-audit.json'), ($record | ConvertTo-Json -Depth 6), [Text.UTF8Encoding]::new($false))
"ArchiveSHA=$actualArchiveHash members=$($members.Count) checkedPayload=$checked listExit=$listExit extractExit=$extractExit"
