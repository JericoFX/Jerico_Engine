$ErrorActionPreference = 'Stop'
$repo = 'F:/JericoEngine/JericoEngine'
$base = Join-Path $repo 'build/n0-validation/local-packages/mcpp-rev2-validation'
$freezePath = Join-Path $repo 'build/n0-validation/local-packages/mcpp-audit/source-freeze-parent17-rev2.json'
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
$consumer = Join-Path $base 'mcpp-consumer-v2'
$package = Join-Path $base 'mcpp-extracted-v2'
$cmake = 'C:/Program Files/CMake/bin/cmake.exe'
$ctest = 'C:/Program Files/CMake/bin/ctest.exe'
if (Test-Path -LiteralPath $consumer) { throw "consumer build directory exists: $consumer" }
function Get-FreezeSnapshot {
  $rows = @()
  foreach ($input in $freeze.inputs) { $actual=(Get-FileHash -LiteralPath $input.path -Algorithm SHA256).Hash.ToLowerInvariant(); if($actual -ne $input.sha256){throw "frozen input changed: $($input.path)"}; $rows += [ordered]@{ path=$input.path; bytes=(Get-Item -LiteralPath $input.path).Length; sha256=$actual } }
  return ,$rows
}
function Invoke-LoggedNative([string]$name,[string]$exe,[string]$cwd,[string[]]$argumentVector) {
  $psi=[Diagnostics.ProcessStartInfo]::new(); $psi.FileName=$exe; $psi.WorkingDirectory=$cwd; $psi.UseShellExecute=$false; $psi.CreateNoWindow=$true; $psi.RedirectStandardOutput=$true; $psi.RedirectStandardError=$true
  if($argumentVector.Count -eq 0){throw "empty argv for $name"}
  foreach($arg in $argumentVector){[void]$psi.ArgumentList.Add($arg)}
  $process=[Diagnostics.Process]::new(); $process.StartInfo=$psi; $watch=[Diagnostics.Stopwatch]::StartNew(); [void]$process.Start(); $stdoutTask=$process.StandardOutput.ReadToEndAsync(); $stderrTask=$process.StandardError.ReadToEndAsync(); $process.WaitForExit(); $exit=$process.ExitCode; $stdout=$stdoutTask.GetAwaiter().GetResult(); $stderr=$stderrTask.GetAwaiter().GetResult(); $watch.Stop()
  [IO.File]::WriteAllText((Join-Path $base ($name+'.stdout.log')),$stdout,[Text.UTF8Encoding]::new($false)); [IO.File]::WriteAllText((Join-Path $base ($name+'.stderr.log')),$stderr,[Text.UTF8Encoding]::new($false))
  return [ordered]@{ name=$name; executable=$exe; workingDirectory=$cwd; arguments=$argumentVector; exitCode=$exit; elapsedSeconds=$watch.Elapsed.TotalSeconds }
}
$pre=Get-FreezeSnapshot
[IO.File]::WriteAllText((Join-Path $base 'mcpp-consumer-actual-inputs-before.json'),($pre|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
$runs=@()
$configureArgs=[string[]]@('-S','F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/mcpp/tests','-B',$consumer,'-G','Visual Studio 17 2022','-A','x64','-T','v143',"-DJE_MCPP_PACKAGE_ROOT:PATH=$package")
if($configureArgs.Count -ne 11 -or $configureArgs[0] -ne '-S' -or $configureArgs[2] -ne '-B'){throw 'configure argv guard failed'}
$runs+=Invoke-LoggedNative 'mcpp-consumer-actual-configure' $cmake $base $configureArgs
if($runs[-1].exitCode -eq 0){
  $runs+=Invoke-LoggedNative 'mcpp-consumer-actual-build-release' $cmake $base ([string[]]@('--build',$consumer,'--config','Release'))
  if($runs[-1].exitCode -eq 0){$runs+=Invoke-LoggedNative 'mcpp-consumer-actual-test-release' $ctest $base ([string[]]@('--test-dir',$consumer,'-C','Release','--output-on-failure'))}
  if($runs[-1].exitCode -eq 0){
    $runs+=Invoke-LoggedNative 'mcpp-consumer-actual-build-debug' $cmake $base ([string[]]@('--build',$consumer,'--config','Debug'))
    if($runs[-1].exitCode -eq 0){$runs+=Invoke-LoggedNative 'mcpp-consumer-actual-test-debug' $ctest $base ([string[]]@('--test-dir',$consumer,'-C','Debug','--output-on-failure'))}
  }
}
$post=Get-FreezeSnapshot
[IO.File]::WriteAllText((Join-Path $base 'mcpp-consumer-actual-inputs-after.json'),($post|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
$record=[ordered]@{ runs=$runs; freezeSha256=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash.ToLowerInvariant(); freezeCount=$freeze.inputs.Count; pre=$pre; post=$post; allCommandsPassed=(@($runs|Where-Object{$_.exitCode -ne 0}).Count -eq 0) }
[IO.File]::WriteAllText((Join-Path $base 'mcpp-consumer-actual-result.json'),($record|ConvertTo-Json -Depth 6),[Text.UTF8Encoding]::new($false))
foreach($run in $runs){"$($run.name): exit=$($run.exitCode), seconds=$($run.elapsedSeconds)"}
"AllCommandsPassed=$($record.allCommandsPassed)"
