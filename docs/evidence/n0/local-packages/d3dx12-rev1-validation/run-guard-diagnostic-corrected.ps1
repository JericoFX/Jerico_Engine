$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $repo 'build\n0-validation\local-packages\d3dx12-rev1-validation'
$dir = Join-Path $base 'guards-diagnostic-corrected'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$producer = Join-Path $repo 'cmake\Tools\LocalPackages\d3dx12\BuildD3dx12Package.cmake'
$archive = 'F:/JericoEngine/Dependencies/archives/o3de-3p-70f0b7b5257bf7cf935bf1cfd270e1b53ccead18.tar.gz'
$hash = 'f69edb8bdf9d3dcd83cf296db31130f4d16c82efb9b4cdebd76c8035ccaa6042'
$url = 'https://codeload.github.com/o3de/3p-package-source/tar.gz/70f0b7b5257bf7cf935bf1cfd270e1b53ccead18'
[IO.Directory]::CreateDirectory($dir) | Out-Null
function Run([string]$name, [string[]]$argv, [string]$expected, [string]$out, [bool]$sentinel) {
    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $cmake
    $psi.WorkingDirectory = $dir
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    foreach ($a in $argv) { [void]$psi.ArgumentList.Add($a) }
    $argv | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $dir "$name.argv.json")
    $p = [Diagnostics.Process]::new()
    $p.StartInfo = $psi
    $sw = [Diagnostics.Stopwatch]::StartNew()
    if (!$p.Start()) { throw "Could not start $name" }
    $ot = $p.StandardOutput.ReadToEndAsync()
    $et = $p.StandardError.ReadToEndAsync()
    $p.WaitForExit()
    $status = $p.ExitCode
    $sw.Stop()
    $stdout = $ot.GetAwaiter().GetResult()
    $stderr = $et.GetAwaiter().GetResult()
    [IO.File]::WriteAllText((Join-Path $dir "$name.stdout.log"), $stdout, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $dir "$name.stderr.log"), $stderr, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $dir "$name.exitcode"), [string]$status)
    [IO.File]::WriteAllText((Join-Path $dir "$name.elapsed.txt"), ('{0:N3}s' -f $sw.Elapsed.TotalSeconds))
    $flat = (($stdout + ' ' + $stderr) -replace '\s+', ' ')
    $diagMatch = $flat.Contains($expected)
    $exists = Test-Path -LiteralPath $out
    $sentinelOkay = !$sentinel -or ((Test-Path -LiteralPath (Join-Path $out 'sentinel.txt')) -and ((Get-Content -Raw -LiteralPath (Join-Path $out 'sentinel.txt')) -eq 'preserve'))
    [pscustomobject]@{ name=$name; nativeExit=$status; expectedDiagnostic=$expected; diagnosticMatched=$diagMatch; outputExists=$exists; sentinelIntact=$sentinelOkay; result=if($status -ne 0 -and $diagMatch -and $sentinelOkay -and (($name -eq 'existing-output') -or !$exists)){'PASS'}else{'NOT_PASS'}; elapsedSeconds=[math]::Round($sw.Elapsed.TotalSeconds,3) }
}
$results = @()
$out = Join-Path $dir 'hash-rejection-output'
$results += Run 'wrong-hash' ([string[]]@("-DJE_D3DX12_SOURCE_ARCHIVE=$archive", '-DJE_D3DX12_SOURCE_SHA256=0000000000000000000000000000000000000000000000000000000000000000', "-DJE_D3DX12_SOURCE_URL=$url", "-DJE_D3DX12_OUTPUT=$out", '-P', $producer)) 'source URL/SHA256 must match the audited O3DE source archive' $out $false
$out = Join-Path $dir 'url-rejection-output'
$results += Run 'wrong-url' ([string[]]@("-DJE_D3DX12_SOURCE_ARCHIVE=$archive", "-DJE_D3DX12_SOURCE_SHA256=$hash", '-DJE_D3DX12_SOURCE_URL=https://invalid.example/wrong.tar.gz', "-DJE_D3DX12_OUTPUT=$out", '-P', $producer)) 'source URL/SHA256 must match the audited O3DE source archive' $out $false
$out = Join-Path $dir 'existing-output'
[IO.Directory]::CreateDirectory($out) | Out-Null
[IO.File]::WriteAllText((Join-Path $out 'sentinel.txt'), 'preserve')
$results += Run 'existing-output' ([string[]]@("-DJE_D3DX12_SOURCE_ARCHIVE=$archive", "-DJE_D3DX12_SOURCE_SHA256=$hash", "-DJE_D3DX12_SOURCE_URL=$url", "-DJE_D3DX12_OUTPUT=$out", '-P', $producer)) 'output already exists; retained outputs are never overwritten' $out $true
$results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $dir 'results.json')
$results | Select-Object name,nativeExit,result,diagnosticMatched,sentinelIntact | Format-Table -AutoSize
if (@($results | Where-Object {$_.result -ne 'PASS'}).Count) { throw 'Corrected diagnostic guard assertion failed' }
