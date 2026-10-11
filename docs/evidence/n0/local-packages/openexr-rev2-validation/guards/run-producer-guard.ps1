param(
    [Parameter(Mandatory)][string]$Name,
    [Parameter(Mandatory)][string]$VariableName,
    [Parameter(Mandatory)][string]$TypedValue,
    [Parameter(Mandatory)][string]$ExpectedText,
    [Parameter(Mandatory)][string]$CheckPath,
    [string]$FreshOutputPath = '',
    [switch]$MustBeAbsent
)
$ErrorActionPreference = 'Stop'
$base = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages'
$evidence = Join-Path $base 'openexr-rev2-validation\guards'
$doc = Get-Content -Raw -LiteralPath (Join-Path $base 'openexr-audit\validation-arguments-v2.json') | ConvertFrom-Json
$vector = [string[]]$doc.producer.argv.Clone()
$index = -1
for ($i = 0; $i -lt $vector.Count; $i++) { if ($vector[$i].StartsWith('-D' + $VariableName + ':')) { $index = $i } }
if ($index -lt 0 -or $vector.Count -ne 6 -or $vector[4] -ne '-P' -or $vector[5] -ne 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/OpenEXR/BuildOpenEXRPackage.cmake') { throw 'Producer guard argv shape mismatch' }
$vector[$index] = '-D' + $VariableName + ':' + $TypedValue
if ($FreshOutputPath) {
    $outputIndex = -1
    for ($i = 0; $i -lt $vector.Count; $i++) { if ($vector[$i].StartsWith('-DJE_OPENEXR_OUTPUT:')) { $outputIndex = $i } }
    if ($outputIndex -lt 0) { throw 'JE_OPENEXR_OUTPUT argv entry missing' }
    $vector[$outputIndex] = '-DJE_OPENEXR_OUTPUT:PATH=' + $FreshOutputPath
}
$vector | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $evidence ($Name + '.argv.json')) -Encoding utf8
$psi = [Diagnostics.ProcessStartInfo]::new(); $psi.FileName = 'C:\Program Files\CMake\bin\cmake.exe'; $psi.WorkingDirectory = $evidence; $psi.UseShellExecute = $false; $psi.RedirectStandardOutput = $true; $psi.RedirectStandardError = $true
foreach ($item in $vector) { [void]$psi.ArgumentList.Add($item) }
$process = [Diagnostics.Process]::new(); $process.StartInfo = $psi; $timer = [Diagnostics.Stopwatch]::StartNew()
if (!$process.Start()) { throw 'CMake did not start' }; $stdoutTask = $process.StandardOutput.ReadToEndAsync(); $stderrTask = $process.StandardError.ReadToEndAsync(); $process.WaitForExit(); $nativeExit = $process.ExitCode; $timer.Stop()
$stdout = $stdoutTask.GetAwaiter().GetResult(); $stderr = $stderrTask.GetAwaiter().GetResult()
[IO.File]::WriteAllText((Join-Path $evidence ($Name + '.stdout.log')), $stdout, [Text.UTF8Encoding]::new($false)); [IO.File]::WriteAllText((Join-Path $evidence ($Name + '.stderr.log')), $stderr, [Text.UTF8Encoding]::new($false)); [IO.File]::WriteAllText((Join-Path $evidence ($Name + '.exitcode')), [string]$nativeExit); [IO.File]::WriteAllText((Join-Path $evidence ($Name + '.elapsed.txt')), ('{0:N3}s' -f $timer.Elapsed.TotalSeconds))
$matched = ($stdout + "`n" + $stderr).Contains($ExpectedText); $pathExists = Test-Path -LiteralPath $CheckPath; $pathCondition = if ($MustBeAbsent) { !$pathExists } else { $pathExists }
$result = [pscustomobject]@{ name=$Name; argvCount=$vector.Count; nativeExit=$nativeExit; elapsedSeconds=[math]::Round($timer.Elapsed.TotalSeconds,3); expectedText=$ExpectedText; diagnosticMatched=$matched; checkedPath=$CheckPath; mustBeAbsent=[bool]$MustBeAbsent; pathExists=$pathExists; pathCondition=$pathCondition }
$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence ($Name + '.result.json')) -Encoding utf8
$result | ConvertTo-Json -Depth 4
if ($nativeExit -eq 0 -or !$matched -or !$pathCondition) { throw "Guard mismatch: $Name" }
