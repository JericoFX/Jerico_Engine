$ErrorActionPreference = 'Stop'
$cwd = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$driver = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-perl-tool-audit\producer-candidate-inputs.cmake'
$outdir = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev1'
$ev = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-rev1-pin-corrected-validation'
$pre = Get-Content -LiteralPath (Join-Path $ev 'preflight.json') -Raw | ConvertFrom-Json
if ($pre.freeze_sha256 -ne '7E730F85776C891B2206FD69990B2E702587695DDCAA8487721579C3C855F1A4' -or -not $pre.freeze_pass -or $pre.freeze_count -ne 21) { throw 'New frozen input preflight failed.' }
if ($pre.manifest_count -ne 20858 -or -not $pre.tool_pass -or $pre.manifest_sha256 -ne 'FB0CC5933D7F126362FD1F2CD6FEBFBA1D8899FE6DBD5288090637947476052A') { throw 'Portable Perl tool preflight failed.' }
if ($pre.driver_sha256 -ne 'F39CEA6362F8E9C02138F4B7F5448234D4C2A04684670501B4D674BF5D238223' -or (Test-Path -LiteralPath $outdir)) { throw 'Candidate driver changed or output is not new.' }
$argv = [string[]]@('-P', $driver)
if ($argv.Count -ne 2 -or $argv[0] -ne '-P' -or $argv[1] -ne $driver) { throw 'Unexpected CMake argv.' }
$argv | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $ev 'producer-argv.json') -Encoding utf8
$stdoutPath = Join-Path $ev 'producer.stdout.log'
$stderrPath = Join-Path $ev 'producer.stderr.log'
$combinedPath = Join-Path $ev 'producer.log'
$started = Get-Date
$timer = [Diagnostics.Stopwatch]::StartNew()
$p = Start-Process -FilePath $cmake -ArgumentList $argv -WorkingDirectory $cwd -NoNewWindow -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
$rootPid = $p.Id
$seen = @{}
$sampleCount = 0
$peakTreeWorkingSet = [int64]0
$peakSingleWorkingSet = [int64]0
while (-not $p.HasExited) {
  $procs = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
  $active = @{}
  $active[$rootPid] = $true
  $changed = $true
  while ($changed) {
    $changed = $false
    foreach ($proc in $procs) {
      if ($active.ContainsKey([int]$proc.ParentProcessId) -and -not $active.ContainsKey([int]$proc.ProcessId)) { $active[[int]$proc.ProcessId] = $true; $changed = $true }
    }
  }
  $sum = [int64]0
  foreach ($proc in $procs) {
    $id = [int]$proc.ProcessId
    if ($active.ContainsKey($id)) {
      $seen[$id] = $true
      $ws = [int64]$proc.WorkingSetSize
      $sum += $ws
      if ($ws -gt $peakSingleWorkingSet) { $peakSingleWorkingSet = $ws }
    }
  }
  if ($sum -gt $peakTreeWorkingSet) { $peakTreeWorkingSet = $sum }
  $sampleCount++
  Start-Sleep -Milliseconds 1000
  $p.Refresh()
}
$p.WaitForExit()
$exitCode = $p.ExitCode
$timer.Stop()
$finished = Get-Date
$lines = @(
  "command: `"$cmake`" -P `"$driver`"",
  "working_directory: $cwd",
  "started_local: $($started.ToString('o'))",
  "finished_local: $($finished.ToString('o'))",
  "elapsed_seconds: $([Math]::Round($timer.Elapsed.TotalSeconds,3))",
  "native_exit_code: $exitCode",
  "stdout_log: $stdoutPath",
  "stderr_log: $stderrPath",
  '--- stdout ---',
  (Get-Content -LiteralPath $stdoutPath -Raw),
  '--- stderr ---',
  (Get-Content -LiteralPath $stderrPath -Raw)
)
$lines -join "`r`n" | Set-Content -LiteralPath $combinedPath -Encoding utf8
@{
  executable=$cmake; argv=$argv; argc=$argv.Count; cwd=$cwd;
  started_local=$started.ToString('o'); finished_local=$finished.ToString('o'); elapsed_seconds=[Math]::Round($timer.Elapsed.TotalSeconds,3);
  native_exit_code=$exitCode; process_tree_observed=$true; process_tree_scope='CMake and descendants visible in 1-second Win32_Process samples; short-lived descendants can be missed';
  peak_sampled_tree_working_set_bytes=$peakTreeWorkingSet; peak_sampled_single_process_working_set_bytes=$peakSingleWorkingSet;
  sampled_process_count=$sampleCount; distinct_process_ids_seen=$seen.Count;
  stdout_log=$stdoutPath; stderr_log=$stderrPath; combined_log=$combinedPath;
  output_created=(Test-Path -LiteralPath $outdir)
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $ev 'producer-result.json') -Encoding utf8
Write-Output "exit=$exitCode elapsed=$([Math]::Round($timer.Elapsed.TotalSeconds,3))s sampledTreePeak=$peakTreeWorkingSet bytes processes=$($seen.Count)"
exit $exitCode
