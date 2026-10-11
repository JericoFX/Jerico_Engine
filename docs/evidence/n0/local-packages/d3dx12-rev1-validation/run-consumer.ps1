$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $repo 'build\n0-validation\local-packages\d3dx12-rev1-validation'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$ctest = 'C:\Program Files\CMake\bin\ctest.exe'
$source = Join-Path $repo 'cmake\Tools\LocalPackages\d3dx12\tests'
$archive = Join-Path $base 'd3dx12-local-v1\packages\d3dx12-headers-je-rev1-windows.tar.xz'
$package = Join-Path $base 'extracted\d3dx12-headers-je-rev1-windows'
$build = Join-Path $base 'consumer'
$sdkHeader = 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\d3d12.h'
$inputFreeze = Join-Path $repo 'build\n0-validation\local-packages\d3dx12-audit\source-freeze-writer12.json'
$freeze = Get-Content -Raw -LiteralPath $inputFreeze | ConvertFrom-Json

function Invoke-Native([string]$name, [string]$exe, [string[]]$vector, [string]$cwd) {
    if (!$vector) { throw "Empty argv for $name" }
    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $exe
    $psi.WorkingDirectory = $cwd
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    foreach ($arg in $vector) { [void]$psi.ArgumentList.Add($arg) }
    $vector | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $base "$name.argv.json")
    $proc = [Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    $timer = [Diagnostics.Stopwatch]::StartNew()
    if (!$proc.Start()) { throw "Could not start $name" }
    $stdoutTask = $proc.StandardOutput.ReadToEndAsync()
    $stderrTask = $proc.StandardError.ReadToEndAsync()
    $proc.WaitForExit()
    $nativeExit = $proc.ExitCode
    $timer.Stop()
    [IO.File]::WriteAllText((Join-Path $base "$name.stdout.log"), $stdoutTask.GetAwaiter().GetResult(), [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $base "$name.stderr.log"), $stderrTask.GetAwaiter().GetResult(), [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $base "$name.exitcode"), [string]$nativeExit)
    [IO.File]::WriteAllText((Join-Path $base "$name.elapsed.txt"), ('{0:N3}s' -f $timer.Elapsed.TotalSeconds))
    if ($nativeExit -ne 0) { throw "$name failed with native exit $nativeExit" }
    return [pscustomobject]@{ name = $name; exitCode = $nativeExit; elapsedSeconds = [math]::Round($timer.Elapsed.TotalSeconds, 3) }
}

if (Test-Path -LiteralPath $package) { throw "Extraction root already exists: $package" }
if (Test-Path -LiteralPath $build) { throw "Consumer build root already exists: $build" }
[IO.Directory]::CreateDirectory($package) | Out-Null
$extract = Invoke-Native 'extract' $cmake ([string[]]@('-E', 'tar', 'xJf', $archive)) $package
$names = @(
    'Findd3dx12.cmake', 'JERequireLocalPath.cmake', 'LICENSES/O3DE/LICENSE.txt',
    'LICENSES/O3DE/LICENSE_APACHE2.TXT', 'LICENSES/O3DE/LICENSE_MIT.TXT',
    'LICENSES/O3DE/OriginalFindd3dx12.cmake', 'LICENSES/O3DE/OriginalPackageInfo.json',
    'NOTICE.txt', 'PackageInfo.json', 'README.md', 'd3dx12/JERecipe.json',
    'd3dx12/include/d3dx12.h', 'd3dx12/license.txt', 'SHA256SUMS'
)
$extracted = @()
foreach ($name in $names) {
    $path = Join-Path $package ($name -replace '/', '\')
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing package member: $name" }
    $extracted += [pscustomobject]@{ path = $name; bytes = (Get-Item -LiteralPath $path).Length; sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant() }
}
$extracted | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $base 'package-extracted-manifest.json')
$packageInfo = Get-Content -Raw -LiteralPath (Join-Path $package 'PackageInfo.json') | ConvertFrom-Json
$packageInfo | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $base 'package-info-audited.json')
$cmakeArgs = [string[]]@('-S', $source, '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143', "-DJE_D3DX12_PACKAGE_ROOT=$package")
if ($cmakeArgs.Count -ne 11 -or $cmakeArgs[0] -ne '-S' -or $cmakeArgs[2] -ne '-B' -or $cmakeArgs[8] -ne '-T') { throw 'Consumer configure argv guard failed' }
$results = @($extract)
$results += Invoke-Native 'consumer-configure' $cmake $cmakeArgs $base
$results += Invoke-Native 'consumer-build-release' $cmake ([string[]]@('--build', $build, '--config', 'Release', '--target', 'JED3dx12Consumer')) $base
$results += Invoke-Native 'consumer-ctest-release' $ctest ([string[]]@('--test-dir', $build, '-C', 'Release', '--output-on-failure')) $base
$lastTest = Join-Path $build 'Testing\Temporary\LastTest.log'
if (!(Test-Path -LiteralPath $lastTest)) { throw 'Release LastTest.log missing' }
Copy-Item -LiteralPath $lastTest -Destination (Join-Path $base 'LastTest-Release.log')
$results += Invoke-Native 'consumer-build-debug' $cmake ([string[]]@('--build', $build, '--config', 'Debug', '--target', 'JED3dx12Consumer')) $base
$results += Invoke-Native 'consumer-ctest-debug' $ctest ([string[]]@('--test-dir', $build, '-C', 'Debug', '--output-on-failure')) $base
if (!(Test-Path -LiteralPath $lastTest)) { throw 'Debug LastTest.log missing' }
Copy-Item -LiteralPath $lastTest -Destination (Join-Path $base 'LastTest-Debug.log')
$results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $base 'consumer-run-results.json')
$after = @()
foreach ($i in $freeze.inputs) {
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $i.path).Hash.ToLowerInvariant()
    $after += [pscustomobject]@{ path = $i.path; expected = $i.sha256; actual = $actual; match = ($actual -eq $i.sha256) }
}
$after | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $base 'consumer-inputs-after.json')
$sdkAfter = [pscustomobject]@{ path = $sdkHeader; exists = (Test-Path -LiteralPath $sdkHeader); bytes = (Get-Item -LiteralPath $sdkHeader).Length; sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $sdkHeader).Hash.ToLowerInvariant() }
$sdkAfter | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $base 'windows-sdk-witness-after-consumer.json')
[pscustomobject]@{ steps = $results; extractedFiles = $extracted.Count; packageInfo = $packageInfo; freezeCount = $after.Count; freezeAllMatch = (@($after | Where-Object { !$_.match }).Count -eq 0); sdkAfter = $sdkAfter } | ConvertTo-Json -Depth 6
