$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $repo 'build\n0-validation\local-packages\d3dx12-rev1-validation'
$guards = Join-Path $base 'guards'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$producer = Join-Path $repo 'cmake\Tools\LocalPackages\d3dx12\BuildD3dx12Package.cmake'
$consumer = Join-Path $repo 'cmake\Tools\LocalPackages\d3dx12\tests'
$helper = Join-Path $repo 'cmake\Tools\LocalPackages\Python\JERequireLocalPath.cmake'
$findModule = Join-Path $repo 'cmake\Tools\LocalPackages\d3dx12\Findd3dx12.cmake'
$package = Join-Path $base 'extracted\d3dx12-headers-je-rev1-windows'
$header = Join-Path $package 'd3dx12\include\d3dx12.h'
$sourceArchive = 'F:/JericoEngine/Dependencies/archives/o3de-3p-70f0b7b5257bf7cf935bf1cfd270e1b53ccead18.tar.gz'
$goodHash = 'f69edb8bdf9d3dcd83cf296db31130f4d16c82efb9b4cdebd76c8035ccaa6042'
$goodUrl = 'https://codeload.github.com/o3de/3p-package-source/tar.gz/70f0b7b5257bf7cf935bf1cfd270e1b53ccead18'
$freezePath = Join-Path $repo 'build\n0-validation\local-packages\d3dx12-audit\source-freeze-writer12.json'
$freeze = Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json

function Invoke-Guard([string]$name, [string[]]$vector, [string]$expected, [string]$kind, [string]$outputPath) {
    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $cmake
    $psi.WorkingDirectory = $guards
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    foreach ($arg in $vector) { [void]$psi.ArgumentList.Add($arg) }
    $vector | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $guards "$name.argv.json")
    $proc = [Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    $timer = [Diagnostics.Stopwatch]::StartNew()
    if (!$proc.Start()) { throw "Could not start $name" }
    $outTask = $proc.StandardOutput.ReadToEndAsync()
    $errTask = $proc.StandardError.ReadToEndAsync()
    $proc.WaitForExit()
    $nativeExit = $proc.ExitCode
    $timer.Stop()
    $stdout = $outTask.GetAwaiter().GetResult()
    $stderr = $errTask.GetAwaiter().GetResult()
    [IO.File]::WriteAllText((Join-Path $guards "$name.stdout.log"), $stdout, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $guards "$name.stderr.log"), $stderr, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $guards "$name.exitcode"), [string]$nativeExit)
    [IO.File]::WriteAllText((Join-Path $guards "$name.elapsed.txt"), ('{0:N3}s' -f $timer.Elapsed.TotalSeconds))
    $combined = $stdout + "`n" + $stderr
    $diagnostic = $combined.Contains($expected)
    $exists = if ($outputPath) { Test-Path -LiteralPath $outputPath } else { $false }
    $pass = ($nativeExit -ne 0) -and $diagnostic
    if ($kind -eq 'no-output') { $pass = $pass -and !$exists }
    if ($kind -eq 'existing-sentinel') {
        $sentinelPath = Join-Path $outputPath 'sentinel.txt'
        $pass = $pass -and $exists -and (Test-Path -LiteralPath $sentinelPath) -and ((Get-Content -Raw -LiteralPath $sentinelPath) -eq 'preserve')
    }
    [pscustomobject]@{ name = $name; nativeExit = $nativeExit; expectedDiagnostic = $expected; diagnosticMatched = $diagnostic; outputPath = $outputPath; outputExists = $exists; result = if ($pass) { 'PASS' } else { 'NOT_PASS' }; elapsedSeconds = [math]::Round($timer.Elapsed.TotalSeconds, 3) }
}

[IO.Directory]::CreateDirectory($guards) | Out-Null
if (Test-Path -LiteralPath (Join-Path $guards 'results.json')) { throw 'Guard result already exists' }
$results = @()
$out = Join-Path $guards 'producer-path-output'
$v = [string[]]@("-DJE_D3DX12_SOURCE_ARCHIVE:STRING=\\server\share\absent.tar.gz", "-DJE_D3DX12_SOURCE_SHA256=$goodHash", "-DJE_D3DX12_SOURCE_URL=$goodUrl", "-DJE_D3DX12_OUTPUT=$out", '-P', $producer)
$results += Invoke-Guard 'producer-unc-source-path' $v 'must be an absolute local path' 'no-output' $out

$out = Join-Path $guards 'producer-hash-output'
$v = [string[]]@("-DJE_D3DX12_SOURCE_ARCHIVE=$sourceArchive", '-DJE_D3DX12_SOURCE_SHA256=0000000000000000000000000000000000000000000000000000000000000000', "-DJE_D3DX12_SOURCE_URL=$goodUrl", "-DJE_D3DX12_OUTPUT=$out", '-P', $producer)
$results += Invoke-Guard 'producer-wrong-hash' $v 'source URL/SHA256 must match the audited O3DE source archive' 'no-output' $out

$out = Join-Path $guards 'producer-url-output'
$v = [string[]]@("-DJE_D3DX12_SOURCE_ARCHIVE=$sourceArchive", "-DJE_D3DX12_SOURCE_SHA256=$goodHash", '-DJE_D3DX12_SOURCE_URL=https://invalid.example/wrong.tar.gz', "-DJE_D3DX12_OUTPUT=$out", '-P', $producer)
$results += Invoke-Guard 'producer-wrong-url' $v 'source URL/SHA256 must match the audited O3DE source archive' 'no-output' $out

$missingArchive = Join-Path $guards 'missing-source.tar.gz'
$out = Join-Path $guards 'producer-missing-output'
if (Test-Path -LiteralPath $missingArchive) { throw 'Missing-source fixture unexpectedly exists' }
$v = [string[]]@("-DJE_D3DX12_SOURCE_ARCHIVE=$missingArchive", "-DJE_D3DX12_SOURCE_SHA256=$goodHash", "-DJE_D3DX12_SOURCE_URL=$goodUrl", "-DJE_D3DX12_OUTPUT=$out", '-P', $producer)
$results += Invoke-Guard 'producer-missing-source' $v 'source archive file is missing' 'no-output' $out

$out = Join-Path $guards 'producer-existing-output'
[IO.Directory]::CreateDirectory($out) | Out-Null
[IO.File]::WriteAllText((Join-Path $out 'sentinel.txt'), 'preserve')
$v = [string[]]@("-DJE_D3DX12_SOURCE_ARCHIVE=$sourceArchive", "-DJE_D3DX12_SOURCE_SHA256=$goodHash", "-DJE_D3DX12_SOURCE_URL=$goodUrl", "-DJE_D3DX12_OUTPUT=$out", '-P', $producer)
$results += Invoke-Guard 'producer-existing-output' $v 'output already exists; retained outputs are never overwritten' 'existing-sentinel' $out

$outside = 'F:/JericoEngine/Dependencies/je-d3dx12-invalid-output'
if (Test-Path -LiteralPath $outside) { throw "Outside fixture unexpectedly exists: $outside" }
$v = [string[]]@("-DJE_D3DX12_SOURCE_ARCHIVE=$sourceArchive", "-DJE_D3DX12_SOURCE_SHA256=$goodHash", "-DJE_D3DX12_SOURCE_URL=$goodUrl", "-DJE_D3DX12_OUTPUT=$outside", '-P', $producer)
$results += Invoke-Guard 'producer-outside-build-output' $v 'output must be beneath this checkout/build' 'no-output' $outside

$missingRoot = Join-Path $guards 'find-missing-root'
[IO.Directory]::CreateDirectory($missingRoot) | Out-Null
Copy-Item -LiteralPath $findModule -Destination $missingRoot
Copy-Item -LiteralPath $helper -Destination $missingRoot
$bin = Join-Path $guards 'find-missing-build'
$v = [string[]]@('-S', $consumer, '-B', $bin, '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143', "-DJE_D3DX12_PACKAGE_ROOT=$missingRoot")
$results += Invoke-Guard 'find-missing-header' $v 'file is missing' 'observe' $bin

$tamperedRoot = Join-Path $guards 'find-tampered-root'
Copy-Item -LiteralPath $package -Destination $tamperedRoot -Recurse
[IO.File]::AppendAllText((Join-Path $tamperedRoot 'd3dx12\include\d3dx12.h'), "`n// test-only tamper`n", [Text.UTF8Encoding]::new($false))
$bin = Join-Path $guards 'find-tampered-build'
$v = [string[]]@('-S', $consumer, '-B', $bin, '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143', "-DJE_D3DX12_PACKAGE_ROOT=$tamperedRoot")
$results += Invoke-Guard 'find-tampered-header' $v 'SHA256 mismatch' 'observe' $bin

$unc = '\\server\share\d3dx12-package'
$bin = Join-Path $guards 'find-unc-build'
$v = [string[]]@('-S', $consumer, '-B', $bin, '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143', "-DJE_D3DX12_PACKAGE_ROOT:STRING=$unc")
$results += Invoke-Guard 'find-unc-package-root' $v 'must be an absolute local path' 'observe' $bin

$results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $guards 'results.json')
$results | Select-Object name,nativeExit,result,diagnosticMatched,outputExists,elapsedSeconds | Format-Table -AutoSize
$failed = @($results | Where-Object { $_.result -ne 'PASS' })
if ($failed.Count) { throw "Guard failures: $($failed.name -join ', ')" }
