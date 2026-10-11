$ErrorActionPreference='Stop'
$cwd='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$producer='F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\LZ4\BuildLz4Package.cmake'
$output='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-v143-rev1'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-rev1-validation'
$pre=Get-Content -LiteralPath (Join-Path $ev 'preflight.json') -Raw|ConvertFrom-Json
$before=Get-Content -LiteralPath (Join-Path $ev 'input-hashes-before.json') -Raw|ConvertFrom-Json
if($pre.freeze_pass -ne $true -or $pre.archive_pass -ne $true -or $pre.snapshot_pass -ne $true -or $pre.freeze_sha256 -ne 'D6D949B25EB2F83B043C6D61A2838245C73BC2951B29F4166B0D0B24503C47E6'){throw 'Preflight failed.'}
if($before.freeze_files.Count -ne 13 -or $before.snapshot_files.Count -ne 187 -or $before.producer_output_exists){throw 'Input snapshot or output state invalid.'}
if(Test-Path -LiteralPath $output){throw 'Producer output already exists; refusing to run.'}
$argv=[string[]]@(
  '-DJE_LZ4_SOURCE_ARCHIVE:FILEPATH=F:/JericoEngine/Dependencies/archives/lz4-v1.9.4.tar.gz',
  '-DJE_LZ4_SOURCE_SHA256:STRING=0b0e3aa07c8c063ddf40b082bdf7e37a1562bda40a0ff5272957f3e987e0e54b',
  '-DJE_LZ4_SOURCE_URL:STRING=https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.9.4',
  '-DJE_LZ4_OUTPUT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-v143-rev1',
  '-P',
  $producer
)
if($argv.Count -ne 6 -or $argv[4] -ne '-P' -or $argv[5] -ne $producer){throw 'Producer argv guard failed.'}
$argv|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $ev 'producer.argv.json') -Encoding utf8
$stdout=Join-Path $ev 'producer.stdout.log';$stderr=Join-Path $ev 'producer.stderr.log';$combined=Join-Path $ev 'producer.log'
$started=Get-Date;$watch=[Diagnostics.Stopwatch]::StartNew()
$p=Start-Process -FilePath $cmake -ArgumentList $argv -WorkingDirectory $cwd -NoNewWindow -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
$rootPid=$p.Id;$seen=@{};$cpuLast=@{};$cpuTicks=[int64]0;$samples=0;$peakTree=[int64]0;$peakSingle=[int64]0
while(-not $p.HasExited){
  $procs=@(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
  $active=@{};$active[$rootPid]=$true;$changed=$true
  while($changed){$changed=$false;foreach($proc in $procs){$pidValue=[int]$proc.ProcessId;$parent=[int]$proc.ParentProcessId;if($active.ContainsKey($parent)-and -not $active.ContainsKey($pidValue)){$active[$pidValue]=$true;$changed=$true}}}
  $sum=[int64]0
  foreach($proc in $procs){$id=[int]$proc.ProcessId;if($active.ContainsKey($id)){$seen[$id]=$true;$ws=[int64]$proc.WorkingSetSize;$sum+=$ws;if($ws -gt $peakSingle){$peakSingle=$ws};$cur=([int64]$proc.KernelModeTime)+([int64]$proc.UserModeTime);if($cpuLast.ContainsKey($id)){$delta=$cur-[int64]$cpuLast[$id];if($delta -gt 0){$cpuTicks+=$delta}};$cpuLast[$id]=$cur}}
  if($sum -gt $peakTree){$peakTree=$sum};$samples++;Start-Sleep -Milliseconds 1000;$p.Refresh()
}
$p.WaitForExit();$exitCode=$p.ExitCode;$watch.Stop();$finished=Get-Date
$log=@("command: `"$cmake`" $($argv -join ' ')","working_directory: $cwd","started_local: $($started.ToString('o'))","finished_local: $($finished.ToString('o'))","elapsed_seconds: $([Math]::Round($watch.Elapsed.TotalSeconds,3))","native_exit_code: $exitCode","stdout_log: $stdout","stderr_log: $stderr",'--- stdout ---',(Get-Content -LiteralPath $stdout -Raw),'--- stderr ---',(Get-Content -LiteralPath $stderr -Raw))
$log -join "`r`n"|Set-Content -LiteralPath $combined -Encoding utf8
$result=[ordered]@{executable=$cmake;argv=$argv;argc=$argv.Count;cwd=$cwd;started_local=$started.ToString('o');finished_local=$finished.ToString('o');elapsed_seconds=[Math]::Round($watch.Elapsed.TotalSeconds,3);native_exit_code=$exitCode;process_tree_scope='CMake and descendants visible in 1-second Win32_Process samples; short-lived descendants can be missed';sample_count=$samples;distinct_process_ids_seen=$seen.Count;peak_sampled_tree_working_set_bytes=$peakTree;peak_sampled_single_process_working_set_bytes=$peakSingle;observed_cpu_delta_seconds=[Math]::Round(($cpuTicks/10000000.0),3);stdout_log=$stdout;stderr_log=$stderr;combined_log=$combined;output_created=(Test-Path -LiteralPath $output)}
$result|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $ev 'producer.result.json') -Encoding utf8
[pscustomobject]@{native_exit=$exitCode;elapsed_seconds=$result.elapsed_seconds;processes_observed=$seen.Count;peak_tree_bytes=$peakTree;peak_single_bytes=$peakSingle;observed_cpu_seconds=$result.observed_cpu_delta_seconds;output_created=$result.output_created}|Format-List
exit $exitCode
